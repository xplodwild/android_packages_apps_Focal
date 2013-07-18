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
#include "source/XIO.hpp"

#include "XMPFiles/source/FormatSupport/WAVE/WAVEBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/Chunk.h"

#include <algorithm>

using namespace IFF_RIFF;

//
// Static init
//
const LittleEndian& WAVEBehavior::mEndian = LittleEndian::getInstance();


//-----------------------------------------------------------------------------
// 
// WAVEBehavior::getRealSize(...)
// 
// Purpose: Validate the passed in size value, identify the valid size if the 
//			passed in isn't valid and return the valid size.
//			Throw an exception if the passed in size isn't valid and there's 
//			no way to identify a valid size.
// 
//-----------------------------------------------------------------------------

XMP_Uns64 WAVEBehavior::getRealSize( const XMP_Uns64 size, const ChunkIdentifier& id, IChunkContainer& tree, XMP_IO* stream )
{
	XMP_Uns64 realSize = size;

	if( size >= kNormalRF64ChunkSize ) // 4GB
	{
		if( this->isRF64( tree ) )
		{
			//
			// RF64 supports sizes beyond 4GB
			//
			DS64* rf64 = this->getDS64( tree, stream );

			if( rf64 != NULL )
			{
				//
				// get 64bit size from RF64 structure
				//
				switch( id.id )
				{
					case kChunk_RF64:	realSize = rf64->riffSize;	break;
					case kChunk_data:	realSize = rf64->dataSize;	break;

					default:
					{
						bool found = false;

						//
						// try to find size value for passed chunk id in the ds64 table
						//
						if( rf64->tableLength > 0 )
						{
							for( std::vector<ChunkSize64>::iterator iter=rf64->table.begin(); iter!=rf64->table.end(); iter++ )
							{
								if( iter->id == id.id )
								{
									realSize = iter->size;
									found = true;
									break;
								}
							}
						}

						if( !found )
						{
							//
							// no size for passed id available
							//
							XMP_Throw( "Unknown size value", kXMPErr_BadFileFormat );
						}
					}
				}
			}
			else
			{
				//
				// no RF64 size info available
				//
				XMP_Throw( "Unknown size value", kXMPErr_BadFileFormat );
			}
		}
		else
		{
			//
			// WAVE doesn't support that size
			//
			XMP_Throw( "Unknown size value", kXMPErr_BadFileFormat );
		}
	}

	return realSize;
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::getMaxChunkSize(...)
// 
// Purpose: Return the maximum size of a single chunk, i.e. the maximum size 
//			of a top-level chunk.
// 
//-----------------------------------------------------------------------------

XMP_Uns64 WAVEBehavior::getMaxChunkSize() const
{
	// simple WAVE 4GByte
	XMP_Uns64 ret = 0x00000000FFFFFFFFLL;

	if( mIsRF64 )
	{
		// RF64: full possible 64bit size
		ret = 0xFFFFFFFFFFFFFFFFLL;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::isValidTopLevelChunk(...)
// 
// Purpose: Return true if the passed identifier is valid for top-level chunks 
//			of a certain format.
// 
//-----------------------------------------------------------------------------

bool WAVEBehavior::isValidTopLevelChunk( const ChunkIdentifier& id, XMP_Uns32 chunkNo )
{
	return ( chunkNo == 0 )												&& 
		   ( ( ( id.id == kChunk_RIFF ) && ( id.type == kType_WAVE ) )	||  
		     ( ( id.id == kChunk_RF64 ) && ( id.type == kType_WAVE ) ) );
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::fixHierarchy(...)
// 
// Purpose: Fix the hierarchy of chunks depending ones based on size changes of 
//			one or more chunks and second based on format specific rules.
//			Throw an exception if the hierarchy can't be fixed.
// 
//-----------------------------------------------------------------------------

void WAVEBehavior::fixHierarchy( IChunkContainer& tree )
{
	XMP_Validate( tree.numChildren() == 1, "WAVE files should only have one top level chunk (RIFF)", kXMPErr_BadFileFormat);
	
	Chunk* riffChunk = tree.getChildAt(0);

	XMP_Validate( (riffChunk->getType() == kType_WAVE || riffChunk->getType() == kChunk_RF64) , "Invalid type for WAVE/RF64 top level chunk (RIFF)", kXMPErr_BadFileFormat);

	if( riffChunk->hasChanged() )
	{
		//
		// move new added chunks to temporary container
		//
		Chunk* tmpContainer = Chunk::createChunk( mEndian );
		this->moveChunks( *riffChunk, *tmpContainer, riffChunk->numChildren() - mChunksAdded );

		//
		// try to arrange chunks at their current position
		//
		this->arrangeChunksInPlace( *riffChunk, *tmpContainer );

		//
		// for all chunks that were moved to the end try to find a FREE chunk for them
		//
		this->arrangeChunksInTree( *tmpContainer, *riffChunk );

		//
		// append all remaining new added chunks to the end of the tree
		//
		this->moveChunks( *tmpContainer, *riffChunk, 0 );
		delete tmpContainer;

		//
		// check for FREE chunks at the end
		//
		Chunk* endFREE = this->mergeFreeChunks( *riffChunk, riffChunk->numChildren() - 1 );

		if( endFREE != NULL )
		{
			riffChunk->removeChildAt( riffChunk->numChildren() - 1 );
			delete endFREE;
		}

		//
		// Fix the offset values of all chunks. Throw an exception in the case that
		// the offset of a non-modified chunk needs to be reset.
		//
		XMP_Validate( riffChunk->getOffset() == 0, "Invalid offset for RIFF top level chunk", kXMPErr_InternalFailure );

		this->validateOffsets( tree );
	
		//
		// update the RF64 chunk (if this is RF64) based on the current chunk sizes
		//
		this->updateRF64( tree );
	}
}

void WAVEBehavior::insertChunk( IChunkContainer& tree, Chunk& chunk )
{
	XMP_Validate( tree.numChildren() == 1, "WAVE files should only have one top level chunk (RIFF)", kXMPErr_BadFileFormat);
	Chunk* riffChunk = tree.getChildAt(0);

	XMP_Validate( riffChunk->getType() == kType_WAVE , "Invalid type for WAVE top level chunk (RIFF)", kXMPErr_BadFileFormat);

	//
	// add new chunk to the end of the RIFF:WAVE
	//
	riffChunk->appendChild(&chunk);

	mChunksAdded++;
}

bool WAVEBehavior::removeChunk( IChunkContainer& tree, Chunk& chunk )
{
	//
	// validate parameter
	//
	XMP_Validate( chunk.getID() != kChunk_RIFF, "Can't remove RIFF chunk!", kXMPErr_InternalFailure );
	XMP_Validate( chunk.getChunkMode() != CHUNK_UNKNOWN, "Cant' remove UNKNOWN Chunk", kXMPErr_InternalFailure );
	XMP_Validate( tree.numChildren() == 1, "WAVE files should only have one top level chunk (RIFF)", kXMPErr_BadFileFormat);	

	//
	// get top-level chunk
	//
	Chunk* riffChunk = tree.getChildAt(0);

	//
	// validate top-level chunk
	//
	XMP_Validate( (riffChunk->getType() == kType_WAVE || riffChunk->getType() == kChunk_RF64) , "Invalid type for WAVE/RF64 top level chunk (RIFF)", kXMPErr_BadFileFormat);

	//
	// calculate index of chunk to remove
	//
	XMP_Uns32 i = std::find( riffChunk->firstChild(), riffChunk->lastChild(), &chunk ) - riffChunk->firstChild();

	//
	// validate index
	//
	XMP_Validate( i < riffChunk->numChildren(), "Invalid chunk in tree", kXMPErr_InternalFailure );

	//
	// adjust new chunks counter
	//
	if( i > riffChunk->numChildren() - mChunksAdded - 1 )
	{
		mChunksAdded--;
	}

	if( i < riffChunk->numChildren()-1 )
	{
		//
		// fill gap with free chunk
		//
		Chunk* free = this->createFREE( chunk.getPadSize( true ) );
		riffChunk->replaceChildAt( i, free );
		free->setAsNew();

		//
		// merge JUNK chunks
		//
		this->mergeFreeChunks( *riffChunk, i );
	}
	else
	{
		//
		// remove chunk from tree
		//
		riffChunk->removeChildAt( i );
	}

	//
	// if there is an entry in the ds64 table for the removed chunk
	// then update the ds64 table entry
	//
	if( mDS64Data != NULL && mDS64Data->tableLength > 0 )
	{
		for( std::vector<ChunkSize64>::iterator iter=mDS64Data->table.begin(); iter!=mDS64Data->table.end(); iter++ )
		{
			if( iter->id == chunk.getID() )
			{
				//
				// don't remove entry but set its size to zero
				//
				iter->size = 0LL;
				break;
			}
		}
	}
	
	return true;
}

Chunk* WAVEBehavior::createFREE( XMP_Uns64 chunkSize )
{
	XMP_Int64 alloc = chunkSize - Chunk::HEADER_SIZE;

	Chunk* chunk = NULL;

	//
	// create a 'JUNK' chunk
	//
	if( alloc > 0 )
	{
		XMP_Uns8* data = new XMP_Uns8[static_cast<size_t>( alloc )];
		memset( data, 0, static_cast<size_t>( alloc ) );

		chunk = Chunk::createUnknownChunk( mEndian, kChunk_JUNK, kType_NONE, alloc );

		chunk->setData( data, alloc );		

		delete[] data;
	}
	else	
	{
		chunk = Chunk::createHeaderChunk( mEndian, kChunk_JUNK );
	}

	// force set dirty flag
	chunk->setChanged();

	return chunk;
}

XMP_Bool WAVEBehavior::isFREEChunk( const Chunk& chunk ) const
{
	// Check for sigature JUNK and JUNQ
	return ( chunk.getID() == kChunk_JUNK || chunk.getID() == kChunk_JUNQ );
}


XMP_Uns64 WAVEBehavior::getMinFREESize() const
{
	// avoid creation of chunks with size==0
	return static_cast<XMP_Uns64>( Chunk::HEADER_SIZE ) + 2;
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::isRF64(...)
// 
// Purpose: Is the current file a RF64 file
// 
//-----------------------------------------------------------------------------

bool WAVEBehavior::isRF64( const IChunkContainer& tree )
{
	// The file format will not change at runtime
	// So if the flag is not already set, have a look at the tree 
	if( ! mIsRF64 && tree.numChildren() != 0 )
	{
		Chunk *chunk = tree.getChildAt(0);
		// Only the TopLevel chunk is interesting
		mIsRF64 = chunk->getID() == kChunk_RF64 && 
				chunk->getType() == kType_WAVE;
	}
	
	return mIsRF64;
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::getDS64(...)
// 
// Purpose: Return RF64 structure.
// 
//-----------------------------------------------------------------------------

WAVEBehavior::DS64* WAVEBehavior::getDS64( IChunkContainer& tree, XMP_IO* stream )
{
	DS64* ret = mDS64Data;

	if( ret == NULL )
	{
		//
		// try to find 'ds64' chunk in the tree
		//
		Chunk* ds64 = NULL;
		Chunk* rf64 = NULL;

		if( tree.numChildren() > 0 )
		{
			rf64 = tree.getChildAt(0);

			if( rf64 != NULL && rf64->getID() == kChunk_RF64 && rf64->numChildren() > 0 )
			{
				//
				// 'ds64' chunk needs to be the very first child of the 'RF64' chunk
				//
				ds64 = rf64->getChildAt(0);
			}

			//
			// Try to create 'ds64' chunk by parsing the stream
			//
			if( ds64 == NULL && stream != NULL )
			{
				//
				// remember file position before start reading from the stream
				//
				XMP_Uns64 filePos = stream->Offset();

				try
				{
					ds64 = Chunk::createChunk( mEndian );
					ds64->readChunk( stream );
				}
				catch( ... )
				{
					delete ds64;
					ds64 = NULL;
				}

				if( ds64 != NULL && ds64->getID() == kChunk_ds64 )
				{
					//
					// Successfully read 'ds64' chunk.
					// Now read its data area as well and
					// add chunk to the 'RF64' chunk
					//
					ds64->cacheChunkData( stream );

					rf64->appendChild( ds64, false );
				}
				else
				{
					//
					// Either the reading failed or the 'ds64' chunk
					// doesn't exists at the expected position.
					// Now clean up and reject the stream position.
					//
					delete ds64;
					ds64 = NULL;

					stream->Seek( filePos, kXMP_SeekFromStart );
				}
			}
			else if( ds64 != NULL && ds64->getID() != kChunk_ds64 )
			{
				//
				// first child of 'RF64' chunk is NOT 'ds64'!
				//
				ds64 = NULL;
			}
		}

		//
		// parse 'ds64' chunk, store the RF64 struct and return it
		//
		if( ds64 != NULL ) 
		{
			DS64* ds64data = new DS64();

			if( this->parseDS64Chunk( *ds64, *ds64data ) )
			{
				mDS64Data	= ds64data;
				ret			= mDS64Data;
			}
			else
			{
				delete ds64data;
			}
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::updateRF64(...)
// 
// Purpose: update the RF64 chunk (if this is RF64) based on the current chunk sizes
// 
//-----------------------------------------------------------------------------

void WAVEBehavior::updateRF64( IChunkContainer& tree )
{
	if( this->isRF64( tree ) )
	{
		XMP_Validate( mDS64Data != NULL, "Missing DS64 structure", kXMPErr_InternalFailure );
		XMP_Validate( tree.numChildren() == 1, "Invalid RF64 tree", kXMPErr_InternalFailure );

		//
		// Check all chunks that sizes have changed and update their related value in the DS64 chunk
		//
		Chunk* rf64 = tree.getChildAt(0);
		XMP_Validate( rf64 != NULL && rf64->getID() == kChunk_RF64 && rf64->numChildren() > 0, "Invalid RF64 chunk", kXMPErr_InternalFailure );

		this->doUpdateRF64( *rf64 );

		//
		// try to find 'ds64' chunk in the tree
		// (needs to be the very first child of the 'RF64' chunk)
		//
		Chunk* ds64 = rf64->getChildAt(0);
		XMP_Validate( ds64 != NULL && ds64->getID() == kChunk_ds64, "Missing 'ds64' chunk", kXMPErr_InternalFailure );

		//
		// serialize DS64 structure and write into ds64 chunk
		//
		this->serializeDS64Chunk( *mDS64Data, *ds64 );		
	}
}

void WAVEBehavior::doUpdateRF64( Chunk& chunk )
{
	//
	// update ds64 entry for chunk if its size has changed
	//
	if( chunk.hasChanged() && chunk.getOriginalSize() > kNormalRF64ChunkSize )
	{
		switch( chunk.getID() )
		{
			case kChunk_RF64:	mDS64Data->riffSize = chunk.getSize();		break;
			case kChunk_data:
				if( chunk.getSize() != chunk.getOriginalSize() )
				{
					XMP_Throw( "Data chunk must not change", kXMPErr_InternalFailure );
				}
				break;
			default:
			{
				bool requireEntry = ( chunk.getSize() > kNormalRF64ChunkSize );
				bool found = false;

				//
				// try to find entry for passed chunk id in the ds64 table
				//
				if( mDS64Data->tableLength > 0 )
				{
					for( std::vector<ChunkSize64>::iterator iter=mDS64Data->table.begin(); iter!=mDS64Data->table.end(); iter++ )
					{
						if( iter->id == chunk.getID() )
						{
							// always set new size even if it's less than 4GB
							iter->size = chunk.getSize();
							found = true;
							break;
						}
					}
				}

				//
				// We can't add new entries to the table. So if we found no entry within 'ds64'
				// for the passed chunk ID and the size of the chunk is larger than 4GB then
				// we have to throw an exception
				//
				XMP_Validate( found || ( ! found && ! requireEntry ), "Can't update 'ds64' chunk", kXMPErr_Unimplemented );
			}
		}
	}

	//
	// go through all children to update ds64 data
	//
	for( XMP_Uns32 i=0; i<chunk.numChildren(); i++ )
	{
		Chunk* child = chunk.getChildAt(i);
		
		this->doUpdateRF64( *child );
	}
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::parseRF64Chunk(...)
// 
// Purpose: Parses the data block of the given RF64 chunk into the internal data structures
// 
//-----------------------------------------------------------------------------

bool WAVEBehavior::parseDS64Chunk( const Chunk& ds64Chunk, WAVEBehavior::DS64& ds64 )
{
	bool ret = false;

	// It is a valid ds64 chunk
	if( ds64Chunk.getID() == kChunk_ds64 && ds64Chunk.getSize() >= kMinimumDS64ChunkSize )
	{
		const XMP_Uns8* data;
		XMP_Uns64 size = ds64Chunk.getData(&data);

		memset( &ds64, 0, kMinimumDS64ChunkSize );

		//
		// copy fix input data into RF64 block (except chunk size table)
		// Safe as fixed size matches size of struct that is #pragma packed(1)
		//
		memcpy( &ds64, data, kMinimumDS64ChunkSize );

		// If there is more data but the table length is <= 0 then this is not a valid ds64 chunk
		if( size > kMinimumDS64ChunkSize && ds64.tableLength > 0 )
		{
			// copy chunk sizes table
			//
			XMP_Assert( size - kMinimumDS64ChunkSize >= ds64.tableLength * sizeof(ChunkSize64));

			XMP_Uns32 offset = kMinimumDS64ChunkSize;
			ChunkSize64 chunkSize;

			for( XMP_Uns32 i = 0 ; i < ds64.tableLength ; i++, offset += sizeof(ChunkSize64) )
			{
				chunkSize.id = mEndian.getUns32( data + offset );
				chunkSize.size = mEndian.getUns64( data + offset + 4 );

				ds64.table.push_back( chunkSize );
			}
		}

		// remember any existing table buffer
		ds64.trailingBytes = static_cast<XMP_Uns32>(size - kMinimumDS64ChunkSize - ds64.tableLength * sizeof(ChunkSize64));
		
		// Either a table has been correctly parsed or there was no table
		ret = (size - kMinimumDS64ChunkSize) >= (ds64.tableLength * sizeof(ChunkSize64));
	}
	
	return ret;
}

//-----------------------------------------------------------------------------
// 
// WAVEBehavior::serializeRF64Chunk(...)
// 
// Purpose: Serializes the internal RF64 data structures into the data part of the given chunk
// 
//-----------------------------------------------------------------------------
bool WAVEBehavior::serializeDS64Chunk( const WAVEBehavior::DS64& ds64, Chunk& ds64Chunk )
{
	if( ds64Chunk.getID() != kChunk_ds64 )
	{
		return false; // not a valid ds64 chunk
	}

	// Calculate needed size
	XMP_Uns32 size = kMinimumDS64ChunkSize + ds64.tableLength * sizeof(ChunkSize64) + ds64.trailingBytes;
	// Create tmp buffer
	XMP_Uns8* data = new XMP_Uns8[size];
	memset( data, 0, size );

	// copy fix input data into buffer (except chunk sizes table)
	// Safe as fixed size matches size of struct that is #pragma packed(1)
	memcpy( data, &ds64, kMinimumDS64ChunkSize );

	// copy chunk sizes table
	if( ds64.tableLength > 0 )
	{
		XMP_Uns32 offset = kMinimumDS64ChunkSize;
		
		for( XMP_Uns32 i = 0 ; i < ds64.tableLength ; i++, offset += sizeof(ChunkSize64) )
		{
			mEndian.putUns32( ds64.table.at(i).id, data + offset );
			mEndian.putUns64( ds64.table.at(i).size, data + offset + 4 );
		}
	}

	ds64Chunk.setData( data, size );

	// free tmp buffer
	delete []data;

	return true;
}
