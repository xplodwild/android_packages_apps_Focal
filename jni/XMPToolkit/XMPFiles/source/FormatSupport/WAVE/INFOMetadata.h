// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _INFOMetadata_h_
#define _INFOMetadata_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

namespace IFF_RIFF
{

/**
 *	LIST INFO Metadata model.
 *	Implements the IMetadata interface
 */
class INFOMetadata : public IMetadata
{
public:
	enum
	{
		kArtist				= 0x49415254,	// 'IART'		std::string
		kComments			= 0x49434d54,	// 'ICMT'		std::string
		kCopyright			= 0x49434f50,	// 'ICOP'		std::string
		kCreationDate		= 0x49435244,	// 'ICRD'		std::string
		kEngineer			= 0x49454e47,	// 'IENG'		std::string
		kGenre				= 0x49474e52,	// 'IGNR'		std::string
		kName				= 0x494e414d,	// 'INAM'		std::string
		kSoftware			= 0x49534654,	// 'ISFT'		std::string
		kMedium				= 0x494d4544,	// 'IMED'		std::string
		kSourceForm			= 0x49535246,	// 'ISRF'		std::string

		// new mappings
		kArchivalLocation	= 0x4941524C,	// 'IARL'
		kCommissioned		= 0x49434D53,	// 'ICMS'
		kKeywords			= 0x494B4559,	// 'IKEY'
		kProduct			= 0x49505244,	// 'IPRD'
		kSubject			= 0x4953424A,	// 'ISBJ'
		kSource				= 0x49535243,	// 'ISRC'
		kTechnican			= 0x49544348,	// 'ITCH'
	};

public:
	/**
	 *ctor/dtor
	 */
						INFOMetadata();
					   ~INFOMetadata();

	/**
	 * @see IMetadata::parse
	 */
	void				parse( const XMP_Uns8* input, XMP_Uns64 size );

	/**
	 * See IMetadata::parse( const LFA_FileRef input )
	 */
	void				parse( XMP_IO* input ) { IMetadata::parse( input ); }
	
	/**
	 * @see IMetadata::serialize
	 */
	XMP_Uns64			serialize( XMP_Uns8** outBuffer );

protected:
	/**
	 * @see IMetadata::isEmptyValue
	 */
	virtual	bool		isEmptyValue( XMP_Uns32 id, ValueObject& valueObj );

private:
	// Operators hidden on purpose
	INFOMetadata( const INFOMetadata& ) {};
	INFOMetadata& operator=( const INFOMetadata& ) { return *this; };
};

}

#endif
