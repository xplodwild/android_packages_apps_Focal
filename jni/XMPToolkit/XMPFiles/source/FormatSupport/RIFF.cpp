// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

// must have access to handler class fields...
#include "XMPFiles/source/FormatSupport/RIFF.hpp"
#include "XMPFiles/source/FormatSupport/RIFF_Support.hpp"
#include "XMPFiles/source/FileHandlers/RIFF_Handler.hpp"

#include <utility>

using namespace RIFF;

namespace RIFF {

// GENERAL STATIC FUNCTIONS ////////////////////////////////////////

Chunk* getChunk ( ContainerChunk* parent, RIFF_MetaHandler* handler )
{
	XMP_IO* file = handler->parent->ioRef;
	XMP_Uns8 level = handler->level;
	XMP_Uns32 peek = XIO::PeekUns32_LE ( file );

	if ( level == 0 )
	{
		XMP_Validate( peek == kChunk_RIFF, "expected RIFF chunk not found", kXMPErr_BadFileFormat );
		XMP_Enforce( parent == NULL );
	}
	else
	{
		XMP_Validate( peek != kChunk_RIFF, "unexpected RIFF chunk below top-level", kXMPErr_BadFileFormat );
		XMP_Enforce( parent != NULL );
	}

	switch( peek )
	{
	case kChunk_RIFF:
		return new ContainerChunk( parent, handler );
	case kChunk_LIST:
		{
		if ( level != 1 ) break; // only care on this level

			// look further (beyond 4+4 = beyond id+size) to check on relevance
			file->Seek ( 8, kXMP_SeekFromCurrent  );
			XMP_Uns32 containerType = XIO::PeekUns32_LE ( file );
			file->Seek ( -8, kXMP_SeekFromCurrent  );

		bool isRelevantList = ( containerType== kType_INFO || containerType == kType_Tdat );
		if ( !isRelevantList ) break;

				return new ContainerChunk( parent, handler );
		}
	case kChunk_XMP:
		if ( level != 1 ) break; // ignore on inappropriate levels (might be compound metadata?)
		return new XMPChunk( parent, handler );
	case kChunk_DISP:
		{
			if ( level != 1 ) break; // only care on this level
			// peek even further to see if type is 0x001 and size is reasonable
			file ->Seek ( 4, kXMP_SeekFromCurrent  ); // jump DISP
			XMP_Uns32 dispSize = XIO::ReadUns32_LE( file );
			XMP_Uns32 dispType = XIO::ReadUns32_LE( file );
			file ->Seek ( -12, kXMP_SeekFromCurrent ); // rewind, be in front of chunkID again

			// only take as a relevant disp if both criteria met,
			// otherwise treat as generic chunk!
			if ( (dispType == 0x0001) && ( dispSize < 256 * 1024 ) )
			{
				ValueChunk* r = new ValueChunk( parent, handler );
				handler->dispChunk = r;
				return r;
			}
			break; // treat as irrelevant (non-0x1) DISP chunks as generic chunk
		}
	case kChunk_bext:
		{
			if ( level != 1 ) break; // only care on this level
			// store for now in a value chunk
			ValueChunk* r = new ValueChunk( parent, handler );
			handler->bextChunk = r;
			return r;
		}
	case kChunk_PrmL:
		{
			if ( level != 1 ) break; // only care on this level
			ValueChunk* r = new ValueChunk( parent, handler );
			handler->prmlChunk = r;
			return r;
		}
	case kChunk_Cr8r:
		{
			if ( level != 1 ) break; // only care on this level
			ValueChunk* r = new ValueChunk( parent, handler );
			handler->cr8rChunk = r;
			return r;
		}
	case kChunk_JUNQ:
	case kChunk_JUNK:
		{
			JunkChunk* r = new JunkChunk( parent, handler );
			return r;
		}
	}
	// this "default:" section must be ouside switch bracket, to be
	// reachable by all those break statements above:


	// digest 'valuable' container chunks: LIST:INFO, LIST:Tdat
	bool insideRelevantList = ( level==2 && parent->id == kChunk_LIST
		&& ( parent->containerType== kType_INFO || parent->containerType == kType_Tdat ));

	if ( insideRelevantList )
	{
		ValueChunk* r = new ValueChunk( parent, handler );
		return r;
	}

	// general chunk of no interest, treat as unknown blob
	return new Chunk( parent, handler, true, chunk_GENERAL );
}

// BASE CLASS CHUNK ///////////////////////////////////////////////
// ad hoc creation
Chunk::Chunk( ContainerChunk* parent, ChunkType c, XMP_Uns32 id )
{
	this->chunkType = c; // base class assumption
	this->parent = parent;
	this->id = id;
	this->oldSize = 0;
	this->newSize = 8;
	this->oldPos = 0; // inevitable for ad-hoc
	this->needSizeFix = false;

	// good parenting for latter destruction
	if ( this->parent != NULL )
	{
		this->parent->children.push_back( this );
		if( this->chunkType == chunk_VALUE )
			this->parent->childmap.insert( std::make_pair( this->id, (ValueChunk*) this ) );
	}
}

// parsing creation
Chunk::Chunk( ContainerChunk* parent, RIFF_MetaHandler* handler, bool skip, ChunkType c )
{
	chunkType = c; // base class assumption
	this->parent = parent;
	this->oldSize = 0;
	this->hasChange = false; // [2414649] valid assumption at creation time

	XMP_IO* file = handler->parent->ioRef;

	this->oldPos = file->Offset();
	this->id = XIO::ReadUns32_LE( file );
	this->oldSize = XIO::ReadUns32_LE( file ) + 8;

	// Make sure the size is within expected bounds.
	XMP_Int64 chunkEnd = this->oldPos + this->oldSize;
	XMP_Int64 chunkLimit = handler->oldFileSize;
	if ( parent != 0 ) chunkLimit = parent->oldPos + parent->oldSize;
	if ( chunkEnd > chunkLimit ) {
		bool isUpdate = XMP_OptionIsSet ( handler->parent->openFlags, kXMPFiles_OpenForUpdate );
		bool repairFile = XMP_OptionIsSet ( handler->parent->openFlags, kXMPFiles_OpenRepairFile );
		if ( (! isUpdate) || (repairFile && (parent == 0)) ) {
			this->oldSize = chunkLimit - this->oldPos;
		} else {
			XMP_Throw ( "Bad RIFF chunk size", kXMPErr_BadFileFormat );
		}
	}

	this->newSize = this->oldSize;
	this->needSizeFix = false;

	if ( skip ) file->Seek ( (this->oldSize - 8), kXMP_SeekFromCurrent );

	// "good parenting", essential for latter destruction.
	if ( this->parent != NULL )
	{
		this->parent->children.push_back( this );
		if( this->chunkType == chunk_VALUE )
			this->parent->childmap.insert( std::make_pair( this->id, (ValueChunk*) this ) );
	}
}

void Chunk::changesAndSize( RIFF_MetaHandler* handler )
{
	// only unknown chunks should reach this method,
	// all others must reach overloads, hence little to do here:
	hasChange = false; // unknown chunk ==> no change, naturally
	this->newSize = this->oldSize;
}

std::string Chunk::toString(XMP_Uns8 level )
{
	char buffer[256];
	snprintf( buffer, 255, "%.4s -- "
							"oldSize: 0x%.8llX,  "
							"newSize: 0x%.8llX,  "
							"oldPos: 0x%.8llX\n",
		(char*)(&this->id), this->oldSize, this->newSize, this->oldPos );
	return std::string(buffer);
}

void Chunk::write( RIFF_MetaHandler* handler, XMP_IO* file , bool isMainChunk )
{
	throw new XMP_Error(kXMPErr_InternalFailure, "Chunk::write never to be called for unknown chunks.");
}

Chunk::~Chunk()
{
	//nothing
}

// CONTAINER CHUNK /////////////////////////////////////////////////
// a) creation
// [2376832] expectedSize - minimum padding "parking size" to use, if not available append to end
ContainerChunk::ContainerChunk( ContainerChunk* parent, XMP_Uns32 id, XMP_Uns32 containerType ) : Chunk( NULL /* !! */, chunk_CONTAINER, id )
{
	// accept no unparented ConatinerChunks
	XMP_Enforce( parent != NULL );

	this->containerType = containerType;
	this->newSize = 12;
	this->parent = parent;

	chunkVect* siblings = &parent->children;

	// add at end. ( oldSize==0 will flag optimization later in the process)
	siblings->push_back( this );
}

// b) parsing
ContainerChunk::ContainerChunk( ContainerChunk* parent, RIFF_MetaHandler* handler ) : Chunk( parent, handler, false, chunk_CONTAINER )
{
	bool repairMode = ( 0 != ( handler->parent->openFlags & kXMPFiles_OpenRepairFile ));

	try
	{
		XMP_IO* file = handler->parent->ioRef;
		XMP_Uns8 level = handler->level;

		// get type of container chunk
		this->containerType = XIO::ReadUns32_LE( file );

		// ensure legality of top-level chunks
		if ( level == 0 && handler->riffChunks.size() > 0 )
		{
			XMP_Validate( handler->parent->format == kXMP_AVIFile, "only AVI may have multiple top-level chunks", kXMPErr_BadFileFormat );
			XMP_Validate( this->containerType == kType_AVIX, "all chunks beyond main chunk must be type AVIX", kXMPErr_BadFileFormat );
		}

		// has *relevant* subChunks? (there might be e.g. non-INFO LIST chunks we don't care about)
		bool hasSubChunks = ( ( this->id == kChunk_RIFF ) ||
							  ( this->id == kChunk_LIST && this->containerType == kType_INFO ) ||
							  ( this->id == kChunk_LIST && this->containerType == kType_Tdat )
						  );
		XMP_Int64 endOfChunk = this->oldPos + this->oldSize;

		// this statement catches beyond-EoF-offsets on any level
		// exception: level 0, tolerate if in repairMode
		if ( (level == 0) && repairMode && (endOfChunk > handler->oldFileSize) )
		{
			endOfChunk = handler->oldFileSize; // assign actual file size
			this->oldSize = endOfChunk - this->oldPos; //reversely calculate correct oldSize
		}

		XMP_Validate( endOfChunk <= handler->oldFileSize, "offset beyond EoF", kXMPErr_BadFileFormat );

		Chunk* curChild = 0;
		if ( hasSubChunks )
		{
			handler->level++;
			while ( file->Offset() < endOfChunk )
			{
				curChild = RIFF::getChunk( this, handler );

				// digest pad byte - no value validation (0), since some 3rd party files have non-0-padding.
				if ( file->Offset() % 2 == 1 )
				{
					// [1521093] tolerate missing pad byte at very end of file:
					XMP_Uns8 pad;
					file->Read ( &pad, 1 );  // Read the pad, tolerate being at EOF.

				}

				// within relevant LISTs, relentlesly delete junk chunks (create a single one
				// at end as part of updateAndChanges()
				if ( (containerType== kType_INFO || containerType == kType_Tdat)
						&& ( curChild->chunkType == chunk_JUNK ) )
				{
						this->children.pop_back();
						delete curChild;
				} // for other chunks: join neighouring Junk chunks into one
				else if ( (curChild->chunkType == chunk_JUNK) && ( this->children.size() >= 2 ) )
				{
					// nb: if there are e.g 2 chunks, then last one is at(1), prev one at(0) ==> '-2'
					Chunk* prevChunk = this->children.at( this->children.size() - 2 );
					if ( prevChunk->chunkType == chunk_JUNK )
					{
						// stack up size to prior chunk
						prevChunk->oldSize += curChild->oldSize;
						prevChunk->newSize += curChild->newSize;
						XMP_Enforce( prevChunk->oldSize == prevChunk->newSize );
						// destroy current chunk
						this->children.pop_back();
						delete curChild;
					}
				}
			}
			handler->level--;
			XMP_Validate( file->Offset() == endOfChunk, "subchunks exceed outer chunk size", kXMPErr_BadFileFormat );

			// pointers for later legacy processing
			if ( level==1 && this->id==kChunk_LIST && this->containerType == kType_INFO )
				handler->listInfoChunk = this;
			if ( level==1 && this->id==kChunk_LIST && this->containerType == kType_Tdat )
				handler->listTdatChunk = this;
		}
		else // skip non-interest container chunk
		{
			file->Seek ( (this->oldSize - 8 - 4), kXMP_SeekFromCurrent );
		} // if - else

	} // try
	catch (XMP_Error& e) {
		this->release(); // free resources
		if ( this->parent != 0)
			this->parent->children.pop_back(); // hereby taken care of, so removing myself...

		throw e;         // re-throw
	}
}

void ContainerChunk::changesAndSize( RIFF_MetaHandler* handler )
{

	// Walk the container subtree adjusting the children that have size changes. The only containers
	// are RIFF and LIST chunks, they are treated differently.
	//
	// LISTs get recomposed as a whole. Existing JUNK children of a LIST are removed, existing real
	// children are left in order with their new size, new children have already been appended. The
	// LIST as a whole gets a new size that is the sum of the final children.
	//
	// Special rules apply to various children of a RIFF container. FIrst, adjacent JUNK children
	// are combined, this simplifies maximal reuse. The children are recursively adjusted in order
	// to get their final size.
	//
	// Try to determine the final placement of each RIFF child using general rules:
	//	- if the size is unchanged: leave at current location
	//	- if the chunk is at the end of the last RIFF chunk and grows: leave at current location
	//	- if there is enough following JUNK: add part of the JUNK, adjust remaining JUNK size
	//	- if it shrinks by 9 bytes or more: carve off trailing JUNK
	//	- try to find adequate JUNK in the current parent
	//
	// Use child-specific rules as a last resort:
	//	- if it is LIST:INFO: delete it, must be in first RIFF chunk
	//	- for others: move to end of last RIFF chunk, make old space JUNK

	// ! Don't create any junk chunks of exactly 8 bytes, just a header and no content. That has a
	// ! size field of zero, which hits a crashing bug in some versions of Windows Media Player.

	bool isRIFFContainer = (this->id == kChunk_RIFF);
	bool isLISTContainer = (this->id == kChunk_LIST);
	XMP_Enforce ( isRIFFContainer | isLISTContainer );

	XMP_Index childIndex;	// Could be local to the loops, this simplifies debuging. Need a signed type!
	Chunk * currChild;

	if ( this->children.empty() ) {
		if ( isRIFFContainer) {
			this->newSize = 12;	// Keep a minimal size container.
		} else {
			this->newSize = 0;	// Will get removed from parent in outer call.
		}
		this->hasChange = true;
		return;	// Nothing more to do without children.
	}

	// Collapse adjacent RIFF junk children, remove all LIST junk children. Work back to front to
	// simplify the effect of .erase() on the loop. Purposely ignore the first chunk.

	for ( childIndex = (XMP_Index)this->children.size() - 1; childIndex > 0; --childIndex ) {

		currChild = this->children[childIndex];
		if ( currChild->chunkType != chunk_JUNK ) continue;

		if ( isRIFFContainer ) {
			Chunk * prevChild = this->children[childIndex-1];
			if ( prevChild->chunkType != chunk_JUNK ) continue;
			prevChild->oldSize += currChild->oldSize;
			prevChild->newSize += currChild->newSize;
			prevChild->hasChange = true;
		}

		this->children.erase ( this->children.begin() + childIndex );
		delete currChild;
		this->hasChange = true;

	}

	// Process the children of RIFF and LIST containers to get their final size. Remove empty
	// children. Work back to front to simplify the effect of .erase() on the loop. Do not ignore
	// the first chunk.

	for ( childIndex = (XMP_Index)this->children.size() - 1; childIndex >= 0; --childIndex ) {

		currChild = this->children[childIndex];

		++handler->level;
		currChild->changesAndSize ( handler );
		--handler->level;

		if ( (currChild->newSize == 8) || (currChild->newSize == 0) ) {	// ! The newSIze is supposed to include the header.
			this->children.erase ( this->children.begin() + childIndex );
			delete currChild;
			this->hasChange = true;
		} else {
			this->hasChange |= currChild->hasChange;
			currChild->needSizeFix = (currChild->newSize != currChild->oldSize);
			if ( currChild->needSizeFix && (currChild->newSize > currChild->oldSize) &&
				 (this == handler->lastChunk) && (childIndex+1 == (XMP_Index)this->children.size()) ) {
				// Let an existing last-in-file chunk grow in-place. Shrinking is conceptually OK,
				// but complicates later sanity check that the main AVI chunk is not OK to append
				// other chunks later. Ignore new chunks, they might reuse junk space.
				if ( currChild->oldSize != 0 ) currChild->needSizeFix = false;
			}
		}

	}

	// Go through the children of a RIFF container, adjusting the placement as necessary. In brief,
	// things can only grow at the end of the last RIFF chunk, and non-junk chunks can't be shifted.

	if ( isRIFFContainer ) {

		for ( childIndex = 0; childIndex < (XMP_Index)this->children.size(); ++childIndex ) {

			currChild = this->children[childIndex];
			if ( ! currChild->needSizeFix ) continue;
			currChild->needSizeFix = false;

			XMP_Int64 sizeDiff = currChild->newSize - currChild->oldSize;	// Positive for growth.
			XMP_Uns8  padSize = (currChild->newSize & 1);	// Need a pad for odd size.

			// See if the following chunk is junk that can be utilized.

			Chunk * nextChild = 0;
			if ( childIndex+1 < (XMP_Index)this->children.size() ) nextChild = this->children[childIndex+1];

			if ( (nextChild != 0) && (nextChild->chunkType == chunk_JUNK) ) {
				if ( nextChild->newSize >= (9 + sizeDiff + padSize) ) {

					// Incorporate part of the trailing junk, or make the trailing junk grow.
					nextChild->newSize -= sizeDiff;
					nextChild->newSize -= padSize;
					nextChild->hasChange = true;
					continue;

				} else if (  nextChild->newSize == (sizeDiff + padSize)  ) {

					// Incorporate all of the trailing junk.
					this->children.erase ( this->children.begin() + childIndex + 1 );
					delete nextChild;
					continue;

				}
			}

			// See if the chunk shrinks enough to turn the leftover space into junk.

			if ( (sizeDiff + padSize) <= -9 ) {
				this->children.insert ( (this->children.begin() + childIndex + 1), new JunkChunk ( NULL, ((-sizeDiff) - padSize) ) );
				continue;
			}

			// Look through the parent for a usable span of junk.

			XMP_Index junkIndex;
			Chunk * junkChunk = 0;
			for ( junkIndex = 0; junkIndex < (XMP_Index)this->children.size(); ++junkIndex ) {
				junkChunk = this->children[junkIndex];
				if ( junkChunk->chunkType != chunk_JUNK ) continue;
				if ( (junkChunk->newSize >= (9 + currChild->newSize + padSize)) ||
					 (junkChunk->newSize == (currChild->newSize + padSize)) ) break;
			}

			if ( junkIndex < (XMP_Index)this->children.size() ) {

				// Use part or all of the junk for the relocated chunk, replace the old space with junk.

				if ( junkChunk->newSize == (currChild->newSize + padSize) ) {

					// The found junk is an exact fit.
					this->children[junkIndex] = currChild;
					delete junkChunk;

				} else {

					// The found junk has excess space. Insert the moving chunk and shrink the junk.
					XMP_Assert ( junkChunk->newSize >= (9 + currChild->newSize + padSize) );
					junkChunk->newSize -= (currChild->newSize + padSize);
					junkChunk->hasChange = true;
					this->children.insert ( (this->children.begin() + junkIndex), currChild );
					if ( junkIndex < childIndex ) ++childIndex;	// The insertion moved the current child.

				}

				if ( currChild->oldSize != 0 ) {
					this->children[childIndex] = new JunkChunk ( 0, currChild->oldSize );	// Replace the old space with junk.
				} else {
					this->children.erase ( this->children.begin() + childIndex );	// Remove the newly created chunk's old location.
					--childIndex;	// Make the next loop iteration not skip a chunk.
				}

				continue;

			}

			// If this is a LIST:INFO chunk not in the last of multiple RIFF chunks, then give up
			// and replace it with oldSize junk. Preserve the first RIFF chunk's original size.

			bool isListInfo = (currChild->id == kChunk_LIST) && (currChild->chunkType == chunk_CONTAINER) &&
							  (((ContainerChunk*)currChild)->containerType == kType_INFO);

			if ( isListInfo && (handler->riffChunks.size() > 1) &&
				 (this->id == kChunk_RIFF) && (this != handler->lastChunk) ) {

				if ( currChild->oldSize != 0 ) {
					this->children[childIndex] = new JunkChunk ( 0, currChild->oldSize );
				} else {
					this->children.erase ( this->children.begin() + childIndex );
					--childIndex;	// Make the next loop iteration not skip a chunk.
				}

				delete currChild;
				continue;

			}

			// Move the chunk to the end of the last RIFF chunk and make the old space junk.

			if ( (this == handler->lastChunk) && (childIndex+1 == (XMP_Index)this->children.size()) ) continue;	// Already last.

			handler->lastChunk->children.push_back( currChild );
			if ( currChild->oldSize != 0 ) {
				this->children[childIndex] = new JunkChunk ( 0, currChild->oldSize );	// Replace the old space with junk.
			} else {
				this->children.erase ( this->children.begin() + childIndex );	// Remove the newly created chunk's old location.
				--childIndex;	// Make the next loop iteration not skip a chunk.
			}

		}

	}

	// Compute the finished container's new size (for both RIFF and LIST).

	this->newSize = 12;	// Start with standard container header.
	for ( childIndex = 0; childIndex < (XMP_Index)this->children.size(); ++childIndex ) {
		currChild = this->children[childIndex];
		this->newSize += currChild->newSize;
		this->newSize += (this->newSize & 1);	// Round up if odd.
	}

	XMP_Validate ( (this->newSize <= 0xFFFFFFFFLL), "No single chunk may be above 4 GB", kXMPErr_Unimplemented );

}

std::string ContainerChunk::toString(XMP_Uns8 level )
{
	XMP_Int64 offset= 12; // compute offsets, just for informational purposes
	// (actually only correct for first chunk)

	char buffer[256];
	snprintf( buffer, 255, "%.4s:%.4s, "
			"oldSize: 0x%8llX, "
			"newSize: 0x%.8llX, "
			"oldPos: 0x%.8llX\n",
			(char*)(&this->id), (char*)(&this->containerType), this->oldSize, this->newSize, this->oldPos );

	std::string r(buffer);
	chunkVectIter iter;
	for( iter = this->children.begin(); iter != this->children.end(); iter++ )
	{
		char buffer[256];
		snprintf( buffer, 250, "offset 0x%.8llX", offset );
		r += std::string ( level*4, ' ' ) + std::string( buffer ) + ":" + (*iter)->toString( level + 1 );
		offset += (*iter)->newSize;
		if ( offset % 2 == 1 )
			offset++;
	}
	return std::string(r);
}

void ContainerChunk::write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk )
{
	if ( isMainChunk )
		file ->Rewind();

	// enforce even position
	XMP_Int64 chunkStart = file->Offset();
	XMP_Int64 chunkEnd = chunkStart + this->newSize;
	XMP_Enforce( chunkStart % 2 == 0 );
	chunkVect *rc = &this->children;

	// [2473303] have to write back-to-front to avoid stomp-on-feet
	XMP_Int64 childStart = chunkEnd;
	for ( XMP_Int32 chunkNo = (XMP_Int32)(rc->size() -1); chunkNo >= 0; chunkNo-- )
	{
		Chunk* cur = rc->at(chunkNo);

		// pad byte first
		if ( cur->newSize % 2 == 1 )
		{
			childStart--;
			file->Seek ( childStart, kXMP_SeekFromStart  );
			XIO::WriteUns8( file, 0 );
		}

		// then contents
		childStart-= cur->newSize;
		file->Seek ( childStart, kXMP_SeekFromStart  );
		switch ( cur->chunkType )
		{
			case chunk_GENERAL: //COULDDO enfore no change, since not write-out-able
				if ( cur->oldPos != childStart )
					XIO::Move( file, cur->oldPos, file, childStart, cur->oldSize );
				break;
			default:
				cur->write( handler, file, false );
				break;
		} // switch

	} // for
	XMP_Enforce ( chunkStart + 12 == childStart);
	file->Seek ( chunkStart, kXMP_SeekFromStart );

	XIO::WriteUns32_LE( file, this->id );
	XIO::WriteUns32_LE( file, (XMP_Uns32) this->newSize - 8 ); // validated in changesAndSize() above
	XIO::WriteUns32_LE( file, this->containerType );

}

