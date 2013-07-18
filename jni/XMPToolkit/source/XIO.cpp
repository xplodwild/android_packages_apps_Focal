// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "source/XIO.hpp"
#include "source/XMP_LibUtils.hpp"
#include "source/UnicodeConversions.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false' (performance warning)
#endif

// =================================================================================================

static inline void MakeLowerCase ( std::string * str )
{
	for ( size_t i = 0, limit = str->size(); i < limit; ++i ) {
		char ch = (*str)[i];
		if ( ('A' <= ch) && (ch <= 'Z') ) (*str)[i] += 0x20;
	}
}

// =================================================================================================
// XIO::SplitLeafName
// ==================

void XIO::SplitLeafName ( std::string * path, std::string * leafName )
{
	size_t dirPos = path->size();
	// Return if path is empty or just the slash
	if ( dirPos == 0 || (dirPos == 1 && (*path)[dirPos-1] == kDirChar) ) 
	{
		leafName->erase();
		path->erase();
		return;
	}

	// Remove trailing slashes
	--dirPos;
#if XMP_WinBuild
	if ( (*path)[dirPos] == '/' ) (*path)[dirPos] = kDirChar;	// Tolerate both '\' and '/'.
#endif
	if ( (*path)[dirPos] == kDirChar )
	{
		path->erase(dirPos);
	}

	// Search next slash
	for ( --dirPos; dirPos > 0; --dirPos ) {
		#if XMP_WinBuild
			if ( (*path)[dirPos] == '/' ) (*path)[dirPos] = kDirChar;	// Tolerate both '\' and '/'.
		#endif
		if ( (*path)[dirPos] == kDirChar ) break;
	}

	if ( (*path)[dirPos] == kDirChar ) {
		leafName->assign ( &(*path)[dirPos+1] );
		path->erase ( dirPos );
	} else if ( dirPos == 0 ) {
		leafName->erase();
		leafName->swap ( *path );
	}

}	// XIO::SplitLeafName

// =================================================================================================
// XIO::SplitFileExtension
// =======================
//
// ! Must only be called after using SplitLeafName!

void XIO::SplitFileExtension ( std::string * leafName, std::string * fileExt )
{

	fileExt->erase();

	size_t extPos = leafName->size();
	if ( extPos == 0 ) return;
	
	for ( --extPos; extPos > 0; --extPos ) if ( (*leafName)[extPos] == '.' ) break;

	if ( (*leafName)[extPos] == '.' ) {
		fileExt->assign ( &((*leafName)[extPos+1]) );
		MakeLowerCase ( fileExt );
		leafName->erase ( extPos );
	}

}	// XIO::SplitFileExtension

// =================================================================================================
// XIO::ReplaceTextFile
// ====================
//
// Replace the contents of a text file in a manner that preserves the meaning of the old content if
// a disk full error occurs. This can mean appended spaces appear in the old content.

void XIO::ReplaceTextFile ( XMP_IO* textFile, const std::string & newContent, bool doSafeUpdate )
{
	XMP_Int64 newContentSize = (XMP_Int64)newContent.size();
	XMP_Enforce ( newContentSize <= (XMP_Int64)0xFFFFFFFFULL );	// Make sure it fits in UInt32 for Write.

	if ( doSafeUpdate ) {
	
		// Safe updates are no problem, the old content is untouched if the temp file write fails.

		XMP_IO* tempFile = textFile->DeriveTemp();
		tempFile->Write ( newContent.data(), (XMP_Uns32)newContentSize );
		textFile->AbsorbTemp();

	} else {
	
		// We're overwriting the existing file. Make sure it is big enough, write the new content,
		// then truncate if necessary.
		
		XMP_Int64 oldContentSize = textFile->Length();
		
		if ( oldContentSize < newContentSize ) {
			size_t spaceCount = (size_t) (newContentSize - oldContentSize);	// AUDIT: newContentSize fits in UInt32.
			std::string spaces;
			spaces.assign ( spaceCount, ' ' );
			textFile->ToEOF();
			textFile->Write ( spaces.data(), (XMP_Uns32)spaceCount );
		}

		XMP_Assert ( newContentSize <= textFile->Length() );
		textFile->Rewind();
		textFile->Write ( newContent.data(), (XMP_Uns32)newContentSize );
		
		if ( oldContentSize > newContentSize ) textFile->Truncate ( newContentSize );
	
	}

}	// XIO::ReplaceTextFile

// =================================================================================================
// XIO::Copy
// =========

void XIO::Copy ( XMP_IO* sourceFile, XMP_IO* destFile, XMP_Int64 length,
				 XMP_AbortProc abortProc /* = 0 */, void* abortArg /* = 0 */ )
{
	const bool checkAbort = (abortProc != 0);
	XMP_Uns8 buffer [64*1024];

	while ( length > 0 ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "XIO::Copy, user abort", kXMPErr_UserAbort );
		}

		XMP_Int32 ioCount = sizeof(buffer);
		if ( length < ioCount ) ioCount = (XMP_Int32)length;

		sourceFile->Read ( buffer, ioCount, XMP_IO::kReadAll );
		destFile->Write ( buffer, ioCount );
		length -= ioCount;

	}

}	// XIO::Copy

// =================================================================================================
// XIO::Move
// =========

// allows to move data within a file (just pass in the same file handle as srcFile and dstFile)
// shadow effects (stumbling over just-written data) are avoided.
//
// * however can also be used to move data between two files *
// (having both option is handy for flexible use in update()/re-write() handler routines)

void XIO::Move ( XMP_IO* srcFile, XMP_Int64 srcOffset,
				 XMP_IO* dstFile, XMP_Int64 dstOffset,
				 XMP_Int64 length, XMP_AbortProc abortProc /* = 0 */, void * abortArg /* = 0 */ )
{
	enum { kBufferLen = 64*1024 };
	XMP_Uns8 buffer [kBufferLen];

	const bool checkAbort = (abortProc != 0);

	if ( srcOffset > dstOffset ) {	// avoiding shadow effects

	// move down -> shift lowest packet first !

		while ( length > 0 ) {

			if ( checkAbort && abortProc(abortArg) ) XMP_Throw ( "XIO::Move - User abort", kXMPErr_UserAbort );
			XMP_Int32 ioCount = kBufferLen;
			if ( length < kBufferLen ) ioCount = (XMP_Int32)length; //smartly avoids 32/64 bit issues

			srcFile->Seek ( srcOffset, kXMP_SeekFromStart );
			srcFile->ReadAll ( buffer, ioCount );
			dstFile->Seek ( dstOffset, kXMP_SeekFromStart );
			dstFile->Write ( buffer, ioCount );
			length -= ioCount;

			srcOffset += ioCount;
			dstOffset += ioCount;

		}

	} else {	// move up -> shift highest packet first

		srcOffset += length; //move to end
		dstOffset += length;

		while ( length > 0 ) {

			if ( checkAbort && abortProc(abortArg) ) XMP_Throw ( "XIO::Move - User abort", kXMPErr_UserAbort );
			XMP_Int32 ioCount = kBufferLen;
			if ( length < kBufferLen ) ioCount = (XMP_Int32)length; //smartly avoids 32/64 bit issues

			srcOffset -= ioCount;
			dstOffset -= ioCount;

			srcFile->Seek ( srcOffset, kXMP_SeekFromStart );
			srcFile->ReadAll ( buffer, ioCount );
			dstFile->Seek ( dstOffset, kXMP_SeekFromStart );
			dstFile->Write ( buffer, ioCount );
			length -= ioCount;

		}

	}

}	// XIO::Move

// =================================================================================================
