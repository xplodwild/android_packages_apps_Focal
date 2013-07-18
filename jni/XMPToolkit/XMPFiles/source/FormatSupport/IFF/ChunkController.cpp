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

#include "XMPFiles/source/FormatSupport/IFF/ChunkController.h"
#include "XMPFiles/source/FormatSupport/IFF/Chunk.h"

#include <cstdio>

using namespace IFF_RIFF;

//-----------------------------------------------------------------------------
// 
// ChunkController::ChunkController(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

ChunkController::ChunkController( IChunkBehavior* chunkBehavior, XMP_Bool bigEndian )
: mEndian					(NULL), 
  mChunkBehavior			(chunkBehavior), 
  mFileSize					(0),
  mRoot						(NULL), 
  mTrailingGarbageSize		(0), 
  mTrailingGarbageOffset	(0)
{
	if (bigEndian)
	{
		mEndian = &BigEndian::getInstance();
	} else {
		mEndian = &LittleEndian::getInstance();
	}

	// create virtual root chunk
	mRoot = Chunk::createChunk(*mEndian);

	// share chunk paths with behavior
	mChunkBehavior->setMovablePaths( &mChunkPaths );
}

ChunkController::~ChunkController()
{
	delete dynamic_cast<Chunk*>(mRoot);
}

//-----------------------------------------------------------------------------
// 
// ChunkController::addChunkPath(...)
// 
// Purpose: Adds the given path to the array of "Chunk's of interest"
// 
//-----------------------------------------------------------------------------

void ChunkController::addChunkPath( const ChunkPath& path )
{
	mChunkPaths.push_back(path);
}

//-----------------------------------------------------------------------------
// 
// ChunkController::compareChunkPaths(...)
// 
// Purpose: The function parses all the sibling chunks. For every chunk it 
//			either caches the chunk, skips it, or calls the function recusivly 
//			for the children chunks
// 
//-----------------------------------------------------------------------------

ChunkPath::MatchResult ChunkController::compareChunkPaths(const ChunkPath& currentPath)
{
	ChunkPath::MatchResult result = ChunkPath::kNoMatch;

	for( PathIterator iter = mChunkPaths.begin(); ( result == ChunkPath::kNoMatch ) && ( iter != mChunkPaths.end() ); iter++ )
	{
		result = iter->match(currentPath);
	}

	return result;
}

//-----------------------------------------------------------------------------
// 
// ChunkController::parseChunks(...)
// 
// Purpose: The function Parses all the sibling chunks. For every chunk it 
//			either caches the chunk, skips it, or calls the function recusivly 
//			for the children chunks
// 
//-----------------------------------------------------------------------------

