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

#include "XMPFiles/source/FormatSupport/WAVE/INFOMetadata.h"
#include "source/Endian.h"

using namespace IFF_RIFF;

typedef std::map<XMP_Uns32, std::string>::const_iterator MapIterator;

static const XMP_Uns32 kSizeChunkID		= 4;
static const XMP_Uns32 kSizeChunkSize	= 4;
static const XMP_Uns32 kSizeChunkType	= 4;
static const XMP_Uns32 kChunkHeaderSize	= kSizeChunkID + kSizeChunkSize;

static const XMP_Uns32 kType_INFO		= 0x494E464F;

//-----------------------------------------------------------------------------
// 
// INFOMetadata::INFOMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

INFOMetadata::INFOMetadata()
{
}

INFOMetadata::~INFOMetadata()
{
}

//-----------------------------------------------------------------------------
// 
// INFOMetadata::parse(...)
// 
// Purpose: @see IMetadata::parse
// 
//-----------------------------------------------------------------------------

void INFOMetadata::parse( const XMP_Uns8* input, XMP_Uns64 size )
{
	if( input != NULL && size >= kSizeChunkType )
	{
		const BigEndian& BE		= BigEndian::getInstance();
		const LittleEndian& LE	= LittleEndian::getInstance();

		XMP_Uns32 type = BE.getUns32( &input[0] );

		XMP_Validate( type == kType_INFO, "Invalid LIST:INFO data", kXMPErr_InternalFailure );

		XMP_Uns64 offset = kSizeChunkType;	// pointer into the buffer

		while( offset < size )
		{
			//
			// continue parsing only if the remaing buffer is greater than
			// the chunk header size
			//
			if( size - offset >= kChunkHeaderSize )
			{
				//
				// read: chunk id
				//		 chunk size
				//		 chunk data
				XMP_Uns32 id		= BE.getUns32( &input[offset] );
				XMP_Uns32 datasize	= LE.getUns32( &input[offset+kSizeChunkID] );

				if( offset + kChunkHeaderSize + datasize <= size )
				{
					if( datasize > 0 )
					{
						//
						// don't store empty values
						//
						std::string value( reinterpret_cast<const char*>( &input[offset+kChunkHeaderSize] ), datasize );

						//
						// set new value
						//
						this->setValue<std::string>( id, value );
					}

					//
					// update pointer
					//
					offset = offset + datasize + kChunkHeaderSize;

					if( datasize & 1 )
					{
						// pad byte
						offset++;
					}
				}
				else
				{
					//
					// invalid chunk, clean up and throw exception
					//
					this->deleteAll();
					XMP_Throw ( "Not a valid LIST:INFO chunk", kXMPErr_BadFileFormat );
				}
			}
			else
			{
				//
				// invalid chunk, clean up and throw exception
				//
				this->deleteAll();
				XMP_Throw ( "Not a valid LIST:INFO chunk", kXMPErr_BadFileFormat );
			}
		}

		this->resetChanges();
	}
	else
	{
		XMP_Throw ( "Not a valid LIST:INFO chunk", kXMPErr_BadFileFormat );
	}
}

//-----------------------------------------------------------------------------
// 
// INFOMetadata::serialize(...)
// 
// Purpose: @see IMetadata::serialize
// 
//-----------------------------------------------------------------------------

XMP_Uns64 INFOMetadata::serialize( XMP_Uns8** outBuffer )
{
	XMP_Uns64 size = 0;

	if( outBuffer != NULL )
	{
		//
		// calculate required buffer size
		//
		for( ValueMap::iterator iter=mValues.begin(); iter!=mValues.end(); iter++ )
		{
			TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(iter->second);

			XMP_Uns32 chunkSize = kChunkHeaderSize + strObj->getValue().length();

			if( chunkSize & 1 )
			{
				// take account of pad byte
				chunkSize++;
			}

			size += chunkSize;
		}
		
		size += kSizeChunkType; // add size of type "INFO"

		if( size > 0 )
		{
			XMP_Uns8* buffer = new XMP_Uns8[static_cast<size_t>(size)];		// output buffer

			memset( buffer, 0, static_cast<size_t>(size) );

			const BigEndian& BE		= BigEndian::getInstance();
			const LittleEndian& LE	= LittleEndian::getInstance();

			// Put type "INFO" in front of the buffer
			XMP_Uns32 typeInfo = BE.getUns32(&kType_INFO);
			memcpy( buffer, &typeInfo, kSizeChunkType );
			XMP_Uns64 offset = kSizeChunkType;						// pointer into the buffer

			//
			// for each stored value
			//
			for( ValueMap::iterator iter=mValues.begin(); iter!=mValues.end(); iter++ )
			{
				//
				// get: chunk data
				//		chunk id
				//		chunk size
				//
				TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(iter->second);
				std::string value	= strObj->getValue();
				XMP_Uns32 id		= iter->first;
				XMP_Uns32 size		= value.length();

				if( size & 1 && strObj->hasChanged() )
				{
					//
					// if we modified the value of this entry
					// then fill chunk data with zero bytes
					// rather than use a pad byte, i.e.
					// size of each LIST:INFO entry has
					// an odd size
					//
					size++;
				}

				//
				// chunk id and chunk size are stored in little endian format
				//
				id		= BE.getUns32( &id );
				size	= LE.getUns32( &size );

				//
				// copy values into output buffer
				//
				memcpy( buffer+offset, &id, kSizeChunkID );
				memcpy( buffer+offset+kSizeChunkID, &size, kSizeChunkSize );
				//size has been changed in little endian format. Change it back to bigendina
				size	= LE.getUns32( &size );
				memcpy( buffer+offset+kChunkHeaderSize, value.c_str(), size );

				//
				// update pointer
				//
				offset += kChunkHeaderSize;
				offset += size;

				if( size & 1 )
				{
					//
					// take account of pad byte
					offset++;
				}
			}

			*outBuffer = buffer;
		}
	}
	else
	{
		XMP_Throw ( "Invalid buffer", kXMPErr_InternalFailure );
	}

	return size;
}

//-----------------------------------------------------------------------------
// 
// INFOMetadata::isEmptyValue(...)
// 
// Purpose: Is the value of the passed ValueObject and its id "empty"?
// 
//-----------------------------------------------------------------------------

bool INFOMetadata::isEmptyValue( XMP_Uns32 id, ValueObject& valueObj )
{
	TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);

	return ( strObj == NULL || ( strObj != NULL && strObj->getValue().empty() ) );
}
