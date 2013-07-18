// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _IMetadata_h_
#define _IMetadata_h_

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"
#include "source/XMP_LibUtils.hpp"
#include "XMPFiles/source/NativeMetadataSupport/ValueObject.h"

#include <map>

/**
 * The class IMetadata is supposed to store any unique metadata.
 * It provides a generic interface to store any data types or arrays of any data types.
 * The requirements for used datatypes are defined by the class ValueObject and its derived classes.
 * For each single value as well as for the container as a whole modification and existing state is provided.
 * It also provids methods to parse a byte block to distinct values or to serialize values to a byte block.
 * IMetadata is an abstract class due to the method isEmptyValue(...). Derived classes needs to implement this 
 * method. It has to define when a certain value is "empty", since "empty" values are not stored but removed
 * from the container.
 * 
 */
class IMetadata
{
public:
	/** 
	 * ctor/dtor 
	 */
	IMetadata();
	virtual ~IMetadata();


	/**
	 * Parses the given memory block and creates a data model representation
	 * We assume that any metadata block is < 4GB so that we can savely use 32bit sizes.
	 * Throws exceptions if parsing is not possible
	 *
	 * @param input		The byte buffer to parse
	 * @param size		Size of the given byte buffer
	 */
	virtual void parse( const XMP_Uns8* input, XMP_Uns64 size );


	/**
	 * Parses the given file and creates a data model representation
	 * Throws exceptions if parsing is not possible
	 *
	 * @param input		The file to parse
	 */
	virtual void parse( XMP_IO* input );
	

	/**
	 * Serializes the data model to a memory block. 
	 * The method creates a buffer and pass it to the parameter 'buffer'. The callee of
	 * the method is responsible to delete the buffer later on.
	 * We assume that any metadata block is < 4GB so that we can savely use 32bit sizes.
	 * Throws exceptions if serializing is not possible
	 *
	 * @param buffer	Buffer that gets filled with serialized data
	 * @param size		Size of passed in buffer
	 *
	 * @ return			Buffer size
	 */
	virtual XMP_Uns64 serialize( XMP_Uns8** buffer );

	/**
	 * Return true if any values of this container was modified
	 */
	virtual bool hasChanged() const;

	/**
	 * Reset dirty flag
	 */
	virtual void resetChanges();

	/**
	 * Return true if the no metadata are available in this container
	 */
	virtual bool isEmpty() const;

	/**
	 * Set value for passed identifier
	 *
	 * @param id	Identifier of value
	 * @param value New value of passed data type
	 */
	template<class T> void	setValue( XMP_Uns32 id, const T& value );
	
	/**
	 * Set array for passed identifier
	 *
	 * @param id			Identifier of value
	 * @param value			Pointer to the array
	 * @param bufferSize	Number of array elements
	 */
	template<class T> void setArray( XMP_Uns32 id, const T* buffer, XMP_Uns32 numElements);

	/**
	 * Return value for passed identifier.
	 * If the value doesn't exists an exception is thrown!
	 *
	 * @param id	Identifier of value
	 * @return		Value of passed data type
	 */
	template<class T> const T&	getValue( XMP_Uns32 id ) const;
	
	/**
	 * Return array for passed identifier
	 * If the array doesn't exists an exception is thrown!
	 *
	 * @param id			Identifier of value
	 * @param outBuffer		Array
	 * @return				Number of array elements
	 */
	template<class T> const T* const getArray( XMP_Uns32 id, XMP_Uns32& outSize ) const;

	/**
	 * Remove value for passed identifier
	 *
	 * @param id	Identifier of value
	 */
	virtual void deleteValue( XMP_Uns32 id );

	/**
	 * Remove all stored values
	 */
	virtual void deleteAll();

	/**
	 * Return true if an value for the passed identifier exists
	 *
	 * @param id	Identifier of value
	 */
	virtual bool valueExists( XMP_Uns32 id ) const;

