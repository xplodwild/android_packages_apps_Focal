// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "source/Host_IO.hpp"
#include "source/XMP_LibUtils.hpp"
#include "source/UnicodeConversions.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false' (performance warning)
#endif

// =================================================================================================
// Host_IO implementations for Windows
// ===================================

#if ! XMP_WinBuild
	#error "This is the Windows implementation of Host_IO."
#endif

static bool IsLongPath ( const std::string& path );
static bool IsNetworkPath ( const std::string& path );
static bool IsRelativePath ( const std::string& path );
static bool GetWidePath ( const char* path, std::string & widePath );
static std::string & CorrectSlashes ( std::string & path );

static bool Exists ( const std::string & widePath );
static Host_IO::FileRef Open ( const std::string & widePath, bool readOnly );
static Host_IO::FileMode GetFileMode ( const std::string & widePath );
static bool HaveWriteAccess( const std::string & widePath );

// =================================================================================================
// File operations
// =================================================================================================

// =================================================================================================
// Host_IO::Exists
// ===============

bool Host_IO::Exists ( const char* filePath )
{
	std::string wideName;
	if ( GetWidePath(filePath, wideName) ) {
		return ::Exists(wideName);
	}
	return false;
}

// =================================================================================================
// Host_IO::Writable
// ===============
bool Host_IO::Writable( const char * path, bool checkCreationPossible )
{
	std::string widePath;
	if ( ! GetWidePath(path, widePath) || widePath.length() == 0)
		XMP_Throw ( "Host_IO::Writable, cannot convert path", kXMPErr_ExternalFailure );

	if ( ::Exists( widePath ) )
	{
		switch ( ::GetFileMode( widePath ) )
		{
		case kFMode_IsFile:
			if ( HaveWriteAccess( widePath ) ) {
				// check for readonly attributes
				DWORD fileAttrs = GetFileAttributesW ( (LPCWSTR) widePath.data() );
				if ( fileAttrs & FILE_ATTRIBUTE_READONLY )
					return false;
				return true;
			}
			return false;
			break;

		case kFMode_IsFolder:
			return HaveWriteAccess( widePath );
			break;

		default:
			return false;
			break;
		}
	} 
	else if ( checkCreationPossible )
	{
		// get the parent path
		std::string utf8Path(path);
		CorrectSlashes(utf8Path);
		size_t pos = utf8Path.find_last_of('\\');
		if (pos != std::string::npos)
		{
			if (pos == 0)
				utf8Path = utf8Path.substr(0, 1);
			else
				utf8Path = utf8Path.substr(0, pos);
		}
		else
		{
			utf8Path = ".";
		}
		return Host_IO::Writable( utf8Path.c_str(), checkCreationPossible );
	}
	else
		return true;
}

// =================================================================================================
// Host_IO::Create
// ===============

bool Host_IO::Create ( const char* filePath )
{
	std::string wideName;
	if ( !GetWidePath ( filePath, wideName ) || wideName.length() == 0 )
		 XMP_Throw ( "Host_IO::Create, cannot convert path", kXMPErr_ExternalFailure );

	if ( ::Exists ( wideName ) ) {
		if ( ::GetFileMode ( wideName ) == kFMode_IsFile ) return false;
		XMP_Throw ( "Host_IO::Create, path exists but is not a file", kXMPErr_InternalFailure );
	}

	Host_IO::FileRef fileHandle;
	fileHandle = CreateFileW ( (LPCWSTR)wideName.data(), (GENERIC_READ | GENERIC_WRITE), 0, 0, CREATE_ALWAYS,
							   (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS), 0 );
	if ( fileHandle == INVALID_HANDLE_VALUE ) XMP_Throw ( "Host_IO::Create, cannot create file", kXMPErr_InternalFailure );;

	CloseHandle ( fileHandle );
	return true;
}	// Host_IO::Create

// =================================================================================================
// FillXMPTime
// ===========

