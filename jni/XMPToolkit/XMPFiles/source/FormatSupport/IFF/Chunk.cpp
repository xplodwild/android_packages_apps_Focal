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

#include "XMPFiles/source/FormatSupport/IFF/Chunk.h"
#include "source/XMP_LibUtils.hpp"
#include "source/XIO.hpp"

#include <cstdio>
#include <typeinfo>

using namespace IFF_RIFF;

//-----------------------------------------------------------------------------
// 
// Chunk::createChunk(...)
// 
// Purpose: [static] Static factory to create an unknown chunk
// 
//-----------------------------------------------------------------------------

Chunk* Chunk::createChunk( const IEndian& endian )
{
	return new Chunk( endian );
}


//-----------------------------------------------------------------------------
// 
// Chunk::createUnknownChunk(...)
// 
// Purpose: [static] Static factory to create an unknown chunk with initial id, 
//					 sizes and offsets.
// 
//-----------------------------------------------------------------------------

Chunk* Chunk::createUnknownChunk(
	const IEndian& endian,
	const XMP_Uns32 id,
	const XMP_Uns32 type,
	const XMP_Uns64 size,
	const XMP_Uns64 originalOffset,
	const XMP_Uns64 offset
)
{
	Chunk *chunk = new Chunk( endian );
	chunk->setID( id );
	chunk->mOriginalOffset = originalOffset;
	chunk->mOffset = offset;

	if (type != 0)
	{
		chunk->setType(type);
	}

	// sizes have to be set after type, otherwise the setType sets the size to 4.
	chunk->mSize = chunk->mOriginalSize = size;
	chunk->mChunkMode = CHUNK_UNKNOWN;
	chunk->mDirty = false;
	return chunk;
}

//-----------------------------------------------------------------------------
// 
// Chunk::createHeaderChunk(...)
// 
// Purpose: [static] Static factory to create a leaf chunk with no data area or
//					 only the type in the data area
// 
//-----------------------------------------------------------------------------

Chunk* Chunk::createHeaderChunk( const IEndian& endian,	const XMP_Uns32 id,	const XMP_Uns32 type /*= kType_NONE*/)
{
	Chunk *chunk = new Chunk( endian );
	chunk->setID( id );

	XMP_Uns64 size = 0;

	if( type != kType_NONE )
	{
		chunk->setType( type );
		size += Chunk::TYPE_SIZE;
	}

	chunk->mSize			= size;
	chunk->mOriginalSize	= size;
	chunk->mChunkMode		= CHUNK_LEAF;
	chunk->mDirty			= false;

	return chunk;
}


//-----------------------------------------------------------------------------
// 
// Chunk::Chunk(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

Chunk::Chunk( const IEndian& endian )
: mEndian( endian )
{
	// initialize private instance variables
	mChunkId.id		= kChunk_NONE;
	mChunkId.type	= kType_NONE;
	mSize			= 0;
	mOriginalSize	= 0;
	mBufferSize		= 0;
	mData			= NULL;
	mParent			= NULL;
	mOriginalOffset	= 0;
	mOffset			= 0;
	mDirty			= false;
	mChunkMode		= CHUNK_UNKNOWN;
}


Chunk::~Chunk()
{
	for( ChunkIterator iter = mChildren.begin(); iter != mChildren.end(); iter++ )
	{
		delete *iter;
	}

	// Free allocated data buffer
	if( mData != NULL )
	{
		delete [] mData;
	}
}


/************************ IChunk interface implementation ************************/

//-----------------------------------------------------------------------------
// 
// Chunk::getData(...)
// 
// Purpose: access data area of Chunk
// 
//-----------------------------------------------------------------------------

XMP_Uns64 Chunk::getData( const XMP_Uns8** data ) const
{
	if( data == NULL )
	{
		XMP_Throw ( "Invalid data pointer.", kXMPErr_BadParam );
	}

	*data = mData;

	return mBufferSize;
}


//-----------------------------------------------------------------------------
// 
// Chunk::setData(...)
// 
// Purpose: Set new data for the chunk. 
//			Will delete an existing internal buffer and recreate a new one
//			and copy the given data into that new buffer.
// 
//-----------------------------------------------------------------------------

