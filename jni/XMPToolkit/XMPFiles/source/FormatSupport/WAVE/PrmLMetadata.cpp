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

#include "XMPFiles/source/FormatSupport/WAVE/PrmLMetadata.h"
#include "source/Endian.h"

using namespace IFF_RIFF;

static const XMP_Uns32 kPrmlSizeFix		= 282;		// always 282 bytes

static const XMP_Uns32 kSizeFilePath	= 260;

// Needed to be able to memcpy directly to this struct.
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86
	struct PrmlBoxContent 
	{
		XMP_Uns32	mMagic;
		XMP_Uns32	mSize;
		XMP_Uns16	mVerAPI;
		XMP_Uns16	mVerCode;
		XMP_Uns32	mExportType;
		XMP_Uns16	mMacVRefNum;
		XMP_Uns32	mMacParID;
		char		mFilePath[kSizeFilePath];
	};
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

//-----------------------------------------------------------------------------
// 
// PrmLMetadata::PrmLMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

PrmLMetadata::PrmLMetadata()
{
}

PrmLMetadata::~PrmLMetadata()
{
}

//-----------------------------------------------------------------------------
// 
// PrmLMetadata::parse(...)
// 
// Purpose: Parses the given memory block and creates a data model representation
//			The implementation expects that the memory block is the data area of
//			the BEXT chunk and its size is at least as big as the minimum size
//			of a BEXT data block.
//			Throws exceptions if parsing is not possible
// 
//-----------------------------------------------------------------------------

void PrmLMetadata::parse( const XMP_Uns8* chunkData, XMP_Uns64 size )
{
	if( size >= kPrmlSizeFix )
	{
		const LittleEndian& LE = LittleEndian::getInstance();

		PrmlBoxContent prml;
		memset( &prml, 0, kPrmlSizeFix );

		//
		// copy input data into Prml block
		// Safe as fixed size matches size of struct that is #pragma packed(1)
		//
		memcpy( &prml, chunkData, kPrmlSizeFix );

		//
		// copy values to map
		//
		this->setValue<XMP_Uns32>( kMagic,			prml.mMagic );
		this->setValue<XMP_Uns32>( kSize,			prml.mSize );
		this->setValue<XMP_Uns16>( kVerAPI,			prml.mVerAPI );
		this->setValue<XMP_Uns16>( kVerCode,		prml.mVerCode );
		this->setValue<XMP_Uns32>( kExportType,		prml.mExportType );
		this->setValue<XMP_Uns16>( kMacVRefNum,		prml.mMacVRefNum );
		this->setValue<XMP_Uns32>( kMacParID,		prml.mMacParID );
		this->setValue<std::string>( kFilePath,		std::string( prml.mFilePath,	kSizeFilePath ) );

		this->resetChanges();
	}
	else
	{
		XMP_Throw ( "Not a valid Prml chunk", kXMPErr_BadFileFormat );
	}
}

//-----------------------------------------------------------------------------
// 
// PrmLMetadata::serialize(...)
// 
// Purpose: Serializes the data model to a memory block. 
//			The memory block will be the data area of a BEXT chunk.
//			Throws exceptions if serializing is not possible
// 
//-----------------------------------------------------------------------------

XMP_Uns64 PrmLMetadata::serialize( XMP_Uns8** outBuffer )
{
	XMP_Uns64 size = 0;

	if( outBuffer != NULL )
	{
		const LittleEndian& LE = LittleEndian::getInstance();

		size = kPrmlSizeFix;

		//
		// setup buffer
		//
		XMP_Uns8* buffer = new XMP_Uns8[static_cast<size_t>(size)];

		//
		// copy values and strings back to BEXT block
		//
		// ! Safe use of strncpy as the fixed size is consistent with the size of the destination buffer
		// But it is intentional here that the string might not be null terminated if
		// the size of the source is equal to the fixed size of the destination
		//
		PrmlBoxContent prml;
		memset( &prml, 0, kPrmlSizeFix );

		if( this->valueExists( kMagic ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kMagic ), &prml.mMagic );
		}
		if( this->valueExists( kSize ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kSize ), &prml.mSize );
		}
		if( this->valueExists( kVerAPI ) )
		{
			LE.putUns16( this->getValue<XMP_Uns16>( kVerAPI ), &prml.mVerAPI );
		}
		if( this->valueExists( kVerCode ) )
		{
			LE.putUns16( this->getValue<XMP_Uns16>( kVerCode ), &prml.mVerCode );
		}
		if( this->valueExists( kExportType ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kExportType ), &prml.mExportType );
		}
		if( this->valueExists( kMacVRefNum ) )
		{
			LE.putUns16( this->getValue<XMP_Uns16>( kMacVRefNum ), &prml.mMacVRefNum );
		}
		if( this->valueExists( kMacParID ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kMacParID ), &prml.mMacParID );
		}
		if( this->valueExists( kFilePath ) )
		{
			strncpy( prml.mFilePath, this->getValue<std::string>( kFilePath ).c_str(), kSizeFilePath );
		}

		//
		// set input buffer to zero
		//
		memset( buffer, 0, static_cast<size_t>(size) );

		//
		// copy Prml block into buffer
		//
		memcpy( buffer, &prml, kPrmlSizeFix );

		*outBuffer = buffer;
	}
	else
	{
		XMP_Throw ( "Invalid buffer", kXMPErr_BadParam );
	}

	return size;
}

//-----------------------------------------------------------------------------
// 
// PrmLMetadata::isEmptyValue(...)
// 
// Purpose: Is the value of the passed ValueObject and its id "empty"?
// 
//-----------------------------------------------------------------------------

bool PrmLMetadata::isEmptyValue( XMP_Uns32 id, ValueObject& valueObj )
{
	bool ret = true;

	switch( id )
	{
		case kFilePath:
		{
			TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);

			ret = ( strObj == NULL || ( strObj != NULL && strObj->getValue().empty() ) );
		}
		break;

		case kMagic:
		case kSize:
		case kVerAPI:
		case kVerCode:
		case kExportType:
		case kMacVRefNum:
		case kMacParID:
		{
			ret = false;
		}
		break;

		default:
		{
			ret = true;
		}
	}

	return ret;
}