static void FillXMPTime ( const SYSTEMTIME & winTime, XMP_DateTime * xmpTime )
{

	// Ignore the fractional seconds for consistency with UNIX and to avoid false newness even on
	// Windows. Some other sources of time only resolve to seconds, we don't want 25.3 looking
	// newer than 25.

	xmpTime->year = winTime.wYear;
	xmpTime->month = winTime.wMonth;
	xmpTime->day = winTime.wDay;
	xmpTime->hasDate = true;

	xmpTime->hour = winTime.wHour;
	xmpTime->minute = winTime.wMinute;
	xmpTime->second = winTime.wSecond;
	xmpTime->nanoSecond = 0;	// See note above; winTime.wMilliseconds * 1000*1000;
	xmpTime->hasTime = true;

	xmpTime->tzSign = kXMP_TimeIsUTC;
	xmpTime->tzHour = 0;
	xmpTime->tzMinute = 0;
	xmpTime->hasTimeZone = true;

}

// =================================================================================================
// Host_IO::GetModifyDate
// ======================

bool Host_IO::GetModifyDate ( const char* filePath, XMP_DateTime * modifyDate )
{
	BOOL ok;
	Host_IO::FileRef fileHandle;

	try {	// Host_IO::Open should not throw - fix after CS6.
		fileHandle = Host_IO::Open ( filePath, Host_IO::openReadOnly );
		if ( fileHandle == Host_IO::noFileRef ) return false;
	} catch ( ... ) {
		return false;
	}

	FILETIME binTime;
	ok = GetFileTime ( fileHandle, 0, 0, &binTime );
	Host_IO::Close ( fileHandle );
	if ( ! ok ) return false;
	
	SYSTEMTIME utcTime;
	ok = FileTimeToSystemTime ( &binTime, &utcTime );
	if ( ! ok ) return false;
	
	FillXMPTime ( utcTime, modifyDate );
	return true;

}	// Host_IO::GetModifyDate

// =================================================================================================
// ConjureDerivedPath
// ==================

static std::string ConjureDerivedPath ( const char* basePath )
{
	std::string tempPath = basePath;
	tempPath += "._nn_";
	char * indexPart = (char*) tempPath.c_str() + strlen(basePath) + 2;

	for ( char ten = '0'; ten <= '9' ; ++ten ) {
		indexPart[0] = ten;
		for ( char one = '0'; one <= '9' ; ++one ) {
			indexPart[1] = one;
			if ( ! Host_IO::Exists ( tempPath.c_str() ) ) return tempPath;
		}
	}

	return "";

}	// ConjureDerivedPath

// =================================================================================================
// Host_IO::CreateTemp
// ===================

std::string Host_IO::CreateTemp ( const char* sourcePath )
{
	std::string tempPath = ConjureDerivedPath ( sourcePath );
	if ( tempPath.empty() ) XMP_Throw ( "Host_IO::CreateTemp, cannot create temp file path", kXMPErr_InternalFailure );
	XMP_Assert ( ! Host_IO::Exists ( tempPath.c_str() ) );

	Host_IO::Create ( tempPath.c_str() );
	return tempPath;

}	// Host_IO::CreateTemp

// =================================================================================================
// Host_IO::Open
// =============
//
// Returns Host_IO::noFileRef (0) if the file does not exist, throws for other errors.

Host_IO::FileRef Host_IO::Open ( const char* filePath, bool readOnly )
{
	std::string wideName;
	if ( !GetWidePath ( filePath, wideName ) || wideName.length() == 0 )
		XMP_Throw ( "Host_IO::Open, GetWidePath failure", kXMPErr_ExternalFailure );

	return ::Open( wideName, readOnly );

}	// Host_IO::Open

// =================================================================================================
// Host_IO::Close
// ==============

void Host_IO::Close ( Host_IO::FileRef fileHandle )
{
	if ( fileHandle == Host_IO::noFileRef ) return;

	BOOL ok = CloseHandle ( fileHandle );
	if ( ! ok ) XMP_Throw ( "Host_IO::Close, CloseHandle failure", kXMPErr_ExternalFailure );

}	// Host_IO::Close

// =================================================================================================
// Host_IO::SwapData
// =================