void Chunk::setData( const XMP_Uns8* const data, XMP_Uns64 size, XMP_Bool writeType /*=false*/ )
{
	// chunk nodes cannot contain data
	if ( mChunkMode == CHUNK_NODE )
	{
		XMP_Throw ( "A chunk node cannot contain data.", kXMPErr_BadParam );
	}
	else if ( data == NULL || size == 0 )
	{
		XMP_Throw ( "Invalid data pointer.", kXMPErr_BadParam );
	}

	if( mData != NULL )
	{
		delete [] mData;
	}

	if( writeType )
	{
		mBufferSize = size + TYPE_SIZE;
		mData = new XMP_Uns8[static_cast<size_t>(mBufferSize)]; // Throws bad_alloc exception in case of being out of memory
		setType( mChunkId.type );
		memcpy( &mData[TYPE_SIZE], data, static_cast<size_t>(size) );
	}
	else
	{
		mBufferSize = size;
		mData = new XMP_Uns8[static_cast<size_t>(mBufferSize)]; // Throws bad_alloc exception in case of being out of memory
		// ! We assume that size IS the actual size of that input buffer, otherwise behavior is undefined
		memcpy( mData, data, static_cast<size_t>(size) );

		// set the type variable
		if( mBufferSize >= TYPE_SIZE )
		{
			//Chunk type is always BE
			//The first four bytes could be the type
			mChunkId.type = BigEndian::getInstance().getUns32( mData );
		}
	}

	mChunkMode = CHUNK_LEAF;
	setChanged();
	adjustSize();
}

//-----------------------------------------------------------------------------
// 
// Chunk::getUns32(...)
// 
// Purpose: The following methods are getter/setter for certain data types. 
//			They always take care of little-endian/big-endian issues.
//			The offset starts at the data area of the Chunk.
// 
//-----------------------------------------------------------------------------

XMP_Uns32 Chunk::getUns32( XMP_Uns64 offset ) const
{
	if( offset + sizeof(XMP_Uns32) > mBufferSize )
	{
		XMP_Throw ( "Data access out of bounds", kXMPErr_BadIndex );
	}
	return mEndian.getUns32( &mData[offset] );
}


void Chunk::setUns32( XMP_Uns32 value, XMP_Uns64 offset )
{
	// chunk nodes cannot contain data
	if ( mChunkMode == CHUNK_NODE )
	{
		XMP_Throw ( "A chunk node cannot contain data.", kXMPErr_BadParam );
	}

	// If the new value exceeds the size of the buffer, recreate the buffer
	adjustInternalBuffer( offset + sizeof(XMP_Uns32) );
	// Write the new value
	mEndian.putUns32( value, &mData[offset] );
	// Chunk becomes leaf chunk when adding data
	mChunkMode = CHUNK_LEAF;
	// Flag the chunk as dirty
	setChanged();
	// If the buffer is bigger than the Chunk size, adjust the Chunk size
	adjustSize();

}


XMP_Uns64 Chunk::getUns64( XMP_Uns64 offset ) const
{
	if( offset + sizeof(XMP_Uns64) > mBufferSize )
	{
		XMP_Throw ( "Data access out of bounds", kXMPErr_BadIndex );
	}
	return mEndian.getUns64( &mData[offset] );
}


void Chunk::setUns64( XMP_Uns64 value, XMP_Uns64 offset )
{
	// chunk nodes cannot contain data
	if ( mChunkMode == CHUNK_NODE )
	{
		XMP_Throw ( "A chunk node cannot contain data.", kXMPErr_BadParam );
	}

	// If the new value exceeds the size of the buffer, recreate the buffer
	adjustInternalBuffer( offset + sizeof(XMP_Uns64) );
	// Write the new value
	mEndian.putUns64( value, &mData[offset] );
	// Chunk becomes leaf chunk when adding data
	mChunkMode = CHUNK_LEAF;
	// Flag the chunk as dirty
	setChanged();
	// If the buffer is bigger than the Chunk size, adjust the Chunk size
	adjustSize();
}


