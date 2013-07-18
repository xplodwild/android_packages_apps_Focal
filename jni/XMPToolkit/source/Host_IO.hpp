#ifndef __Host_IO_hpp__
#define __Host_IO_hpp__	1

// =================================================================================================
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include <string>

#if XMP_WinBuild
	#include <Windows.h>
#elif XMP_MacBuild
	#include <CoreServices/CoreServices.h>
	#include <dirent.h>	// Mac uses the POSIX folder functions.
#elif XMP_UNIXBuild | XMP_iOSBuild
	#include <dirent.h>
#else
	#error "Unknown host platform."
#endif

// =================================================================================================
// Host_IO is a collection of minimal convenient wrappers for host I/O services. It is intentionally
// a namespace and not a class. No state is kept here, these are just wrappers that provide a common
// internal API for basic I/O services that differ from host to host.

namespace Host_IO {

	// =============================================================================================
	// File operations
	// ===============
	//
	// ! The file operations should only be used in the implementation of XMPFiles_IO.
	//
	// Exists - Returns true if the path exists, whether as a file, folder, or anything else. Never
	// throws an exception.
	//
	// Writable - Returns true 
	//   a. In case checkCreationPossible is false check for existence and writable permissions.
	//   b. In case checkCreationPossible is true and path is not existence, check permissions of parent folder.
	//
	// Create - Create a file if possible, return true if successful. Return false if the file
	// already exists. Throw an XMP_Error exception if the file cannot be created or if the path
	// already exists but is not a file.
	//
	// GetModifyDate - Return the file system modification date. Returns false if the file or folder
	// does not exist.
	//
	// CreateTemp - Create a (presumably) temporary file related to some other file. The source
	// file path is passed in, a derived name is selected in the same folder. The source file need
	// not exist, but all folders in the path must exist. The derived name is guaranteed to not
	// already exist. A limited number of attempts are made to select a derived name. Returns the
	// temporary file path if successful. Throws an XMP_Error exception if no derived name is found
	// of if the temporary file cannot be created.
	//
	// Open - Open a file for read-only or read-write access. Returns the host-specific FileRef if
	// successful, returns noFileRef if the path does not exist. Throws an XMP_Error exception for
	// other errors.
	//
	// Close - Close a file. Does nothing if the FileRef is noFileRef. Throws an XMP_Error
	// exception for any errors.
	//
	// SwapData - Swap the contents of two files. Both should be closed. Used as part of safe-save
	// operations. On Mac, also swaps all non-data forks. Ideally just the contents should be
	// swapped, but a 3-way rename will be used instead of reading and writing the contents. Uses a
	// host file-swap service if available, even if that swaps more than the contents. Throws an
	// XMP_Error exception for any errors.
	//
	// Rename - Rename a file or folder. The new path must not exist. Throws an XMP_Error exception
	// for any errors.
	//
	// Delete - Deletes a file or folder. Does nothing if the path does not exist. Throws an
	// XMP_Error exception for any errors.
	//
	// Seek - Change the I/O position of an open file, returning the new absolute offset. Uses the
	// native host behavior for seeking beyond EOF. Throws an XMP_Error exception for any errors.
	//
	// Read - Read into a buffer returning the number of bytes read. Requests are limited to less
	// than 2GB in case the host uses an SInt32 count. Throws an XMP_Error exception for errors.
	// Reaching EOF or being at EOF is not an error.
	//
	// Write - Write from a buffer. Requests are limited to less than 2GB in case the host uses an
	// SInt32 count. Throws an XMP_Error exception for any errors.
	//
	// Length - Returns the length of an open file in bytes. The I/O position is not changed.
	// Throws an XMP_Error exception for any errors.
	//
	// SetEOF - Sets a new EOF offset. The I/O position may be changed. Throws an XMP_Error
	// exception for any errors.