void ContainerChunk::release()
{
	// free subchunks
	Chunk* curChunk;
	while( ! this->children.empty() )
	{
		curChunk = this->children.back();
		delete curChunk;
		this->children.pop_back();
	}
}

ContainerChunk::~ContainerChunk()
{
	this->release(); // free resources
}

// XMP CHUNK ///////////////////////////////////////////////
// a) create

// a) creation
XMPChunk::XMPChunk( ContainerChunk* parent ) : Chunk( parent, chunk_XMP , kChunk_XMP )
{
	// nothing
}

// b) parse
XMPChunk::XMPChunk( ContainerChunk* parent, RIFF_MetaHandler* handler ) : Chunk( parent, handler, false, chunk_XMP )
{
	chunkType = chunk_XMP;
	XMP_IO* file = handler->parent->ioRef;
	XMP_Uns8 level = handler->level;

	handler->packetInfo.offset = this->oldPos + 8;
	handler->packetInfo.length = (XMP_Int32) this->oldSize - 8;

	handler->xmpPacket.reserve ( handler->packetInfo.length );
	handler->xmpPacket.assign ( handler->packetInfo.length, ' ' );
	file->ReadAll ( (void*)handler->xmpPacket.data(), handler->packetInfo.length );

	handler->containsXMP = true; // last, after all possible failure

	// pointer for later processing
	handler->xmpChunk = this;
}

