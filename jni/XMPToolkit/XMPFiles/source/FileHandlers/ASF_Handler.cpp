// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/ASF_Handler.hpp"

// =================================================================================================
/// \file ASF_Handler.hpp
/// \brief File format handler for ASF.
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// ASF_MetaHandlerCTor
// ====================

XMPFileHandler * ASF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new ASF_MetaHandler ( parent );

}	// ASF_MetaHandlerCTor

// =================================================================================================
// ASF_CheckFormat
// ===============

bool ASF_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
                       XMP_IO*    fileRef,
                       XMPFiles *     parent )
{

	IgnoreParam(format); IgnoreParam(fileRef); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_WMAVFile );

	if ( fileRef->Length() < guidLen ) return false;
	GUID guid;

	fileRef->Rewind();
	fileRef->Read ( &guid, guidLen );
	if ( ! IsEqualGUID ( ASF_Header_Object, guid ) ) return false;

	return true;

}	// ASF_CheckFormat

// =================================================================================================
// ASF_MetaHandler::ASF_MetaHandler
// ==================================

ASF_MetaHandler::ASF_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kASF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}

// =================================================================================================
// ASF_MetaHandler::~ASF_MetaHandler
// ===================================

ASF_MetaHandler::~ASF_MetaHandler()
{
	// Nothing extra to do.
}

// =================================================================================================
// ASF_MetaHandler::CacheFileData
// ===============================

void ASF_MetaHandler::CacheFileData()
{

	this->containsXMP = false;

	XMP_IO* fileRef ( this->parent->ioRef );
	if ( fileRef == 0 ) return;

	ASF_Support support ( &this->legacyManager,0 );
	ASF_Support::ObjectState objectState;
	long numTags = support.OpenASF ( fileRef, objectState );
	if ( numTags == 0 ) return;

	if ( objectState.xmpLen != 0 ) {

		// XMP present

		XMP_Int32 len = XMP_Int32 ( objectState.xmpLen );

		this->xmpPacket.reserve( len );
		this->xmpPacket.assign ( len, ' ' );

		bool found = ASF_Support::ReadBuffer ( fileRef, objectState.xmpPos, objectState.xmpLen,
											   const_cast<char *>(this->xmpPacket.data()) );
		if ( found ) {
			this->packetInfo.offset = objectState.xmpPos;
			this->packetInfo.length = len;
			this->containsXMP = true;
		}

	}

}	// ASF_MetaHandler::CacheFileData

// =================================================================================================
// ASF_MetaHandler::ProcessXMP
// ============================
//
// Process the raw XMP and legacy metadata that was previously cached.

void ASF_MetaHandler::ProcessXMP()
{

	this->processedXMP = true;	// Make sure we only come through here once.

	// Process the XMP packet.

	if ( this->xmpPacket.empty() ) {

		// import legacy in any case, when no XMP present
		legacyManager.ImportLegacy ( &this->xmpObj );
		this->legacyManager.SetDigest ( &this->xmpObj );

	} else {

		XMP_Assert ( this->containsXMP );
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = (XMP_StringLen)this->xmpPacket.size();

		this->xmpObj.ParseFromBuffer ( packetStr, packetLen );

		if ( ! legacyManager.CheckDigest ( this->xmpObj ) ) {
			legacyManager.ImportLegacy ( &this->xmpObj );
		}

	}

	// Assume we now have something in the XMP.
	this->containsXMP = true;

}	// ASF_MetaHandler::ProcessXMP

// =================================================================================================
// ASF_MetaHandler::UpdateFile
// ============================

