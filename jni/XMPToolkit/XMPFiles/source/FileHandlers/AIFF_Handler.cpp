// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FileHandlers/AIFF_Handler.hpp"
#include "XMPFiles/source/FormatSupport/AIFF/AIFFBehavior.h"
#include "XMPFiles/source/FormatSupport/AIFF/AIFFReconcile.h"
#include "XMPFiles/source/NativeMetadataSupport/MetadataSet.h"
#include "source/XIO.hpp"

using namespace IFF_RIFF;

// =================================================================================================
/// \file AIFF_Handler.cpp
/// \brief File format handler for AIFF.
// =================================================================================================


// =================================================================================================
// AIFF_MetaHandlerCTor
// ====================

XMPFileHandler * AIFF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new AIFF_MetaHandler ( parent );
}

// =================================================================================================
// AIFF_CheckFormat
// ===============
//
// Checks if the given file is a valid AIFF or AIFC file.
// The first 12 bytes are checked. The first 4 must be "FORM"
// Bytes 8 to 12 must be "AIFF" or "AIFC"

bool AIFF_CheckFormat ( XMP_FileFormat  format,
					   XMP_StringPtr    filePath,
			           XMP_IO*      file,
			           XMPFiles*        parent )
{
	// Reset file pointer position
	file ->Rewind();

	XMP_Uns8 chunkID[12];
	XMP_Int32 got = file->Read ( chunkID, 12 );

	// Reset file pointer position
	file ->Rewind();

	// Need to have at least ID, size and Type of first chunk
	if ( got < 12 )
	{
		return false;
	}

	const BigEndian& endian = BigEndian::getInstance();
	if ( endian.getUns32(chunkID) != kChunk_FORM ) 
	{
		return false;
	}

	XMP_Uns32 type = AIFF_MetaHandler::whatAIFFFormat( &chunkID[8] );
	if ( type == kType_AIFF || type == kType_AIFC ) 
	{
		return true;
	}

	return false;
}	// AIFF_CheckFormat


// =================================================================================================
// AIFF_MetaHandler::whatAIFFFormat
// ===============

XMP_Uns32 AIFF_MetaHandler::whatAIFFFormat( XMP_Uns8* buffer )
{
	XMP_Uns32 type = 0;

	const BigEndian& endian = BigEndian::getInstance();

	if( buffer != 0 )
	{
		if( endian.getUns32( buffer ) == kType_AIFF )
		{
			type = kType_AIFF;
		}
		else if( endian.getUns32( buffer ) == kType_AIFC )
		{
			type = kType_AIFC;
		}
	}

	return type;
}	// whatAIFFFormat


// Static inits

// ChunkIdentifier
// FORM:AIFF/APPL:XMP
const ChunkIdentifier AIFF_MetaHandler::kAIFFXMP[2] = { { kChunk_FORM, kType_AIFF }, { kChunk_APPL, kType_XMP } };
// FORM:AIFC/APPL:XMP
const ChunkIdentifier AIFF_MetaHandler::kAIFCXMP[2] = { { kChunk_FORM, kType_AIFC }, { kChunk_APPL, kType_XMP } };
// FORM:AIFF/NAME
const ChunkIdentifier AIFF_MetaHandler::kAIFFName[2] = { { kChunk_FORM, kType_AIFF }, { kChunk_NAME, kType_NONE } };
// FORM:AIFC/NAME
const ChunkIdentifier AIFF_MetaHandler::kAIFCName[2] = { { kChunk_FORM, kType_AIFC }, { kChunk_NAME, kType_NONE } };
// FORM:AIFF/AUTH
const ChunkIdentifier AIFF_MetaHandler::kAIFFAuth[2] = { { kChunk_FORM, kType_AIFF }, { kChunk_AUTH, kType_NONE } };
// FORM:AIFC/AUTH
const ChunkIdentifier AIFF_MetaHandler::kAIFCAuth[2] = { { kChunk_FORM, kType_AIFC }, { kChunk_AUTH, kType_NONE } };
// FORM:AIFF/(c)
const ChunkIdentifier AIFF_MetaHandler::kAIFFCpr[2] = { { kChunk_FORM, kType_AIFF }, { kChunk_CPR, kType_NONE } };
// FORM:AIFC/(c)
const ChunkIdentifier AIFF_MetaHandler::kAIFCCpr[2] = { { kChunk_FORM, kType_AIFC }, { kChunk_CPR, kType_NONE } };
// FORM:AIFF/ANNO
const ChunkIdentifier AIFF_MetaHandler::kAIFFAnno[2] = { { kChunk_FORM, kType_AIFF }, { kChunk_ANNO, kType_NONE } };
// FORM:AIFC/ANNO
const ChunkIdentifier AIFF_MetaHandler::kAIFCAnno[2] = { { kChunk_FORM, kType_AIFC }, { kChunk_ANNO, kType_NONE } };

