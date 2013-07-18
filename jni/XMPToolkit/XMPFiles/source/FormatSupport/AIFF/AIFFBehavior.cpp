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

#include "XMPFiles/source/FormatSupport/AIFF/AIFFBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/Chunk.h"
#include "source/XMP_LibUtils.hpp"
#include "source/XIO.hpp"

#include <algorithm>

using namespace IFF_RIFF;

//
// Static init
//
const BigEndian& AIFFBehavior::mEndian = BigEndian::getInstance();

//-----------------------------------------------------------------------------
// 
// AIFFBehavior::getRealSize(...)
// 
// Purpose: Validate the passed in size value, identify the valid size if the 
//			passed in isn't valid and return the valid size.
//			Throw an exception if the passed in size isn't valid and there's 
//			no way to identify a valid size.
// 
//-----------------------------------------------------------------------------

XMP_Uns64 AIFFBehavior::getRealSize( const XMP_Uns64 size, const ChunkIdentifier& id, IChunkContainer& tree, XMP_IO* stream )
{
	if( (size & 0x80000000) > 0 )
	{
		XMP_Throw( "Unknown size value", kXMPErr_BadFileFormat );
	}

	return size;
}

//-----------------------------------------------------------------------------
// 
// AIFFBehavior::isValidTopLevelChunk(...)
// 
// Purpose: Return true if the passed identifier is valid for top-level chunks 
//			of a certain format.
// 
//-----------------------------------------------------------------------------

bool AIFFBehavior::isValidTopLevelChunk( const ChunkIdentifier& id, XMP_Uns32 chunkNo )
{
	return (chunkNo == 0) && (id.id == kChunk_FORM) && ((id.type == kType_AIFF) || (id.type == kType_AIFC));
}

//-----------------------------------------------------------------------------
// 
// AIFFBehavior::getMaxChunkSize(...)
// 
// Purpose: Return the maximum size of a single chunk, i.e. the maximum size 
//			of a top-level chunk.
// 
//-----------------------------------------------------------------------------

XMP_Uns64 AIFFBehavior::getMaxChunkSize() const
{
	return 0x80000000LL;	// 2 GByte
}

//-----------------------------------------------------------------------------
// 
// AIFFBehavior::fixHierarchy(...)
// 
// Purpose: Fix the hierarchy of chunks depending ones based on size changes of 
//			one or more chunks and second based on format specific rules.
//			Throw an exception if the hierarchy can't be fixed.
// 
//-----------------------------------------------------------------------------

void AIFFBehavior::fixHierarchy( IChunkContainer& tree )
{
	XMP_Validate( tree.numChildren() == 1, "AIFF files should only have one top level chunk (FORM)", kXMPErr_BadFileFormat);
	Chunk* formChunk = tree.getChildAt(0);

	XMP_Validate( (formChunk->getType() == kType_AIFF) || (formChunk->getType() == kType_AIFC), "Invalid type for AIFF/AIFC top level chunk (FORM)", kXMPErr_BadFileFormat);

	if( formChunk->hasChanged() )
	{
		//
		// none of the modified chunks should be smaller than 12Byte
		//
		for( XMP_Uns32 i=0; i<formChunk->numChildren(); i++ )
		{
			Chunk* chunk = formChunk->getChildAt(i);

			if( chunk->hasChanged() && chunk->getSize() != chunk->getOriginalSize() )
			{
				XMP_Validate( chunk->getSize() >= Chunk::TYPE_SIZE, "Modified chunk smaller than 12bytes", kXMPErr_InternalFailure );
			}
		}

		//
		// move new added chunks to temporary container
		//
		Chunk* tmpContainer = Chunk::createChunk( mEndian );
		this->moveChunks( *formChunk, *tmpContainer, formChunk->numChildren() - mChunksAdded );

		//
		// for all children chunks until the last child of the initial list is reached
		// try to arrange the chunks at the current location using exisiting free space
		// or FREE chunks around, otherwise move the chunk to the end
		//
		this->arrangeChunksInPlace( *formChunk, *tmpContainer );

		//
		// for all chunks that were moved to the end try to find a FREE chunk for them
		//
		this->arrangeChunksInTree( *tmpContainer, *formChunk );

		//
		// append all remaining new added chunks to the end of the tree
		//
		this->moveChunks( *tmpContainer, *formChunk, 0 );
		delete tmpContainer;

		//
		// check for FREE chunks at the end
		//
		Chunk* endFREE = this->mergeFreeChunks( *formChunk, formChunk->numChildren() - 1 );

		if( endFREE != NULL )
		{
			formChunk->removeChildAt( formChunk->numChildren() - 1 );
			delete endFREE;
		}

		//
		// Fix the offset values of all chunks. Throw an exception in the case that
		// the offset of a non-modified chunk needs to be reset.
		//
		XMP_Validate( formChunk->getOffset() == 0, "Invalid offset for AIFF/AIFC top level chunk (FORM)", kXMPErr_InternalFailure );

		this->validateOffsets( tree );
	}
}

