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

#include "XMPFiles/source/FileHandlers/WAVE_Handler.hpp"
#include "XMPFiles/source/FormatSupport/WAVE/WAVEBehavior.h"
#include "XMPFiles/source/FormatSupport/WAVE/WAVEReconcile.h"
#include "XMPFiles/source/NativeMetadataSupport/MetadataSet.h"
#include "source/XIO.hpp"

using namespace IFF_RIFF;

// =================================================================================================
/// \file WAVE_Handler.cpp
/// \brief File format handler for WAVE.
// =================================================================================================


// =================================================================================================
// WAVE_MetaHandlerCTor
// ====================

XMPFileHandler * WAVE_MetaHandlerCTor ( XMPFiles * parent )
{
	return new WAVE_MetaHandler ( parent );
}

// =================================================================================================
// WAVE_CheckFormat
// ===============
//
// Checks if the given file is a valid WAVE file.
// The first 12 bytes are checked. The first 4 must be "RIFF"
// Bytes 8 to 12 must be "WAVE"

bool WAVE_CheckFormat ( XMP_FileFormat  format,
					   XMP_StringPtr    filePath,
			           XMP_IO*			file,
			           XMPFiles*        parent )
{
	// Reset file pointer position
	file->Rewind();

	XMP_Uns8 buffer[12];
	XMP_Int32 got = file->Read ( buffer, 12 );
	// Reset file pointer position
	file->Rewind();

	// Need to have at least ID, size and Type of first chunk
	if ( got < 12 ) 
	{
		return false;
	}

	XMP_Uns32 type = WAVE_MetaHandler::whatRIFFFormat( buffer );
	if ( type != kChunk_RIFF && type != kChunk_RF64 ) 
	{
		return false;
	}

	const BigEndian& endian = BigEndian::getInstance();
	if ( endian.getUns32(&buffer[8]) == kType_WAVE ) 
	{
		return true;
	}

	return false;
}	// WAVE_CheckFormat


// =================================================================================================
// WAVE_MetaHandler::whatRIFFFormat
// ===============

XMP_Uns32 WAVE_MetaHandler::whatRIFFFormat( XMP_Uns8* buffer )
{
	XMP_Uns32 type = 0;

	const BigEndian& endian = BigEndian::getInstance();

	if( buffer != 0 )
	{
		if( endian.getUns32( buffer ) == kChunk_RIFF )
		{
			type = kChunk_RIFF;
		}
		else if( endian.getUns32( buffer ) == kChunk_RF64 )
		{
			type = kChunk_RF64;
		}
	}

	return type;
}	// whatRIFFFormat


// Static inits

// ChunkIdentifier
// RIFF:WAVE/PMX_
const ChunkIdentifier WAVE_MetaHandler::kRIFFXMP[2] = { { kChunk_RIFF, kType_WAVE }, { kChunk_XMP, kType_NONE} };
// RIFF:WAVE/LIST:INFO
const ChunkIdentifier WAVE_MetaHandler::kRIFFInfo[2] = { { kChunk_RIFF, kType_WAVE }, { kChunk_LIST, kType_INFO } };
// RIFF:WAVE/DISP
const ChunkIdentifier WAVE_MetaHandler::kRIFFDisp[2] = { { kChunk_RIFF, kType_WAVE }, { kChunk_DISP, kType_NONE } };
// RIFF:WAVE/BEXT
const ChunkIdentifier WAVE_MetaHandler::kRIFFBext[2] = { { kChunk_RIFF, kType_WAVE }, { kChunk_bext, kType_NONE } };
// RIFF:WAVE/cart
const ChunkIdentifier WAVE_MetaHandler::kRIFFCart[2] = { { kChunk_RIFF, kType_WAVE }, { kChunk_cart, kType_NONE } };
// cr8r is not yet required for WAVE
// RIFF:WAVE/Cr8r
// const ChunkIdentifier WAVE_MetaHandler::kWAVECr8r[2] = { { kChunk_RIFF, kType_WAVE }, { kChunk_Cr8r, kType_NONE } };
// RF64:WAVE/PMX_
const ChunkIdentifier WAVE_MetaHandler::kRF64XMP[2] = { { kChunk_RF64, kType_WAVE }, { kChunk_XMP, kType_NONE} };
// RF64:WAVE/LIST:INFO
const ChunkIdentifier WAVE_MetaHandler::kRF64Info[2] = { { kChunk_RF64, kType_WAVE }, { kChunk_LIST, kType_INFO } };
// RF64:WAVE/DISP
const ChunkIdentifier WAVE_MetaHandler::kRF64Disp[2] = { { kChunk_RF64, kType_WAVE }, { kChunk_DISP, kType_NONE } };
// RF64:WAVE/BEXT
const ChunkIdentifier WAVE_MetaHandler::kRF64Bext[2] = { { kChunk_RF64, kType_WAVE }, { kChunk_bext, kType_NONE } };
// RF64:WAVE/cart
const ChunkIdentifier WAVE_MetaHandler::kRF64Cart[2] = { { kChunk_RF64, kType_WAVE }, { kChunk_cart, kType_NONE } };
// cr8r is not yet required for WAVE
// RF64:WAVE/Cr8r
// const ChunkIdentifier WAVE_MetaHandler::kRF64Cr8r[2] = { { kChunk_RF64, kType_WAVE }, { kChunk_Cr8r, kType_NONE } };