void ChunkController::parseChunks( XMP_IO* stream, ChunkPath& currentPath, XMP_OptionBits* options /* = NULL */, Chunk* parent /* = NULL */)
{
	XMP_Uns64 filePos 		= stream->Offset();
	XMP_Bool isRoot			= (parent == mRoot);
	XMP_Uns64 parseLimit	= mFileSize;
	XMP_Uns32 chunkCnt		= 0; 
	
	parent = ( parent == NULL ? dynamic_cast<Chunk*>(mRoot) : parent );

	//
	// calculate the parse limit
	//
	if ( !isRoot )
	{
		parseLimit = parent->getOriginalOffset() + parent->getSize( true );

		if( parseLimit > mFileSize )
		{
			parseLimit = mFileSize;
		}
	}

	while ( filePos < parseLimit )
	{
		XMP_Uns64 fileTail = mFileSize - filePos;

		//
		// check if there is enough space (at least for id and size)
		//
		if ( fileTail < Chunk::HEADER_SIZE )
		{
			//preserve rest of bytes (fileTail)
			mTrailingGarbageOffset = filePos;
			mTrailingGarbageSize = fileTail;
			break; // stop parsing
		}
		else
		{
			bool chunkJump = false;

			//
			// create a new Chunk
			//
			Chunk* chunk = Chunk::createChunk(* mEndian );
			
			bool readFailure = false;
			//
			// read the Chunk (id, size, [type]) without caching the data
			//
			try
			{
				chunk->readChunk( stream );
			}
			catch( ... )
			{
				// remember exception during reading the chunk
				readFailure = true;
			}

			//
			// validate chunk ID for top-level chunks
			//
			if( isRoot && ! mChunkBehavior->isValidTopLevelChunk( chunk->getIdentifier(), chunkCnt ) )
			{
				// notValid: preserve rest of bytes (fileTail)
				mTrailingGarbageOffset = filePos;
				mTrailingGarbageSize = fileTail;
				//delete unused chunk (because these are undefined trailing bytes)
				delete chunk; 
				break; // stop parsing
			}
			else if ( readFailure )
			{
				delete chunk;
				XMP_Throw ( "Bad RIFF chunk", kXMPErr_BadFileFormat );
			}

			//
			// parenting 
			// (as early as possible in order to be able to clean up 
			// the tree correctly in the case of an exception)
			//
			parent->appendChild(chunk, false);

			// count top-level chunks
			if( isRoot )
			{
				chunkCnt++;
			}

			//
			// check size if value exceeds 4GB border
			//
			if( chunk->getSize() >= 0x00000000FFFFFFFFLL )
			{
				// remember file position
				XMP_Int64 currentFilePos = stream->Offset();

				// ask for the "real" size value
				XMP_Uns64 realSize = mChunkBehavior->getRealSize( chunk->getSize(), 
																  chunk->getIdentifier(),
																  *mRoot,
																  stream );

				// set new size at chunk
				chunk->setSize( realSize, true );

				// set flag if the file position changed
				chunkJump = currentFilePos < stream->Offset();
			}

			//
			// Repair if needed
			//
			if ( filePos + chunk->getSize(true) > mFileSize )
			{
				bool isUpdate =		( options != NULL ? XMP_OptionIsSet ( *options, kXMPFiles_OpenForUpdate ) : false );
				bool repairFile =	( options != NULL ? XMP_OptionIsSet ( *options, kXMPFiles_OpenRepairFile ) : false );

				if ( ( ! isUpdate ) || ( repairFile && isRoot ) ) 
				{
					chunk->setSize( mFileSize-filePos-Chunk::HEADER_SIZE, true );
				} 
				else 
				{
					XMP_Throw ( "Bad RIFF chunk size", kXMPErr_BadFileFormat );
				}
			}

			// extend search path
			currentPath.append( chunk->getIdentifier() );

			// first 4 bytes might be already read by the chunk->readChunk function
			XMP_Uns64 offsetOfChunkRead = stream->Offset() -  filePos - Chunk::HEADER_SIZE;

			switch ( compareChunkPaths(currentPath) )
			{
				case ChunkPath::kFullMatch :
				{
					chunk->cacheChunkData( stream );
				}
				break;

				case ChunkPath::kPartMatch :
				{
					parseChunks( stream, currentPath, options, chunk);
					// recalculate the size based on the sizes of its children
					chunk->calculateSize( true );
				}
				break;

				case ChunkPath::kNoMatch :
				{
					// Not a chunk we are interested in, so mark it as not changed
					// It will then be ignored by any further logic
					chunk->resetChanges();

					if ( !chunkJump && chunk->getSize() > 0) // if chunk not empty
					{
						XMP_Validate( stream->Offset() + chunk->getSize() - offsetOfChunkRead <= mFileSize , "ERROR: want's to skip beyond EOF", kXMPErr_InternalFailure);
						stream->Seek ( chunk->getSize() - offsetOfChunkRead , kXMP_SeekFromCurrent );
					}
				}
				break;
			}

			// remove last identifier from current path
			currentPath.remove();

			// update current file position
			filePos = stream->Offset();

			// skip pad byte if there is one (if size odd)
			if( filePos < mFileSize &&
				( ( chunkJump && ( stream->Offset() & 1 ) > 0 ) ||
				( !chunkJump && ( chunk->getSize() & 1 ) > 0 ) ) )
			{
				stream->Seek ( 1 , kXMP_SeekFromCurrent );
				filePos++;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// 
// ChunkController::parseFile(...)
// 
// Purpose: construct the tree, parse children for list of interesting Chunks
//			All requested leaf chunks are cached, the parent chunks are created 
//			but not cached and the rest is skipped
// 
//-----------------------------------------------------------------------------

void ChunkController::parseFile( XMP_IO* stream, XMP_OptionBits* options /* = NULL */ )
{
	// store file information in root node
	mFileSize = stream ->Length();
	ChunkPath currentPath;

	// Make sure the tree is clean before parsing
	cleanupTree();

	try
	{
		parseChunks( stream, currentPath, options, dynamic_cast<Chunk*>(mRoot) );
	}
	catch( ... )
	{
		this->cleanupTree();
		throw;
	}
}


//-----------------------------------------------------------------------------
// 
// ChunkController::writeFile(...)
// 
// Purpose: Called by the handler to write back the changes to the file.
// 
//-----------------------------------------------------------------------------
void ChunkController::writeFile( XMP_IO* stream ,XMP_ProgressTracker * progressTracker )

{
	//
	// if any of the top-level chunks exceeds their maximum size then skip writing and throw an exception
	//
	for( XMP_Uns32 i=0; i<mRoot->numChildren(); i++ )
	{
		Chunk* toplevel = mRoot->getChildAt(i);
		XMP_Validate( toplevel->getSize() < mChunkBehavior->getMaxChunkSize(), "Exceeded maximum chunk size.", kXMPErr_AssertFailure );
	}

	//
	// if exception is thrown write chunk is skipped
	//
	mChunkBehavior->fixHierarchy(*mRoot);

	if (mRoot->numChildren() > 0)
	{
		// The new file size (without trailing garbage) is the offset of the last top-level chunk + its size.
		// NOTE: the padding bytes can be ignored, as the top-level chunk is always a node, not a leaf.
		Chunk* lastChild = mRoot->getChildAt(mRoot->numChildren() - 1);
		XMP_Uns64 newFileSize = lastChild->getOffset() + lastChild->getSize(true);
		if ( progressTracker != 0 ) 
		{
			float fileWriteSize=0.0f;
			for( XMP_Uns32 i = 0; i < mRoot->numChildren(); i++ )
			{
				Chunk* child = mRoot->getChildAt(i);
				fileWriteSize+=child->calculateWriteSize(  );
			}
			XMP_Assert ( progressTracker->WorkInProgress() );
			progressTracker->AddTotalWork ( fileWriteSize );
		}

		// Move garbage tail after last top-level chunk,
		// BEFORE the chunks are written -- in case the file shrinks
		if (mTrailingGarbageSize > 0  &&  newFileSize != mTrailingGarbageOffset)
		{
			if ( progressTracker != 0 ) 
			{
				XMP_Assert ( progressTracker->WorkInProgress() );
				progressTracker->AddTotalWork ( (float)mTrailingGarbageSize );
			}
			XIO::Move( stream, mTrailingGarbageOffset, stream, newFileSize, mTrailingGarbageSize );
			newFileSize += mTrailingGarbageSize;
		}

		// Write changed and new chunks to the file
		for( XMP_Uns32 i = 0; i < mRoot->numChildren(); i++ )
		{
			Chunk* child = mRoot->getChildAt(i);
			child->writeChunk( stream );
		}

		// file has been completely written,
		// truncate the file it has been bigger before
		if (newFileSize < mFileSize)
		{
			stream->Truncate ( newFileSize  );
		}
	}
}

//-----------------------------------------------------------------------------
// 
// ChunkController::getChunk(...)
// 
// Purpose: returns a certain Chunk
// 
//-----------------------------------------------------------------------------

IChunkData* ChunkController::getChunk( const ChunkPath& path, XMP_Bool last ) const
{
	IChunkData* ret = NULL;

	if( path.length() > 0 )
	{
		ChunkPath current;
		ret = this->findChunk( path, current, *(dynamic_cast<Chunk*>(mRoot)), last );
	}

	return ret;
}


//-----------------------------------------------------------------------------
// 
// ChunkController::findChunk(...)
// 
// Purpose: Find a chunk described by path in the hierarchy of chunks starting 
//			at the passed chunk.
//			The position of chunk in the hierarchy is described by the parameter 
//			currentPath.
//			This method is supposed to be recursively.
// 
//-----------------------------------------------------------------------------

Chunk* ChunkController::findChunk( const ChunkPath& path, ChunkPath& currentPath, const Chunk& chunk, XMP_Bool last ) const
{
	Chunk* ret = NULL;
	XMP_Uns32 cnt = 0;

	if( path.length() > currentPath.length() )
	{
		for( XMP_Uns32 i=0; i<chunk.numChildren() && ret == NULL; i++ )
		{
			//if last is true go backwards
			last ? cnt=chunk.numChildren()-1-i : cnt=i;

			Chunk* child = NULL;

			try
			{
				child = chunk.getChildAt(cnt);
			}
			catch(...)
			{
				child = NULL;
			}

			if( child != NULL )
			{
				currentPath.append( child->getIdentifier() );

				switch( path.match( currentPath ) )
				{
					case ChunkPath::kFullMatch:
					{
						ret = child;
					}
					break;

					case ChunkPath::kPartMatch:
					{
						ret = this->findChunk( path, currentPath, *child, last );
					}
					break;

					case ChunkPath::kNoMatch:
					{
						// Nothing to do
					}
					break;
				}

				currentPath.remove();
			}
		}
	}

	return ret;
}


//-----------------------------------------------------------------------------
// 
// ChunkController::getChunks(...)
// 
// Purpose: Returns all chunks that match completely to the passed path.
// 
//-----------------------------------------------------------------------------

const std::vector<IChunkData*>& ChunkController::getChunks( const ChunkPath& path )
{
	mSearchResults.clear();

	if( path.length() > 0 )
	{
		ChunkPath current;
		this->findChunks( path, current, *(dynamic_cast<Chunk*>(mRoot)) );
	}

	return mSearchResults;
}//getChunks


//-----------------------------------------------------------------------------
// 
// ChunkController::getTopLevelTypes(...)
// 
// Purpose: Return an array containing the types of the top level nodes
//			Top level nodes are the ones beneath ROOT
// 
//-----------------------------------------------------------------------------

const std::vector<XMP_Uns32> ChunkController::getTopLevelTypes()
{
	std::vector<XMP_Uns32> typeList;

	for( XMP_Uns32 i = 0; i < mRoot->numChildren(); i++ )
	{
		typeList.push_back( mRoot->getChildAt( i )->getType() );
	}

	return typeList;
}// getTopLevelTypes


//-----------------------------------------------------------------------------
// 
// ChunkController::findChunks(...)
// 
// Purpose: Find all chunks described by path in the hierarchy of chunks starting 
//			at the passed chunk.
//			The position of chunks in the hierarchy is described by the parameter 
//			currentPath. Found chunks that match to the path are stored in the 
//			member mSearchResults.
//			This method is supposed to be recursively.
// 
//-----------------------------------------------------------------------------

void ChunkController::findChunks( const ChunkPath& path, ChunkPath& currentPath, const Chunk& chunk )
{
	if( path.length() > currentPath.length() )
	{
		for( XMP_Uns32 i=0; i<chunk.numChildren(); i++ )
		{
			Chunk* child = NULL;

			try
			{
				child = chunk.getChildAt(i);
			}
			catch(...)
			{
				child = NULL;
			}

			if( child != NULL )
			{
				currentPath.append( child->getIdentifier() );

				switch( path.match( currentPath ) )
				{
					case ChunkPath::kFullMatch:
					{
						mSearchResults.push_back( child );
					}
					break;

					case ChunkPath::kPartMatch:
					{
						this->findChunks( path, currentPath, *child );
					}
					break;

					case ChunkPath::kNoMatch:
					{
						// Nothing to do
					}
					break; 
				}

				currentPath.remove();
			}
		}
	}
}//findChunks


//-----------------------------------------------------------------------------
// 
// ChunkController::cleanupTree(...)
// 
// Purpose: Cleanup function called from destructor and in case of an exception
// 
//-----------------------------------------------------------------------------

void ChunkController::cleanupTree()
{
	delete dynamic_cast<Chunk*>(mRoot);
	mRoot = Chunk::createChunk(*mEndian);
}


//-----------------------------------------------------------------------------
// 
// ChunkController::dumpTree(...)
// 
// Purpose: dumps the tree structure
// 
//-----------------------------------------------------------------------------

std::string ChunkController::dumpTree( )
{
	std::string ret;
	char buffer[256];

	if ( mRoot != NULL )
	{
		ret = mRoot->toString();
	}

	if ( mTrailingGarbageSize != 0 )
	{
		snprintf( buffer, 255, "\n Trailing Bytes: %llu",	mTrailingGarbageSize );

		std::string str(buffer);
		ret.append(str);
	}
	return ret;
}

//-----------------------------------------------------------------------------
// 
// ChunkController::createChunk(...)
// 
// Purpose: Create a new empty chunk
// 
//-----------------------------------------------------------------------------

IChunkData* ChunkController::createChunk( XMP_Uns32 id, XMP_Uns32 type /*= kType_NONE*/ )
{
	Chunk* chunk = Chunk::createChunk(* mEndian );

	chunk->setID( id );
	if( type != kType_NONE )
	{
		chunk->setType( type );
	}

	return chunk;
}

//-----------------------------------------------------------------------------
// 
// ChunkController::insertChunk(...)
// 
// Purpose: Insert a new chunk. The position of this new chunk within the 
//			hierarchy is determined internally by the behavior.
//			Throws an exception if a chunk cannot be inserted into the tree
// 
//-----------------------------------------------------------------------------

void ChunkController::insertChunk( IChunkData* chunk )
{
	XMP_Validate( chunk != NULL, "ERROR inserting Chunk. Chunk is NULL.", kXMPErr_InternalFailure );
	Chunk* ch = dynamic_cast<Chunk*>(chunk);
	mChunkBehavior->insertChunk( *mRoot, *ch );
	// sets OriginalSize = Size / OriginalOffset = Offset
	ch->setAsNew();
	// force set dirty flag
	ch->setChanged();
}

//-----------------------------------------------------------------------------
// 
// ChunkController::removeChunk(...)
// 
// Purpose: Delete a chunk or remove/delete it from the tree. 
//			If the chunk exists within the chunk hierarchy the chunk gets removed 
//			from the tree and deleted.
//			If it is not in the tree, then it is only destroyed.
// 
//-----------------------------------------------------------------------------

void ChunkController::removeChunk( IChunkData* chunk )
{
	if( chunk != NULL )
	{
		Chunk* chk = dynamic_cast<Chunk*>(chunk);

		if( this->isInTree( chk ) )
		{
			if( mChunkBehavior->removeChunk( *mRoot, *chk ) )
			{
				delete chk;
			}
		}
		else
		{
			delete chk;
		}
	}
}

//-----------------------------------------------------------------------------
// 
// ChunkController::isInTree(...)
// 
// Purpose: return true if the passed in Chunk is part of the Chunk tree
// 
//-----------------------------------------------------------------------------

bool ChunkController::isInTree( Chunk* chunk )
{
	bool ret = ( mRoot == chunk );

	if( !ret && chunk != NULL )
	{
		Chunk* parent = chunk->getParent();

		while( !ret && parent != NULL )
		{
			ret = ( mRoot == parent );
			parent = parent->getParent();
		}
	}

	return ret;
}