// =================================================================================================
// AIFF_MetaHandler::AIFF_MetaHandler
// ================================

AIFF_MetaHandler::AIFF_MetaHandler ( XMPFiles * _parent )
	: mChunkBehavior(NULL), mChunkController(NULL),
	mAiffMeta(), mXMPChunk(NULL),
	mNameChunk(NULL), mAuthChunk(NULL),
	mCprChunk(NULL), mAnnoChunk(NULL)
{
	this->parent = _parent;
	this->handlerFlags = kAIFF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	this->mChunkBehavior = new AIFFBehavior();
	this->mChunkController = new ChunkController( mChunkBehavior, true );

}	// AIFF_MetaHandler::AIFF_MetaHandler

// =================================================================================================
// AIFF_MetaHandler::~AIFF_MetaHandler
// =================================

AIFF_MetaHandler::~AIFF_MetaHandler()
{
	if( mChunkController != NULL )
	{
		delete mChunkController;
	}

	if( mChunkBehavior != NULL )
	{
		delete mChunkBehavior;
	}
}	// AIFF_MetaHandler::~AIFF_MetaHandler


// =================================================================================================
// AIFF_MetaHandler::CacheFileData
// ==============================

void AIFF_MetaHandler::CacheFileData()
{
	// Need to determine the file type, need the first 12 bytes of the file

	// Reset file pointer position
	this->parent->ioRef ->Rewind();

	XMP_Uns8 buffer[12];
	XMP_Int32 got = this->parent->ioRef->Read ( buffer, 12 );
	XMP_Assert( got == 12 );
	
	XMP_Uns32 type = AIFF_MetaHandler::whatAIFFFormat( &buffer[8] );
	XMP_Assert( type == kType_AIFF || type == kType_AIFC );

	// Reset file pointer position
	this->parent->ioRef ->Rewind();

	// Add the relevant chunk paths for the determined AIFF format
	if( type == kType_AIFF )
	{
		mAIFFXMPChunkPath.append( kAIFFXMP, SizeOfCIArray(kAIFFXMP) );
		mAIFFNameChunkPath.append( kAIFFName, SizeOfCIArray(kAIFFName) );
		mAIFFAuthChunkPath.append( kAIFFAuth, SizeOfCIArray(kAIFFAuth) );
		mAIFFCprChunkPath.append( kAIFFCpr, SizeOfCIArray(kAIFFCpr) );
		mAIFFAnnoChunkPath.append( kAIFFAnno, SizeOfCIArray(kAIFFAnno) );
	}
	else // kType_AIFC
	{
		mAIFFXMPChunkPath.append( kAIFCXMP, SizeOfCIArray(kAIFCXMP) );
		mAIFFNameChunkPath.append( kAIFCName, SizeOfCIArray(kAIFCName) );
		mAIFFAuthChunkPath.append( kAIFCAuth, SizeOfCIArray(kAIFCAuth) );
		mAIFFCprChunkPath.append( kAIFCCpr, SizeOfCIArray(kAIFCCpr) );
		mAIFFAnnoChunkPath.append( kAIFCAnno, SizeOfCIArray(kAIFCAnno) );
	}

	mChunkController->addChunkPath( mAIFFXMPChunkPath );
	mChunkController->addChunkPath( mAIFFNameChunkPath );
	mChunkController->addChunkPath( mAIFFAuthChunkPath );
	mChunkController->addChunkPath( mAIFFCprChunkPath );
	mChunkController->addChunkPath( mAIFFAnnoChunkPath );


	// Parse the given file
	// Throws exception if the file cannot be parsed
	mChunkController->parseFile( this->parent->ioRef, &this->parent->openFlags );

	// Check if the file contains XMP (last one if there are multiple chunks)
	mXMPChunk = mChunkController->getChunk( mAIFFXMPChunkPath, true );
	
	// Retrieve XMP packet info
	if( mXMPChunk != NULL )
	{
		// subtract the type size that is contained in the XMP data chunk
		this->packetInfo.length = static_cast<XMP_Int32>(mXMPChunk->getSize() - 4);
		this->packetInfo.charForm  = kXMP_Char8Bit;
		this->packetInfo.writeable = true;

		// Get actual the XMP packet without the 4byte type
		this->xmpPacket.assign ( mXMPChunk->getString( this->packetInfo.length, 4 ) );

		// set state
		this->containsXMP = true;
	}
} // AIFF_MetaHandler::CacheFileData


