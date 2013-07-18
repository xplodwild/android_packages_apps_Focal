// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _IChunkBehavior_h_
#define _IChunkBehavior_h_

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"
#include <vector>

namespace IFF_RIFF
{

/**
	The IChunkBehavior is an interface that provides access to algorithm
	for the read and write process of IFF/RIFF formated streams.
	A file format specific instance based on this interface gets injected into
	the class ChunkController and offers format specific algorithm wherever
	the processing of a certain file format differs from the general specification
	of RIFF/IFF.
	That is e.g. the RF64 format where it is possible that the size value of the
	top level chunk doesn't represent the real size. Or AVI, where are special rules
	if the size of a chunk exceed the 4GB border.
*/

class IChunkContainer;
class Chunk;
struct ChunkIdentifier;
class ChunkPath;

class IChunkBehavior
{
public:
			 IChunkBehavior() : mMovablePaths(NULL) {}
	virtual ~IChunkBehavior() {};

	/**
	 * Set list of chunk paths of chunks that might be moved within the hierarchy
	 */
	inline void setMovablePaths( std::vector<ChunkPath>* paths )		{ mMovablePaths = paths; }

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
	virtual	XMP_Uns64		getRealSize( const XMP_Uns64 size, const ChunkIdentifier& id, IChunkContainer& tree, XMP_IO* stream )	= 0;

	/**
		Return the maximum size of a single chunk, i.e. the maximum size of a top-level chunk.

		@return		Maximum size
	*/
	virtual XMP_Uns64		getMaxChunkSize() const																							= 0;

	/**
		Return true if the passed identifier is valid for top-level chunks of a certain format.

		@param	id			Chunk identifier
		@param	chunkNo		order number of top-level chunk
		@return				true, if passed id is a valid top-level chunk
	*/
	virtual	bool			isValidTopLevelChunk( const ChunkIdentifier& id, XMP_Uns32 chunkNo )											= 0;

	/**
		Fix the hierarchy of chunks depending ones based on size changes of one or more chunks
		and second based on format specific rules.
		Throw an exception if the hierarchy can't be fixed.

		@param	tree		Vector of root chunks.
	*/
	virtual	void			fixHierarchy( IChunkContainer& tree )																			= 0;

	/**
		Insert a new chunk into the hierarchy of chunks. The behavior needs to decide the position
		of the new chunk and has to do the insertion.

		@param	tree	Chunk tree
		@param	chunk	New chunk
	*/
	virtual	void			insertChunk( IChunkContainer& tree, Chunk& chunk )																= 0;

	/**
		Remove the chunk described by the passed ChunkPath.

		@param	tree	Chunk tree
		@param	path	Path to the chunk that needs to be removed

		@return			true if the chunk was removed and need to be deleted
	*/
	virtual	bool			removeChunk( IChunkContainer& tree, Chunk& chunk )																= 0;

protected:
		/**
		Create a FREE chunk.
		If the chunkSize is smaller than the header+type - size then create an annotation chunk.
		If the passed size is odd, then add a pad byte.

		@param chunkSize	Total size including header
		@return				New FREE chunk
	*/
	virtual Chunk*			createFREE( XMP_Uns64 chunkSize )																				= 0;
	
	/**
		Check if the passed chunk is a FREE chunk.
		(Could be also a small annotation chunk with zero bytes in its data)

		@param	chunk	A chunk

		@return			true if the passed chunk is a FREE chunk
	*/
	virtual XMP_Bool		isFREEChunk( const Chunk& chunk ) const																			= 0;

	/**
		Return the minimum size of a FREE chunk
	*/
	virtual XMP_Uns64		getMinFREESize( ) const			
																																			= 0;
protected:
	/************************************************************************/
	/* END of Interface. The following are helper functions for all derived */
	/* Behavior Classes                                                     */
	/************************************************************************/

	/**
		Find a FREE chunk with the passed total size (including header). If the FREE chunk is found then
		take care of the fact that is has to be that large (or larger) then the minimum size of a FREE chunk.
		The method takes also into account that the passed size probably not includes a pad byte

		@param	tree			Parent tree
		@param	requiredSize	Required total size (including header)

		@return					Index of found FREE chunk
	*/
	XMP_Int32		findFREEChunk( const IChunkContainer& tree, XMP_Uns64 requiredSize );
	
	/**
		May we move a chunk of passed id/type

		@param	identifier		id and type of chunk
		@return					true if such a chunk might be moved within the tree
	*/
	bool			isMovable( const Chunk& chunk ) const;

	/**
		Validate recursively the offset values of all chunks. 
		Throws an exception if there is any discrepancy with the calculated offset.

		@param	tree			(Sub-)tree of chunks
		@param	startOffset		First offset in the (sub-)tree
	*/
	void			validateOffsets( IChunkContainer& tree, XMP_Uns64 startOffset = 0 );

	/**
		Retrieve the free space at the passed position in the child list of the parent tree. 
		If there's a FREE chunk then return it.

		@param outFreeBytes		On return it takes the number of free bytes
		@param tree				Parent tree
		@param index			Position in the child list of the parent tree

		@return					FREE chunk if available
	*/
	Chunk*			getFreeSpace( XMP_Int64& outFreeBytes, const IChunkContainer& tree, XMP_Uns32 index ) const ;

	/**
		Try to arrange all chunks of the source tree at their current location. 
		The method looks for FREE chunk around or for size changes of the chunks around and try that space.
		If a chunk can't be arrange at its location it is moved to the end of the destination tree.

		@param srcTree		Tree that consists of the chunks that needs to be arranged
		@param destTree		Tree where chunks are added to if they can't be arranged

		@return				Index of last proceeded chunk
		*/
	void			arrangeChunksInPlace( IChunkContainer& srcTree, IChunkContainer& destTree );

	/**
		This method proceeds the list of Chunks of the source tree in the passed range and looks for FREE chunks
		in the destination tree to move the source chunks to.
		Source tree and destination tree could be one and the same but it's not required. If both trees are the
		same then it's not allowed to cross source and destination ranges.

		@param srcTree		Tree that consists of the chunks that needs to be arranged
		@param destTree		Tree where the method looks for FREE chunks
		@param srcStart		Start index within the source tree
		@param srcEnd		End index within the source tree (if booth, srcStart and srcEnd are zero then the complete list
							of the source tree is proceeded)
		@param destStart	Start index within the destination tree
		@param destEnd		End index within the destination tree (if booth, destStart and destEnd are zero then the complete list
							of the destination tree is proceeded)
	*/
	void	 		arrangeChunksInTree( IChunkContainer& srcTree, IChunkContainer& destTree );

	/**
		Try to merge existing FREE chunks at the passed position in the child list
		of the passed parent tree. 
		The algorithm looks at the position, before the position and after the position.

		@param tree			Parent tree
		@param index		Position in the child list of the parent tree

		@return				FREE chunk if available at the passed position (in case of a merge
							the merged FREE chunk)
	*/
	Chunk*			mergeFreeChunks( IChunkContainer& tree, XMP_Uns32 index );

	/**
		Move a range of chunks from one container to another starting at the start index up to the
		end of the srcTree.
		 
		@param srcTree		Source container
		@param destTree		Destination container
		@param start		Start index of source container
	 */
	void			moveChunks( IChunkContainer& srcTree, IChunkContainer& destTree, XMP_Uns32 start );

private:
	std::vector<ChunkPath>*	 mMovablePaths;
};

} // IChunkBehavior

#endif
