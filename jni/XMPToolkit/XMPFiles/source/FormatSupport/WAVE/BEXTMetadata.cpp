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

#include "XMPFiles/source/FormatSupport/WAVE/BEXTMetadata.h"
#include "source/Endian.h"

using namespace IFF_RIFF;

static const XMP_Uns32 kBEXTSizeMin					= 602;							// at minimum 602 bytes

static const XMP_Uns32 kSizeDescription				= 256;
static const XMP_Uns32 kSizeOriginator				= 32;
static const XMP_Uns32 kSizeOriginatorReference		= 32;
static const XMP_Uns32 kSizeOriginationDate			= 10;
static const XMP_Uns32 kSizeOriginationTime			= 8;

// Needed to be able to memcpy directly to this struct.
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86
	struct BEXT
	{
		char			mDescription[256];
		char			mOriginator[32];
		char			mOriginatorReference[32];
		char			mOriginationDate[10];
		char			mOriginationTime[8];
		XMP_Uns32		mTimeReferenceLow;
		XMP_Uns32		mTimeReferenceHigh;
		XMP_Uns16		mVersion;
		XMP_Uns8		mUMID[64];
		XMP_Uns8		mReserved[190];
	};
#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

//-----------------------------------------------------------------------------
// 
// [static] convertLF(...)
// 
// Purpose: Convert Mac/Unix line feeds to CR/LF
// 
//-----------------------------------------------------------------------------

void BEXTMetadata::NormalizeLF( std::string& str )
{
	XMP_Uns32 i = 0;
	while( i < str.length() )
	{
		char ch = str[i];

		if( ch == 0x0d )
		{
			//
			// possible Mac lf
			//
			if( i+1 < str.length() )
			{
				if( str[i+1] != 0x0a )
				{
					//
					// insert missing LF character
					//
					str.insert( i+1, 1, 0x0a );
				}

				i += 2;
			}
			else
			{
				str.push_back( 0x0a );
			}
		}
		else if( ch == 0x0a )
		{
			//
			// possible Unix LF
			//
			if( i == 0 || str[i-1] != 0x0d )
			{
				//
				// insert missing CR character
				//
				str.insert( i, 1, 0x0d );
				i += 2;
			}
			else
			{
				i++;
			}
		}
		else
		{
			i++;
		}
	}
}

//-----------------------------------------------------------------------------
// 
// BEXTMetadata::BEXTMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

BEXTMetadata::BEXTMetadata()
{
}

BEXTMetadata::~BEXTMetadata()
{
}

//-----------------------------------------------------------------------------
// 
// BEXTMetadata::parse(...)
// 
// Purpose: Parses the given memory block and creates a data model representation
//			The implementation expects that the memory block is the data area of
//			the BEXT chunk and its size is at least as big as the minimum size
//			of a BEXT data block.
//			Throws exceptions if parsing is not possible
// 
//-----------------------------------------------------------------------------

void BEXTMetadata::parse( const XMP_Uns8* chunkData, XMP_Uns64 size )
{
	if( size >= kBEXTSizeMin )
	{
		const LittleEndian& LE = LittleEndian::getInstance();

		BEXT bext;
		memset( &bext, 0, kBEXTSizeMin );

		//
		// copy input data into BEXT block (except CodingHistory field)
		// Safe as fixed size matches size of struct that is #pragma packed(1)
		//
		memcpy( &bext, chunkData, kBEXTSizeMin );

		//
		// copy CodingHistory
		//
		if( size > kBEXTSizeMin )
		{
			this->setValue<std::string>( kCodingHistory, std::string( reinterpret_cast<const char*>(&chunkData[kBEXTSizeMin]), static_cast<std::string::size_type>(size - kBEXTSizeMin) ) );
		}

		//
		// copy values to map
		//
		this->setValue<std::string>( kDescription,			std::string( bext.mDescription, kSizeDescription ) );
		this->setValue<std::string>( kOriginator,			std::string( bext.mOriginator, kSizeOriginator ) );
		this->setValue<std::string>( kOriginatorReference,	std::string( bext.mOriginatorReference, kSizeOriginatorReference ) );
		this->setValue<std::string>( kOriginationDate,		std::string( bext.mOriginationDate, kSizeOriginationDate ) );
		this->setValue<std::string>( kOriginationTime,		std::string( bext.mOriginationTime, kSizeOriginationTime ) );

		this->setValue<XMP_Uns64>( kTimeReference,			LE.getUns64( &bext.mTimeReferenceLow ) );
		this->setValue<XMP_Uns16>( kVersion,				LE.getUns16( &bext.mVersion ) );

		this->setArray<XMP_Uns8>( kUMID,					bext.mUMID, 64 );

		this->resetChanges();
	}
	else
	{
		XMP_Throw ( "Not a valid BEXT chunk", kXMPErr_BadFileFormat );
	}
}