void XMPChunk::changesAndSize( RIFF_MetaHandler* handler )
{
	XMP_Enforce( &handler->xmpPacket != 0 );
	XMP_Enforce( handler->xmpPacket.size() > 0 );
	this->newSize = 8 + handler->xmpPacket.size();

	XMP_Validate( this->newSize <= 0xFFFFFFFFLL, "no single chunk may be above 4 GB", kXMPErr_InternalFailure );

	// a complete no-change would have been caught in XMPFiles common code anyway
	this->hasChange = true;
}

void XMPChunk::write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk )
{
	XIO::WriteUns32_LE( file, kChunk_XMP );
	XIO::WriteUns32_LE( file, (XMP_Uns32) this->newSize - 8 ); // validated in changesAndSize() above
	file->Write ( handler->xmpPacket.data(), (XMP_Int32)handler->xmpPacket.size()  );
}

// Value CHUNK ///////////////////////////////////////////////
// a) creation
ValueChunk::ValueChunk( ContainerChunk* parent, std::string value, XMP_Uns32 id ) : Chunk( parent, chunk_VALUE, id )
{
	this->oldValue = std::string();
	this->SetValue( value );
}

// b) parsing
ValueChunk::ValueChunk( ContainerChunk* parent, RIFF_MetaHandler* handler ) : Chunk( parent, handler, false, chunk_VALUE )
{
	// set value: -----------------
	XMP_IO* file = handler->parent->ioRef;
	XMP_Uns8 level = handler->level;

	// unless changed through reconciliation, assume for now.
	// IMPORTANT to stay true to the original (no \0 cleanup or similar)
	// since unknown value chunks might not be fully understood,
	// hence must be precisely preserved !!!

	XMP_Int32 length = (XMP_Int32) this->oldSize - 8;
	this->oldValue.reserve( length );
	this->oldValue.assign( length + 1, '\0' );
	file->ReadAll ( (void*)this->oldValue.data(), length );

	this->newValue = this->oldValue;
	this->newSize = this->oldSize;
}

