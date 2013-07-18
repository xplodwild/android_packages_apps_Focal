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

#include "XMPFiles/source/FileHandlers/SWF_Handler.hpp"
#include "XMPFiles/source/FormatSupport/SWF_Support.hpp"

using namespace std;

// =================================================================================================
/// \file SWF_Handler.hpp
/// \brief File format handler for SWF.
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// SWF_MetaHandlerCTor
// ===================

XMPFileHandler * SWF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new SWF_MetaHandler ( parent );

}	// SWF_MetaHandlerCTor

// =================================================================================================
// SWF_CheckFormat
// ===============

bool SWF_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
                       XMP_IO *       fileRef,
                       XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(fileRef); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_SWFFile );
	
	// Make sure the file is long enough for an empty SWF stream. Check the signature.

	if ( fileRef->Length() < (XMP_Int64)SWF_IO::HeaderPrefixSize ) return false;

	fileRef->Rewind();
	XMP_Uns8 buffer [4];
	fileRef->ReadAll ( buffer, 4 );
	XMP_Uns32 signature = GetUns32LE ( &buffer[0] ) & 0xFFFFFF;	// Discard the version byte.
	
	return ( (signature == SWF_IO::CompressedSignature) || (signature == SWF_IO::ExpandedSignature) );

}	// SWF_CheckFormat

// =================================================================================================
// SWF_MetaHandler::SWF_MetaHandler
// ================================

SWF_MetaHandler::SWF_MetaHandler ( XMPFiles * _parent )
	: isCompressed(false), hasFileAttributes(false), hasMetadata(false), brokenSWF(false), expandedSize(0), firstTagOffset(0)
{
	this->parent = _parent;
	this->handlerFlags = kSWF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}

// =================================================================================================
// SWF_MetaHandler::~SWF_MetaHandler
// =================================

SWF_MetaHandler::~SWF_MetaHandler()
{
	// Nothing to do at this time.
}

// =================================================================================================
// SWF_MetaHandler::CacheFileData
// ==============================
//
// SWF files are pretty small, have simple metadata, and often have ZIP compression. Because they
// are small and often compressed, we always cache the fully expanded SWF in memory. That is used
// for both reading and updating. Note that SWF_CheckFormat has already done basic checks on the
// size and signature, they don't need to be repeated here.
//
// Try to find the FileAttributes and Metadata tags, saving their offsets for later use if updating
// the file. We need to be tolerant when reading, allowing the FileAttributes tag to be anywhere and
// allowing Metadata without FileAttributes or with the HasMetadata flag clear. The SWF spec is not
// clear enough about the rules for SWF 7 and earlier, there are 3rd party tools that don't put
// FileAttributes first.

void SWF_MetaHandler::CacheFileData() {

	XMP_Assert ( (! this->processedXMP) && (! this->containsXMP) );
	XMP_Assert ( this->expandedSWF.empty() );

	XMP_IO * fileRef = this->parent->ioRef;
	XMP_Int64 fileLength = fileRef->Length();
	XMP_Enforce ( fileLength <= SWF_IO::MaxExpandedSize );

	// Get the uncompressed SWF stream into memory.
	
	fileRef->Rewind();
	XMP_Uns8 buffer [SWF_IO::HeaderPrefixSize];	// Read the uncompressed file header prefix.
	fileRef->ReadAll ( buffer, SWF_IO::HeaderPrefixSize );

	XMP_Uns32 signature = GetUns32LE ( &buffer[0] ) & 0xFFFFFF;	// Discard the version byte.
	this->expandedSize = GetUns32LE ( &buffer[4] );
	if ( signature == SWF_IO::CompressedSignature ) this->isCompressed = true;
	
	if ( this->isCompressed ) {
	
		// Expand the SWF file into memory.
		this->expandedSWF.reserve ( this->expandedSize );	// Try to avoid reallocations.
		SWF_IO::DecompressFileToMemory ( fileRef, &this->expandedSWF );
		this->expandedSize = this->expandedSWF.size();	// Use the true length.
	
	} else {
	
		// Read the entire uncompressed file into memory.
		this->expandedSize = (XMP_Uns32)fileLength;	// Use the true length.
		this->expandedSWF.insert ( this->expandedSWF.end(), (size_t)fileLength, 0 );
		fileRef->Rewind();
		fileRef->ReadAll ( &this->expandedSWF[0], (XMP_Uns32)fileLength );
	
	}
	
	// Look for the FileAttributes and Metadata tags.
	
	this->firstTagOffset = SWF_IO::FileHeaderSize ( this->expandedSWF[SWF_IO::HeaderPrefixSize] );
	
	XMP_Uns32 currOffset = this->firstTagOffset;
	SWF_IO::TagInfo currTag;
	
	for ( ; currOffset < this->expandedSize; currOffset = SWF_IO::NextTagOffset(currTag) ) {
	
		bool ok = SWF_IO::GetTagInfo ( this->expandedSWF, currOffset, &currTag );
		if ( ! ok ) {
			this->brokenSWF = true;	// Let the read finish, but refuse to update.
			break;
		}
		
		if ( currTag.tagID == SWF_IO::FileAttributesTagID ) {
			this->fileAttributesTag = currTag;
			this->hasFileAttributes = true;
			if ( this->hasMetadata ) break;	// Exit if we have both.
		}
		
		if ( currTag.tagID == SWF_IO::MetadataTagID ) {
			this->metadataTag = currTag;
			this->hasMetadata = true;
			if ( this->hasFileAttributes ) break;	// Exit if we have both.
		}
		
	}
	
	if ( this->hasMetadata ) {
		this->packetInfo.offset = SWF_IO::ContentOffset ( this->metadataTag );
		this->packetInfo.length = this->metadataTag.contentLength;
		this->xmpPacket.assign ( (char*)&this->expandedSWF[(size_t)this->packetInfo.offset], (size_t)this->packetInfo.length );
		FillPacketInfo ( this->xmpPacket, &this->packetInfo );
		this->containsXMP = true;
	}
	
}	// SWF_MetaHandler::CacheFileData

