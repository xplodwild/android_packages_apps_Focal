// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _ValueObject_h_
#define _ValueObject_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

/**
 * The class ValueObject acts as a base class for the two generic classes
 * TValueObject and TValueArray.
 * The overall purpose of these classes is to store a single value (or array)
 * of any data types and a modification state. The modification state is
 * always set if the existing value has changed. It is not set if a new value
 * is set that is equal to the existing value.
 * If a ValueObject based instance is created or a new value is passed to an 
 * instance the actual value gets copied.
 *
 * So there're some requirements of possible data types:
 *   - data type needs to provide a relational operator
 *   - data type needs to provide an assign operator
 */

/**
 * The class ValueObject is the base class for the following generic classes.
 * It provids common functionality for handle the modification state.
 * This class can't be used directly!
 */
class ValueObject
{
public:
	virtual ~ValueObject()        {}

	inline bool		hasChanged() const				{ return mDirty;  }
	inline void		resetChanged()					{ mDirty = false; }

protected:
	ValueObject() : mDirty(false) {}

protected:
	bool	mDirty;
};

/**
 * The generic class TValueObject is supposed to store values of any data types.
 * It is based on the class ValueObject and support modification state functionality.
 * See above for the requirements of possible data types.
 */
template <class T> class TValueObject : public ValueObject
{
public:
	TValueObject( const T& value ) : mValue(value) {}
	~TValueObject() {}

	inline const T&	getValue() const				{ return mValue; }
	inline void		setValue( const T& value )		{ mDirty = !( mValue == value ); mValue = value; }

private:
	T		mValue;
};

/**
 * The generic class TArrayObject is supposed to store arrays of any data types.
 * It is based on the class ValueObject and supports modification state functionality.
 * See above for the requirements of possible data types.
 */
template <class T> class TArrayObject : public ValueObject
{
public:
	TArrayObject( const T* buffer, XMP_Uns32 bufferSize );
	~TArrayObject();

   inline const T* const getArray( XMP_Uns32& outSize ) const		{ outSize = mSize; return mArray; }
   void setArray( const T* buffer, XMP_Uns32 numElements);

private:
	T*			mArray;
	XMP_Uns32	mSize;
};

template<class T> inline TArrayObject<T>::TArrayObject( const T* buffer, XMP_Uns32 bufferSize ) 
: mArray(NULL), mSize(0)
{
	this->setArray( buffer, bufferSize );
	mDirty = false;
}

template<class T> inline TArrayObject<T>::~TArrayObject()
{
	if( mArray != NULL )
	{
		delete[] mArray;
	}
}

template<class T> inline void TArrayObject<T>::setArray( const T* buffer, XMP_Uns32 numElements )
{
	if( buffer != NULL && numElements > 0 )
	{
		bool doSet = true;

		if( mArray != NULL &&  mSize == numElements )
		{
			doSet = ( memcmp( mArray, buffer, numElements*sizeof(T) ) != 0 );
		}

		if( doSet )
		{
			if( mArray != NULL )
			{
				delete[] mArray;
			}

			mArray = new T[numElements];
			mSize  = numElements;

			memcpy( mArray, buffer, numElements*sizeof(T) );

			mDirty = true;
		}
	}
	else
	{
		mDirty = ( mArray != NULL );

		if( mArray != NULL )
		{
			delete[] mArray;
		}

		mArray = NULL;
		mSize  = 0;
	}
}

#endif