	#if XMP_WinBuild
		typedef HANDLE FileRef;
		static const FileRef noFileRef = INVALID_HANDLE_VALUE;
	#elif XMP_MacBuild
		typedef FSIORefNum FileRef;
		static const FileRef noFileRef = -1;
	#elif XMP_UNIXBuild | XMP_iOSBuild
		typedef int FileRef;
		static const FileRef noFileRef = -1;
	#endif

	bool Exists ( const char* filePath );
	bool Writable ( const char* path, bool checkCreationPossible = false);
	bool Create ( const char* filePath );	// Returns true if file exists or was created.
	
	bool GetModifyDate ( const char* filePath, XMP_DateTime* modifyDate );

	std::string CreateTemp ( const char* sourcePath );

	enum { openReadOnly = true, openReadWrite = false };

	FileRef	Open   ( const char* filePath, bool readOnly );
	void	Close  ( FileRef file );

	void    SwapData ( const char* sourcePath, const char* destPath );
	void	Rename   ( const char* oldPath, const char* newPath );
	void	Delete   ( const char* filePath );

	XMP_Int64	Seek     ( FileRef file, XMP_Int64 offset, SeekMode mode );
	XMP_Uns32	Read     ( FileRef file, void* buffer, XMP_Uns32 count );
	void		Write    ( FileRef file, const void* buffer, XMP_Uns32 count );
	XMP_Int64	Length   ( FileRef file );
	void		SetEOF   ( FileRef file, XMP_Int64 length );

	inline XMP_Int64 Offset ( FileRef file ) { return Host_IO::Seek ( file, 0, kXMP_SeekFromCurrent ); };
	inline XMP_Int64 Rewind ( FileRef file ) { return Host_IO::Seek ( file, 0, kXMP_SeekFromStart ); };	// Always returns 0.
	inline XMP_Int64 ToEOF  ( FileRef file ) { return Host_IO::Seek ( file, 0, kXMP_SeekFromEnd ); };

	// =============================================================================================
	// Folder operations
	// =================
	//
	// ! The folder operations may be used anywhere.
	//
	// GetFileMode - Returns an enum telling if a path names a file, folder, other, or nothing.
	// Never throws an exception.
	//
	// GetChildMode - Same as GetFileMode, but has separate parent path and child name parameters.
	//
	// OpenFolder - Initializes the iteration of a folder.
	//
	// CloseFolder - Terminates the iteration of a folder.
	//
	// GetNextChild - Steps an iteration of a folder. Returns false at the end. Otherwise returns
	// true and the local name of the next child. All names starting with '.' are skipped.
	//
	// AutoFolder - A utility class to make sure a folder iteration is terminated at scope exit.

	enum { kFMode_DoesNotExist, kFMode_IsFile, kFMode_IsFolder, kFMode_IsOther };
	typedef XMP_Uns8 FileMode;

	FileMode GetFileMode  ( const char * path );
	FileMode GetChildMode ( const char * parentPath, const char * childName );

	#if XMP_WinBuild
		typedef HANDLE FolderRef;
		static const FolderRef noFolderRef = INVALID_HANDLE_VALUE;
	#elif XMP_MacBuild
		typedef DIR* FolderRef;
		static const FolderRef noFolderRef = 0;
	#elif XMP_UNIXBuild | XMP_iOSBuild
		typedef DIR* FolderRef;
		static const FolderRef noFolderRef = 0;
	#endif

	FolderRef OpenFolder   ( const char* folderPath );
	void      CloseFolder  ( FolderRef folder );
	bool      GetNextChild ( FolderRef folder, std::string* childName );

	class AutoFolder {	// Used to make sure folder is closed at scope exit.
	public:
		FolderRef folder;
		AutoFolder() : folder(noFolderRef) {};
		~AutoFolder() { this->Close(); };
		void Close() { CloseFolder ( this->folder ); this->folder = noFolderRef; };
	};

};

#endif	// __Host_IO_hpp__
