#ifndef __PNG_Support_hpp__
#define __PNG_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

#define PNG_SIGNATURE_LEN		8
#define PNG_SIGNATURE_DATA		"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"

#define ITXT_CHUNK_TYPE			"iTXt"

#define ITXT_HEADER_LEN			22
#define ITXT_HEADER_DATA		"XML:com.adobe.xmp\0\0\0\0\0"

namespace PNG_Support
{
	class ChunkData
	{
		public:
			ChunkData() : pos(0), len(0), type(0), xmp(false) {}
			virtual ~ChunkData() {}

			// | length |  type  |    data     | crc(type+data) |
			// |   4    |   4    | val(length) |       4        |
			//
			XMP_Uns64	pos;		// file offset of chunk
			XMP_Uns32	len;		// length of chunk data
			long		type;		// name/type of chunk
			bool		xmp;		// iTXt-chunk with XMP ?
	};

	typedef std::vector<ChunkData> ChunkVector;
	typedef ChunkVector::iterator ChunkIterator;

	class ChunkState
	{
		public:
			ChunkState() : xmpPos(0), xmpLen(0) {}
			virtual ~ChunkState() {}

			XMP_Uns64	xmpPos;
			XMP_Uns32	xmpLen;
			ChunkData	xmpChunk;
			ChunkVector chunks;	/* vector of chunks */
	};

	long OpenPNG ( XMP_IO* fileRef, ChunkState& inOutChunkState );

	bool ReadChunk ( XMP_IO* fileRef, ChunkState& inOutChunkState, long* chunkType, XMP_Uns32* chunkLength, XMP_Uns64& inOutPosition );
	bool WriteXMPChunk ( XMP_IO* fileRef, XMP_Uns32 len, const char* inBuffer );
	bool CopyChunk ( XMP_IO* sourceRef, XMP_IO* destRef, ChunkData& chunk );
	unsigned long UpdateChunkCRC( XMP_IO* fileRef, ChunkData& inOutChunkData );

	bool CheckIHDRChunkHeader ( ChunkData& inOutChunkData );
	unsigned long CheckiTXtChunkHeader ( XMP_IO* fileRef, ChunkState& inOutChunkState, ChunkData& inOutChunkData );

	bool ReadBuffer ( XMP_IO* fileRef, XMP_Uns64& pos, XMP_Uns32 len, char* outBuffer );
	bool WriteBuffer ( XMP_IO* fileRef, XMP_Uns64& pos, XMP_Uns32 len, const char* inBuffer );

	unsigned long CalculateCRC( unsigned char* inBuffer, XMP_Uns32 len );

} // namespace PNG_Support

#endif	// __PNG_Support_hpp__