void Host_IO::SwapData ( const char* sourcePath, const char* destPath )
{

	// For lack of a better approach, do a 3-way rename.

	std::string thirdPath = ConjureDerivedPath ( sourcePath );
	if ( thirdPath.empty() ) XMP_Throw ( "Cannot create temp file path", kXMPErr_InternalFailure );
	XMP_Assert ( ! Host_IO::Exists ( thirdPath.c_str() ) );

	Host_IO::Rename ( sourcePath, thirdPath.c_str() );
	
	try {
		Host_IO::Rename ( destPath, sourcePath );
	} catch ( ... ) {
		Host_IO::Rename ( thirdPath.c_str(), sourcePath );
		throw;
	}
	
	try {
		Host_IO::Rename ( thirdPath.c_str(), destPath );
	} catch ( ... ) {
		Host_IO::Rename ( sourcePath, destPath );
		Host_IO::Rename ( thirdPath.c_str(), sourcePath );
		throw;
	}

}	// Host_IO::SwapData

// =================================================================================================
// Host_IO::Rename
// ===============

void Host_IO::Rename ( const char* oldPath, const char* newPath )
{
	std::string wideOldPath, wideNewPath;
	if ( !GetWidePath ( oldPath, wideOldPath ) || wideOldPath.length() == 0 )
		XMP_Throw ( "Host_IO::Rename, GetWidePath failure", kXMPErr_ExternalFailure );

	if ( !GetWidePath ( newPath, wideNewPath ) || wideNewPath.length() == 0 )
		XMP_Throw ( "Host_IO::Rename, GetWidePath failure", kXMPErr_ExternalFailure );

	if ( ::Exists ( wideNewPath ) ) XMP_Throw ( "Host_IO::Rename, new path exists", kXMPErr_InternalFailure );

	

	BOOL ok = MoveFileW ( (LPCWSTR)wideOldPath.data(), (LPCWSTR)wideNewPath.data() );
	if ( ! ok ) XMP_Throw ( "Host_IO::Rename, MoveFileW failure", kXMPErr_ExternalFailure );

}	// Host_IO::Rename

// =================================================================================================
// Host_IO::Delete
// ===============

void Host_IO::Delete ( const char* filePath )
{
	std::string wideName;
	if ( !GetWidePath ( filePath, wideName ) || wideName.length() == 0 )
		XMP_Throw ( "Host_IO::Delete, GetWidePath failure", kXMPErr_ExternalFailure );
	
	if ( !::Exists ( wideName ) ) return;

	

	BOOL ok = DeleteFileW ( (LPCWSTR)wideName.data() );
	if ( ! ok ) {
		DWORD errCode = GetLastError();
		if ( errCode != ERROR_FILE_NOT_FOUND ) {
			XMP_Throw ( "Host_IO::Delete, DeleteFileW failure", kXMPErr_ExternalFailure );
		}
	}

}	// Host_IO::Delete

// =================================================================================================
// Host_IO::Seek
// =============

XMP_Int64 Host_IO::Seek ( Host_IO::FileRef fileHandle, XMP_Int64 offset, SeekMode mode )
{
	DWORD method;
	switch ( mode ) {
		case kXMP_SeekFromStart :
			method = FILE_BEGIN;
			break;
		case kXMP_SeekFromCurrent :
			method = FILE_CURRENT;
			break;
		case kXMP_SeekFromEnd :
			method = FILE_END;
			break;
		default :
			XMP_Throw ( "Invalid seek mode", kXMPErr_InternalFailure );
			break;
	}

	LARGE_INTEGER seekOffset, newPos;
	seekOffset.QuadPart = offset;

	BOOL ok = SetFilePointerEx ( fileHandle, seekOffset, &newPos, method );
	if ( ! ok ) XMP_Throw ( "Host_IO::Seek, SetFilePointerEx failure", kXMPErr_ExternalFailure );

	return newPos.QuadPart;

}	// Host_IO::Seek

// =================================================================================================
// Host_IO::Read
// =============

#define TwoGB (XMP_Uns32)(2*1024*1024*1024UL)

XMP_Uns32 Host_IO::Read ( Host_IO::FileRef fileHandle, void * buffer, XMP_Uns32 count )
{
	if ( count >= TwoGB ) XMP_Throw ( "Host_IO::Read, request too large", kXMPErr_EnforceFailure );

	DWORD bytesRead;
	BOOL ok = ReadFile ( fileHandle, buffer, count, &bytesRead, 0 );
	if ( ! ok ) XMP_Throw ( "Host_IO::Read, ReadFile failure", kXMPErr_ReadError );

	return bytesRead;

}	// Host_IO::Read