void ASF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{

	bool updated = false;

	if ( ! this->needsUpdate ) return;

	XMP_IO* fileRef ( this->parent->ioRef );
	if ( fileRef == 0 ) return;

	ASF_Support support(0,this->parent->progressTracker);
	ASF_Support::ObjectState objectState;
	long numTags = support.OpenASF ( fileRef, objectState );
	if ( numTags == 0 ) return;

	XMP_StringLen packetLen = (XMP_StringLen)xmpPacket.size();

	this->legacyManager.ExportLegacy ( this->xmpObj );
	if ( this->legacyManager.hasLegacyChanged() ) {

		this->legacyManager.SetDigest ( &this->xmpObj );

		// serialize with updated digest
		if ( objectState.xmpLen == 0 ) {

			// XMP does not exist, use standard padding
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );

		} else {

			// re-use padding with static XMP size
			try {
				XMP_OptionBits compactExact = (kXMP_UseCompactFormat | kXMP_ExactPacketLength);
				this->xmpObj.SerializeToBuffer ( &this->xmpPacket, compactExact, XMP_StringLen(objectState.xmpLen) );
			} catch ( ... ) {
				// re-use padding with exact packet length failed (legacy-digest needed too much space): try again using standard padding
				this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
			}

		}

	}

	XMP_StringPtr packetStr = xmpPacket.c_str();
	packetLen = (XMP_StringLen)xmpPacket.size();
	if ( packetLen == 0 ) return;

	// value, when guessing for sufficient legacy padding (line-ending conversion etc.)
	const int paddingTolerance = 50;

	bool xmpGrows = ( objectState.xmpLen && (packetLen > objectState.xmpLen) && ( ! objectState.xmpIsLastObject) );

	bool legacyGrows = ( this->legacyManager.hasLegacyChanged() &&
						 (this->legacyManager.getLegacyDiff() > (this->legacyManager.GetPadding() - paddingTolerance)) );

	if ( doSafeUpdate || legacyGrows || xmpGrows ) {

		// do a safe update in any case
		updated = SafeWriteFile();

	} else {

		// possibly we can do an in-place update

		if ( objectState.xmpLen < packetLen ) {

			updated = SafeWriteFile();

		} else {
			
			XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
			if ( progressTracker != 0 ) progressTracker->BeginWork ( (float)packetLen );
			
			// current XMP chunk size is sufficient -> write (in place update)
			updated = ASF_Support::WriteBuffer(fileRef, objectState.xmpPos, packetLen, packetStr );

			// legacy update
			if ( updated && this->legacyManager.hasLegacyChanged() ) {

				ASF_Support::ObjectIterator curPos = objectState.objects.begin();
				ASF_Support::ObjectIterator endPos = objectState.objects.end();

				for ( ; curPos != endPos; ++curPos ) {

					ASF_Support::ObjectData object = *curPos;

					// find header-object
					if ( IsEqualGUID ( ASF_Header_Object, object.guid ) ) {
						// update header object
						updated = support.UpdateHeaderObject ( fileRef, object, legacyManager );
					}

				}

			}

			if ( progressTracker != 0  ) progressTracker->WorkComplete();
		}

	}

	if ( ! updated ) return;	// If there's an error writing the chunk, bail.

	this->needsUpdate = false;

}	// ASF_MetaHandler::UpdateFile

// =================================================================================================
// ASF_MetaHandler::WriteTempFile
// ==============================

void ASF_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	bool ok;
	XMP_IO* originalRef = this->parent->ioRef;

	ASF_Support support(0,this->parent->progressTracker);
	ASF_Support::ObjectState objectState;
	long numTags = support.OpenASF ( originalRef, objectState );
	if ( numTags == 0 ) return;

	tempRef->Truncate ( 0 );

	ASF_Support::ObjectIterator curPos = objectState.objects.begin();
	ASF_Support::ObjectIterator endPos = objectState.objects.end();
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
	if ( progressTracker != 0 ) {
		float nonheadersize = (float)(xmpPacket.size()+kASF_ObjectBaseLen+8);
		bool legacyChange=this->legacyManager.hasLegacyChanged( );
		for ( ; curPos != endPos; ++curPos ) {
			if (curPos->xmp) continue;
			//header objects are taken care of in ASF_Support::WriteHeaderObject
			if ( ! ( IsEqualGUID ( ASF_Header_Object, curPos->guid) && legacyChange ) ) {
				nonheadersize+=(curPos->len);
			}
		}
		curPos = objectState.objects.begin();
		endPos = objectState.objects.end();
		progressTracker->BeginWork ( nonheadersize );
	}
	for ( ; curPos != endPos; ++curPos ) {

		ASF_Support::ObjectData object = *curPos;

		// discard existing XMP object
		if ( object.xmp ) continue;

		// update header-object, when legacy needs update
		if ( IsEqualGUID ( ASF_Header_Object, object.guid) && this->legacyManager.hasLegacyChanged( ) ) {
			// rewrite header object
			ok = support.WriteHeaderObject ( originalRef, tempRef, object, this->legacyManager, false );
			if ( ! ok ) XMP_Throw ( "Failure writing ASF header object", kXMPErr_InternalFailure );
		} else {
			// copy any other object
			ok = ASF_Support::CopyObject ( originalRef, tempRef, object );
			if ( ! ok ) XMP_Throw ( "Failure copyinh ASF object", kXMPErr_InternalFailure );
		}

		// write XMP object immediately after the (one and only) top-level DataObject
		if ( IsEqualGUID ( ASF_Data_Object, object.guid ) ) {
			XMP_StringPtr packetStr = xmpPacket.c_str();
			XMP_StringLen packetLen = (XMP_StringLen)xmpPacket.size();
			ok = ASF_Support::WriteXMPObject ( tempRef, packetLen, packetStr );
			if ( ! ok ) XMP_Throw ( "Failure writing ASF XMP object", kXMPErr_InternalFailure );
		}

	}

	ok = support.UpdateFileSize ( tempRef );
	if ( ! ok ) XMP_Throw ( "Failure updating ASF file size", kXMPErr_InternalFailure );
	if ( progressTracker != 0  ) progressTracker->WorkComplete();

}	// ASF_MetaHandler::WriteTempFile

// =================================================================================================
// ASF_MetaHandler::SafeWriteFile
// ==============================

bool ASF_MetaHandler::SafeWriteFile()
{
	XMP_IO* originalFile = this->parent->ioRef;
	XMP_IO* tempFile = originalFile->DeriveTemp();
	if ( tempFile == 0 ) XMP_Throw ( "Failure creating ASF temp file", kXMPErr_InternalFailure );

	this->WriteTempFile ( tempFile );
	originalFile->AbsorbTemp();

	return true;

} // ASF_MetaHandler::SafeWriteFile

// =================================================================================================
