#ifndef __SWF_Support_hpp__
#define __SWF_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include <zlib.h>

// =================================================================================================

namespace SWF_IO {

	const XMP_Int64 MaxExpandedSize	= 0xFFFFFFFFUL;	// The file header has a UInt32 expanded size field.
	
	const size_t HeaderPrefixSize	= 8;	// The uncompressed first part of the file header.
	const size_t HeaderFixedSize	= 12;	// The fixed size part of the file header, omits the RECT.

	const XMP_Uns32 CompressedSignature	= 0x00535743;	// The low 3 bytes are "SWC".
	const XMP_Uns32 ExpandedSignature	= 0x00535746;	// The low 3 bytes are "SWF".
		// Note: Can't use char* here, it causes duplicate symbols with xcode.
	
	const XMP_Uns16 FileAttributesTagID	= 69;
	const XMP_Uns16 MetadataTagID		= 77;
	
	const XMP_Uns8 TagLengthMask	= 0x3F;
	const XMP_Uns8 HasMetadataMask	= 0x10;

	// A SWF file begins with a variable length header. The header layout is:
	//
	//	UInt8[3] - "FWS" for uncompressed SWF and "CWS" for compressed SWF
	//	UInt8 - SWF format version
	//	UInt32 - Length of uncompressed file, little endian
	//	RECT - packed bit RECT structure
	//	UInt16 - frame rate, little endian, really 8.8 fixed point
	//	UInt16 - frame count, little endian
	//
	// If the first 4 bytes are read as a little endian UInt32 they become "vSWC" and "vSWF", where
	// the "v" byte is the version format version.
	//
	// SWF compression starts 8 bytes into the file, after the length field in the header.
	// The length in the header is everything. If compressed this is 8 plus the decompressed size.
	//
	// Following the header is a sequence of tags. Each tag begins with a little endian UInt16 whose
	// upper 10 bits are the tag ID and lower 6 bits are a length for the content. If this length is
	// 63 (0x3F) then a little endian Int32 follows with the content length.
	//
	// The FileAttributes tag, #69, has a flag byte and 3 reserved bytes following the header. There
	// is only 1 flag bit that we care about, HasMetadata with the mask 0x10.
	//
	// The Metadata tag, #77, has content that is the UTF-8 XMP, preferably as small as possible.

	XMP_Uns32 FileHeaderSize ( XMP_Uns8 rectBits );
	
	class TagInfo {
	public:
		bool hasLongHeader;
		XMP_Uns16 tagID;
		XMP_Uns32 tagOffset, contentLength;
		TagInfo() : hasLongHeader(false), tagID(0), tagOffset(0), contentLength(0) {};
		~TagInfo() {};
	};

	bool GetTagInfo ( const RawDataBlock & swfStream, XMP_Uns32 tagOffset, TagInfo * info );
	XMP_Uns32 FullTagLength ( const TagInfo & info );
	XMP_Uns32 ContentOffset ( const TagInfo & info );
	XMP_Uns32 NextTagOffset ( const TagInfo & info );
	
	XMP_Int64 DecompressFileToMemory ( XMP_IO * fileIn, RawDataBlock * dataOut );
	XMP_Int64 CompressMemoryToFile ( const RawDataBlock & dataIn, XMP_IO*  fileOut );
	
};	// SWF_IO

#endif	// __SWF_Support_hpp__
