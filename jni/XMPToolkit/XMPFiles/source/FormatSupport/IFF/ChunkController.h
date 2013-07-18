// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _ChunkController_h_
#define _ChunkController_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "source/XMP_LibUtils.hpp"
#include "source/XMP_ProgressTracker.hpp"

#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "XMPFiles/source/FormatSupport/IFF/IChunkBehavior.h"

class IEndian;

namespace IFF_RIFF
{
/**
	The class ChunkController is supposed to act as an controller between the IRIFFHandler and the actual chunks (Chunk instances).
	It controls the parsing and writing of the passed stream.
*/

class IChunkData;
class IChunkContainer;
class Chunk;

class ChunkController
{
	public:
		/**
		 * Constructor:
		 * Creates an IEndian based instance for further usage.
		 *
		 * @param	IChunkBehavior* chunkBehavior : for AVI the IChunkBehavior instance would be an instance of a IChunkBehavior class, that knows
		 *				 about the 1,2,4 GB border, padding byte special cases, AVIX stuff and so on. That knowledge would
		 *				 be used during writeFile()
		 *				 In the case of WAVE it would be an instance of WAVEBehavior that would know how to get the 64bit
		 *				 size values for RF64 if required. That knowledge would be used during parseFile()
		 * @param	XMP_Bool bigEndian set True if file chunk data is big endian (e.g. AIFF).
		 *				Must explicitely be set, so that handlers do not accidentaly use the wrong endianess
		 */
		ChunkController( IChunkBehavior* chunkBehavior, XMP_Bool bigEndian );

		~ChunkController();

		/**
		 * Adds the given path to the array of "Chunk's of interest",
		 *
		 * @param path List of Paths that should be parsed
		 * example AVI: [ RIFF:AVI/LIST:INFO , RIFF:AVIX/LIST:INFO, RIFF:AVI/LIST:TDAT  ]
		 */
		void addChunkPath( const ChunkPath& path );

		/**
		 * construct the tree, parse children for list of interesting Chunks
		 * All requested leaf chunks are cached, the parent chunks are created but not cached
		 * and the rest is skipped.
		 *
		 * @param stream the open [file] stream with file pointer at the beginning of the file
		 *
		 */
		void parseFile( XMP_IO* stream, XMP_OptionBits* options  = NULL );

		/**
		 * Create a new empty chunk
		 *
		 * @param	id		Chunk identifier
		 * @param	type	Chunk type [optional]
		 * @return	New IChunkData with passed id/type
		 */
		IChunkData* createChunk( XMP_Uns32 id, XMP_Uns32 type = kType_NONE );

		/**
		 * Insert a new chunk. The position of this new chunk within the hierarchy
		 * is determined internally by the behavior.
		 * Throws an exception if a chunk cannot be inserted into the tree
		 *
		 * @param	chunk The chunk to insert into the tree
		 */
		void insertChunk( IChunkData* chunk );

		/**
		 * Delete a chunk or remove/delete it from the tree.
		 * If the chunk exists within the chunk hierarchy the chunk gets removed from the tree and deleted.
		 * If it is not in the tree, then it is only destroyed.
		 *
		 * @param	chunk	Chunk to remove/delete
		 */
		void removeChunk( IChunkData* chunk );

		/**
		 * Called by the handler to write back the changes to the file.
		 * 1. fix the file tree (ChunkBehavior#fixHierarchy),
		 *    offsets are corrected, no overlapping chunks;
		 *    if rearranging fails, the file is not touched
		 * 2. write the changed chunks to the file
		 *
		 * @param stream the open [file] stream for writing, the file pointer must be at the beginning
		 * @param progressTracker Progress tracker to track the file write progress and reporting it to client
		 */
		void writeFile( XMP_IO* stream,XMP_ProgressTracker * progressTracker  );

		/**
		 * Returns the first (or last) Chunk that matches the passed path.
		 *
		 * @param path the path of the Chunk to return
		 * @param last in case of duplicates return the last one
		 * @return Returns Chunk or NULL
		 */
		IChunkData* getChunk( const ChunkPath& path, XMP_Bool last = false ) const;