// =================================================================================================
// AIFF_MetaHandler::ProcessXMP
// ============================

void AIFF_MetaHandler::ProcessXMP()
{
	// Must be done only once
	if ( this->processedXMP )
	{
		return;
	}
	// Set the status at start, in case something goes wrong in this method
	this->processedXMP = true;

	// Parse the XMP
	if ( ! this->xmpPacket.empty() ) {

		XMP_Assert ( this->containsXMP );

		FillPacketInfo ( this->xmpPacket, &this->packetInfo );

		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );

		this->containsXMP = true;
	}

	// Then import native properties
	MetadataSet metaSet;
	AIFFReconcile recon;

	// Fill the AIFF metadata object with values
	
	// Get NAME (title) legacy chunk
	mNameChunk = mChunkController->getChunk( mAIFFNameChunkPath, true );
	if( mNameChunk != NULL )
	{
		mAiffMeta.setValue<std::string>( AIFFMetadata::kName, mNameChunk->getString() );
	}
	// Get AUTH (author) legacy chunk
	mAuthChunk = mChunkController->getChunk( mAIFFAuthChunkPath, true );
	if( mAuthChunk != NULL )
	{
		mAiffMeta.setValue<std::string>( AIFFMetadata::kAuthor, mAuthChunk->getString() );
	}
	// Get CPR (Copyright) legacy chunk
	mCprChunk = mChunkController->getChunk( mAIFFCprChunkPath, true );
	if( mCprChunk != NULL )
	{
		mAiffMeta.setValue<std::string>( AIFFMetadata::kCopyright, mCprChunk->getString() );
	}
	// Get ANNO (annotation) legacy chunk(s)
	// Get the list of Annotation chunks and pick the last one not being empty
	const std::vector<IChunkData*> &annoChunks = mChunkController->getChunks( mAIFFAnnoChunkPath );

	mAnnoChunk = selectLastNonEmptyAnnoChunk( annoChunks );
	if( mAnnoChunk != NULL )
	{
		mAiffMeta.setValue<std::string>( AIFFMetadata::kAnnotation, mAnnoChunk->getString() );
	}

	// Only interested in AIFF metadata
	metaSet.append( &mAiffMeta );
	// Do the import
	if( recon.importToXMP( this->xmpObj, metaSet ) )
	{
		// Remember if anything has changed
		this->containsXMP = true;
	}

} // AIFF_MetaHandler::ProcessXMP


IChunkData* AIFF_MetaHandler::selectLastNonEmptyAnnoChunk( const std::vector<IChunkData*> &annoChunks )
{
	IChunkData* annoChunk = NULL;
	for ( std::vector<IChunkData*>::const_reverse_iterator iter = annoChunks.rbegin(); iter != annoChunks.rend(); iter++ )
	{
		if( ! (*iter)->getString().empty() && (*iter)->getString()[0] != '\0' )
		{
			annoChunk = *iter;
			break;
		}
	}
	return annoChunk;
} // selectFirstNonEmptyAnnoChunk


// =================================================================================================
// AIFF_MetaHandler::UpdateFile
// ===========================

