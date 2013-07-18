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

#include "XMPFiles/source/FormatSupport/WAVE/Cr8rMetadata.h"
#include "source/Endian.h"

using namespace IFF_RIFF;

static const XMP_Uns32 kCr8rSizeFix		= 84;		// always 84 bytes

static const XMP_Uns32 kSizeFileExt		= 16;
static const XMP_Uns32 kSizeAppOtions	= 16;
static const XMP_Uns32 kSizeAppName		= 32;

// Needed to be able to memcpy directly to this struct.
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86
	struct Cr8rBoxContent 
	{
		XMP_Uns32	mMagic;
		XMP_Uns32	mSize;
		XMP_Uns16	mMajorVer;
		XMP_Uns16	mMinorVer;
		XMP_Uns32	mCreatorCode;
		XMP_Uns32	mAppleEvent;
		char		mFileExt[kSizeFileExt];
		char		mAppOptions[kSizeAppOtions];
		char		mAppName[kSizeAppName];
	};
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

//-----------------------------------------------------------------------------
// 
// Cr8rMetadata::Cr8rMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

Cr8rMetadata::Cr8rMetadata()
{
}

Cr8rMetadata::~Cr8rMetadata()
{
}

//-----------------------------------------------------------------------------
// 
// Cr8rMetadata::parse(...)
// 
// Purpose: Parses the given memory block and creates a data model representation
//			The implementation expects that the memory block is the data area of
//			the BEXT chunk and its size is at least as big as the minimum size
//			of a BEXT data block.
//			Throws exceptions if parsing is not possible
// 
//-----------------------------------------------------------------------------

void Cr8rMetadata::parse( const XMP_Uns8* chunkData, XMP_Uns64 size )
{
	if( size >= kCr8rSizeFix )
	{
		const LittleEndian& LE = LittleEndian::getInstance();

		Cr8rBoxContent cr8r;
		memset( &cr8r, 0, kCr8rSizeFix );

		//
		// copy input data into Cr8r block
		// Safe as fixed size matches size of struct that is #pragma packed(1)
		//
		memcpy( &cr8r, chunkData, kCr8rSizeFix );

		//
		// copy values to map
		//
		this->setValue<XMP_Uns32>( kMagic,			cr8r.mMagic );
		this->setValue<XMP_Uns32>( kSize,			cr8r.mSize );
		this->setValue<XMP_Uns16>( kMajorVer,		cr8r.mMajorVer );
		this->setValue<XMP_Uns16>( kMinorVer,		cr8r.mMinorVer );
		this->setValue<XMP_Uns32>( kCreatorCode,	cr8r.mCreatorCode );
		this->setValue<XMP_Uns32>( kAppleEvent,		cr8r.mAppleEvent );
		this->setValue<std::string>( kFileExt,		std::string( cr8r.mFileExt,		kSizeFileExt ) );
		this->setValue<std::string>( kAppOptions,	std::string( cr8r.mAppOptions,	kSizeAppOtions ) );
		this->setValue<std::string>( kAppName,		std::string( cr8r.mAppName,		kSizeAppName ) );

		this->resetChanges();
	}
	else
	{
		XMP_Throw ( "Not a valid Cr8r chunk", kXMPErr_BadFileFormat );
	}
}

//-----------------------------------------------------------------------------
// 
// Cr8rMetadata::serialize(...)
// 
// Purpose: Serializes the data model to a memory block. 
//			The memory block will be the data area of a BEXT chunk.
//			Throws exceptions if serializing is not possible
// 
//-----------------------------------------------------------------------------

XMP_Uns64 Cr8rMetadata::serialize( XMP_Uns8** outBuffer )
{
	XMP_Uns64 size = 0;

	if( outBuffer != NULL )
	{
		const LittleEndian& LE = LittleEndian::getInstance();

		size = kCr8rSizeFix;

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
		Cr8rBoxContent cr8r;
		memset( &cr8r, 0, kCr8rSizeFix );

		if( this->valueExists( kMagic ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kMagic ), &cr8r.mMagic );
		}
		if( this->valueExists( kSize ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kSize ), &cr8r.mSize );
		}
		if( this->valueExists( kMajorVer ) )
		{
			LE.putUns16( this->getValue<XMP_Uns16>( kMajorVer ), &cr8r.mMajorVer );
		}
		if( this->valueExists( kMinorVer ) )
		{
			LE.putUns16( this->getValue<XMP_Uns16>( kMinorVer ), &cr8r.mMinorVer );
		}
		if( this->valueExists( kCreatorCode ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kCreatorCode ), &cr8r.mCreatorCode );
		}
		if( this->valueExists( kAppleEvent ) )
		{
			LE.putUns32( this->getValue<XMP_Uns32>( kAppleEvent ), &cr8r.mAppleEvent );
		}
		if( this->valueExists( kFileExt ) )
		{
			strncpy( cr8r.mFileExt, this->getValue<std::string>( kFileExt ).c_str(), kSizeFileExt );
		}
		if( this->valueExists( kAppOptions ) )
		{
			strncpy( cr8r.mAppOptions, this->getValue<std::string>( kAppOptions ).c_str(), kSizeAppOtions );
		}
		if( this->valueExists( kAppName ) )
		{
			strncpy( cr8r.mAppName, this->getValue<std::string>( kAppName ).c_str(), kSizeAppName );
		}

		//
		// set input buffer to zero
		//
		memset( buffer, 0, static_cast<size_t>(size) );

		//
		// copy Cr8r block into buffer
		//
		memcpy( buffer, &cr8r, kCr8rSizeFix );

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
// Cr8rMetadata::isEmptyValue(...)
// 
// Purpose: Is the value of the passed ValueObject and its id "empty"?
// 
//-----------------------------------------------------------------------------

bool Cr8rMetadata::isEmptyValue( XMP_Uns32 id, ValueObject& valueObj )
{
	bool ret = true;

	switch( id )
	{
		case kFileExt:
		case kAppOptions:
		case kAppName:
		{
			TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);

			ret = ( strObj == NULL || ( strObj != NULL && strObj->getValue().empty() ) );
		}
		break;

		case kMagic:
		case kSize:
		case kMajorVer:
		case kMinorVer:
		case kCreatorCode:
		case kAppleEvent:
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