		/**
		 * Returns all chunks that match completely to the passed path.
		 * E.g. if FORM:AIFF/LIST is given, it would return all LIST chunks in FORM:AIFF
		 *
		 * @param path the path of the Chunk to return
		 * @return list of found chunks or empty list
		 */
		const std::vector<IChunkData*>& getChunks( const ChunkPath& path );

		/**
		 * returns the number of the bytes after the last valid IFF chunk
		 */
		inline XMP_Int64 getTrailingGarbageSize() { return mTrailingGarbageSize; };

		/**
		 * returns the file size
		 */
		inline XMP_Int64 getFileSize() { return mFileSize; };

		/**
		 * Return an array containing the types of the top level nodes
		 * Top level nodes are the ones beneath ROOT
		 */
		const std::vector<XMP_Uns32> getTopLevelTypes();

		/**
		 * dumps the tree structure
		 *
		 */
		std::string dumpTree( );

	protected:
		/**
		 * Standard Constructor:
		 * Hidden on purpose. Must not be used!
		 * A Controller must have a behavior!
		 */
		ChunkController() { XMP_Throw("Ctor hidden", kXMPErr_InternalFailure); }

		/**
		 * The function Parses all the sibling chunks. For every chunk it either caches the chunk,
		 * skips it, or calls the function recusivly for the children chunks
		 *
		 * @param stream the file stream
		 * @param currentPath the path/id of the Chunk to return
		 * @param options handler options
		 * @param parent pointer to the parent chunk
		 */
		void parseChunks( XMP_IO* stream, ChunkPath& currentPath, XMP_OptionBits* options = NULL, Chunk* parent = NULL );

		/**
		 * The function parses all the sibling chunks. For every chunk it either caches the chunk,
		 * skips it, or calls the function recusivly for the children chunks
		 *
		 * @param ChunkPath& currentPath: the path/id of the Chunk to return
		 */
		ChunkPath::MatchResult compareChunkPaths( const ChunkPath& currentPath );

		/**
		 * Find a chunk described by path in the hierarchy of chunks starting at the passed chunk.
		 * The position of chunk in the hierarchy is described by the parameter currentPath.
		 * This method is supposed to be recursively.
		 */
		Chunk* findChunk( const ChunkPath& path, ChunkPath& currentPath, const Chunk& chunk, XMP_Bool last = false ) const;

		/**
		 * Find all chunks described by path in the hierarchy of chunks starting at the passed chunk.
		 * The position of chunks in the hierarchy is described by the parameter currentPath.
		 * Found chunks that match to the path are stored in the member mSearchResults.
		 * This method is supposed to be recursively.
		 */
		void findChunks( const ChunkPath& path, ChunkPath& currentPath, const Chunk& chunk );

		/**
		 * Cleanup function called from destructor and in case of an exception
		 */
		void cleanupTree();

		/**
		 * return true if the passed in Chunk is part of the Chunk tree
		 *
		 * @param chunk the chunk that shall be checked.
		 */
		bool isInTree( Chunk* chunk );


		// Members

		/**
		 * Endian class. Either BigEndian oder LittleEndian. Based on the file format.
		 */
		const IEndian* mEndian;

		/**
		 * Chunk behaviour class. Has file format specific function for getting the size and
		 * rearranging the chunk tree.
		 */
		IChunkBehavior* mChunkBehavior;

		/** The list of chunks wich should be cached. */
		std::vector<ChunkPath>	mChunkPaths;

		/** Iterator for the list of chunk paths */
		typedef std::vector<ChunkPath>::iterator PathIterator;

		/** The overall filesize after parsing the file stream */
		XMP_Uns64 mFileSize;

		/** The root of the Chunk Tree (top level list) */
		IChunkContainer*	mRoot;

		/** Offset of trailing garbage characters */
		XMP_Uns64 mTrailingGarbageOffset;

		/** Size of trailing garbage characters */
		XMP_Uns64 mTrailingGarbageSize;

		/** search results of method getChunks(...) */
		ChunkPath mSearchPath;

		/** Cached search results */
		std::vector<IChunkData*> mSearchResults;
}; // ChunkController

} // namespace

#endif