// =================================================================================================
// SWF_MetaHandler::ProcessXMP
// ===========================

void SWF_MetaHandler::ProcessXMP()
{

	this->processedXMP = true;	// Make sure we only come through here once.

	if ( ! this->xmpPacket.empty() ) {
		XMP_Assert ( this->containsXMP );
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = (XMP_StringLen)this->xmpPacket.size();
		this->xmpObj.ParseFromBuffer ( packetStr, packetLen );
	}

}	// SWF_MetaHandler::ProcessXMP

// =================================================================================================
// XMPFileHandler::GetSerializeOptions
// ===================================
//
// Override default implementation to ensure omitting XMP wrapper.

XMP_OptionBits SWF_MetaHandler::GetSerializeOptions()
{

	return (kXMP_OmitPacketWrapper | kXMP_OmitAllFormatting | kXMP_OmitXMPMetaElement);

} // XMPFileHandler::GetSerializeOptions

// =================================================================================================
// SWF_MetaHandler::UpdateFile
// ===========================
//
// Update the expanded SWF in memory, then write it to the file.

void SWF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{

	if ( doSafeUpdate ) XMP_Throw ( "SWF_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );

	if ( ! this->needsUpdate ) return;
	this->needsUpdate = false; // Don't come through here twice, even if there are errors.
	
	if ( this->brokenSWF ) {
		XMP_Throw ( "SWF is broken, can't update.", kXMPErr_BadFileFormat );
	}
	
	// Make sure there is a FileAttributes tag at the front, with the HasMetadata flag set.
	
	if ( ! this->hasFileAttributes ) {
	
		// Insert a new FileAttributes tag as the first tag.
		
		XMP_Uns8 buffer [6];	// Two byte header plus four byte content.
		PutUns16LE ( ((SWF_IO::FileAttributesTagID << 6) | 4), &buffer[0] );
		PutUns32LE ( SWF_IO::HasMetadataMask, &buffer[2] );
		
		this->expandedSWF.insert ( (this->expandedSWF.begin() + this->firstTagOffset), 6, 0 );
		memcpy ( &this->expandedSWF[this->firstTagOffset], &buffer[0], 6 );

		this->hasFileAttributes = true;
		bool ok = SWF_IO::GetTagInfo ( this->expandedSWF, this->firstTagOffset, &this->fileAttributesTag );
		XMP_Assert ( ok );
		
		if ( this->hasMetadata ) this->metadataTag.tagOffset += 6;	// The Metadata tag is now further back.

	} else {
		
		// Make sure the HasMetadata flag is set.
		if ( this->fileAttributesTag.contentLength > 0 ) {
			XMP_Uns32 flagsOffset = SWF_IO::ContentOffset ( this->fileAttributesTag );
			this->expandedSWF[flagsOffset] |= SWF_IO::HasMetadataMask;
		}

		// Make sure the FileAttributes tag is the first tag.
		if ( this->fileAttributesTag.tagOffset != this->firstTagOffset ) {

			RawDataBlock attrTag;
			XMP_Uns32 attrTagLength = SWF_IO::FullTagLength ( this->fileAttributesTag );
			attrTag.assign ( attrTagLength, 0 );
			memcpy ( &attrTag[0], &this->expandedSWF[this->fileAttributesTag.tagOffset], attrTagLength );
			
			RawDataBlock::iterator attrTagPos = this->expandedSWF.begin() + this->fileAttributesTag.tagOffset;
			RawDataBlock::iterator attrTagEnd = attrTagPos + attrTagLength;
			this->expandedSWF.erase ( attrTagPos, attrTagEnd );	// Remove the old FileAttributes tag;
			
			if ( this->hasMetadata && (this->metadataTag.tagOffset < this->fileAttributesTag.tagOffset) ) {
				this->metadataTag.tagOffset += attrTagLength;	// The FileAttributes tag will become in front.
			}
			
			this->expandedSWF.insert ( (this->expandedSWF.begin() + this->firstTagOffset), attrTagLength, 0 );
			memcpy ( &this->expandedSWF[this->firstTagOffset], &attrTag[0], attrTagLength );
			
			this->fileAttributesTag.tagOffset = this->firstTagOffset;

		}

	}
	
	// Make sure the XMP is as small as possible. Write the XMP as the second tag.
	
	XMP_Assert ( this->hasFileAttributes );
	
	XMP_OptionBits smallOptions = kXMP_OmitPacketWrapper | kXMP_UseCompactFormat | kXMP_OmitAllFormatting | kXMP_OmitXMPMetaElement;
	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, smallOptions );
	
	if ( this->hasMetadata ) {
		// Remove the old XMP, the size and location have probably changed.
		XMP_Uns32 oldMetaLength = SWF_IO::FullTagLength ( this->metadataTag );
		RawDataBlock::iterator oldMetaPos = this->expandedSWF.begin() + this->metadataTag.tagOffset;
		RawDataBlock::iterator oldMetaEnd = oldMetaPos + oldMetaLength;
		this->expandedSWF.erase ( oldMetaPos, oldMetaEnd );
	}
	
	this->metadataTag.hasLongHeader = true;
	this->metadataTag.tagID = SWF_IO::MetadataTagID;
	this->metadataTag.tagOffset = SWF_IO::NextTagOffset ( this->fileAttributesTag );
	this->metadataTag.contentLength = this->xmpPacket.size();

	XMP_Uns32 newMetaLength = 6 + this->metadataTag.contentLength;	// Always use a long tag header.
	this->expandedSWF.insert ( (this->expandedSWF.begin() + this->metadataTag.tagOffset), newMetaLength, 0 );

	PutUns16LE ( ((SWF_IO::MetadataTagID << 6) | SWF_IO::TagLengthMask), &this->expandedSWF[this->metadataTag.tagOffset] );
	PutUns32LE ( this->metadataTag.contentLength, &this->expandedSWF[this->metadataTag.tagOffset+2] );
	memcpy ( &this->expandedSWF[this->metadataTag.tagOffset+6], this->xmpPacket.c_str(), this->metadataTag.contentLength );

	this->hasMetadata = true;
	
	// Update the uncompressed file length and rewrite the file.
	
	PutUns32LE ( this->expandedSWF.size(), &this->expandedSWF[4] );

	XMP_IO * fileRef = this->parent->ioRef;
	fileRef->Rewind();
	fileRef->Truncate ( 0 );
	
	if ( this->isCompressed ) {
		SWF_IO::CompressMemoryToFile ( this->expandedSWF, fileRef );
	} else {
		fileRef->Write ( &this->expandedSWF[0], this->expandedSWF.size() );
	}

}	// SWF_MetaHandler::UpdateFile

// =================================================================================================
// SWF_MetaHandler::WriteTempFile
// ==============================

// ! See important notes in SWF_Handler.hpp about file handling.

void SWF_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{

	// ! WriteTempFile is not supposed to be called for SWF.
	XMP_Throw ( "SWF_MetaHandler::WriteTempFile should not be called", kXMPErr_InternalFailure );

}	// SWF_MetaHandler::WriteTempFile
