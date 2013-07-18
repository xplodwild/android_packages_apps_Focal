// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _BEXTMetadata_h_
#define _BEXTMetadata_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

namespace IFF_RIFF
{

/**
 *	BEXT Metadata model.
 *	Implements the IMetadata interface
 */
class BEXTMetadata : public IMetadata
{
public:
	enum
	{
		kDescription,			// std::string
		kOriginator,			// std::string
		kOriginatorReference,	// std::string
		kOriginationDate,		// std::string
		kOriginationTime,		// std::string
		kTimeReference,			// XMP_Uns64
		kVersion,				// XMP_Uns16
		kUMID,					// XMP_Uns8[64]
		kCodingHistory			// std::string
	};

public:
	/**
	 *ctor/dtor
	 */
				BEXTMetadata();
			   ~BEXTMetadata();

	/**
	 * Parses the given memory block and creates a data model representation
	 * The implementation expects that the memory block is the data area of
	 * the BEXT chunk.
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
	 * The memory block will be the data area of a BEXT chunk.
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

	/**
	 * Normalize line feeds to \CR\LF
	 * Mac <OS9: \r
	 * Linux: \n
	 * Win: \r\n
	 */
	static void NormalizeLF( std::string& str );

private:
	// Operators hidden on purpose
	BEXTMetadata( const BEXTMetadata& ) {};
	BEXTMetadata& operator=( const BEXTMetadata& ) { return *this; };
};

}

#endif