// =================================================================================================
// Host_IO::Write
// ==============

void Host_IO::Write ( Host_IO::FileRef fileHandle, const void * buffer, XMP_Uns32 count )
{
	if ( count >= TwoGB ) XMP_Throw ( "Host_IO::Write, request too large", kXMPErr_EnforceFailure );

	DWORD bytesWritten;
	BOOL ok = WriteFile ( fileHandle, buffer, count, &bytesWritten, 0 );
	if ( (! ok) || (bytesWritten != count) ) {
		DWORD osCode = GetLastError();
		if ( osCode == ERROR_DISK_FULL ) {
			XMP_Throw ( "Host_IO::Write, disk full", kXMPErr_DiskSpace );
		} else {
			XMP_Throw ( "Host_IO::Write, WriteFile failure", kXMPErr_WriteError );
		}
	}

}	// Host_IO::Write

// =================================================================================================
// Host_IO::Length
// ===============

XMP_Int64 Host_IO::Length ( Host_IO::FileRef fileHandle )
{
	LARGE_INTEGER length;
	BOOL ok = GetFileSizeEx ( fileHandle, &length );
	if ( ! ok ) XMP_Throw ( "Host_IO::Length, GetFileSizeEx failure", kXMPErr_ExternalFailure );

	return length.QuadPart;

}	// Host_IO::Length

// =================================================================================================
// Host_IO::SetEOF
// ===============

void Host_IO::SetEOF ( Host_IO::FileRef fileHandle, XMP_Int64 length )
{
	LARGE_INTEGER winLength;
	winLength.QuadPart = length;

	BOOL ok = SetFilePointerEx ( fileHandle, winLength, 0, FILE_BEGIN );
	if ( ! ok ) XMP_Throw ( "Host_IO::SetEOF, SetFilePointerEx failure", kXMPErr_ExternalFailure );
	ok = SetEndOfFile ( fileHandle );
	if ( ! ok ) XMP_Throw ( "Host_IO::SetEOF, SetEndOfFile failure", kXMPErr_ExternalFailure );

}	// Host_IO::SetEOF

// =================================================================================================
// Folder operations
// =================================================================================================

// =================================================================================================
// Host_IO::GetFileMode
// ====================

static DWORD kOtherAttrs = (FILE_ATTRIBUTE_DEVICE);

Host_IO::FileMode Host_IO::GetFileMode ( const char * path )
{
	std::string utf16;	// GetFileAttributes wants native UTF-16.
	GetWidePath ( path, utf16 );
	return ::GetFileMode( utf16 );
}	// Host_IO::GetFileMode

// =================================================================================================
// Host_IO::GetChildMode
// =====================

Host_IO::FileMode Host_IO::GetChildMode ( const char * parentPath, const char * childName )
{
	std::string fullPath = parentPath;
	char lastChar = fullPath[fullPath.length() - 1];
	if ( lastChar != '\\' && lastChar != '/' )
		fullPath += '\\';
	fullPath += childName;

	return GetFileMode ( fullPath.c_str() );

}	// Host_IO::GetChildMode

// =================================================================================================
// Host_IO::OpenFolder
// ===================

Host_IO::FolderRef Host_IO::OpenFolder ( const char* folderPath )
{

	switch ( Host_IO::GetFileMode ( folderPath ) ) {

		case Host_IO::kFMode_IsFolder :
		{
			WIN32_FIND_DATAW childInfo;
			std::string findPath = folderPath;
			// Looking for all children of that folder, add * as search criteria
			findPath += findPath[findPath.length() - 1] == '\\' ? "*" : "\\*";

			std::string utf16;	// FindFirstFile wants native UTF-16.
			GetWidePath ( findPath.c_str(), utf16 );

			Host_IO::FolderRef folder = FindFirstFileW ( (LPCWSTR) utf16.c_str(), &childInfo );
			if ( folder == noFolderRef ) XMP_Throw ( "Host_IO::OpenFolder - FindFirstFileW failed", kXMPErr_ExternalFailure );
			// The first child should be ".", which we want to ignore anyway.
			XMP_Assert ( (folder == noFolderRef) || (childInfo.cFileName[0] == '.') );

			return folder;
		}

		case Host_IO::kFMode_DoesNotExist :
			return Host_IO::noFolderRef;

		default :
			XMP_Throw ( "Host_IO::OpenFolder, path is not a folder", kXMPErr_ExternalFailure );

	}

	XMP_Throw ( "Host_IO::OpenFolder, should not get here", kXMPErr_InternalFailure );

}	// Host_IO::OpenFolder