XMP_Int32 Chunk::getInt32( XMP_Uns64 offset ) const
{
	if( offset + sizeof(XMP_Int32) > mBufferSize )
	{
		XMP_Throw ( "Data access out of bounds", kXMPErr_BadIndex );
	}
	return mEndian.getUns32( &mData[offset] );
}


void Chunk::setInt32( XMP_Int32 value, XMP_Uns64 offset )
{
	// chunk nodes cannot contain data
	if ( mChunkMode == CHUNK_NODE )
	{
		XMP_Throw ( "A chunk node cannot contain data.", kXMPErr_BadParam );
	}

	// If the new value exceeds the size of the buffer, recreate the buffer
	adjustInternalBuffer( offset + sizeof(XMP_Int32) );
	// Write the new value
	mEndian.putUns32( value, &mData[offset] );
	// Chunk becomes leaf chunk when adding data
	mChunkMode = CHUNK_LEAF;
	// Flag the chunk as dirty
	setChanged();
	// If the buffer is bigger than the Chunk size, adjust the Chunk size
	adjustSize();
}


XMP_Int64 Chunk::getInt64( XMP_Uns64 offset ) const
{
	if( offset + sizeof(XMP_Int64) > mBufferSize )
	{
		XMP_Throw ( "Data access out of bounds", kXMPErr_BadIndex );
	}
	return mEndian.getUns64( &mData[offset] );
}


void Chunk::setInt64( XMP_Int64 value, XMP_Uns64 offset )
{
	// chunk nodes cannot contain data
	if ( mChunkMode == CHUNK_NODE )
	{
		XMP_Throw ( "A chunk node cannot contain data.", kXMPErr_BadParam );
	}

	// If the new value exceeds the size of the buffer, recreate the buffer
	adjustInternalBuffer( offset + sizeof(XMP_Int64) );
	// Write the new value
	mEndian.putUns64( value, &mData[offset] );
	// Chunk becomes leaf chunk when adding data
	mChunkMode = CHUNK_LEAF;
	// Flag the chunk as dirty
	setChanged();
	// If the buffer is bigger than the Chunk size, adjust the Chunk size
	adjustSize();
}


std::string Chunk::getString( XMP_Uns64 size /*=0*/, XMP_Uns64 offset /*=0*/ ) const
{
	if( offset + size > mBufferSize )
	{
		XMP_Throw ( "Data access out of bounds", kXMPErr_BadIndex );
	}

	XMP_Uns64 requestedSize = size != 0 ? size : mBufferSize - offset;

	std::string str((char *)&mData[offset],static_cast<size_t>(requestedSize));
	
	return str;
}


void Chunk::setString( std::string value, XMP_Uns64 offset )
{
	if ( mChunkMode == CHUNK_NODE )
	{
		XMP_Throw ( "A chunk node cannot contain data.", kXMPErr_BadParam );
	}

	// If the new value exceeds the size of the buffer, recreate the buffer
	adjustInternalBuffer( offset + value.length() );
	// Write the new value
	memcpy( &mData[offset], value.data(), value.length() );
	// Chunk becomes leaf chunk when adding data
	mChunkMode = CHUNK_LEAF;
	// Flag the chunk as dirty
	setChanged();
	// If the buffer is bigger than the Chunk size, adjust the Chunk size
	adjustSize();
}


/************************ Chunk public methods ************************/

//-----------------------------------------------------------------------------
// 
// Chunk::setID(...)
// 
// Purpose: Sets the chunk id.
// 
//-----------------------------------------------------------------------------

void Chunk::setID( XMP_Uns32 id )
{
	mChunkId.id = id;
	setChanged();
}


//-----------------------------------------------------------------------------
// 
// Chunk::setType(...)
// 
// Purpose: Sets the chunk type
// 
//-----------------------------------------------------------------------------

void Chunk::setType( XMP_Uns32 type )
{
	mChunkId.type = type;

	// reserve space for type
	// setChanged() and adjustSize() implicitly called
	// make sure that no exception is thrown
	ChunkMode existing = mChunkMode;
	mChunkMode = CHUNK_UNKNOWN;
	setUns32(0, 0);
	mChunkMode = existing;

	BigEndian::getInstance().putUns32( type, mData );
}

