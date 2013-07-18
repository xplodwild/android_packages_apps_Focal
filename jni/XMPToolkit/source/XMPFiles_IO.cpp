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

#include "source/XMP_LibUtils.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"


#define EMPTY_FILE_PATH ""
#define XMP_FILESIO_STATIC_START try { int a;
#define XMP_FILESIO_STATIC_END1(errorCallbackPtr, filePath, severity)											\
		a = 1;																									\
	} catch ( XMP_Error & error ) {																				\
		if ( (errorCallbackPtr) != NULL ) (errorCallbackPtr)->NotifyClient ( (severity), error, (filePath) );	\
		else throw;																								\
	}
#define XMP_FILESIO_START try { int b;
#define XMP_FILESIO_END1(severity)																				\
		b = 1;																									\
	} catch ( XMP_Error & error ) {																				\
		if ( errorCallback != NULL ) errorCallback->NotifyClient ( (severity), error, filePath.c_str() );		\
		else throw;																								\
	}
#define XMP_FILESIO_END2(filePath, severity)																	\
		b = 1;																									\
	} catch ( XMP_Error & error ) {																				\
		if ( errorCallback != NULL ) errorCallback->NotifyClient ( (severity), error, (filePath) );				\
		else throw;																								\
	}
#define XMP_FILESIO_STATIC_NOTIFY_ERROR(errorCallbackPtr, filePath, severity, error)							\
	if ( (errorCallbackPtr) != NULL ) errorCallbackPtr->NotifyClient ( (severity), (error), (filePath) );
#define XMP_FILESIO_NOTIFY_ERROR(filePath, severity, error)														\
	XMP_FILESIO_STATIC_NOTIFY_ERROR(errorCallback, (filePath), (severity), (error))


// =================================================================================================
// XMPFiles_IO::New_XMPFiles_IO
// ============================

/* class static */
XMPFiles_IO * XMPFiles_IO::New_XMPFiles_IO (
	const char * filePath,
	bool readOnly,
	GenericErrorCallback * _errorCallback,
	XMP_ProgressTracker * _progressTracker )
{
	XMP_FILESIO_STATIC_START
	Host_IO::FileRef hostFile = Host_IO::noFileRef;
	
	switch ( Host_IO::GetFileMode ( filePath ) ) {
		case Host_IO::kFMode_IsFile:
			hostFile = Host_IO::Open ( filePath, readOnly );
			break;
		case Host_IO::kFMode_DoesNotExist:
			break;
		default:
			XMP_Throw ( "New_XMPFiles_IO, path must be a file or not exist", kXMPErr_FilePathNotAFile );
	}
	if ( hostFile == Host_IO::noFileRef ) {
		XMP_Error error (kXMPErr_NoFile, "New_XMPFiles_IO, file does not exist");
		XMP_FILESIO_STATIC_NOTIFY_ERROR ( _errorCallback, filePath, kXMPErrSev_Recoverable, error );
		return 0;
	}

	Host_IO::Rewind ( hostFile );	// Make sure offset really is 0.

	XMPFiles_IO * newFile = new XMPFiles_IO ( hostFile, filePath, readOnly, _errorCallback, _progressTracker );
	return newFile;
	XMP_FILESIO_STATIC_END1 ( _errorCallback, filePath, kXMPErrSev_FileFatal )
	return NULL;

}	// XMPFiles_IO::New_XMPFiles_IO

// =================================================================================================
// XMPFiles_IO::XMPFiles_IO
// ========================

XMPFiles_IO::XMPFiles_IO (
	Host_IO::FileRef hostFile,
	const char * _filePath,
	bool _readOnly,
	GenericErrorCallback * _errorCallback,
	XMP_ProgressTracker * _progressTracker )
	: readOnly(_readOnly)
	, filePath(_filePath)
	, fileRef(hostFile)
	, currOffset(0)
	, isTemp(false)
	, derivedTemp(0)
	, errorCallback(_errorCallback)
	, progressTracker(_progressTracker)
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );

	this->currLength = Host_IO::Length ( this->fileRef );
	XMP_FILESIO_END2 ( _filePath, kXMPErrSev_FileFatal )
}	// XMPFiles_IO::XMPFiles_IO

// =================================================================================================
// XMPFiles_IO::~XMPFiles_IO
// =========================

XMPFiles_IO::~XMPFiles_IO()
{
	try {
		XMP_FILESIO_START
		if ( this->derivedTemp != 0 ) this->DeleteTemp();
		if ( this->fileRef != Host_IO::noFileRef ) Host_IO::Close ( this->fileRef );
		if ( this->isTemp && (! this->filePath.empty()) ) Host_IO::Delete ( this->filePath.c_str() );
		XMP_FILESIO_END1 ( kXMPErrSev_Recoverable )
	} catch ( ... ) {
		// All of the above is fail-safe cleanup, ignore problems.
	}
}	// XMPFiles_IO::~XMPFiles_IO

