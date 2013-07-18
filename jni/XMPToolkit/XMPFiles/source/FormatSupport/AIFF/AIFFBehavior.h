// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _AIFFBEHAVIOR_h_
#define _AIFFBEHAVIOR_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

#include "XMPFiles/source/FormatSupport/IFF/IChunkBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "source/Endian.h"

namespace IFF_RIFF
{

/**
	AIFF behavior class.

	Implements the IChunkBehavior interface
*/


class AIFFBehavior : public IChunkBehavior
{
public:
	/**
		ctor/dtor
	*/
					 AIFFBehavior() : mChunksAdded(0) {}
					~AIFFBehavior() {}

	/**
		Validate the passed in size value, identify the valid size if the passed in isn't valid
		and return the valid size.
		throw an exception if the passed in size isn't valid and there's no way to identify a
		valid size.

		@param	size	Size value
		@param	id		Identifier of chunk
		@param	tree	Chunk tree
		@param	stream	Stream handle

		@return		Valid size value.
	*/
	XMP_Uns64		getRealSize( const XMP_Uns64 size, const ChunkIdentifier& id, IChunkContainer& tree, XMP_IO* stream );

	/**
		Return the maximum size of a single chunk, i.e. the maximum size of a top-level chunk.

		@return		Maximum size
	*/
	XMP_Uns64		getMaxChunkSize() const;

	/**
		Return true if the passed identifier is valid for top-level chunks of a certain format.

		@param	id		Chunk identifier
		@param	chunkNo	order number of top-level chunk
		@return			true, if passed id is a valid top-level chunk
	*/
	bool			isValidTopLevelChunk( const ChunkIdentifier& id, XMP_Uns32 chunkNo );

	/**
		Fix the hierarchy of chunks depending ones based on size changes of one or more chunks
		and second based on format specific rules.
		Throw an exception if the hierarchy can't be fixed.

		@param	tree		Vector of root chunks.
	*/
	void			fixHierarchy( IChunkContainer& tree );

	/**
		Insert a new chunk into the hierarchy of chunks. The behavior needs to decide the position
		of the new chunk and has to do the insertion.

		@param	tree	Chunk tree
		@param	chunk	New chunk
	*/
	void			insertChunk( IChunkContainer& tree, Chunk& chunk )	;

	/**
		Remove the chunk described by the passed ChunkPath.

		@param	tree	Chunk tree
		@param	path	Path to the chunk that needs to be removed

		@return			true if the chunk was removed and need to be deleted
	*/
	bool			removeChunk( IChunkContainer& tree, Chunk& chunk )	;

private:


	/**
		Create a FREE chunk.
		If the chunkSize is smaller than the header+type - size then create an annotation chunk.
		If the passed size is odd, then add a pad byte.

		@param chunkSize	Total size including header
		@return				New FREE chunk
	*/
	Chunk*			createFREE( XMP_Uns64 chunkSize );

	/**
		Check if the passed chunk is a FREE chunk.
		(Could be also a small annotation chunk with zero bytes in its data)

		@param	chunk	A chunk

		@return			true if the passed chunk is a FREE chunk
	*/
	XMP_Bool		isFREEChunk( const Chunk& chunk ) const;

	/**
		Retrieve the free space at the passed position in the child list of the parent tree. 
		If there's a FREE chunk then return it.

		@param outFreeBytes		On return it takes the number of free bytes
		@param tree				Parent tree
		@param index			Position in the child list of the parent tree

		@return					FREE chunk if available
	*/
	Chunk*			getFreeSpace( XMP_Int64& outFreeBytes, const IChunkContainer& tree, XMP_Uns32 index ) const;

	/**
		Return the minimum size of a FREE chunk
	*/
	XMP_Uns64		getMinFREESize( ) const;

private:
	XMP_Uns32	mChunksAdded;

	/** AIFF is always Big Endian */
	static const BigEndian& mEndian;

}; // IFF_RIFF

}
#endif