	/**
	 * Return true if the value for the passed identifier was changed
	 *
	 * @param id	Identifier of value
	 */
	virtual bool valueChanged( XMP_Uns32 id ) const;

protected:
	/**
	 * Is the value of the passed ValueObject that belongs to the given id "empty"?
	 * Derived classes are required to implement this method and define "empty"
	 * for its values.
	 * Needed for setValue and setArray.
	 *
	 * @param id		Identifier of passed value
	 * @param valueObj	Value to inspect
	 *
	 * @return			True if the value is "empty"
	 */
	virtual	bool isEmptyValue( XMP_Uns32 id, ValueObject& valueObj ) = 0;

private:
	// Operators hidden on purpose
	IMetadata( const IMetadata& ) {};
	IMetadata& operator=( const IMetadata& ) { return *this; };

protected:
	typedef std::map<XMP_Uns32, ValueObject*> ValueMap;
	ValueMap		mValues;
	bool			mDirty;
};

template<class T> void IMetadata::setValue( XMP_Uns32 id, const T& value )
{
	TValueObject<T>* valueObj = NULL;

	//
	// find existing value for id
	//
	ValueMap::iterator iterator = mValues.find( id );

	if( iterator != mValues.end() )
	{
		//
		// value exists, set new value
		//
		valueObj = dynamic_cast<TValueObject<T>*>( iterator->second );

		if( valueObj != NULL )
		{
			valueObj->setValue( value );
		}
		else
		{
			XMP_Throw ( "Invalid identifier", kXMPErr_InternalFailure );
		}
	}
	else
	{
		//
		// value doesn't exists yet and is not "empty"
		// so add a new value to the map
		//
		valueObj = new TValueObject<T>( value );
		mValues[id] = valueObj;

		mDirty = true;	// the new created value isn't dirty, but the container becomes dirty
	}

	//
	// check if the value is "empty"
	//
	if( this->isEmptyValue( id, *valueObj ) )
	{
		//
		// value is "empty", delete it
		//
		this->deleteValue( id );
	}
}

template<class T> void IMetadata::setArray( XMP_Uns32 id, const T* buffer, XMP_Uns32 numElements )
{
	TArrayObject<T>* arrayObj = NULL;

	//
	// find existing value for id
	//
	ValueMap::iterator iterator = mValues.find( id );

	if( iterator != mValues.end() )
	{
		//
		// value exists, set new value
		//
		arrayObj = dynamic_cast<TArrayObject<T>*>( iterator->second );

		if( arrayObj != NULL )
		{
			arrayObj->setArray( buffer, numElements );
		}
		else
		{
			XMP_Throw ( "Invalid identifier", kXMPErr_InternalFailure );
		}
	}
	else
	{
		//
		// value doesn't exists yet and is not "empty"
		// so add a new value to the map
		//
		arrayObj = new TArrayObject<T>( buffer, numElements );
		mValues[id] = arrayObj;

		mDirty = true;	// the new created value isn't dirty, but the container becomes dirty
	}

	//
	// check if the value is "empty"
	//
	if( this->isEmptyValue( id, *arrayObj ) )
	{
		//
		// value is "empty", delete it
		//
		this->deleteValue( id );
	}
}

template<class T> const T& IMetadata::getValue( XMP_Uns32 id ) const
{
	ValueMap::const_iterator iterator = mValues.find( id );

	if( iterator != mValues.end() )
	{
		//
		// values exists, return value string
		//
		TValueObject<T>* valueObj = dynamic_cast<TValueObject<T>*>( iterator->second );

		if( valueObj != NULL )
		{
			return valueObj->getValue();
		}
		else
		{
			XMP_Throw ( "Invalid identifier", kXMPErr_InternalFailure );
		}
	}
	else
	{
		//
		// value for id doesn't exists, throw an exception
		//
		XMP_Throw ( "Invalid identifier", kXMPErr_InternalFailure );
	}
}

template<class T> const T* const IMetadata::getArray( XMP_Uns32 id, XMP_Uns32& outSize ) const
{
	ValueMap::const_iterator iterator = mValues.find( id );

	if( iterator != mValues.end() )
	{
		//
		// values exists, return value string
		//
		TArrayObject<T>* arrayObj = dynamic_cast<TArrayObject<T>*>( iterator->second );

		if( arrayObj != NULL )
		{
			return arrayObj->getArray( outSize );
		}
		else
		{
			XMP_Throw ( "Invalid identifier", kXMPErr_InternalFailure );
		}
	}
	else
	{
		//
		// value for id doesn't exists, throw an exception
		//
		XMP_Throw ( "Invalid identifier", kXMPErr_InternalFailure );
	}
}

#endif