void ValueChunk::SetValue( std::string value, bool optionalNUL /* = false */ )
{
	this->newValue.assign( value );
	if ( (! optionalNUL) || ((value.size() & 1) == 1) ) {
		// ! The NUL should be optional in WAV to avoid a parsing bug in Audition 3 - can't handle implicit pad byte.
		this->newValue.append( 1, '\0' ); // append zero termination as explicit part of string
	}
	this->newSize = this->newValue.size() + 8;
}

void ValueChunk::changesAndSize( RIFF_MetaHandler* handler )
{
	// Don't simply assign to this->hasChange, it might already be true.
	if ( this->newValue.size() != this->oldValue.size() ) {
		this->hasChange = true;
	} else if ( strncmp ( this->oldValue.c_str(), this->newValue.c_str(), this->newValue.size() ) != 0 ) {
		this->hasChange = true;
	}
}

void ValueChunk::write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk )
{
	XIO::WriteUns32_LE( file, this->id );
	XIO::WriteUns32_LE( file, (XMP_Uns32)this->newSize - 8 );
	file->Write ( this->newValue.data() , (XMP_Int32)this->newSize - 8  );
}

/* remove value chunk if existing.
   return true if it was existing. */
bool ContainerChunk::removeValue( XMP_Uns32	id )
{
	valueMap* cm = &this->childmap;
	valueMapIter iter = cm->find( id );

	if( iter == cm->end() )
		return false;  //not found

	ValueChunk* propChunk = iter->second;

	// remove from vector (difficult)
	chunkVect* cv = &this->children;
	chunkVectIter cvIter;
	for (cvIter = cv->begin(); cvIter != cv->end(); ++cvIter )
	{
		if ( (*cvIter)->id == id )
			break; // found!
	}
	XMP_Validate( cvIter != cv->end(), "property not found in children vector", kXMPErr_InternalFailure );
	cv->erase( cvIter );

	// remove from map (easy)
	cm->erase( iter );

	delete propChunk;
	return true; // found and removed
}

