// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/Basic_Handler.hpp"

using namespace std;

// =================================================================================================
/// \file Basic_Handler.cpp
/// \brief Base class for basic handlers that only process in-place XMP.
///
/// This header ...
///
// =================================================================================================

// =================================================================================================
// Basic_MetaHandler::~Basic_MetaHandler
// =====================================

Basic_MetaHandler::~Basic_MetaHandler()
{
	// ! Inherit the base cleanup.

}	// Basic_MetaHandler::~Basic_MetaHandler

// =================================================================================================
// Basic_MetaHandler::UpdateFile
// =============================

// ! This must be called from the destructor for all derived classes. It can't be called from the
// ! Basic_MetaHandler destructor, by then calls to the virtual functions would not go to the
// ! actual implementations for the derived class.

void Basic_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	IgnoreParam ( doSafeUpdate );
	XMP_Assert ( ! doSafeUpdate );	// Not supported at this level.
	if ( ! this->needsUpdate ) return;

	XMP_IO* fileRef = this->parent->ioRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;
	std::string &    xmpPacket  = this->xmpPacket;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	this->CaptureFileEnding ( fileRef );	// ! Do this first, before any location info changes.
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "Basic_MetaHandler::UpdateFile - User abort", kXMPErr_UserAbort );
	}

	this->NoteXMPRemoval ( fileRef );
	this->ShuffleTrailingContent ( fileRef );
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "Basic_MetaHandler::UpdateFile - User abort", kXMPErr_UserAbort );
	}

	XMP_Int64 tempLength = this->xmpFileOffset - this->xmpPrefixSize + this->trailingContentSize;
	fileRef->Truncate ( tempLength );

	packetInfo.offset = tempLength + this->xmpPrefixSize;
	this->NoteXMPInsertion ( fileRef );

	fileRef->ToEOF();
	this->WriteXMPPrefix ( fileRef );
	fileRef->Write ( xmpPacket.c_str(), (XMP_StringLen)xmpPacket.size() );
	this->WriteXMPSuffix ( fileRef );
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "Basic_MetaHandler::UpdateFile - User abort", kXMPErr_UserAbort );
	}

	this->RestoreFileEnding ( fileRef );

	this->xmpFileOffset = packetInfo.offset;
	this->xmpFileSize = packetInfo.length;
	this->needsUpdate = false;

}	// Basic_MetaHandler::UpdateFile

// =================================================================================================
// Basic_MetaHandler::WriteTempFile
// ================================

// *** What about computing the new file length and pre-allocating the file?

void Basic_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_IO* originalRef = this->parent->ioRef;

	// Capture the "back" of the original file.

	this->CaptureFileEnding ( originalRef );
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "Basic_MetaHandler::UpdateFile - User abort", kXMPErr_UserAbort );
	}

	// Seek to the beginning of the original and temp files, truncate the temp.

	originalRef->Rewind();
	tempRef->Rewind();
	tempRef->Truncate ( 0 );

	// Copy the front of the original file to the temp file. Note the XMP (pseudo) removal and
	// insertion. This mainly updates info about the new XMP length.

	XMP_Int64 xmpSectionOffset = this->xmpFileOffset - this->xmpPrefixSize;
	XMP_Int32 oldSectionLength = this->xmpPrefixSize + this->xmpFileSize + this->xmpSuffixSize;

	XIO::Copy ( originalRef, tempRef, xmpSectionOffset, abortProc, abortArg );
	this->NoteXMPRemoval ( originalRef );
	packetInfo.offset = this->xmpFileOffset;	// ! The packet offset does not change.
	this->NoteXMPInsertion ( tempRef );
	tempRef->ToEOF();
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "Basic_MetaHandler::WriteFile - User abort", kXMPErr_UserAbort );
	}

	// Write the new XMP section to the temp file.

	this->WriteXMPPrefix ( tempRef );
	tempRef->Write ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	this->WriteXMPSuffix ( tempRef );
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "Basic_MetaHandler::WriteFile - User abort", kXMPErr_UserAbort );
	}

	// Copy the trailing file content from the original and write the "back" of the file.

	XMP_Int64 remainderOffset = xmpSectionOffset + oldSectionLength;

	originalRef->Seek ( remainderOffset, kXMP_SeekFromStart );
	XIO::Copy ( originalRef, tempRef, this->trailingContentSize, abortProc, abortArg );
	this->RestoreFileEnding ( tempRef );

	// Done.

	this->xmpFileOffset = packetInfo.offset;
	this->xmpFileSize = packetInfo.length;
	this->needsUpdate = false;

}	// Basic_MetaHandler::WriteTempFile

// =================================================================================================
// ShuffleTrailingContent
// ======================
//
// Shuffle the trailing content portion of a file forward. This does not include the final "back"
// portion of the file, just the arbitrary length content between the XMP section and the back.
// Don't use XIO::Copy, that assumes separate files and hence separate I/O positions.

// ! The XMP packet location and prefix/suffix sizes must still reflect the XMP section that is in
// ! the process of being removed.

void Basic_MetaHandler::ShuffleTrailingContent ( XMP_IO* fileRef )
{
	XMP_Int64 readOffset  = this->packetInfo.offset + xmpSuffixSize;
	XMP_Int64 writeOffset = this->packetInfo.offset - xmpPrefixSize;

	XMP_Int64 remainingLength = this->trailingContentSize;

	enum { kBufferSize = 64*1024 };
	char buffer [kBufferSize];

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	while ( remainingLength > 0 ) {

		XMP_Int32 ioCount = kBufferSize;
		if ( remainingLength < kBufferSize ) ioCount = (XMP_Int32)remainingLength;

		fileRef->Seek ( readOffset, kXMP_SeekFromStart );
		fileRef->ReadAll ( buffer, ioCount );
		fileRef->Seek ( writeOffset, kXMP_SeekFromStart );
		fileRef->Write ( buffer, ioCount );

		readOffset  += ioCount;
		writeOffset += ioCount;
		remainingLength -= ioCount;

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "Basic_MetaHandler::ShuffleTrailingContent - User abort", kXMPErr_UserAbort );
		}

	}

}	// ShuffleTrailingContent

// =================================================================================================
// Dummies needed for VS.Net
// =========================

void Basic_MetaHandler::WriteXMPPrefix ( XMP_IO* fileRef )
{
	XMP_Throw ( "Basic_MetaHandler::WriteXMPPrefix - Needs specific override", kXMPErr_InternalFailure );
}

void Basic_MetaHandler::WriteXMPSuffix ( XMP_IO* fileRef )
{
	XMP_Throw ( "Basic_MetaHandler::WriteXMPSuffix - Needs specific override", kXMPErr_InternalFailure );
}

void Basic_MetaHandler::NoteXMPRemoval ( XMP_IO* fileRef )
{
	XMP_Throw ( "Basic_MetaHandler::NoteXMPRemoval - Needs specific override", kXMPErr_InternalFailure );
}

void Basic_MetaHandler::NoteXMPInsertion ( XMP_IO* fileRef )
{
	XMP_Throw ( "Basic_MetaHandler::NoteXMPInsertion - Needs specific override", kXMPErr_InternalFailure );
}

void Basic_MetaHandler::CaptureFileEnding ( XMP_IO* fileRef )
{
	XMP_Throw ( "Basic_MetaHandler::CaptureFileEnding - Needs specific override", kXMPErr_InternalFailure );
}

void Basic_MetaHandler::RestoreFileEnding ( XMP_IO* fileRef )
{
	XMP_Throw ( "Basic_MetaHandler::RestoreFileEnding - Needs specific override", kXMPErr_InternalFailure );
}

// =================================================================================================