// =================================================================================================
// XMPFiles_IO::operator=
// ======================

void XMPFiles_IO::operator = ( const XMP_IO& in )
{
	XMP_FILESIO_START
	XMP_Throw ( "No assignment for XMPFiles_IO", kXMPErr_InternalFailure );
	XMP_FILESIO_END1 ( kXMPErrSev_OperationFatal )

};	// XMPFiles_IO::operator=

// =================================================================================================
// XMPFiles_IO::Read
// =================

XMP_Uns32 XMPFiles_IO::Read ( void * buffer, XMP_Uns32 count, bool readAll /* = false */ )
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );
	XMP_Assert ( this->currOffset == Host_IO::Offset ( this->fileRef ) );
	XMP_Assert ( this->currLength == Host_IO::Length ( this->fileRef ) );
	XMP_Assert ( this->currOffset <= this->currLength );

	if ( count > (this->currLength - this->currOffset) ) {
		if ( readAll ) XMP_Throw ( "XMPFiles_IO::Read, not enough data", kXMPErr_EnforceFailure );
		count = (XMP_Uns32) (this->currLength - this->currOffset);
	}

	XMP_Uns32 amountRead = Host_IO::Read ( this->fileRef, buffer, count );
	XMP_Enforce ( amountRead == count );

	this->currOffset += amountRead;
	return amountRead;
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )
	return 0;

}	// XMPFiles_IO::Read

// =================================================================================================
// XMPFiles_IO::Write
// ==================

void XMPFiles_IO::Write ( const void * buffer, XMP_Uns32 count )
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );
	XMP_Assert ( this->currOffset == Host_IO::Offset ( this->fileRef ) );
	XMP_Assert ( this->currLength == Host_IO::Length ( this->fileRef ) );
	XMP_Assert ( this->currOffset <= this->currLength );

	try {
		if ( this->readOnly )
			XMP_Throw ( "New_XMPFiles_IO, write not permitted on read only file", kXMPErr_FilePermission );
		Host_IO::Write ( this->fileRef, buffer, count );
		if ( this->progressTracker != 0 ) this->progressTracker->AddWorkDone ( (float) count );
	} catch ( ... ) {
		try {
			// we should try to maintain the state as best as possible
			// but no exception should escape from this backup plan.
			// Make sure the internal state reflects partial writes.
			this->currOffset = Host_IO::Offset ( this->fileRef );
			this->currLength = Host_IO::Length ( this->fileRef );
		} catch ( ... ) {
			// don't do anything
		}
		throw;
	}

	this->currOffset += count;
	if ( this->currOffset > this->currLength ) this->currLength = this->currOffset;
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )

}	// XMPFiles_IO::Write

// =================================================================================================
// XMPFiles_IO::Seek
// =================

XMP_Int64 XMPFiles_IO::Seek ( XMP_Int64 offset, SeekMode mode )
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );
	XMP_Assert ( this->currOffset == Host_IO::Offset ( this->fileRef ) );
	XMP_Assert ( this->currLength == Host_IO::Length ( this->fileRef ) );

	XMP_Int64 newOffset = offset;
	if ( mode == kXMP_SeekFromCurrent ) {
		newOffset += this->currOffset;
	} else if ( mode == kXMP_SeekFromEnd ) {
		newOffset += this->currLength;
	}
	XMP_Enforce ( newOffset >= 0 );

	if ( newOffset <= this->currLength ) {
		this->currOffset = Host_IO::Seek ( this->fileRef, offset, mode );
	} else if ( this->readOnly ) {
		XMP_Throw ( "XMPFiles_IO::Seek, read-only seek beyond EOF", kXMPErr_EnforceFailure );
	} else {
		Host_IO::SetEOF ( this->fileRef, newOffset );	// Extend a file open for writing.
		this->currLength = newOffset;
		this->currOffset = Host_IO::Seek ( this->fileRef, 0, kXMP_SeekFromEnd );
	}

	XMP_Assert ( this->currOffset == newOffset );
	return this->currOffset;
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal );
	return -1;

}	// XMPFiles_IO::Seek

// =================================================================================================
// XMPFiles_IO::Length
// ===================

XMP_Int64 XMPFiles_IO::Length()
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );
	XMP_Assert ( this->currOffset == Host_IO::Offset ( this->fileRef ) );
	XMP_Assert ( this->currLength == Host_IO::Length ( this->fileRef ) );
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )
	return this->currLength;

}	// XMPFiles_IO::Length