/* returns iterator to (first) occurence of this chunk.
   iterator to the end of the map if chunk pointer is not found  */
chunkVectIter ContainerChunk::getChild( Chunk* needle )
{
	chunkVectIter iter;
	for( iter = this->children.begin(); iter != this->children.end(); iter++ )
	{
		Chunk* temp1 = *iter;
		Chunk* temp2 = needle;
		if ( (*iter) == needle ) return iter;
	}
	return this->children.end();
}

/* replaces a chunk by a JUNK chunk.
   Also frees memory of prior chunk. */
void ContainerChunk::replaceChildWithJunk( Chunk* child, bool deleteChild )
{
	chunkVectIter iter = getChild( child );
	if ( iter == this->children.end() ) {
		throw new XMP_Error(kXMPErr_InternalFailure, "replaceChildWithJunk: childChunk not found.");
	}

	*iter = new JunkChunk ( NULL, child->oldSize );
	if ( deleteChild ) delete child;

	this->hasChange = true;
}

// JunkChunk ///////////////////////////////////////////////////
// a) creation
JunkChunk::JunkChunk( ContainerChunk* parent, XMP_Int64 size ) : Chunk( parent, chunk_JUNK, kChunk_JUNK )
{
	XMP_Assert( size >= 8 );
	this->oldSize = size;
	this->newSize = size;
	this->hasChange = true;
}