// =================================================================================================
// WAVE_MetaHandler::WAVE_MetaHandler
// ================================

WAVE_MetaHandler::WAVE_MetaHandler ( XMPFiles * _parent )
	: mChunkBehavior(NULL), mChunkController(NULL),
	mINFOMeta(), mBEXTMeta(), mCartMeta(), mDISPMeta(),
	mXMPChunk(NULL), mINFOChunk(NULL),
	mBEXTChunk(NULL), mCartChunk(NULL), mDISPChunk(NULL)
{
	this->parent = _parent;
	this->handlerFlags = kWAVE_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	this->mChunkBehavior = new WAVEBehavior();
	this->mChunkController = new ChunkController( mChunkBehavior, false );

}	// WAVE_MetaHandler::WAVE_MetaHandler


// =================================================================================================
// WAVE_MetaHandler::~WAVE_MetaHandler
// =================================

WAVE_MetaHandler::~WAVE_MetaHandler()
{
	if( mChunkController != NULL )
	{
		delete mChunkController;
	}

	if( mChunkBehavior != NULL )
	{
		delete mChunkBehavior;
	}
}	// WAVE_MetaHandler::~WAVE_MetaHandler


// =================================================================================================
// WAVE_MetaHandler::CacheFileData
// ==============================

void WAVE_MetaHandler::CacheFileData()
{
	// Need to determine the file type, need the first four bytes of the file

	// Reset file pointer position
	this->parent->ioRef->Rewind();

	XMP_Uns8 buffer[4];
	XMP_Int32 got = this->parent->ioRef->Read ( buffer, 4 );
	XMP_Assert( got == 4 );
	
	XMP_Uns32 type = WAVE_MetaHandler::whatRIFFFormat( buffer );
	XMP_Assert( type == kChunk_RIFF || type == kChunk_RF64 );

	// Reset file pointer position
	this->parent->ioRef->Rewind();

	// Add the relevant chunk paths for the determined RIFF format
	if( type == kChunk_RIFF )
	{
		mWAVEXMPChunkPath.append( kRIFFXMP, SizeOfCIArray(kRIFFXMP) );
		mWAVEInfoChunkPath.append( kRIFFInfo, SizeOfCIArray(kRIFFInfo) );
		mWAVEDispChunkPath.append( kRIFFDisp, SizeOfCIArray(kRIFFDisp) );
		mWAVEBextChunkPath.append( kRIFFBext, SizeOfCIArray(kRIFFBext) );
		mWAVECartChunkPath.append( kRIFFCart, SizeOfCIArray(kRIFFCart) );
		// cr8r is not yet required for WAVE
		//mWAVECr8rChunkPath.append( kWAVECr8r, SizeOfCIArray(kWAVECr8r) );
	}
	else // RF64
	{
		mWAVEXMPChunkPath.append( kRF64XMP, SizeOfCIArray(kRF64XMP) );
		mWAVEInfoChunkPath.append( kRF64Info, SizeOfCIArray(kRF64Info) );
		mWAVEDispChunkPath.append( kRF64Disp, SizeOfCIArray(kRF64Disp) );
		mWAVEBextChunkPath.append( kRF64Bext, SizeOfCIArray(kRF64Bext) );
		mWAVECartChunkPath.append( kRF64Cart, SizeOfCIArray(kRF64Cart) );
		// cr8r is not yet required for WAVE
		//mWAVECr8rChunkPath.append( kRF64Cr8r, SizeOfCIArray(kRF64Cr8r) );
	}

	mChunkController->addChunkPath( mWAVEXMPChunkPath );
	mChunkController->addChunkPath( mWAVEInfoChunkPath );
	mChunkController->addChunkPath( mWAVEDispChunkPath );
	mChunkController->addChunkPath( mWAVEBextChunkPath );
	mChunkController->addChunkPath( mWAVECartChunkPath );
	// cr8r is not yet required for WAVE
	//mChunkController->addChunkPath( mWAVECr8rChunkPath );

	// Parse the given file
	// Throws exception if the file cannot be parsed
	mChunkController->parseFile( this->parent->ioRef, &this->parent->openFlags );

	// Retrieve the file type, it must have at least FORM:WAVE
	std::vector<XMP_Uns32> typeList = mChunkController->getTopLevelTypes();

	// If file is neither WAVE, throw exception
	XMP_Validate( typeList.at(0)  == kType_WAVE , "File is not of type WAVE", kXMPErr_BadFileFormat );

	// Check if the file contains XMP (last if there are duplicates)
	mXMPChunk = mChunkController->getChunk( mWAVEXMPChunkPath, true );
	

	// Retrieve XMP packet info
	if( mXMPChunk != NULL )
	{
		this->packetInfo.length = static_cast<XMP_Int32>(mXMPChunk->getSize());
		this->packetInfo.charForm  = kXMP_Char8Bit;
		this->packetInfo.writeable = true;

		// Get actual the XMP packet
		this->xmpPacket.assign ( mXMPChunk->getString( this->packetInfo.length) );

		// set state
		this->containsXMP = true;
	}
} // WAVE_MetaHandler::CacheFileData