// =================================================================================================
// Host_IO::CloseFolder
// ====================

void Host_IO::CloseFolder ( Host_IO::FolderRef folder )
{
	if ( folder == noFolderRef ) return;

	BOOL ok = FindClose ( folder );
	if ( ! ok ) XMP_Throw ( "Host_IO::CloseFolder, FindClose failure", kXMPErr_ExternalFailure );

}	// Host_IO::CloseFolder

// =================================================================================================
// Host_IO::GetNextChild
// =====================

bool Host_IO::GetNextChild ( Host_IO::FolderRef folder, std::string* childName )
{
	bool found;
	WIN32_FIND_DATAW childInfo;

	if ( folder == Host_IO::noFolderRef ) return false;

	do {	// Ignore all children with names starting in '.'. This covers ., .., .DS_Store, etc.
		found = (bool) FindNextFile ( folder, &childInfo );
	} while ( found && (childInfo.cFileName[0] == '.') );
	if ( ! found ) return false;

	if ( childName != 0 ) {
		size_t len16 = 0;	// The cFileName field is native UTF-16.
		while ( childInfo.cFileName[len16] != 0 ) ++len16;
		FromUTF16Native ( (UTF16Unit*)childInfo.cFileName, len16, childName );
	}

	return true;

}	// Host_IO::GetNextChild

// =================================================================================================
// IsLongPath
// =====================

bool IsLongPath ( const std::string& path ) {
	if ( path.find ( "\\\\?\\" ) == 0 ) return true;
	return false;
}

// =================================================================================================
// IsNetworkPath
// =====================

bool IsNetworkPath ( const std::string& path ) {
	if ( path.find ( "\\\\" ) == 0 ) return true;
	return false;
}

// =================================================================================================
// IsRelativePath
// =====================

bool IsRelativePath ( const std::string& path ) {
	if ( path.length() > 2) {
		char driveLetter = path[0];
		if ( ( driveLetter >= 'a' && driveLetter <= 'z' ) || ( driveLetter >= 'A' && driveLetter <= 'Z' ) ) {
			if (path[1] == ':' && path[2] == '\\' ) {
				if ( path.find(".\\") == std::string::npos )
					return false;
			}
		}
	}
	return true;
}

// =================================================================================================
// GetWidePath
// =====================

bool GetWidePath( const char* path, std::string & widePath ) {
	std::string utfPath ( path );
	CorrectSlashes ( utfPath);

	if ( !IsLongPath ( utfPath ) ) {
		if ( IsNetworkPath ( utfPath ) ) {
			utfPath = "\\\\?\\UNC\\" + utfPath.substr ( 2 );
		} else if ( IsRelativePath ( utfPath ) ) {
			//don't do anything
		} else { // absolute path
			utfPath = "\\\\?\\" + utfPath;
		}
	}

	widePath.clear();
	const size_t utf8Len = utfPath.size();
	const size_t maxLen = 2 * (utf8Len + 1);

	widePath.reserve ( maxLen );
	widePath.assign ( maxLen, ' ' );
	int wideLen = MultiByteToWideChar ( CP_UTF8, 0, utfPath.c_str(), -1, (LPWSTR)widePath.data(), (int)maxLen );
	if ( wideLen == 0 ) return false;
	widePath.append ( 2, '\0' );	// Make sure there are at least 2 final zero bytes.
	return true;
}

// =================================================================================================
// CorrectSlashes
// =====================

std::string & CorrectSlashes ( std::string & path ) {
	size_t idx = 0;

	while( (idx = path.find_first_of('/',idx)) != std::string::npos )
		path.replace( idx, 1, "\\" );
	return path;
}

bool Exists ( const std::string & widePath )
{
	DWORD attrs = GetFileAttributesW ( (LPCWSTR)widePath.data() );
	return ( attrs != INVALID_FILE_ATTRIBUTES);
}


