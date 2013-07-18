// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _DISPMetadata_h_
#define _DISPMetadata_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

namespace IFF_RIFF
{

/**
 *	DISP Metadata model.
 *	Implements the IMetadata interface
 */
class DISPMetadata : public IMetadata
{
public:
	enum
	{
		kTitle			// std::string
	};

public:
	/**
	 * Return true if the type is 0x0001 and it's large enough for 
	 * more content
	 *
	 * @param chunkData		Data area of chunk
	 * @param size			Size of data area
	 *
	 * @return				True if it's a valid DISP chunk
	 */
	static bool		isValidDISP( const XMP_Uns8* chunkData, XMP_Uns64 size );

public:
	/**
	 *ctor/dtor
	 */
						DISPMetadata();
	virtual			   ~DISPMetadata();

	/**
	 * Parses the given memory block and creates a data model representation
	 * The implementation expects that the memory block is the data area of
	 * the DISP chunk.
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
	 * The method creates a buffer and pass it to the parameter 'buffer'. The callee of
	 * the method is responsible to delete the buffer later on.
	 * The memory block will be the data area of a DISP chunk.
	 * Throws exceptions if serializing is not possible
	 *
	 * @param buffer	Buffer that gets filled with serialized data
	 * @param size		Size of passed in buffer
	 *
	 * @ return			Buffer size
	 */
	virtual XMP_Uns64 serialize( XMP_Uns8** buffer );

protected:
	/**
	 * @see IMetadata::isEmptyValue
	 */
	virtual	bool isEmptyValue( XMP_Uns32 id, ValueObject& valueObj );

private:
	// Operators hidden on purpose
	DISPMetadata( const DISPMetadata& ) {};
	DISPMetadata& operator=( const DISPMetadata& ) { return *this; };
};

}

#endif
