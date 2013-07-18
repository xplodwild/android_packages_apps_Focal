// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "source/XMP_LibUtils.hpp"

#include <vector>

using namespace IFF_RIFF;

typedef std::vector<ChunkIdentifier>::size_type  ChunkSizeType;

//-----------------------------------------------------------------------------
// 
// ChunkPath::ChunkPath(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

ChunkPath::ChunkPath( const ChunkIdentifier* path /*= NULL*/, XMP_Uns32 size /*=0*/ )
{
	if( path != NULL )
	{
		for( XMP_Uns32 i=0; i<size; i++ )
		{
			this->append( path[i] );
		}
	}
}

ChunkPath::ChunkPath( const ChunkPath& path )
{
	for( XMP_Int32 i=0; i<path.length(); i++ )
	{
		this->append( path.identifier(i) );
	}
}

ChunkPath::ChunkPath( const ChunkIdentifier& identifier )
{
	this->append( identifier );
}

ChunkPath::~ChunkPath()
{
	this->clear();
}


ChunkPath &	ChunkPath::operator=( const ChunkPath &rhs )
{
	for( XMP_Int32 i = 0; i < rhs.length(); i++ )
	{
		this->append( rhs.identifier(i) );
	}
	
	return *this;
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::clear(...)
// 
// Purpose: Remove all ChunkIdentifier's from the path
// 
//-----------------------------------------------------------------------------

void ChunkPath::clear()
{
	mPath.clear();
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::append(...)
// 
// Purpose: Append a ChunkIdentifier to the end of the path
// 
//-----------------------------------------------------------------------------

void ChunkPath::append( XMP_Uns32 id, XMP_Uns32 type /*= kType_NONE*/ )
{
	ChunkIdentifier ci;
	
	ci.id	 = id;
	ci.type = type;
	
	mPath.push_back(ci);
}


void ChunkPath::append( const ChunkIdentifier& identifier )
{
	mPath.push_back(identifier);
}


void ChunkPath::append( const ChunkIdentifier* path, XMP_Uns32 size )
{
	if( path != NULL )
	{
		for( XMP_Uns32 i=0; i < size; i++ )
		{
			this->append( path[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::insert(...)
// 
// Purpose: Insert an identifier
// 
//-----------------------------------------------------------------------------

void ChunkPath::insert( const ChunkIdentifier& identifier, XMP_Uns32 pos /*= 0*/ )
{
	if( pos >= mPath.size() )
	{
		this->append( identifier );
	}
	else
	{
		mPath.insert( mPath.begin() + pos, identifier );
	}
}
	
//-----------------------------------------------------------------------------
// 
// ChunkPath::remove(...)
// 
// Purpose: Remove the endmost ChunkIdentifier from the path
// 
//-----------------------------------------------------------------------------

void ChunkPath::remove()
{
	mPath.pop_back();
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::removeAt(...)
// 
// Purpose: Remove the ChunkIdentifier at the passed position in the path
// 
//-----------------------------------------------------------------------------

void ChunkPath::removeAt( XMP_Int32 pos )
{
	if( ! mPath.empty() && pos >= 0 && (ChunkSizeType)pos < mPath.size() )
	{
		mPath.erase( mPath.begin() + pos );
	}
	else
	{
		XMP_Throw( "Index out of range.", kXMPErr_BadIndex );
	}
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::identifier(...)
// 
// Purpose: Return ChunkIdentifier at the passed position
// 
//-----------------------------------------------------------------------------

const ChunkIdentifier& ChunkPath::identifier( XMP_Int32 pos ) const
{
	return mPath.at(pos);
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::length(...)
// 
// Purpose: Return the number of ChunkIdentifier's in the path
// 
//-----------------------------------------------------------------------------

XMP_Int32 ChunkPath::length() const
{
	return (XMP_Int32)mPath.size();
}

//-----------------------------------------------------------------------------
// 
// ChunkPath::match(...)
// 
// Purpose: Compare the passed ChunkPath with this path.
// 
//-----------------------------------------------------------------------------

ChunkPath::MatchResult ChunkPath::match( const ChunkPath& path ) const
{
	MatchResult ret			= kNoMatch;

	if( path.length() > 0 )
	{
		XMP_Int32 depth			= ( this->length() > path.length() ? path.length() : this->length() );
		XMP_Int32 matchCount	= 0;

		for( XMP_Int32 i=0; i<depth; i++ )
		{
			const ChunkIdentifier& id1 = this->identifier(i);
			const ChunkIdentifier& id2 = path.identifier(i);

			if( id1.id == id2.id )
			{
				if( i == this->length() - 1 && id1.type == kType_NONE )
				{
					matchCount++;
				}
				else if( id1.type == id2.type )
				{
					matchCount++;
				}
			}
			else
				break;
		}

		if( matchCount == depth )
		{
			ret = ( path.length() >= this->length() ? kFullMatch : kPartMatch );
		}
	}

	return ret;
}