Host_IO::FileMode GetFileMode ( const std::string & widePath )
{
	// ! A shortcut is seen as a file, we would need extra code to recognize it and find the target.
	DWORD fileAttrs = GetFileAttributesW ( (LPCWSTR) widePath.c_str() );
	if ( fileAttrs == INVALID_FILE_ATTRIBUTES ) return Host_IO::kFMode_DoesNotExist;	// ! Any failure turns into does-not-exist.

	if ( fileAttrs & FILE_ATTRIBUTE_DIRECTORY ) return Host_IO::kFMode_IsFolder;
	if ( fileAttrs & kOtherAttrs ) return Host_IO::kFMode_IsOther;
	return Host_IO::kFMode_IsFile;

}

Host_IO::FileRef Open ( const std::string & widePath, bool readOnly )
{
	DWORD access = GENERIC_READ;	// Assume read mode.
	DWORD share  = FILE_SHARE_READ;

	if ( ! readOnly ) {
		access |= GENERIC_WRITE;
		share = 0;
	}

	Host_IO::FileRef fileHandle;
	fileHandle = CreateFileW ( (LPCWSTR)widePath.data(), access, share, 0, OPEN_EXISTING,
		(FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS), 0 );
	if ( fileHandle == INVALID_HANDLE_VALUE ) {
		DWORD osCode = GetLastError();
		if ( (osCode == ERROR_FILE_NOT_FOUND) || (osCode == ERROR_PATH_NOT_FOUND) || (osCode == ERROR_FILE_OFFLINE) ) {
			return Host_IO::noFileRef;
		} else if ( osCode == ERROR_ACCESS_DENIED ) {
			XMP_Throw ( "Open, file permission error", kXMPErr_FilePermission );
		} else {
			XMP_Throw ( "Open, other failure", kXMPErr_ExternalFailure );
		}
	}

	return fileHandle;


}

bool HaveWriteAccess ( const std::string & widePath )
{
	bool writable = false;

	DWORD length = 0;
	LPCWSTR pathLPtr = (LPCWSTR)widePath.data();
	const static SECURITY_INFORMATION requestedFileInfomration =  OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

	if (!::GetFileSecurityW( pathLPtr, requestedFileInfomration, NULL, NULL, &length ) && ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
	{
		std::string tempBuffer;
		tempBuffer.reserve(length);
		PSECURITY_DESCRIPTOR security = (PSECURITY_DESCRIPTOR)tempBuffer.data();
		if ( security && ::GetFileSecurity( pathLPtr, requestedFileInfomration, security, length, &length ) )
		{
			HANDLE hToken = NULL;
			const static DWORD tokenDesiredAccess = TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ;
			if ( !::OpenThreadToken( ::GetCurrentThread(), tokenDesiredAccess, TRUE, &hToken) )
			{
				if ( !::OpenProcessToken( GetCurrentProcess(), tokenDesiredAccess, &hToken ) )
				{
					XMP_Throw ( "Unable to get any thread or process token", kXMPErr_InternalFailure );
				}
			}

			HANDLE hImpersonatedToken = NULL;
			if ( ::DuplicateToken( hToken, SecurityImpersonation, &hImpersonatedToken ) )
			{
				GENERIC_MAPPING mapping = { 0xFFFFFFFF };
				PRIVILEGE_SET privileges = { 0 };
				DWORD grantedAccess = 0, privilegesLength = sizeof( privileges );
				BOOL result = FALSE;

				mapping.GenericRead = FILE_GENERIC_READ;
				mapping.GenericWrite = FILE_GENERIC_WRITE;
				mapping.GenericExecute = FILE_GENERIC_EXECUTE;
				mapping.GenericAll = FILE_ALL_ACCESS;
				
				DWORD genericAccessRights = FILE_GENERIC_WRITE;
				::MapGenericMask( &genericAccessRights, &mapping );
				
				if ( ::AccessCheck( security, hImpersonatedToken, genericAccessRights, &mapping, &privileges, &privilegesLength, &grantedAccess, &result ) )
				{
					writable = (result == TRUE);
				}
				::CloseHandle( hImpersonatedToken );
			}
			
			::CloseHandle( hToken );
		}
	}
	return writable;
}

