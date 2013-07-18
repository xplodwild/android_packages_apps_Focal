// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _Cr8rMetadata_h_
#define _Cr8rMetadata_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

namespace IFF_RIFF
{

/**
 *	Cr8r Metadata model.
 *	Implements the IMetadata interface
 */
class Cr8rMetadata : public IMetadata
{
public:
	enum
	{
		kMagic,				// XMP_Uns32 
		kSize,				// XMP_Uns32 
		kMajorVer,			// XMP_Uns16 
		kMinorVer,			// XMP_Uns16 
		kCreatorCode,		// XMP_Uns32
		kAppleEvent,		// XMP_Uns32
		kFileExt,			// char[16]
		kAppOptions,		// char[16]
		kAppName			// char[32]
	};

public:
	/**
	 *ctor/dtor
	 */
				Cr8rMetadata();
			   ~Cr8rMetadata();

	/**
	 * Parses the given memory block and creates a data model representation
	 * The implementation expects that the memory block is the data area of
	 * the Cr8rMetadata chunk.
	 * Throws exceptions if parsing is not possible
	 *
	 * @param input		The byte buffer to parse
	 * @param size		Size of the given byte buffer
	 */
	void		parse( const XMP_Uns8* chunkData, XMP_Uns64 size );
	
	/**
	 * See IMetadata::parse( const LFA_FileRef input )
	 */
	void		parse( XMP_IO* input ) { IMetadata::parse( input ); }
	
	/**
	 * Serializes the data model to a memory block. 
	 * The memory block will be the data area of a Cr8rMetadata chunk.
	 * Throws exceptions if serializing is not possible
	 *
	 * @param buffer	Buffer that gets filled with serialized data
	 * @param size		Size of passed in buffer
	 *
	 * @return			Size of serialzed data (might be smaller than buffer size)
	 */
	XMP_Uns64		serialize( XMP_Uns8** buffer );

protected:
	/**
	 * @see IMetadata::isEmptyValue
	 */
	virtual	bool isEmptyValue( XMP_Uns32 id, ValueObject& valueObj );

private:
	// Operators hidden on purpose
	Cr8rMetadata( const Cr8rMetadata& ) {};
	Cr8rMetadata& operator=( const Cr8rMetadata& ) { return *this; };
};

}

#endif
