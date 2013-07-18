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

#include "XMPFiles/source/FormatSupport/IFF/IChunkBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/Chunk.h"

#include <algorithm>

using namespace IFF_RIFF;

//-----------------------------------------------------------------------------
// 
// getIndex(...)
// 
// Purpose: [static] Calculate index of chunk in tree
// 
//-----------------------------------------------------------------------------

static XMP_Uns32 getIndex( const IChunkContainer& tree, const Chunk& chunk )
{
	const Chunk& parent = dynamic_cast<const Chunk&>( tree );

	return std::find( parent.firstChild(), parent.lastChild(), &chunk ) - parent.firstChild();
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::arrangeChunksInPlace(...)
// 
// Purpose: Try to arrange all chunks of the source tree at their current location. 
//			In a loop the method takes each chunk of the srcTree.
//			* If a chunk is a FREE chunk then it is removed.
//			* If a chunk is a known movable chunk then adjust its offset so that there's
//			  no gap to its previous chunk. If the chunk offset was adjusted and/or the
//			  chunk grew/shrank in its size then remember the offset difference for
//			  further processing
//			* If the chunk is neither movable nor a FREE chunk and there is a offset 
//			  difference then fill possible gaps with FREE chunk or move chunks to
//			  the destTree.
// 
//-----------------------------------------------------------------------------

void IChunkBehavior::arrangeChunksInPlace( IChunkContainer& srcTree, IChunkContainer& destTree )
{
	XMP_Validate( &srcTree != &destTree, "Source and destination tree mustn't be the same", kXMPErr_InternalFailure );

	XMP_Int64 offsetAdjust = 0;

	for( XMP_Int32 index=0; index<static_cast<XMP_Int32>( srcTree.numChildren() ); index++ )
	{
		Chunk* chunk = srcTree.getChildAt( index );

		//
		// Is chunk one that might be moved in the tree
		// (and so it's a chunk that might be modified)
		//
		if( this->isMovable( *chunk ) )
		{
			//
			// Are there FREE chunks above this chunk?
			// Then remove it.
			//
			Chunk* freeChunk = NULL;

			if( index > 0 )
			{
				// find FREE chunk and merge possible multiple FREE chunks to one chunk
				freeChunk = this->mergeFreeChunks( srcTree, index-1 );
			}

			if( freeChunk != NULL )
			{
				// update running index
				index = ::getIndex( srcTree, *chunk );

				// subtract size of FREE chunk from offset adjust value
				offsetAdjust -= static_cast<XMP_Int64>( freeChunk->getPadSize(true) );

				// remove FREE chunk from the tree 
				srcTree.removeChildAt( index-1 );
				delete freeChunk;

				// update running index because one chunk was removed from the tree
				index--;
			}

			//
			// offset needs to be adjusted
			//
			if( offsetAdjust != 0 )
			{
				chunk->setOffset( chunk->getOffset() + offsetAdjust );
			}

			//
			// update adjust value if the size of the chunk has changed 
			// (and so the offsets of following chunks needs to be adjusted)
			//
			offsetAdjust += chunk->getPadSize() - chunk->getOriginalPadSize();
		}
		else if( this->isFREEChunk( *chunk ) && offsetAdjust != 0 )
		{
			//
			// chunk is a FREE chunk, just remove it
			//

			// merge FREE chunks
			chunk = this->mergeFreeChunks( srcTree, index );

			// update running index (in case multiple FREE chunk were merged)
			index = ::getIndex( srcTree, *chunk );

			// update adjust value about the total size of the FREE chunk
			offsetAdjust -= static_cast<XMP_Int64>( chunk->getPadSize(true) );

			// remove FREE chunk from tree
			srcTree.removeChildAt( index );
			delete chunk;

			// update running index
			index--;
		}
		else if( offsetAdjust != 0 )
		{
			//
			// the current chunk can't be moved,
			// so we can't adjust the offset of this chunk
			//
			XMP_Uns64 gap = 0;

			if( offsetAdjust > 0 )
			{
				//
				// One or more foregoing chunks grew in their seize and so
				// the offset of following chunks needs to be adjusted.
				// But since the current chunk can't be moved one or more previous
				// chunks are now overlapping over the current chunk.
				//
				// So now one or more of the previous chunks needs to be removed 
				// (moved to the destTree) so that the offset value of the current
				// chunk can stay where it is.
				// A possible gap will be filled with a FREE chunk.
				//

				Chunk* preChunk = NULL;

				//
				// count Chunks that needs to be moved
				//
				XMP_Validate( index-1 >= 0, "There shouldn't be an offset adjust value for the first chunk", kXMPErr_InternalFailure );

				XMP_Int32 preIndex = index;
				XMP_Uns64 preSize  = 0;

				do 
				{
					preIndex--;
					preChunk = srcTree.getChildAt( preIndex );

					XMP_Validate( this->isMovable( *preChunk ) || this->isFREEChunk( *preChunk ), "Movable or FREE chunk expected", kXMPErr_InternalFailure );

					preSize += preChunk->getPadSize( true );

				} while( static_cast<XMP_Int64>( preSize ) < offsetAdjust && preIndex > 0 );

				//
				// move chunks
				//
				for( XMP_Uns32 rem=preIndex; rem<static_cast<XMP_Uns32>( index ); rem++ )
				{
					// always fetch chunk at the first index of the range because
					// these chunks are removed from the tree
					preChunk = srcTree.removeChildAt( preIndex );

					if( this->isFREEChunk( *preChunk ) )
					{
						delete preChunk;
					}
					else
					{
						destTree.appendChild( preChunk, false );
					}
				}

				// update current index
				index = ::getIndex( srcTree, *chunk );

				//
				// calculate size of gap
				//
				XMP_Uns64 curOffset = chunk->getOffset();
				XMP_Uns64 preOffset = Chunk::HEADER_SIZE + Chunk::TYPE_SIZE;

				if( index > 0 )
				{
					preChunk = srcTree.getChildAt( index-1 );
					preOffset = preChunk->getOffset() + preChunk->getPadSize( true );
				}

				gap = curOffset - preOffset;
			}
			else if( offsetAdjust < 0 )
			{
				//
				// There is a gap between the previous chunk and the current one.
				// Fill the gap with a FREE chunk.
				//
				gap = offsetAdjust * (-1);
			}

			//
			// if there is a gap we need to fill it with a FREE chunk
			//
			if( gap > 0 )
			{
				//
				// The gap must be at least as big as the minimum size of FREE chunks.
				// If that's not the case we need to move more chunks to expand
				// the gap.
				//
				while( gap < this->getMinFREESize() )
				{
					XMP_Validate( index > 0, "Not enough space to insert FREE chunk", kXMPErr_Unimplemented );

					Chunk* preChunk = srcTree.removeChildAt( index-1 );
					gap += preChunk->getPadSize(true);
					destTree.appendChild( preChunk, false );

					// update running index
					index--;
				}

				//
				// Fill the gap with a FREE chunk.
				//
				Chunk* freeChunk = this->createFREE( gap );
				srcTree.insertChildAt( index, freeChunk );
				freeChunk->setAsNew();

				// update running index
				index++;
			}

			// reset adjust value
			offsetAdjust = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::arrangeChunksInTree(...)
// 
// Purpose: This method proceeds the list of Chunks of the source tree in the 
//			passed range and looks for FREE chunks in the destination tree to 
//			move the source chunks to.
//			Source tree and destination tree could be one and the same but it's 
//			not required. If both trees are the	same then it's not allowed to 
//			cross source and destination ranges.
// 
//-----------------------------------------------------------------------------

void IChunkBehavior::arrangeChunksInTree( IChunkContainer& srcTree, IChunkContainer& destTree )
{
	XMP_Validate( &srcTree != &destTree, "Source and destination tree mustn't be the same", kXMPErr_InternalFailure );

	if( srcTree.numChildren() > 0 )
	{
		//
		// for all chunks that were moved to the end try to find a FREE chunk for them
		//
		for( XMP_Int32 index=srcTree.numChildren()-1; index>=0; index-- )
		{
			Chunk* chunk = srcTree.getChildAt(index);

			//
			// find a FREE chunk where the chunk would fit in
			//
			XMP_Int32 freeIndex = this->findFREEChunk( destTree, chunk->getSize(true) );

			if( freeIndex >= 0 )
			{
				Chunk* freeChunk = destTree.getChildAt( freeIndex );

				// remove chunk from source tree
				srcTree.removeChildAt( index );

				// insert chunk at new location
				destTree.insertChildAt( freeIndex, chunk );

				// remove the FREE chunk
				destTree.removeChildAt( freeIndex+1 );

				//
				// if the size of the FREE chunk is larger than the size of the chunk then fill
				// the gap with a new FREE chunk (the method findFREEChunk takes care that the
				// remaining space is large enough for a new FREE chunk, but findFREEChunk also
				// takes account of possible pad bytes in its calculations! Therefore following
				// calculations have to take account of a possible pad byte as well!)
				//
				if( freeChunk->getPadSize( true ) > chunk->getPadSize( true ) )
				{
					Chunk* remainFreeChunk = this->createFREE( freeChunk->getPadSize( true ) - chunk->getPadSize( true ) );
					destTree.insertChildAt( freeIndex+1, remainFreeChunk );
					remainFreeChunk->setAsNew();
				}

				delete freeChunk;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::validateOffsets(...)
// 
// Purpose: Fix recursively the offset values of all modified chunks. 
//			At the same time the method checks the offset value of all not 
//			modified chunks and throws an exception if there is any discrepance
//			with the calculated offset.
// 
//-----------------------------------------------------------------------------

void IChunkBehavior::validateOffsets( IChunkContainer& tree, XMP_Uns64 startOffset /*= 0*/ )
{
	XMP_Uns64 offset = startOffset;

	//
	// for all children of the tree
	//
	for( XMP_Uns32 i=0; i<tree.numChildren(); i++ )
	{
		Chunk* chunk = tree.getChildAt(i);

		// the offset of a not modified chunk should match the calculated offset
		XMP_Validate( chunk->getOffset() == offset, "Invalid offset", kXMPErr_InternalFailure );

		if( !this->isMovable( *chunk ) )
		{
			XMP_Validate( chunk->getOffset() == chunk->getOriginalOffset(), "Invalid offset non-modified chunk", kXMPErr_InternalFailure );
		}

		// go through children
		if( chunk->getChunkMode() == CHUNK_NODE )
		{
			this->validateOffsets( *chunk, offset + Chunk::HEADER_SIZE + Chunk::TYPE_SIZE );
		}

		// calculate next offset
		offset += chunk->getPadSize(true);
	}
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::getFreeSpace(...)
// 
// Purpose: Retrieve the free space at the passed position in the child list of 
//			the parent tree. If there's	a FREE chunk then return it.
// 
//-----------------------------------------------------------------------------

Chunk* IChunkBehavior::getFreeSpace( XMP_Int64& outFreeBytes, const IChunkContainer& tree, XMP_Uns32 index ) const
{
	// validate index
	XMP_Validate( index < tree.numChildren(), "Invalid index", kXMPErr_InternalFailure );

	Chunk* ret = NULL;

	Chunk* chunk = tree.getChildAt( index );

	if( this->isFREEChunk( *chunk ) )
	{
		//
		// chunk is a FREE chunk
		//
		outFreeBytes = chunk->getSize( true );
		ret = chunk;
	}
	else if( chunk->getChunkMode() != CHUNK_UNKNOWN && chunk->hasChanged() )
	{
		//
		// chunk is NOT a FREE chunk but the size of this chunk has changed
		//
		outFreeBytes = chunk->getOriginalSize() - chunk->getSize();
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::mergeFreeChunks(...)
// 
// Purpose: Try to merge existing FREE chunks at the passed position in the 
//			child list of the passed parent tree. The algorithm looks at the 
//			position, before the position and after the position.
// 
//-----------------------------------------------------------------------------

Chunk* IChunkBehavior::mergeFreeChunks( IChunkContainer& tree, XMP_Uns32 index )
{
	// validate index
	XMP_Validate( index < tree.numChildren(), "Invalid index", kXMPErr_InternalFailure );

	Chunk* ret = NULL;

	Chunk* chunk = tree.getChildAt( index );

	//
	// is chunk a FREE chunk
	//
	if( this->isFREEChunk( *chunk ) )
	{
		XMP_Uns32 indexStart	= index;
		XMP_Uns32 indexEnd		= index;

		XMP_Uns64 size			= chunk->getPadSize( true );

		//
		// find FREE chunks before start chunk
		//
		if( index > 0 )
		{
			XMP_Int32 i = XMP_Int32( index-1 );
			Chunk* c	= NULL;

			do
			{
				c = tree.getChildAt(i);

				if( this->isFREEChunk( *c ) )
				{
					size += c->getPadSize( true );
					indexStart = XMP_Uns32(i);
					i--;
				}
				else
				{
					c = NULL;
				}

			} while( i >= 0 && c != NULL );
		}

		//
		// find FREE chunks after start chunk
		//
		if( index+1 < tree.numChildren() )
		{
			XMP_Uns32 i = index+1;
			Chunk* c	= NULL;

			do
			{
				c = tree.getChildAt(i);

				if( this->isFREEChunk( *c ) )
				{
					size += c->getPadSize( true );
					indexEnd = i;
					i++;
				}
				else
				{
					c = NULL;
				}

			} while( i < tree.numChildren() && c != NULL );
		}

		if( indexStart < indexEnd )
		{
			//
			// more than one FREE chunks, so merge them
			//
			for( XMP_Uns32 i=indexStart; i<=indexEnd; i++ )
			{
				Chunk* f = tree.getChildAt( indexStart );
				tree.removeChildAt( indexStart );
				delete f;
			}

			ret  = this->createFREE( size );
			tree.insertChildAt( indexStart, ret );
			ret->setAsNew();
		}
		else
		{
			//
			// one single FREE chunk
			//
			ret = chunk;
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::findFREEChunk(...)
// 
// Purpose: Find a FREE chunk with the passed total size (including header). 
//			If the FREE chunk is found then take care of the fact that is has 
//			to be that large (or larger) then the minimum size of a FREE chunk.
//			The method takes also into account that the passed size probably 
//			not includes a pad byte
// 
//-----------------------------------------------------------------------------

XMP_Int32 IChunkBehavior::findFREEChunk( const IChunkContainer& tree, XMP_Uns64 requiredSize /*including header*/ )
{
	XMP_Int32 ret = -1;

	for( XMP_Uns32 i=0; i<tree.numChildren(); i++ )
	{
		Chunk* chunk = tree.getChildAt(i);

		XMP_Uns64 requiredSizePad	= requiredSize + ( requiredSize % 2 );	// required size including pad byte

		if( this->isFREEChunk( *chunk ) && 
			( chunk->getPadSize( true ) == requiredSizePad || 
			chunk->getPadSize( true ) >= requiredSizePad + getMinFREESize() ) )
		{
			ret = i;
			break;
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::moveChunks(...)
// 
// Purpose: Move a range of chunks from one container to another.
// 
//-----------------------------------------------------------------------------

void IChunkBehavior::moveChunks( IChunkContainer& srcTree, IChunkContainer& destTree, XMP_Uns32 start )
{
	XMP_Validate( &srcTree != &destTree, "Source tree and destination tree shouldn't be the same", kXMPErr_InternalFailure );

	XMP_Uns32 end = srcTree.numChildren();

	for( XMP_Uns32 index=start; index<end; index++ )
	{
		Chunk* chunk = srcTree.removeChildAt( start );
		destTree.appendChild( chunk, true );
	}
}

//-----------------------------------------------------------------------------
// 
// IChunkBehavior::isMovable(...)
// 
// Purpose: May we move a chunk of passed id/type
// 
//-----------------------------------------------------------------------------

bool IChunkBehavior::isMovable( const Chunk& chunk ) const
{
	bool ret = false;

	if( !this->isFREEChunk( chunk ) && mMovablePaths != NULL )
	{
		ChunkPath path( chunk.getIdentifier() );
		Chunk* parent = chunk.getParent();

		while( parent != NULL && parent->getID() != kChunk_NONE )
		{
			path.insert( parent->getIdentifier() );
			parent = parent->getParent();
		}

		for( std::vector<ChunkPath>::iterator iter=mMovablePaths->begin(); iter!=mMovablePaths->end() && !ret; iter++ )
		{
			ret = ( iter->match( path ) == ChunkPath::kFullMatch );
		}
	}

	return ret;
}
