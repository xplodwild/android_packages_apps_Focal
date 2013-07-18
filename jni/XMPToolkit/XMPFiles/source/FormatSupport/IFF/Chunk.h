// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _Chunk_h_
#define _Chunk_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

#include "source/Endian.h"
#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "XMPFiles/source/FormatSupport/IFF/IChunkData.h"
#include "XMPFiles/source/FormatSupport/IFF/IChunkContainer.h"

namespace IFF_RIFF
{
	/**
	 * CHUNK_UNKNOWN	= Either new chunk or a chunk that was read, but not cached
	 *					  (it is not decided yet whether it becones a node or leaf or is not cached at all)
	 * CHUNK_NODE		= Node chunk that contains children, but no own data (except the optional type)
	 * CHUNK_LEAF		= Leaf chunk that contains data but no children
	 */
	enum ChunkMode { CHUNK_UNKNOWN = 0, CHUNK_NODE = 1, CHUNK_LEAF = 2 };

/**
 *	Each Chunk of the IFF/RIFF based file formats (e.g. WAVE, AVI, AIFF) are represented by
 *	instances of the class Chunk.
 *  A chunk can be a node chunk containing children, a leaf chunk containing data or an "unknown" chunk,
 *	which means that its content has not cached/loaded yet (or will never be during the file handling);
 *	see ChunkMode for more details.
 *
 *	Note: A Chunk can either have a chunk OR a list of child chunks, but never both.
 *
 *	This class provides access to the children or the data of a chunk, depending of the type.
 *	It keeps track of its size. When the size is changed (by adding or removing data/ or children),
 *  the size is also fixed for the parent hierarchy.
 *
 *  The dirty flag (hasChanged()) that is set for each change of a chunk is also promoted to the parents.
 *  The chunk stores its original and new offset within the host file, but its *not* automatically correctin the offset;
 *	this is done by the IChunkBehavior class.
 *	The Chunk class provides an interface to iterate through the tree structure of Chunks.
 *	There are methods to insert, remove and move Chunk's in its children tree structure.
 *
 *	The chunk can read itself from a host file (readChunk()), but it does not automatically read its children,
 *	because they are not necessarily used by the file handler.
 *	The method writeChunk() recurses through the complete chunk tree and writes the *changed* chunks back to the host file.
 *	It is important that the offsets have been fixed before.
 *
 *	Table about endianess in the different RIFF file formats:
 *
 *			|	ID 		size 	type 	data
 *	-----------------------------------------
 *	AVI		|	BE		LE		BE		LE
 *	WAV		|	BE		LE		BE		LE
 *	AIFF	|	BE		BE		BE		BE
 */
class Chunk : public IChunkData,
			  public IChunkContainer
{
	public:
		/** Factory to create an empty chunk */
		static Chunk* createChunk( const IEndian& endian );

		/** Factory to create an empty chunk */
		static Chunk* createUnknownChunk(
			const IEndian& endian,
			const XMP_Uns32 id,
			const XMP_Uns32 type,
			const XMP_Uns64 size,
			const XMP_Uns64 originalOffset = 0,
			const XMP_Uns64 offset = 0
		);

		/** Static factory to create a leaf chunk with no data area or only the type in the data area */
		static Chunk* createHeaderChunk( const IEndian& endian,	const XMP_Uns32 id,	const XMP_Uns32 type = kType_NONE );

		/**
		 * dtor
		 */
		~Chunk();


		//===================== IChunkData interface implementation ================

		/**
		 * Get the chunk ID
		 *
		 * @return	Return the ID, 0 if the chunk does not have an ID.
		 */
		inline XMP_Uns32 getID() const		{ return mChunkId.id; }

		/**
		 * Get the chunk type (if available)
		 * (the first four data bytes of the chunk could be a chunk type)
		 *
		 * @return	Return the type, kType_NONE if the chunk does not contain data.
		 */
		inline XMP_Uns32 getType() const		{ return mChunkId.type; }

		/**
		 * Get the chunk identifier [id and type]
		 *
		 * @return	Return the identifier
		 */
		inline const ChunkIdentifier& getIdentifier() const		{ return mChunkId; }

		/**
		 * Access the data of the chunk.
		 *
		 * @param data OUT pointer to the byte array
		 * @return size of the data block, 0 if no data is available
		 */
		XMP_Uns64 getData( const XMP_Uns8** data ) const;

		/**
		 * Set new data for the chunk.
		 * Will delete an existing internal buffer and recreate a new one
		 * and copy the given data into that new buffer.
		 *
		 * @param data pointer to the data to put into the chunk
		 * @param size Size of the data block
		 * @param writeType if true, the type of the chunk (getType()) is written in front of the data block.
		 */
		void setData( const XMP_Uns8* const data, XMP_Uns64 size, XMP_Bool writeType = false );

		/**
		* Returns the current size of the Chunk.
		*
		* @param includeHeader if set, the returned size will be the whole chunk size including the header
		* @return Returns either the size of the data block of the chunk or size of the whole chunk, including the eight byte header.
		*/
		XMP_Uns64 getSize( bool includeHeader = false ) const		{ return includeHeader ? mSize + HEADER_SIZE : mSize; }

		/**
		 * Returns the current size of the Chunk including a pad byte if the size isn't a even number
		 *
		 * @param includeHeader		if set, the returned size will be the whole chunk size including the header
		 * @return					Returns either the size of the data block of the chunk or size of the whole chunk, including the eight byte header.
		 */
		XMP_Uns64 getPadSize( bool includeHeader = false ) const;
		/**
		 * @return Returns the mode of the chunk (see ChunkMode definition).
		 */
		ChunkMode getChunkMode() const { return mChunkMode; }

		/* The following methods are getter/setter for certain data types.
		 * They always take care of little-endian/big-endian issues.
		 * The offset starts at the data area of the Chunk. */

		XMP_Uns32 getUns32( XMP_Uns64 offset=0 ) const;
		void setUns32( XMP_Uns32 value, XMP_Uns64 offset=0 );

		XMP_Uns64 getUns64( XMP_Uns64 offset=0 ) const;
		void setUns64( XMP_Uns64 value, XMP_Uns64 offset=0 );

		XMP_Int32 getInt32( XMP_Uns64 offset=0 ) const;
		void setInt32( XMP_Int32 value, XMP_Uns64 offset=0 );

		XMP_Int64 getInt64( XMP_Uns64 offset=0 ) const;
		void setInt64( XMP_Int64 value, XMP_Uns64 offset=0 );

		std::string getString( XMP_Uns64 size = 0, XMP_Uns64 offset=0 ) const;
		void setString( std::string value, XMP_Uns64 offset=0 );


		//===================== IChunk interface implementation ================

		//FIXME XMP exception if size cast from 64 to 32 looses data

		/**
		 * Sets the chunk id.
		 */
		void setID( XMP_Uns32 id );

		/**
		 * Sets the chunk type.
		 */
		void setType( XMP_Uns32 type );

		/** 
		 * Sets the chunk size.
		 * NOTE: Should only be used for repairing wrong sizes in files (repair flag).
		 * Normally Size is changed by changing the data automatically!
		 */
		inline void setSize( XMP_Uns64 newSize, bool setOriginal = false )	{ mDirty = mSize != newSize; mSize = newSize; mOriginalSize = setOriginal ? newSize : mOriginalSize; }

		/** 
		 * Calculate the size of the chunks that are dirty including the size 
		 * of its children
		 */
		XMP_Int64 calculateWriteSize( ) const;

		/**
		 * Calculate the size of this chunks based on its children sizes. 
		 * If this chunk has no children then no new size will be calculated.
		 */
		XMP_Uns64 calculateSize( bool setOriginal = false );

		/**
		 * @return Returns the offset of the chunk within the stream.
		 */
		inline XMP_Uns64 getOffset () const		{ return mOffset; }

		/**
		 * @return Returns the original offset of the chunk within the stream.
		 */
		inline XMP_Uns64 getOriginalOffset () const		{ return mOriginalOffset; }

		/**
		* Returns the original size of the Chunk
		*
		 * @param includeHeader		if set, the returned original size will be the whole chunk size including the header
		 * @return					Returns the original size of the chunk within the stream (inluding/excluding headerSize).
		 */
		inline XMP_Uns64 getOriginalSize( bool includeHeader = false ) const		{ return includeHeader ? mOriginalSize + HEADER_SIZE : mOriginalSize; }

		/**
		 * Returns the original size of the Chunk including a pad byte if the size isn't a even number
		 *
		 * @param includeHeader		if set, the returned size will be the whole chunk size including the header
		 * @return					Returns either the size of the data block of the chunk or size of the whole chunk, including the eight byte header.
		 */
		XMP_Uns64 getOriginalPadSize( bool includeHeader = false ) const;
		
		/**
		 * Adjust the offset that this chunk has within the file.
		 *
		 * @param	newOffset the new offset within the file stream
		 */
		void setOffset (XMP_Uns64 newOffset);   // changes during rearranging

		/**
		 * Has the Chunk class changes, or has the position within the file been changed?
		 * If the result is true the chunk has to be written back to the file
		 * (all parent chunks are also set to dirty in that case).
		 *
		 * @return		Returns true if the chunk node has been modified.
		 */
		XMP_Bool hasChanged() const		{ return mDirty; }

		/**
		 * Sets this node and all of its parents up to the tree root dirty.
		 */
		void setChanged();

		/**
		 * Resets the dirty status for this chunk and its children to false
		 */
		void resetChanges();

		/**
		 *Sets all necessary member variables to flag this chunk as a new one being inserted into the tree
		 */
		void setAsNew();

		/**
		 * @return Returns the parent chunk (can be NULL if this is the root of the tree).
		 */
		inline Chunk* getParent() const		{ return mParent; }

		/**
		 * Creates a string representation of the chunk (debug method).
		 */
		std::string toString( std::string tabs = std::string() , XMP_Bool showOriginal = false );


		//-------------------
		//  file access
		//-------------------

		/**
		 * Read id, size and offset and create a chunk with mode CHUNK_UNKNOWN.
		 * The file is expected to be open and is not closed!
		 *
		 * @param	file	File reference to read the chunk from
		 */
		void readChunk( XMP_IO* file );

		/**
		 * Stores the data in the class (only called if required).
		 * The file is expected to be open and is not closed!
		 *
		 * @param	file	File reference to cache the chunk data
		 */
		void cacheChunkData( XMP_IO* file );

		/**
		 * Write or updates chunk (new data, new size, new position).
		 * The file is expected to be open and is not closed!
		 *
		 * Behavior for the different chunk types:
		 *
		 * CHUNK_UNKNOWN:
		 *     - does not write anything back
		 *     - throws exception if hasChanged == true
		 *
		 * CHUNK_LEAF:
		 *     - writes ID (starting with offset)
		 *     - writes size
		 *     - writes buffer (including the optional type at the beginning)
		 *
		 * CHUNK_NODE:
		 *     - writes ID (starting with offset)
		 *     - writes size
		 *     - writes type if defined
		 *     - calls writeChunk on it's children
		 *
		 * Note: readChunk() and optionally cacheChunkData() has to be called before!
		 *
		 * @param	file	File reference to write the chunk to
		 */
		void writeChunk( XMP_IO* file );


		//-------------------
		//  children access
		//-------------------

		/**
		 * @return	Returns the number children chunks.
		 */
		XMP_Uns32 numChildren() const;

		/**
		 * Returns a child node.
		 *
		 * @param	pos		position of the child node to return
		 * @return			Returns the child node at the given position.
		 */
		Chunk* getChildAt( XMP_Uns32 pos ) const;

		/**
		 * Appends a child node at the end of the children list.
		 *
		 * @param	node		the new node
		 * @param	adjustSizes	adjust size of chunk and parents
		 * @return				Returns the added node.
		 */
		void appendChild( Chunk* node, XMP_Bool adjustSizes = true );

		/**
		 * Inserts a child node at a certain position.
		 *
		 * @param	pos			position in the children list to add the new node
		 * @param	node		the new node
		 * @return				Returns the added node.
		 */
		void insertChildAt( XMP_Uns32 pos, Chunk* node );

		/**
		 * Removes a child node at a given position.
		 *
		 * @param	pos				position of the node to delete in the children list
		 *
		 * @return					The removed chunk
		 */
		Chunk* removeChildAt( XMP_Uns32 pos );

		/**
		 * Remove child at the passed position and insert the new chunk
		 *
		 * @param pos			Position of chunk that will be replaced
		 * @param chunk			New chunk
		 *
		 * @return		Replaced chunk
		 */
		Chunk* replaceChildAt( XMP_Uns32 pos, Chunk* node );

		//--------------------
		//  children iteration
		//--------------------

		typedef std::vector<Chunk*>::iterator ChunkIterator;
		typedef std::vector<Chunk*>::const_iterator ConstChunkIterator;

		ConstChunkIterator firstChild() const;

		ConstChunkIterator lastChild() const;

		/** The size of the header (id+size) */
		static const XMP_Uns8 HEADER_SIZE = 8;
		/** The size of the type */
		static const XMP_Uns8 TYPE_SIZE = 4;


	private:
		/** stores the chunk header */
		ChunkIdentifier mChunkId;
		/** Original size of chunk without the header */
		XMP_Uns64 mOriginalSize;
		/** size of chunk without the header */
		XMP_Uns64 mSize;
		/** size of the internal buffer */
		XMP_Uns64 mBufferSize;
		/** buffer for the chunk data without the header, but including the type (first 4 bytes). */
		XMP_Uns8* mData;
		/** Buffer to hold the first 4 bytes that are used for either the type or as data.
		 * Only used for ReadChunk and CacheChunk */
		ChunkMode mChunkMode;

		/**
		 * Current position in stream (file). Can only be changed by moving the chunks around
		 * (by using an IChunkBehavior class).
		 * Note: Sizes are stored in chunk because it can be changed by the handler (i.e. by changing the data)
		 */
		XMP_Uns64 mOriginalOffset;
		/**
		 * New position of the chunk in the stream.
		 * It is initialized to MAXINT64 when there is no new offset.
		 * If the offset has been changed the dirty flag has to be set.
		 */
		XMP_Uns64 mOffset;

		/**
		 * The dirty flag indicates that the chunk (and all parent chunks) has been modified or moved and
		 * that it therefore needs to be written to file.
		 */
		XMP_Bool  mDirty; // has Chunk data changed? has Chunk position changed?

		/** The parent of this node; only the root node does not have a parent. */
		Chunk* mParent;

		/** Stores the byte order for this node.
		 *	Note: The endianess does not change within one file */
		const IEndian& mEndian;

		/** The list of child nodes. */
		std::vector<Chunk*>	mChildren;

		/**
		 * private ctor, prevents direct invokation.
		 *
		 *	@param	endian	Endian util
		 */
        Chunk( const IEndian& endian );

		/**
		 * Resizes the internal byte buffer to the given size if the new size is bigger than the current one.
		 * If the new size is smaller, the buffer is not adjusted
		 */
		void adjustInternalBuffer( XMP_Uns64 newSize );

		/**
		 * Adjusts the chunk size and the parents chunk sizes.
		 * - Leaf chunks always have the size of their data, inluding the 4-byte type and excluding the header.
		 *   Leaf chunks can have an ODD size!
		 * - Node chunks have the added size of all of their children, including the childrens header, but excluding it's own header.
		 *   IMPORTANT: When a leaf child node has an ODD size of data,
		 *   a pad byte is added during the writing process and the parent's size INCLUDES the pad byte.
		 */
		void adjustSize( XMP_Int64 sizeChange = 0 );

}; // Chunk

} // namespace

#endif