//-----------------------------------------------------------------------------
// 
// BEXTMetadata::serialize(...)
// 
// Purpose: Serializes the data model to a memory block. 
//			The memory block will be the data area of a BEXT chunk.
//			Throws exceptions if serializing is not possible
// 
//-----------------------------------------------------------------------------

XMP_Uns64 BEXTMetadata::serialize( XMP_Uns8** outBuffer )
{
	XMP_Uns64 size = 0;

	if( outBuffer != NULL )
	{
		const LittleEndian& LE = LittleEndian::getInstance();

		size = kBEXTSizeMin;

		std::string codingHistory;

		if( this->valueExists( kCodingHistory ) )
		{
			codingHistory = this->getValue<std::string>( kCodingHistory );
			NormalizeLF( codingHistory );

			size += codingHistory.length();
		}

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
		BEXT bext;
		memset( &bext, 0, kBEXTSizeMin );

		if( this->valueExists( kDescription ) )
		{
			strncpy( bext.mDescription, this->getValue<std::string>( kDescription ).c_str(), kSizeDescription );
		}
		if( this->valueExists( kOriginator ) )
		{
			strncpy( bext.mOriginator, this->getValue<std::string>( kOriginator ).c_str(), kSizeOriginator );
		}
		if( this->valueExists( kOriginatorReference ) )
		{
			strncpy( bext.mOriginatorReference, this->getValue<std::string>( kOriginatorReference ).c_str(), kSizeOriginatorReference );
		}
		if( this->valueExists( kOriginationDate ) )
		{
			strncpy( bext.mOriginationDate, this->getValue<std::string>( kOriginationDate ).c_str(), kSizeOriginationDate );
		}
		if( this->valueExists( kOriginationTime ) )
		{
			strncpy( bext.mOriginationTime, this->getValue<std::string>( kOriginationTime ).c_str(), kSizeOriginationTime );
		}

		if( this->valueExists( kTimeReference ) )
		{
			LE.putUns64( this->getValue<XMP_Uns64>( kTimeReference ), &bext.mTimeReferenceLow );
		}

		if( this->valueExists( kVersion ) )
		{
			LE.putUns16( this->getValue<XMP_Uns16>( kVersion ), &bext.mVersion );
		}
		else // Special case: If no value is given, a value of "1" is the default!
		{
			LE.putUns16( 1, &bext.mVersion );
		}

		if( this->valueExists( kUMID ) )
		{
			XMP_Uns32 muidSize = 0;
			const XMP_Uns8* const muid = this->getArray<XMP_Uns8>( kUMID, muidSize );
			
			// Make sure to copy 64 bytes max.
			muidSize = muidSize > 64 ? 64 : muidSize;
			memcpy( bext.mUMID, muid, muidSize );
		}
		//
		// set input buffer to zero
		//
		memset( buffer, 0, static_cast<size_t>(size) );

		//
		// copy BEXT block into buffer (except CodingHistory field)
		//
		memcpy( buffer, &bext, kBEXTSizeMin );

		//
		// copy CodingHistory field into buffer
		//
		if( ! codingHistory.empty() )
		{
			memcpy( buffer + kBEXTSizeMin, codingHistory.c_str(), static_cast<size_t>(size - kBEXTSizeMin) );
		}

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
// BEXTMetadata::isEmptyValue(...)
// 
// Purpose: Is the value of the passed ValueObject and its id "empty"?
// 
//-----------------------------------------------------------------------------

bool BEXTMetadata::isEmptyValue( XMP_Uns32 id, ValueObject& valueObj )
{
	bool ret = true;

	switch( id )
	{
		case kDescription:
		case kOriginator:
		case kOriginatorReference:
		case kOriginationDate:
		case kOriginationTime:
		case kCodingHistory:
		{
			TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);

			ret = ( strObj == NULL || ( strObj != NULL && strObj->getValue().empty() ) );
		}
		break;

		case kTimeReference:
		case kVersion:
			ret = false;
			break;
		case kUMID:
		{
			TArrayObject<XMP_Uns8>* obj = dynamic_cast<TArrayObject<XMP_Uns8>*>(&valueObj);

			if( obj != NULL )
			{
				XMP_Uns32 size	 = 0;
				const XMP_Uns8* const buffer = obj->getArray( size );

				ret = ( size == 0 );
			}
		}
		break;

		default:
			ret = true;
	}

	return ret;
}