// =================================================================================================
// WAVE_MetaHandler::ProcessXMP
// ============================

void WAVE_MetaHandler::ProcessXMP()
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
	WAVEReconcile recon;

	// Parse the WAVE metadata object with values

	const XMP_Uns8* buffer = NULL; // temporary buffer
	XMP_Uns64 size = 0;
	// Get LIST:INFO legacy chunk
	mINFOChunk = mChunkController->getChunk( mWAVEInfoChunkPath, true );
	if( mINFOChunk != NULL )
	{			
		size = mINFOChunk->getData( &buffer );
		mINFOMeta.parse( buffer, size );
	}

	// Parse Bext legacy chunk
	mBEXTChunk = mChunkController->getChunk( mWAVEBextChunkPath, true );
	if( mBEXTChunk != NULL )
	{	
		size = mBEXTChunk->getData( &buffer );
		mBEXTMeta.parse( buffer, size );
	 }

	// Parse cart legacy chunk
	mCartChunk = mChunkController->getChunk( mWAVECartChunkPath, true );
	if( mCartChunk != NULL )
	{	
		size = mCartChunk->getData( &buffer );
		mCartMeta.parse( buffer, size );
	 }

	// Parse DISP legacy chunk
	const std::vector<IChunkData*>& disps = mChunkController->getChunks( mWAVEDispChunkPath );

	if( ! disps.empty() )
	{
		for( std::vector<IChunkData*>::const_reverse_iterator iter=disps.rbegin(); iter!=disps.rend(); iter++ )
		{
			size = (*iter)->getData( &buffer );
			
			if( DISPMetadata::isValidDISP( buffer, size ) )
			{
				mDISPChunk = (*iter);
				break;
			}
		}
	}

	if( mDISPChunk != NULL )
	{	
		size = mDISPChunk->getData( &buffer );
		mDISPMeta.parse( buffer, size );
	}
	
	//cr8r is not yet required for WAVE
	//// Parse Cr8r legacy chunk
	//mCr8rChunk = mChunkController->getChunk( mWAVECr8rChunkPath );
	//if( mCr8rChunk != NULL )
	//{	
	//	size = mCr8rChunk->getData( &buffer );
	//	mCr8rMeta.parse( buffer, size );
	//}
	
	// app legacy to the metadata list
	metaSet.append( &mINFOMeta );
	metaSet.append( &mBEXTMeta );
	metaSet.append( &mCartMeta );
	metaSet.append( &mDISPMeta );

	// cr8r is not yet required for WAVE
	// metaSet.append( &mCr8rMeta );
	
	// Do the import
	if( recon.importToXMP( this->xmpObj, metaSet ) )
	{
		// Remember if anything has changed
		this->containsXMP = true;
	}
	
} // WAVE_MetaHandler::ProcessXMP


