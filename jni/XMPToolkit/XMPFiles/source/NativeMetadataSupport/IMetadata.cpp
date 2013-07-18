// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

//-----------------------------------------------------------------------------
// 
// IMetadata::IMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

IMetadata::IMetadata() 
: mDirty(false) 
{
}

IMetadata::~IMetadata() 
{
	for( ValueMap::iterator iter = mValues.begin(); iter != mValues.end(); iter++ )
	{
		delete iter->second;
	}
}

//-----------------------------------------------------------------------------
// 
// IMetadata::parse(...)
// 
// Purpose: Parses the given memory block and creates a data model representation
//			We assume that any metadata block is < 4GB so that we can savely use 
//			32bit sizes.
//			Throws exceptions if parsing is not possible
// 
//-----------------------------------------------------------------------------

void IMetadata::parse( const XMP_Uns8* input, XMP_Uns64 size )
{
	XMP_Throw ( "Method not implemented", kXMPErr_Unimplemented );
}

//-----------------------------------------------------------------------------
// 
// IMetadata::parse(...)
// 
// Purpose: Parses the given file and creates a data model representation
//			Throws exceptions if parsing is not possible
// 
//-----------------------------------------------------------------------------

void IMetadata::parse( XMP_IO* input )
{
	XMP_Throw ( "Method not implemented", kXMPErr_Unimplemented );
}

//-----------------------------------------------------------------------------
// 
// IMetadata::serialize(...)
// 
// Purpose: Serializes the data model to a memory block. 
//			The method creates a buffer and pass it to the parameter 'buffer'. 
//			The callee of the method is responsible to delete the buffer later on.
//			We assume that any metadata block is < 4GB so that we can savely use 
//			32bit sizes.
//			Throws exceptions if serializing is not possible
// 
//-----------------------------------------------------------------------------

XMP_Uns64 IMetadata::serialize( XMP_Uns8** buffer )
{
	XMP_Throw ( "Method not implemented", kXMPErr_Unimplemented );
}

//-----------------------------------------------------------------------------
// 
// IMetadata::hasChanged(...)
// 
// Purpose: Return true if any values of this container was modified
// 
//-----------------------------------------------------------------------------

bool IMetadata::hasChanged() const
{
	bool isDirty = mDirty;

	for( ValueMap::const_iterator iter=mValues.begin(); ! isDirty && iter != mValues.end(); iter++ )
	{
		isDirty = iter->second->hasChanged();
	}

	return isDirty;
}

//-----------------------------------------------------------------------------
// 
// IMetadata::resetChanges(...)
// 
// Purpose: Reset dirty flag
// 
//-----------------------------------------------------------------------------

void IMetadata::resetChanges()
{
	mDirty = false;

	for( ValueMap::iterator iter=mValues.begin(); iter != mValues.end(); iter++ )
	{
		iter->second->resetChanged();
	}
}

//-----------------------------------------------------------------------------
// 
// IMetadata::isEmpty(...)
// 
// Purpose: Return true if the no metadata are available in this container
// 
//-----------------------------------------------------------------------------

bool IMetadata::isEmpty() const
{
	return mValues.empty();
}

//-----------------------------------------------------------------------------
// 
// IMetadata::deleteValue(...)
// 
// Purpose: Remove value for passed identifier
// 
//-----------------------------------------------------------------------------

void IMetadata::deleteValue( XMP_Uns32 id )
{
	ValueMap::iterator iterator = mValues.find( id );

	if( iterator != mValues.end() )
	{
		delete iterator->second;
		mValues.erase( iterator );

		mDirty = true;
	}
}

//-----------------------------------------------------------------------------
// 
// IMetadata::deleteAll(...)
// 
// Purpose: Remove all stored values
// 
//-----------------------------------------------------------------------------

void IMetadata::deleteAll()
{
	mDirty = ( mValues.size() > 0 );

	for( ValueMap::iterator iter = mValues.begin(); iter != mValues.end(); iter++ )
	{
		delete iter->second;
	}

	mValues.clear();
}

//-----------------------------------------------------------------------------
// 
// IMetadata::valueExists(...)
// 
// Purpose: Return true if an value for the passed identifier exists
// 
//-----------------------------------------------------------------------------

bool IMetadata::valueExists( XMP_Uns32 id ) const
{
	ValueMap::const_iterator iterator = mValues.find( id );

	return ( iterator != mValues.end() );
}

//-----------------------------------------------------------------------------
// 
// IMetadata::valueChanged(...)
// 
// Purpose: Return true if the value for the passed identifier was changed
// 
//-----------------------------------------------------------------------------

bool IMetadata::valueChanged( XMP_Uns32 id ) const
{
	ValueMap::const_iterator iterator = mValues.find( id );

	if( iterator != mValues.end() )
	{
		return iterator->second->hasChanged();
	}

	return false;
}