//-----------------------------------------------------------------------------
// 
// Chunk::getPadSize(...)
// 
// Purpose: Returns the original size of the Chunk including a pad byte if 
//			the size isn't a even number
// 
//-----------------------------------------------------------------------------

XMP_Uns64 Chunk::getOriginalPadSize( bool includeHeader /*= false*/ ) const
{
	XMP_Uns64 ret = this->getOriginalSize( includeHeader );

	if( ret & 1 )
	{
		ret++;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// Chunk::getPadSize(...)
// 
// Purpose: Returns the current size of the Chunk including a pad byte if the 
//			size isn't a even number
// 
//-----------------------------------------------------------------------------

XMP_Uns64 Chunk::getPadSize( bool includeHeader /*= false*/ ) const
{
	XMP_Uns64 ret = this->getSize( includeHeader );

	if( ret & 1 )
	{
		ret++;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// 
// Chunk::calculateSize(...)
// 
// Purpose: Calculate the size of this chunks based on its children sizes. 
//			If this chunk has no children then no new size will be calculated.
// 
//-----------------------------------------------------------------------------

XMP_Uns64 Chunk::calculateSize( bool setOriginal /*= false*/ )
{
	XMP_Uns64 size = 0LL;

	//
	// calculate only foe nodes
	//
	if( this->getChunkMode() == CHUNK_NODE )
	{
		//
		// calculate size of all children
		//
		for( ChunkIterator iter = mChildren.begin(); iter != mChildren.end(); iter++ )
		{
			XMP_Uns64 childSize = (*iter)->getSize(true);

			size += childSize;

			//
			// take account of pad byte
			//
			if( childSize & 1 )
			{
				size++;
			}
		}

		//
		// assume that we have a type
		//
		size += Chunk::TYPE_SIZE;

		//
		// set dirty flag only if something has changed
		//
		if( size != mSize || ( setOriginal && size != mOriginalSize ) )
		{
			this->setChanged();
		}

		//
		// set new size(s)
		//
		if( setOriginal )
		{
			mOriginalSize = size;
		}

		mSize = size;
	}
	else
		size = mSize;

	return size;
}


//-----------------------------------------------------------------------------
// 
// Chunk::calculateWriteSize(...)
// 
// Purpose: Calculate the size of the chunks that are dirty including the size 
//			of its children
// 
//-----------------------------------------------------------------------------

XMP_Int64 Chunk::calculateWriteSize( ) const
{
	XMP_Int64 size=0;
	if (hasChanged())
	{
		size+=(sizeof(XMP_Uns32)*2);
		if (mChunkMode == CHUNK_LEAF)
		{
			if ( mSize % 2 == 1 )
			{
				// for odd file sizes, a pad byte is written
				size+=(mSize+1);
			}
			else
			{
				size+=mSize;
			}
		}
		else	// mChunkMode == CHUNK_NODE
		{
			// writes type if defined
			if (mChunkId.type != kType_NONE)
			{
				size+=sizeof(XMP_Uns32);
			}

			 // calls calculateWriteSize recursively on it's children
			for( ConstChunkIterator iter = mChildren.begin(); iter != mChildren.end(); iter++ )
			{
				size+=(*iter)->calculateWriteSize(  );
			}
		}
	}

	return size;
}

//-----------------------------------------------------------------------------
// 
// Chunk::setOffset(...)
// 
// Purpose: Adjust the offset that this chunk has within the file
// 
//-----------------------------------------------------------------------------

void Chunk::setOffset (XMP_Uns64 newOffset)   // changes during rearranging
{
	XMP_Uns64 oldOffset = mOffset;
	mOffset = newOffset;

	if( mOffset != oldOffset )
	{
		setChanged();
	}
}


//-----------------------------------------------------------------------------
// 
// Chunk::resetChanges(...)
// 
// Purpose: Resets the dirty status for this chunk and its children to false
// 
//-----------------------------------------------------------------------------

void Chunk::resetChanges()
{
	mDirty = false;

	for( ChunkIterator iter = mChildren.begin(); iter != mChildren.end(); iter++ )
	{
		(*iter)->resetChanges();
	}
} //resetChanges


//-----------------------------------------------------------------------------
// 
// Chunk::setAsNew(...)
// 
// Purpose: Sets all necessary member variables to flag this chunk as a new one 
//			being inserted into the tree
// 
//-----------------------------------------------------------------------------

void Chunk::setAsNew()
{
	mOriginalSize = mSize;
	mOriginalOffset = mOffset;
}

//-----------------------------------------------------------------------------
// 
// Chunk::toString(...)
// 
// Purpose: Creates a string representation of the chunk (debug method)
// 
//-----------------------------------------------------------------------------

std::string Chunk::toString( std::string tabs, XMP_Bool showOriginal )
{
	const BigEndian &BE = BigEndian::getInstance();
	char buffer[256];
	XMP_Uns32 id = BE.getUns32(&this->mChunkId.id);
	XMP_Uns32 type = BE.getUns32(&this->mChunkId.type);

	XMP_Uns64 size, offset;

	if ( showOriginal )
	{
		size = mEndian.getUns64(&this->mOriginalSize);
		offset = mEndian.getUns64(&this->mOriginalOffset);
	}
	else
	{
		size = mEndian.getUns64(&this->mSize);
		offset = mEndian.getUns64(&this->mOffset);
	}

	snprintf( buffer, 255, "%.4s -- "
								"size: 0x%.8llX,  "
								"type: %.4s,  "
								"offset: 0x%.8llX",
				(char*)(&id),
				size,
				(char*)(&type),
				offset );
	std::string str(buffer);

	// Dump children
	if ( mChildren.size() > 0)
	{
		tabs.append("\t");
	}

	for( ChunkIterator iter = mChildren.begin(); iter != mChildren.end(); iter++ )
	{
		str += "\n";
		str += tabs;
		str += (*iter)->toString(tabs , showOriginal);
	}

	return str;
}


/************************ file access ************************/

//-----------------------------------------------------------------------------
// 
// Chunk::readChunk(...)
// 
// Purpose: Read id, size and offset and create a chunk with mode CHUNK_UNKNOWN.
//			The file is expected to be open and is not closed!
// 
//-----------------------------------------------------------------------------

void Chunk::readChunk( XMP_IO* file )
{
	if( file == NULL )
	{
		XMP_Throw( "Chunk::readChunk: Must pass a valid file pointer", kXMPErr_BadParam );
	}

	if( mChunkId.id != kChunk_NONE )
	{
		XMP_Throw ( "readChunk must not be called more than once", kXMPErr_InternalFailure );
	}
	// error handling is done in the controller
	// determine offset in the file
	mOriginalOffset = mOffset = file->Offset();
	//ID is always BE
	mChunkId.id = XIO::ReadUns32_BE( file );
	// Size can be both
	if (typeid(mEndian) == typeid(LittleEndian))
	{
		mOriginalSize = mSize = XIO::ReadUns32_LE( file );

	}
	else
	{
		mOriginalSize = mSize = XIO::ReadUns32_BE( file );
	}

	// For Type do not assume any format as it could be data, read it as bytes
	if (mSize >= TYPE_SIZE)
	{
		mData = new XMP_Uns8[TYPE_SIZE];

		for ( XMP_Uns32 i = 0; i < TYPE_SIZE ; i++ )
		{
			mData[i] = XIO::ReadUns8( file );
		}
		//Chunk type is always BE
		//The first four bytes could be the type
		mChunkId.type = BigEndian::getInstance().getUns32( mData );
	}

	mDirty = false;
}//readChunk


//-----------------------------------------------------------------------------
// 
// Chunk::cacheChunkData(...)
// 
// Purpose: Stores the data in the class (only called if required).
//			The file is expected to be open and is not closed!
// 
//-----------------------------------------------------------------------------

void Chunk::cacheChunkData( XMP_IO* file )
{
	XMP_Enforce( file != NULL );

	if( mChunkMode != CHUNK_UNKNOWN )
	{
		XMP_Throw ( "chunk already has either data or children.", kXMPErr_BadParam );
	}

	// error handling is done in the controller

	// continue only when the chunk contains data
	if (mSize != 0)
	{
		mBufferSize = mSize;
		XMP_Uns8* tmp = new XMP_Uns8[XMP_Uns32(mSize)];

		// Do we have a type?
		if (mSize >= TYPE_SIZE)
		{
			// add type in front of new buffer
			for ( XMP_Uns32 i = 0; i < TYPE_SIZE ; i++ )
			{
				tmp[i] = mData[i];
			}
			// Read rest of data from file
			if( mSize != TYPE_SIZE )
			{
				// Chunks that are cached are very probably not bigger than 2GB, so cast is safe
				file->ReadAll ( &tmp[TYPE_SIZE], static_cast<XMP_Int32>(mSize - TYPE_SIZE) );
			}
		}
		else
		{
			// Chunks that are cached are very probably not bigger than 2GB, so cast is safe
			file->ReadAll ( tmp, static_cast<XMP_Int32>(mSize) );
		}
		// deletes the existing array
		delete [] mData;
		//assign the new buffer
		mData = tmp;
	}

	// Remember that this method has been called
	mDirty = false;
	mChunkMode = CHUNK_LEAF;
}


//-----------------------------------------------------------------------------
// 
// Chunk::writeChunk(...)
// 
// Purpose: Write or updates chunk (new data, new size, new position).
//			The file is expected to be open and is not closed!
// 
//-----------------------------------------------------------------------------

void Chunk::writeChunk( XMP_IO* file )
{
	if( file == NULL )
	{
		XMP_Throw( "Chunk::writeChunk: Must pass a valid file pointer", kXMPErr_BadParam );
	}

	if (mChunkMode == CHUNK_UNKNOWN)
	{
		if (hasChanged())
		{
			XMP_Throw ( "A chunk with mode unknown must not be changed & written.", kXMPErr_BadParam );
		}

		// do nothing
	}
	else if (hasChanged())
	{
		// positions the file pointer
		file->Seek ( mOffset, kXMP_SeekFromStart );


		// ============ This part is identical for CHUNK_LEAF and CHUNK_TYPE ============

		 // writes ID (starting with offset)
		XIO::WriteInt32_BE( file, mChunkId.id );

		// writes size, which is always 32bit
		XMP_Uns32 outSize = ( mSize >= 0x00000000FFFFFFFF ? 0xFFFFFFFF : static_cast<XMP_Uns32>( mSize & 0x00000000FFFFFFFF ) );

		if (typeid(mEndian) == typeid(LittleEndian))
		{	
			XIO::WriteUns32_LE( file, static_cast<XMP_Uns32>(mSize) );
		}
		else
		{
			XIO::WriteUns32_BE( file, static_cast<XMP_Uns32>(mSize) );
		}


		// ============ This part is different for CHUNK_LEAF and CHUNK_TYPE ============
		if (mChunkMode == CHUNK_LEAF)
		{
			// writes buffer (including the optional type at the beginning)
			// Cached chunks will very probably not be bigger than 2GB, so cast is safe
			file->Write ( mData, static_cast<XMP_Int32>(mSize)  );
			if ( mSize % 2 == 1 )
			{
				// for odd file sizes, a pad byte is written
				XIO::WriteUns8 ( file, 0 );
			}
		}
		else	// mChunkMode == CHUNK_NODE
		{
			// writes type if defined
			if (mChunkId.type != kType_NONE)
			{
				XIO::WriteInt32_BE( file, mChunkId.type );
			}

			 // calls writeChunk on it's children
			for( ChunkIterator iter = mChildren.begin(); iter != mChildren.end(); iter++ )
			{
				(*iter)->writeChunk( file );
			}
		}
	}

	// set back dirty state
	mDirty = false;
}


/************************ children access ************************/

//-----------------------------------------------------------------------------
// 
// Chunk::numChildren(...)
// 
// Purpose: Returns the number children chunks
// 
//-----------------------------------------------------------------------------

XMP_Uns32 Chunk::numChildren() const
{
	return static_cast<XMP_Uns32>( mChildren.size() );
}


//-----------------------------------------------------------------------------
// 
// Chunk::getChildAt(...)
// 
// Purpose: Returns a child node
// 
//-----------------------------------------------------------------------------

Chunk* Chunk::getChildAt( XMP_Uns32 pos ) const
{
	try
	{
		return mChildren.at(pos);
	}
	catch( ... )
	{
		XMP_Throw ( "Non-existing child requested.", kXMPErr_BadIndex );
	}
}


//-----------------------------------------------------------------------------
// 
// Chunk::appendChild(...)
// 
// Purpose: Appends a child node at the end of the children list
// 
//-----------------------------------------------------------------------------

void Chunk::appendChild( Chunk* child, XMP_Bool adjustSizes )
{
	if (mChunkMode == CHUNK_LEAF)
	{
		XMP_Throw ( "A chunk leaf cannot contain children.", kXMPErr_BadParam );
	}

	try
	{
		mChildren.push_back( child );
		// make this the parent of the new node
		child->mParent = this;
		mChunkMode = CHUNK_NODE;

		// set offset of new child
		XMP_Uns64 childOffset = 0;

		if( this->numChildren() == 1 )
		{
			// first added child
			if( this->getID() != kChunk_NONE )
			{
				childOffset = this->getOffset() + Chunk::HEADER_SIZE + ( this->getType() == kType_NONE ? 0 : Chunk::TYPE_SIZE );
			}
		}
		else
		{
			Chunk* predecessor = this->getChildAt( this->numChildren() - 2 );
			childOffset = predecessor->getOffset() + predecessor->getPadSize( true );
		}

		child->setOffset( childOffset );

		setChanged();
		
		if ( adjustSizes )
		{
			// to fix the sizes of this node and parents
			adjustSize( child->getSize(true) );
		}
	}
	catch (...)
	{
		XMP_Throw ( "Vector error in appendChild", kXMPErr_InternalFailure );
	}
}


//-----------------------------------------------------------------------------
// 
// Chunk::insertChildAt(...)
// 
// Purpose: Inserts a child node at a certain position
// 
//-----------------------------------------------------------------------------

void Chunk::insertChildAt( XMP_Uns32 pos, Chunk* child )
{
	if (mChunkMode == CHUNK_LEAF)
	{
		XMP_Throw ( "A chunk leaf cannot contain children.", kXMPErr_BadParam );
	}

	try
	{
		if (pos <= mChildren.size())
		{
			mChildren.insert(mChildren.begin() + pos, child);
			// make this the parent of the new node
			child->mParent = this;
			mChunkMode = CHUNK_NODE;

			// set offset of new child
			XMP_Uns64 childOffset = 0;

			if( pos == 0 )
			{
				if( this->getID() != kChunk_NONE )
				{
					childOffset = this->getOffset() + Chunk::HEADER_SIZE + ( this->getType() == kType_NONE ? 0 : Chunk::TYPE_SIZE );
				}
			}
			else
			{
				Chunk* predecessor = this->getChildAt( pos-1 );
				childOffset = predecessor->getOffset() + predecessor->getPadSize( true );
			}

			child->setOffset( childOffset );

			setChanged();

			// to fix the sizes of this node and parents
			adjustSize( child->getSize(true) );
		}
		else
		{
			XMP_Throw ( "Index not valid.", kXMPErr_BadIndex );
		}
	}
	catch (...)
	{
		XMP_Throw ( "Index not valid.", kXMPErr_BadIndex );
	}
}


//-----------------------------------------------------------------------------
// 
// Chunk::removeChildAt(...)
// 
// Purpose: Removes a child node at a given position
// 
//-----------------------------------------------------------------------------

Chunk* Chunk::removeChildAt( XMP_Uns32 pos )
{
	Chunk* toDelete = NULL;

	try
	{
		toDelete = mChildren.at(pos);
		// to fix the size of this node
		XMP_Int64 sizeDeleted = static_cast<XMP_Int64>(toDelete->getSize(true));
		mChildren.erase(mChildren.begin() + pos);

		setChanged();

		// to fix the sizes of this node and parents
		adjustSize(-sizeDeleted);
	}
	catch (...)
	{
		XMP_Throw ( "Index not valid.", kXMPErr_BadIndex );
	}

	return toDelete;
}

//-----------------------------------------------------------------------------
// 
// replaceChildAt(...)
// 
// Purpose: Remove child at the passed position and insert the new chunk
// 
//-----------------------------------------------------------------------------

Chunk* Chunk::replaceChildAt( XMP_Uns32 pos, Chunk* child )
{
	Chunk* toDelete = NULL;

	try
	{
		//
		// removed old chunk
		//
		toDelete = mChildren.at(pos);
		mChildren.erase(mChildren.begin() + pos);

		//
		// insert new chunk
		//
		mChildren.insert(mChildren.begin() + pos, child);
		// make this the parent of the new node
		child->mParent = this;
		mChunkMode = CHUNK_NODE;

		// set offset
		child->setOffset( toDelete->getOffset() );

		setChanged();

		// to fix the sizes of this node and parents
		adjustSize( child->getPadSize() - toDelete->getPadSize() );
	}
	catch (...)
	{
		XMP_Throw ( "Index not valid.", kXMPErr_BadIndex );
	}

	return toDelete;
}

//-----------------------------------------------------------------------------
// 
// Chunk::firstChild(...)
// 
// Purpose: iterators
// 
//-----------------------------------------------------------------------------

Chunk::ConstChunkIterator Chunk::firstChild() const
{
	return mChildren.begin();
}


Chunk::ConstChunkIterator Chunk::lastChild() const
{
	return mChildren.end();
}


/******************* Private Methods ***************************/

//-----------------------------------------------------------------------------
// 
// Chunk::setChanged(...)
// 
// Purpose: Sets this node and all of its parents up to the tree root dirty
// 
//-----------------------------------------------------------------------------

void  Chunk::setChanged()
{
	mDirty = true;

	if (mParent != NULL)
	{
		mParent->setChanged();
	}
}


//-----------------------------------------------------------------------------
// 
// Chunk::adjustInternalBuffer(...)
// 
// Purpose: Resizes the internal byte buffer to the given size if the new size 
//			is bigger than the current one.
//			If the new size is smaller, the buffer is not adjusted
// 
//-----------------------------------------------------------------------------

void Chunk::adjustInternalBuffer( XMP_Uns64 newSize )
{
	// only adjust if the new size is bigger than the old one.
	// If it is smaller, leave the buffer alone
	if( newSize > mBufferSize )
	{
		XMP_Uns8 *tmp = new XMP_Uns8[static_cast<size_t>(newSize)]; // Might throw bad_alloc exception
			
		// Do we have an old buffer?
		if( mData != NULL )
		{
			// Copy it to the new one and delete the old one
			memcpy( tmp, mData, static_cast<size_t>(mBufferSize) );
				
			delete [] mData;
		}
		mData = tmp;
		mBufferSize = newSize;
	}
}//adjustInternalBuffer


//-----------------------------------------------------------------------------
// 
// Chunk::adjustSize(...)
// 
// Purpose: Adjusts the chunk size and the parents chunk sizes
// 
//-----------------------------------------------------------------------------

void Chunk::adjustSize( XMP_Int64 sizeChange )
{
	// Calculate leaf sizeChange
	if (mChunkMode == CHUNK_LEAF)
	{
		// Note: The leave nodes size is equal to the buffer size can have odd and even sizes.
		XMP_Uns64 sizeInclPad = mSize + (mSize % 2);
		sizeChange = mBufferSize - sizeInclPad;
		mSize = mBufferSize;

		// if the difference is odd, the corrected even size has be incremented by 1
		sizeChange += abs(sizeChange % 2);
	}
	else // mChunkMode == CHUNK_NODE/CHUNK_UNKNOWN
	{
		// if the difference is odd, the corrected even size has be incremented by 1
		// (or decremented by 1 when < 0).
		sizeChange += sizeChange % 2;

		// the chunk node gets the corrected (odd->even) size
		mSize += sizeChange;
	}


	if (mParent != NULL)
	{
		// adjusts the parents size with the corrected (odd->even) size difference of this node
		mParent->adjustSize(sizeChange);
	}
}//adjustSize
