// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _WAVEBEHAVIOR_h_
#define _WAVEBEHAVIOR_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/FormatSupport/IFF/IChunkBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "source/Endian.h"

namespace IFF_RIFF
{

/**
	WAVE behavior class.
	
	Implements the IChunkBehavior interface
*/


class WAVEBehavior : public IChunkBehavior
{
// Internal structure to hold RF64 related data
public:
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86
	struct ChunkSize64
	{
		XMP_Uns64 size;
		XMP_Uns32 id;
		// Ctor
		ChunkSize64(): size(0), id(0) {}
	};

	struct DS64
	{
		XMP_Uns64 riffSize;
		XMP_Uns64 dataSize;
		XMP_Uns64 sampleCount;
		XMP_Uns32 tableLength;
		// fix part ends here
		XMP_Uns32 trailingBytes;
		std::vector<ChunkSize64> table;

		// ctor
		DS64(): riffSize(0), dataSize(0), sampleCount(0), tableLength(0), trailingBytes(0) {}
	};
#if SUNOS_SPARC || SUNOS_X86
#pragma pack (  )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86


	/**
		ctor/dtor
	*/
	WAVEBehavior() : mChunksAdded(0), mIsRF64(false), mDS64Data(NULL) {}
	
	virtual ~WAVEBehavior() 
	{
		if( mDS64Data != NULL )
		{
			delete mDS64Data;
		}
	}

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

protected:
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
		Return the minimum size of a FREE chunk
	*/
	XMP_Uns64		getMinFREESize( ) const;

	/**
		Is the current file a RF64 file?

		@param tree		The whole chunk tree beginning with the root node

		@return			true if the current file is in the RF64 format
	*/
	bool			isRF64( const IChunkContainer& tree );

	/**
		Return RF64 structure.

		If the related chunk ('ds64') is not yet parsed then it should be parsed by this method.

		@param tree		The chunk tree (probably a subtree)
		@param stream	File stream

		@return			DS64 structure
	*/
	DS64*			getDS64( IChunkContainer& tree, XMP_IO* stream );

	/**
		update the RF64 chunk (if this is RF64) based on the current chunk sizes
	*/
	void			updateRF64( IChunkContainer& tree );

	/**
	 * Parses the data block of the given RF64 chunk into the internal data structures
	 * @param rf64Chunk the RF64 Chunk
	 * @param rf64 OUT the RF64 data structure 
	 * @return The parsing was successful (true) or not (false)
	 */
	bool			parseDS64Chunk( const Chunk& ds64Chunk, DS64& rf64 );

	/**
	 * Serializes the internal RF64 data structures into the data part of the given chunk
	 * @param rf64 the RF64 data structure
	 * @param rf64Chunk OUT the RF64 Chunk
	 * @return The serialization was successful (true) or not (false)
	 */
	bool			serializeDS64Chunk( const WAVEBehavior::DS64& rf64, Chunk& ds64Chunk );

private:
	void			doUpdateRF64( Chunk& chunk );

private:
	XMP_Uns32	mChunksAdded;
	bool		mIsRF64;
	DS64*		mDS64Data;
	
	static const LittleEndian& mEndian;				// WAVE is always Little Endian
	static const XMP_Uns32 kNormalRF64ChunkSize = 0xFFFFFFFF;
	static const XMP_Uns32 kMinimumDS64ChunkSize = 28;
}; // IFF_RIFF

}
#endif
