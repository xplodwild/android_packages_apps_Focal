/**************************************************************************
*
* ADOBE SYSTEMS INCORPORATED
* Copyright 2010 Adobe Systems Incorporated
* All Rights Reserved
*
* NOTICE: Adobe permits you to use, modify, and distribute this file in 
* accordance with the terms of the Adobe license agreement accompanying it.
*
**************************************************************************/

#ifndef _Endian_h_
#define _Endian_h_ 1

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"
#include "source/EndianUtils.hpp"

class IEndian
{
public:
	virtual ~IEndian() {};

	virtual XMP_Uns16	getUns16	( const void* value ) const	= 0;
	virtual XMP_Uns32	getUns32	( const void* value ) const	= 0;
	virtual XMP_Uns64	getUns64	( const void* value ) const	= 0;
	virtual float		getFloat	( const void* value ) const	= 0;
	virtual double		getDouble	( const void* value ) const	= 0;

	virtual void	putUns16	( XMP_Uns16 value, void* dest ) const	= 0;
	virtual void	putUns32	( XMP_Uns32 value, void* dest ) const	= 0;
	virtual void	putUns64	( XMP_Uns64 value, void* dest ) const	= 0;
	virtual void	putFloat	( float value, void* dest ) const	= 0;
	virtual void	putDouble	( double value, void* dest ) const	= 0;
};


class LittleEndian : public IEndian
{
private:
	// Private Constructors / operators to prevent direkt creation
	LittleEndian() {};
	LittleEndian( const LittleEndian& ) {};
	LittleEndian& operator=( const LittleEndian& ) { return *this; };

public:
	// Singleton Factory
	static const LittleEndian& getInstance()
	{
		// Singleton instance (on Stack)
		static LittleEndian instance;

		return instance;
	}

	virtual XMP_Uns16	getUns16	( const void* value ) const		{ return GetUns16LE( value ); }
	virtual XMP_Uns32	getUns32	( const void* value ) const		{ return GetUns32LE( value ); }
	virtual XMP_Uns64	getUns64	( const void* value ) const		{ return GetUns64LE( value ); }
	virtual float		getFloat	( const void* value ) const		{ return GetFloatLE( value ); }
	virtual double		getDouble	( const void* value ) const		{ return GetDoubleLE( value ); }

	virtual void	putUns16	( XMP_Uns16 value, void* dest ) const	{ PutUns16LE( value, dest ); }
	virtual void	putUns32	( XMP_Uns32 value, void* dest ) const	{ PutUns32LE( value, dest ); }
	virtual void	putUns64	( XMP_Uns64 value, void* dest ) const	{ PutUns64LE( value, dest ); }
	virtual void	putFloat	( float value, void* dest ) const		{ PutFloatLE( value, dest ); }
	virtual void	putDouble	( double value, void* dest ) const		{ PutDoubleLE( value, dest ); }
}; // LittleEndian


class BigEndian : public IEndian
{
private:
	// Private Constructors / operators to prevent direkt creation
	BigEndian() {};
	BigEndian( const BigEndian& ) {};
	BigEndian& operator=( const BigEndian& ) { return *this; };

public:
	// Singleton Factory
	static const BigEndian& getInstance()
	{
		// Singleton instance (on Stack)
		static BigEndian instance;

		return instance;
	}

	virtual XMP_Uns16	getUns16	( const void* value ) const		{ return GetUns16BE( value ); }
	virtual XMP_Uns32	getUns32	( const void* value ) const		{ return GetUns32BE( value ); }
	virtual XMP_Uns64	getUns64	( const void* value ) const		{ return GetUns64BE( value ); }
	virtual float		getFloat	( const void* value ) const		{ return GetFloatBE( value ); }
	virtual double		getDouble	( const void* value ) const		{ return GetDoubleBE( value ); }

	virtual void	putUns16	( XMP_Uns16 value, void* dest ) const	{ PutUns16BE( value, dest ); }
	virtual void	putUns32	( XMP_Uns32 value, void* dest ) const	{ PutUns32BE( value, dest ); }
	virtual void	putUns64	( XMP_Uns64 value, void* dest ) const	{ PutUns64BE( value, dest ); }
	virtual void	putFloat	( float value, void* dest ) const		{ PutFloatBE( value, dest ); }
	virtual void	putDouble	( double value, void* dest ) const		{ PutDoubleBE( value, dest ); }
}; // BigEndian

#endif