// =================================================================================================
// XMPFiles_IO::Truncate
// =====================

void XMPFiles_IO::Truncate ( XMP_Int64 length )
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );
	XMP_Assert ( this->currOffset == Host_IO::Offset ( this->fileRef ) );
	XMP_Assert ( this->currLength == Host_IO::Length ( this->fileRef ) );

	if ( this->readOnly )
		XMP_Throw ( "New_XMPFiles_IO, truncate not permitted on read only file", kXMPErr_FilePermission );

	XMP_Enforce ( length <= this->currLength );
	Host_IO::SetEOF ( this->fileRef, length );

	this->currLength = length;
	if ( this->currOffset > this->currLength ) this->currOffset = this->currLength;

	// ! Seek to the expected offset, some versions of Host_IO::SetEOF implicitly seek to EOF.
	Host_IO::Seek ( this->fileRef, this->currOffset, kXMP_SeekFromStart );
	XMP_Assert ( this->currOffset == Host_IO::Offset ( this->fileRef ) );
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )

}	// XMPFiles_IO::Truncate

// =================================================================================================
// XMPFiles_IO::DeriveTemp
// =======================

XMP_IO* XMPFiles_IO::DeriveTemp()
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );

	if ( this->derivedTemp != 0 ) return this->derivedTemp;

	if ( this->readOnly ) {
		XMP_Throw ( "XMPFiles_IO::DeriveTemp, can't derive from read-only", kXMPErr_InternalFailure );
	}
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )

	std::string tempPath;

	XMP_FILESIO_START
	tempPath = Host_IO::CreateTemp ( this->filePath.c_str() );

	XMPFiles_IO* newTemp = XMPFiles_IO::New_XMPFiles_IO ( tempPath.c_str(), Host_IO::openReadWrite );
	if ( newTemp == 0 ) {
		Host_IO::Delete ( tempPath.c_str() );
		XMP_Throw ( "XMPFiles_IO::DeriveTemp, can't open temp file", kXMPErr_InternalFailure );
	}

	newTemp->isTemp = true;
	this->derivedTemp = newTemp;
	newTemp->progressTracker = this->progressTracker;	// Automatically track writes to the temp file.
	XMP_FILESIO_END2 ( tempPath.c_str(), kXMPErrSev_FileFatal )
	return this->derivedTemp;

}	// XMPFiles_IO::DeriveTemp

// =================================================================================================
// XMPFiles_IO::AbsorbTemp
// =======================

void XMPFiles_IO::AbsorbTemp()
{
	XMP_FILESIO_START
	XMP_Assert ( this->fileRef != Host_IO::noFileRef );

	XMPFiles_IO * temp = this->derivedTemp;
	if ( temp == 0 ) {
		XMP_Throw ( "XMPFiles_IO::AbsorbTemp, no temp to absorb", kXMPErr_InternalFailure );
	}
	XMP_Assert ( temp->isTemp );

	this->Close();
	temp->Close();

	Host_IO::SwapData ( this->filePath.c_str(), temp->filePath.c_str() );
	this->DeleteTemp();

	this->fileRef = Host_IO::Open ( this->filePath.c_str(), Host_IO::openReadWrite );
	this->currLength = Host_IO::Length ( this->fileRef );
	this->currOffset = 0;
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )

}	// XMPFiles_IO::AbsorbTemp

// =================================================================================================
// XMPFiles_IO::DeleteTemp
// =======================

void XMPFiles_IO::DeleteTemp()
{
	XMP_FILESIO_START
	XMPFiles_IO * temp = this->derivedTemp;

	if ( temp != 0 ) {

		if ( temp->fileRef != Host_IO::noFileRef ) {
			Host_IO::Close ( temp->fileRef );
			temp->fileRef = Host_IO::noFileRef;
		}

		if ( ! temp->filePath.empty() ) {
			Host_IO::Delete ( temp->filePath.c_str() );
			temp->filePath.erase();
		}

		delete temp;
		this->derivedTemp = 0;

	}
	XMP_FILESIO_END2 ( this->derivedTemp->filePath.c_str(), kXMPErrSev_FileFatal )

}	// XMPFiles_IO::DeleteTemp

// =================================================================================================
// XMPFiles_IO::Close
// ==================

void XMPFiles_IO::Close()
{
	XMP_FILESIO_START
	if ( this->fileRef != Host_IO::noFileRef ) {
		Host_IO::Close ( this->fileRef );
		this->fileRef = Host_IO::noFileRef;
	}
	XMP_FILESIO_END1 ( kXMPErrSev_FileFatal )

}	// XMPFiles_IO::Close

// =================================================================================================