void AIFF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) {	// If needsUpdate is set then at least the XMP changed.
		return;
	}

	if ( doSafeUpdate )
	{
		XMP_Throw ( "AIFF_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );
	}

	//update/create XMP chunk
	if( this->containsXMP )
	{
		this->xmpObj.SerializeToBuffer ( &(this->xmpPacket) );

		if( mXMPChunk != NULL )
		{
			mXMPChunk->setData( reinterpret_cast<const XMP_Uns8 *>(this->xmpPacket.c_str()), this->xmpPacket.length(), true );
		}
		else // create XMP chunk
		{
			mXMPChunk = mChunkController->createChunk( kChunk_APPL, kType_XMP );
			mXMPChunk->setData( reinterpret_cast<const XMP_Uns8 *>(this->xmpPacket.c_str()), this->xmpPacket.length(), true );
			mChunkController->insertChunk( mXMPChunk );
		}
	}
	// XMP Packet is never completely removed from the file.

	// Export XMP to legacy chunks. Create/delete them if necessary
	MetadataSet metaSet;
	AIFFReconcile recon;

	metaSet.append( &mAiffMeta );

	// If anything changes, update/create/delete the legacy chunks
	if( recon.exportFromXMP( metaSet, this->xmpObj ) )
	{
		updateLegacyChunk( &mNameChunk, kChunk_NAME, AIFFMetadata::kName );
		updateLegacyChunk( &mAuthChunk, kChunk_AUTH, AIFFMetadata::kAuthor );
		updateLegacyChunk( &mCprChunk, kChunk_CPR, AIFFMetadata::kCopyright );
		updateLegacyChunk( &mAnnoChunk, kChunk_ANNO, AIFFMetadata::kAnnotation );
	}
	
	XMP_ProgressTracker* progressTracker=this->parent->progressTracker;
	// local progess tracking required because  for Handlers incapable of 
	// kXMPFiles_CanRewrite XMPFiles call this Update method after making
	// a copy of the orignal file
	bool localProgressTracking=false;
	if ( progressTracker != 0 )
	{
		if ( ! progressTracker->WorkInProgress() ) 
		{
			localProgressTracking = true;
			progressTracker->BeginWork ();
		}
	}
	//write tree back to file
	mChunkController->writeFile( this->parent->ioRef ,progressTracker);
	if ( localProgressTracking && progressTracker != 0 ) progressTracker->WorkComplete();

	this->needsUpdate = false;	// Make sure this is only called once.
}	// AIFF_MetaHandler::UpdateFile


void AIFF_MetaHandler::updateLegacyChunk( IChunkData **chunk, XMP_Uns32 chunkID, XMP_Uns32 legacyId )
{
	// If there is a legacy value, update/create the appropriate chunk
	if( mAiffMeta.valueExists( legacyId ) )
	{
		std::string chunkValue;
		std::string legacyValue = mAiffMeta.getValue<std::string>( legacyId );

		// If the length is < 4 we need to fill up the value with \0 to a size of 4
		// This ensures that the overall size of text chunks is 12 bytes so that they can be
		// converted to free chunks if necessary
		if( legacyValue.length() < 4 )
		{
			char buffer[4];
			memset( buffer, 0, 4 );
			memcpy( buffer, legacyValue.c_str(), legacyValue.length() );
			chunkValue.assign( buffer, 4 );
		}
		else // take the value as is
		{
			chunkValue = legacyValue;
		}

		if( *chunk != NULL )
		{
			(*chunk)->setData( reinterpret_cast<const XMP_Uns8 *>(chunkValue.c_str()), chunkValue.length() );
		}
		else
		{
			*chunk = mChunkController->createChunk( chunkID, kType_NONE );
			(*chunk)->setData( reinterpret_cast<const XMP_Uns8 *>(chunkValue.c_str()), chunkValue.length() );
			mChunkController->insertChunk( *chunk );
		}
	}
	else //delete chunk if existing
	{
		mChunkController->removeChunk ( *chunk );
	}
}	// updateLegacyChunk


// =================================================================================================
// AIFF_MetaHandler::WriteTempFile
// ===============================

void AIFF_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_Throw ( "AIFF_MetaHandler::WriteTempFile is not Implemented!", kXMPErr_Unimplemented );
}	// AIFF_MetaHandler::WriteTempFile