void AIFFBehavior::insertChunk( IChunkContainer& tree, Chunk& chunk )
{
	XMP_Validate( tree.numChildren() == 1, "AIFF files should only have one top level chunk (FORM)", kXMPErr_BadFileFormat);
	Chunk* formChunk = tree.getChildAt(0);

	XMP_Validate( (formChunk->getType() == kType_AIFF) || (formChunk->getType() == kType_AIFC), "Invalid type for AIFF/AIFC top level chunk (FORM)", kXMPErr_BadFileFormat);

	// add new chunk to the end of the AIFF:FORM
	formChunk->appendChild(&chunk);

	mChunksAdded++;
}

bool AIFFBehavior::removeChunk( IChunkContainer& tree, Chunk& chunk )
{
	XMP_Validate( chunk.getID() != kChunk_FORM, "Can't remove FORM chunk!", kXMPErr_InternalFailure );
	XMP_Validate( chunk.getChunkMode() != CHUNK_UNKNOWN, "Cant' remove UNKNOWN Chunk", kXMPErr_InternalFailure );
	
	XMP_Validate( tree.numChildren() == 1, "AIFF files should only have one top level chunk (FORM)", kXMPErr_BadFileFormat);	

	Chunk* formChunk = tree.getChildAt(0);

	XMP_Validate( (formChunk->getType() == kType_AIFF) || (formChunk->getType() == kType_AIFC), "Invalid type for AIFF/AIFC top level chunk (FORM)", kXMPErr_BadFileFormat);

	XMP_Uns32 i = std::find( formChunk->firstChild(), formChunk->lastChild(), &chunk ) - formChunk->firstChild();

	XMP_Validate( i < formChunk->numChildren(), "Invalid chunk in tree", kXMPErr_InternalFailure );

	//
	// adjust new chunks counter
	//
	if( i > formChunk->numChildren() - mChunksAdded - 1 )
	{
		mChunksAdded--;
	}

	if( i < formChunk->numChildren()-1 )
	{
		//
		// fill gap with free chunk
		//
		Chunk* free = this->createFREE( chunk.getPadSize( true ) );
		formChunk->replaceChildAt( i, free );
		free->setAsNew();

		//
		// merge JUNK chunks
		//
		this->mergeFreeChunks( *formChunk, i );
	}
	else
	{
		//
		// remove chunk from tree
		//
		formChunk->removeChildAt( i );
	}

	return true;
}

XMP_Bool AIFFBehavior::isFREEChunk( const Chunk& chunk ) const
{
	XMP_Bool ret = ( chunk.getID() == kChunk_APPL && chunk.getType() == kType_FREE );

	//
	// if the signature is not 'APPL':'FREE' the it could be an annotation chunk
	// (ID: 'ANNO') which data area is smaller than 4bytes and the data is zero
	//
	if( !ret && chunk.getID() == kChunk_ANNO && chunk.getSize() < Chunk::TYPE_SIZE )
	{
		ret = chunk.getSize() == 0;
		
		if( !ret )
		{
			const XMP_Uns8* buffer;
			chunk.getData( &buffer );

			XMP_Uns8* data = new XMP_Uns8[static_cast<size_t>( chunk.getSize() )];
			memset( data, 0, static_cast<size_t>( chunk.getSize() ) );

			ret = ( memcmp( data, buffer, static_cast<size_t>( chunk.getSize() ) ) == 0 );

			delete[] data;
		}
	}

	return ret;
}

Chunk* AIFFBehavior::createFREE( XMP_Uns64 chunkSize )
{
	XMP_Int64 alloc = chunkSize - Chunk::HEADER_SIZE;

	Chunk* chunk = NULL;
	XMP_Uns8* data = NULL;

	if( alloc > 0 )
	{
		data = new XMP_Uns8[static_cast<size_t>( alloc )];
		memset( data, 0, static_cast<size_t>( alloc ) );
	}

	if( alloc < Chunk::TYPE_SIZE )
	{
		//
		// if the required size is smaller than the minimum size of a 'APPL':'FREE' chunk
		// then create an annotation chunk 'ANNO' and zero the data
		//
		if( alloc > 0 )
		{
			chunk = Chunk::createUnknownChunk( mEndian, kChunk_ANNO, 0, alloc );
			chunk->setData( data, alloc );
		}
		else
		{
			chunk = Chunk::createHeaderChunk( mEndian, kChunk_ANNO );
		}
	}
	else
	{
		//
		// create a 'APPL':'FREE' chunk
		//
		alloc -= Chunk::TYPE_SIZE;

		if( alloc > 0 )
		{
			chunk = Chunk::createUnknownChunk( mEndian, kChunk_APPL, kType_FREE, alloc+Chunk::TYPE_SIZE );
			chunk->setData( data, alloc, true );
		}
		else
		{
			chunk = Chunk::createHeaderChunk( mEndian, kChunk_APPL, kType_FREE );
		}
	}

	delete[] data;

	// force set dirty flag
	chunk->setChanged();

	return chunk;
}

XMP_Uns64 AIFFBehavior::getMinFREESize() const
{
	// avoid creation of chunks with size==0
	return static_cast<XMP_Uns64>( Chunk::HEADER_SIZE ) + 2;
}
