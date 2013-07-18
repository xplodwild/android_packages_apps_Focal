// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2005 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"
#include "source/IOUtils.hpp"

#include "XMPFiles/source/FileHandlers/MPEG2_Handler.hpp"
#include "../FormatSupport/PackageFormat_Support.hpp"
using namespace std;

// =================================================================================================
/// \file MPEG2_Handler.cpp
/// \brief File format handler for MPEG2.
///
/// BLECH! YUCK! GAG! MPEG-2 is done using a sidecar and recognition only by file extension! BARF!!!!!
///
// =================================================================================================

// =================================================================================================
// FindFileExtension
// =================

static inline XMP_StringPtr FindFileExtension ( XMP_StringPtr filePath )
{

	XMP_StringPtr pathEnd = filePath + strlen(filePath);
	XMP_StringPtr extPtr;

	for ( extPtr = pathEnd-1; extPtr > filePath; --extPtr ) {
		if ( (*extPtr == '.') || (*extPtr == '/') ) break;
		#if XMP_WinBuild
			if ( (*extPtr == '\\') || (*extPtr == ':') ) break;
		#endif
	}

	if ( (extPtr < filePath) || (*extPtr != '.') ) return pathEnd;
	return extPtr;

}	// FindFileExtension

// =================================================================================================
// MPEG2_MetaHandlerCTor
// =====================

XMPFileHandler * MPEG2_MetaHandlerCTor ( XMPFiles * parent )
{
	return new MPEG2_MetaHandler ( parent );

}	// MPEG2_MetaHandlerCTor

// =================================================================================================
// MPEG2_CheckFormat
// =================

// The MPEG-2 handler uses just the file extension, not the file content. Worse yet, it also uses a
// sidecar file for the XMP. This works better if the handler owns the file, we open the sidecar
// instead of the actual MPEG-2 file.

bool MPEG2_CheckFormat ( XMP_FileFormat format,
						XMP_StringPtr  filePath,
						XMP_IO*    fileRef,
						XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath); IgnoreParam(fileRef);

	XMP_Assert ( (format == kXMP_MPEGFile) || (format == kXMP_MPEG2File) );
	XMP_Assert ( fileRef == 0 );

	return ( (parent->format == kXMP_MPEGFile) || (parent->format == kXMP_MPEG2File) );	// ! Just use the first call's format hint.

}	// MPEG2_CheckFormat

// =================================================================================================
// MPEG2_MetaHandler::MPEG2_MetaHandler
// ====================================

MPEG2_MetaHandler::MPEG2_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kMPEG2_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	XMP_StringPtr filePath = this->parent->GetFilePath().c_str();
	XMP_StringPtr extPtr = FindFileExtension ( filePath );
	this->sidecarPath.assign ( filePath, (extPtr - filePath) );
	this->sidecarPath += ".xmp";
}	// MPEG2_MetaHandler::MPEG2_MetaHandler

// =================================================================================================
// MPEG2_MetaHandler::~MPEG2_MetaHandler
// =====================================

MPEG2_MetaHandler::~MPEG2_MetaHandler()
{
	// Nothing to do.

}	// MPEG2_MetaHandler::~MPEG2_MetaHandler

// =================================================================================================
// MPEG2_MetaHandler::GetFileModDate
// =================================

bool MPEG2_MetaHandler::GetFileModDate ( XMP_DateTime * modDate )
{
	if ( ! Host_IO::Exists ( this->sidecarPath.c_str() ) ) return false;
	return Host_IO::GetModifyDate ( this->sidecarPath.c_str(), modDate );

}	// MPEG2_MetaHandler::GetFileModDate

// =================================================================================================
// MPEG2_MetaHandler::FillAssociatedResources
// =================================
void MPEG2_MetaHandler::FillAssociatedResources ( std::vector<std::string> * resourceList )
{
	resourceList->push_back(this->parent->GetFilePath());
	PackageFormat_Support::AddResourceIfExists(resourceList, this->sidecarPath);
}	// MPEG2_MetaHandler::FillAssociatedResources

// =================================================================================================
// MPEG2_MetaHandler::IsMetadataWritable
// =================================
bool MPEG2_MetaHandler::IsMetadataWritable ( )
{
	return Host_IO::Writable( this->sidecarPath.c_str(), true );
}	// MPEG2_MetaHandler::IsMetadataWritable

// =================================================================================================
// MPEG2_MetaHandler::CacheFileData
// ================================

void MPEG2_MetaHandler::CacheFileData()
{
	bool readOnly = (! (this->parent->openFlags & kXMPFiles_OpenForUpdate));

	if ( this->parent->UsesClientIO() ) {
		XMP_Throw ( "MPEG2 cannot be used with client-managed I/O", kXMPErr_InternalFailure );
	}

	this->containsXMP = false;
	this->processedXMP = true;	// Whatever we do here is all that we do for XMPFiles::OpenFile.

	// Try to open the sidecar XMP file. Tolerate an open failure, there might not be any XMP.
	// Note that MPEG2_CheckFormat can't save the sidecar path because the handler doesn't exist then.

	if ( ! Host_IO::Exists ( this->sidecarPath.c_str() ) ) return;	// OK to not have XMP.

	XMPFiles_IO * localFile = XMPFiles_IO::New_XMPFiles_IO ( this->sidecarPath.c_str(), readOnly );
	if ( localFile == 0 ) XMP_Throw ( "Failure opening MPEG-2 XMP file", kXMPErr_ExternalFailure );
	this->parent->ioRef = localFile;

	// Extract the sidecar's contents and parse.

	this->packetInfo.offset = 0;	// We take the whole sidecar file.
	this->packetInfo.length = (XMP_Int32) localFile->Length();

	if ( this->packetInfo.length > 0 ) {

		this->xmpPacket.assign ( this->packetInfo.length, ' ' );
		localFile->ReadAll ( (void*)this->xmpPacket.c_str(), this->packetInfo.length );

		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
		this->containsXMP = true;

	}

	if ( readOnly ) {
		localFile->Close();
		delete localFile;
		this->parent->ioRef = 0;
	}

}	// MPEG2_MetaHandler::CacheFileData

// =================================================================================================
// MPEG2_MetaHandler::UpdateFile
// =============================

void MPEG2_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;

	XMP_Assert ( this->parent->UsesLocalIO() );

	if ( this->parent->ioRef == 0 ) {
		XMP_Assert ( ! Host_IO::Exists ( this->sidecarPath.c_str() ) );
		Host_IO::Create ( this->sidecarPath.c_str() );
		this->parent->ioRef = XMPFiles_IO::New_XMPFiles_IO ( this->sidecarPath.c_str(), Host_IO::openReadWrite );
		if ( this->parent->ioRef == 0 ) XMP_Throw ( "Failure opening MPEG-2 XMP file", kXMPErr_ExternalFailure );
	}

	XMP_IO* fileRef = this->parent->ioRef;
	XMP_Assert ( fileRef != 0 );
	XIO::ReplaceTextFile ( fileRef, this->xmpPacket, doSafeUpdate );

	XMPFiles_IO* localFile = (XMPFiles_IO*)fileRef;
	localFile->Close();
	delete localFile;
	this->parent->ioRef = 0;

	this->needsUpdate = false;

}	// MPEG2_MetaHandler::UpdateFile

// =================================================================================================
// MPEG2_MetaHandler::WriteTempFile
// ================================

void MPEG2_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	IgnoreParam(tempRef);

	XMP_Throw ( "MPEG2_MetaHandler::WriteTempFile: Should never be called", kXMPErr_Unavailable );

}	// MPEG2_MetaHandler::WriteTempFile