// b) parsing
JunkChunk::JunkChunk( ContainerChunk* parent, RIFF_MetaHandler* handler ) : Chunk( parent, handler, true, chunk_JUNK )
{
	chunkType = chunk_JUNK;
}

void JunkChunk::changesAndSize( RIFF_MetaHandler* handler )
{
	this->newSize = this->oldSize; // optimization at a later stage
	XMP_Validate( this->newSize <= 0xFFFFFFFFLL, "no single chunk may be above 4 GB", kXMPErr_InternalFailure );
	if ( this->id == kChunk_JUNQ ) this->hasChange = true;	// Force ID change to JUNK.
}

// zeroBuffer, etc to write out empty native padding
const static XMP_Uns32 kZeroBufferSize64K = 64 * 1024;
static XMP_Uns8 kZeroes64K [ kZeroBufferSize64K ]; // C semantics guarantee zero initialization.

void JunkChunk::write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk )
{
	XIO::WriteUns32_LE( file, kChunk_JUNK );		// write JUNK, never JUNQ
	XMP_Enforce( this->newSize < 0xFFFFFFFF );
	XMP_Enforce( this->newSize >= 8 );			// minimum size of any chunk
	XMP_Uns32 innerSize = (XMP_Uns32)this->newSize - 8;
	XIO::WriteUns32_LE( file, innerSize );

	// write out in 64K chunks
	while ( innerSize > kZeroBufferSize64K )
	{
		file->Write ( kZeroes64K , kZeroBufferSize64K  );
		innerSize -= kZeroBufferSize64K;
	}
	file->Write ( kZeroes64K , innerSize  );
}

} // namespace RIFF