// =================================================================================================
// RIFF_MetaHandler::UpdateFile
// ===========================

void WAVE_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) {	// If needsUpdate is set then at least the XMP changed.
		return;
	}

	if ( doSafeUpdate )
	{
		XMP_Throw ( "WAVE_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );
	}

	// Export XMP to legacy chunks. Create/delete them if necessary
	MetadataSet metaSet;
	WAVEReconcile recon;
	
	metaSet.append( &mINFOMeta );
	metaSet.append( &mBEXTMeta );
	metaSet.append( &mCartMeta );
	metaSet.append( &mDISPMeta );

	// cr8r is not yet required for WAVE
	// metaSet.append( &mCr8rMeta );

	// If anything changes, update/create/delete the legacy chunks
	if( recon.exportFromXMP( metaSet, this->xmpObj ) )
	{		
		if ( mINFOMeta.hasChanged( ))
		{
			updateLegacyChunk( &mINFOChunk, kChunk_LIST, kType_INFO, mINFOMeta );
		}

		if ( mBEXTMeta.hasChanged( ))
		{
			updateLegacyChunk( &mBEXTChunk, kChunk_bext, kType_NONE, mBEXTMeta );
		}

		if ( mCartMeta.hasChanged( ))
		{
			updateLegacyChunk( &mCartChunk, kChunk_cart, kType_NONE, mCartMeta );
		}

		if ( mDISPMeta.hasChanged( ))
		{
			updateLegacyChunk( &mDISPChunk, kChunk_DISP, kType_NONE, mDISPMeta );
		}

		//cr8r is not yet required for WAVE
		//if ( mCr8rMeta.hasChanged( ))
		//{
		//	updateLegacyChunk( &mCr8rChunk, kChunk_Cr8r, kType_NONE, mCr8rMeta );
		//}
	}

	//update/create XMP chunk
	if( this->containsXMP )
	{
		this->xmpObj.SerializeToBuffer ( &(this->xmpPacket) );

		if( mXMPChunk != NULL )
		{
			mXMPChunk->setData( reinterpret_cast<const XMP_Uns8 *>(this->xmpPacket.c_str()), this->xmpPacket.length() );
		}
		else // create XMP chunk
		{
			mXMPChunk = mChunkController->createChunk( kChunk_XMP, kType_NONE );
			mXMPChunk->setData( reinterpret_cast<const XMP_Uns8 *>(this->xmpPacket.c_str()), this->xmpPacket.length() );
			mChunkController->insertChunk( mXMPChunk );
		}
	}
	// XMP Packet is never completely removed from the file.
	
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
}	// WAVE_MetaHandler::UpdateFile


void WAVE_MetaHandler::updateLegacyChunk( IChunkData **chunk, XMP_Uns32 chunkID, XMP_Uns32 chunkType, IMetadata &legacyData )
{
	// If there is a legacy value, update/create the appropriate chunk
	if( ! legacyData.isEmpty() )
	{
		XMP_Uns8* buffer = NULL;
		XMP_Uns64 size = legacyData.serialize( &buffer );

		if( *chunk != NULL )
		{			
			(*chunk)->setData( buffer, size, false );
		}
		else
		{
			*chunk = mChunkController->createChunk( chunkID, chunkType );
			(*chunk)->setData( buffer, size, false );
			mChunkController->insertChunk( *chunk );
		}

		delete[] buffer;
	}
	else //delete chunk if existing
	{
		mChunkController->removeChunk ( *chunk );
	}
}//updateLegacyChunk


// =================================================================================================
// RIFF_MetaHandler::WriteFile
// ==========================

void WAVE_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_Throw( "WAVE_MetaHandler::WriteTempFile is not Implemented!", kXMPErr_Unimplemented );
}//WAVE_MetaHandler::WriteFile
