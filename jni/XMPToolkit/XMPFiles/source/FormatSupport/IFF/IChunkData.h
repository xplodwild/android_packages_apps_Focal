// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _IChunkData_h_
#define _IChunkData_h_

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"
#include "source/Endian.h"
#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include <string>

namespace IFF_RIFF
{

/**
 *	This interface allow access to only the data part of a chunk.
 */
class IChunkData
{
	public:		

		virtual ~IChunkData() {};

		/**
		 * Get the chunk ID
		 *
		 * @return	Return the ID, 0 if the chunk does not have an ID.
		 */
		virtual XMP_Uns32 getID() const = 0;
		
		/**
		 * Get the chunk type (if available)
		 * (the first four data bytes of the chunk could be a chunk type)
		 *
		 * @return	Return the type, kType_NONE if the chunk does not contain data.
		 */
		virtual XMP_Uns32 getType() const = 0;

		/**
		 * Get the chunk identifier [id and type]
		 *
		 * @return	Return the identifier
		 */
		virtual const ChunkIdentifier& getIdentifier() const = 0;
		
		/**
		 * Access the data of the chunk.
		 *
		 * @param data OUT pointer to the byte array
		 * @return size of the data block, 0 if no data is available
		 */
		virtual XMP_Uns64 getData( const XMP_Uns8** data ) const = 0;

		/**
		 * Set new data for the chunk. 
		 * Will delete an existing internal buffer and recreate a new one
		 * and copy the given data into that new buffer.
		 *
		 * @param data pointer to the data to put into the chunk
		 * @param size Size of the data block
		 */
		virtual void setData( const XMP_Uns8* const data, XMP_Uns64 size, XMP_Bool writeType = false ) = 0;
		
		/** 
		 * Returns the current size of the Chunk without pad byte.
		 *
		 * @param includeHeader if set, the returned size will be the whole chunk size including the header
		 * @return either size of the data block of the chunk or size of the whole chunk
		 */
		virtual XMP_Uns64 getSize( bool includeHeader = false ) const = 0; 
		

		/* The following methods are getter/setter for certain data types. 
		  They always take care of little-endian/big-endian issues.
		  The offset starts at the data area of the Chunk. */

		virtual XMP_Uns32 getUns32( XMP_Uns64 offset=0 ) const = 0;
		virtual void setUns32( XMP_Uns32 value, XMP_Uns64 offset=0 ) = 0;

		virtual XMP_Uns64 getUns64( XMP_Uns64 offset=0 ) const = 0;
		virtual void setUns64( XMP_Uns64 value, XMP_Uns64 offset=0 ) = 0;

		virtual XMP_Int32 getInt32( XMP_Uns64 offset=0 ) const = 0;
		virtual void setInt32( XMP_Int32 value, XMP_Uns64 offset=0 ) = 0;

		virtual XMP_Int64 getInt64( XMP_Uns64 offset=0 ) const = 0;
		virtual void setInt64( XMP_Int64 value, XMP_Uns64 offset=0 ) = 0;

		virtual std::string getString( XMP_Uns64 size = 0, XMP_Uns64 offset=0 ) const = 0;
		virtual void setString( std::string value, XMP_Uns64 offset=0 ) = 0;

		/**
		 * Creates a string representation of the chunk and its children.
		 */
		virtual std::string toString( std::string tabs = std::string() , XMP_Bool showOriginal = false ) = 0;
	
}; // IChunkData

} // namespace

#endif
