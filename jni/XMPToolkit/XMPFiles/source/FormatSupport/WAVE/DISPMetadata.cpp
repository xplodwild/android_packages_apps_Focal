// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include <string.h>

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/WAVE/DISPMetadata.h"
#include "source/Endian.h"

using namespace IFF_RIFF;

//-----------------------------------------------------------------------------
// 
// DISPMetadata::isValidDISP(...)
// 
// Purpose: [static] Check if the passed data is a valid DISP chunk. Valid in
//					 of is it a DISP chunk that XMP should process.
// 
//-----------------------------------------------------------------------------

bool DISPMetadata::isValidDISP( const XMP_Uns8* chunkData, XMP_Uns64 size )
{
	return ( ( size >= 4 ) && ( LittleEndian::getInstance().getUns32( chunkData ) == 0x0001 ) );
}

//-----------------------------------------------------------------------------
// 
// DISPMetadata::DISPMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

DISPMetadata::DISPMetadata()
{
}

DISPMetadata::~DISPMetadata()
{
}

//-----------------------------------------------------------------------------
// 
// DISPMetadata::parse(...)
// 
// Purpose: Parses the given memory block and creates a data model representation
//			The implementation expects that the memory block is the data area of
//			the DISP chunk.
//			Throws exceptions if parsing is not possible
// 
//-----------------------------------------------------------------------------

void DISPMetadata::parse( const XMP_Uns8* chunkData, XMP_Uns64 size )
{
	if( DISPMetadata::isValidDISP( chunkData, size ) )
	{
		this->setValue<std::string>( kTitle, std::string( (char*)&chunkData[4], static_cast<std::string::size_type>(size-4) ) );
		this->resetChanges();
	}
	else
	{
		XMP_Throw ( "Not a valid DISP chunk", kXMPErr_BadFileFormat );
	}
}

//-----------------------------------------------------------------------------
// 
// DISPMetadata::serialize(...)
// 
// Purpose: Serializes the data model to a memory block. 
//			The memory block will be the data area of a DISP chunk.
//			Throws exceptions if serializing is not possible
// 
//-----------------------------------------------------------------------------

XMP_Uns64 DISPMetadata::serialize( XMP_Uns8** outBuffer )
{
	XMP_Uns64 size = 0;

	if( outBuffer != NULL && this->valueExists( kTitle ) )
	{
		std::string title = this->getValue<std::string>( kTitle );

		size = 4 + title.length();	// at least 4bytes for the type value of the DISP chunk
		
		// [2500563] DISP chunk must be of even length for WAVE, 
		// as pad byte is not interpreted correctly by third-party tools
		if ( size % 2 != 0 )
		{
			size++;
		}
		XMP_Uns8* buffer = new XMP_Uns8[static_cast<size_t>(size)];

		memset( buffer, 0, static_cast<size_t>(size) );

		//
		// DISP type
		//
		buffer[0] = 1;

		//
		// copy string into buffer
		//

		memcpy( &buffer[4], title.c_str(), title.length() );

		*outBuffer = buffer;
	}
	else
	{
		XMP_Throw ( "Invalid buffer", kXMPErr_InternalFailure );
	}

	return size;
}

//-----------------------------------------------------------------------------
// 
// DISPMetadata::isEmptyValue(...)
// 
// Purpose: Is the value of the passed ValueObject and its id "empty"?
// 
//-----------------------------------------------------------------------------

bool DISPMetadata::isEmptyValue( XMP_Uns32 id, ValueObject& valueObj )
{
	TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);

	return ( strObj == NULL || ( strObj != NULL && strObj->getValue().empty() ) );
}
