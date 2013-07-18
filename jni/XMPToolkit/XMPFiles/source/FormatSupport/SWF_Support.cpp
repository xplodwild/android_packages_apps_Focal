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
#include "source/XIO.hpp"

#include "XMPFiles/source/FormatSupport/SWF_Support.hpp"

#include "third-party/zlib/zlib.h"

// =================================================================================================

XMP_Uns32 SWF_IO::FileHeaderSize ( XMP_Uns8 rectBits ) {

	// Return the full size of the SWF header, adding the fixed size to the variable RECT size.
	
	XMP_Uns8 bitsPerField = rectBits >> 3;
	XMP_Uns32 rectBytes = ((5 + (4 * bitsPerField)) / 8) + 1;
	
	return SWF_IO::HeaderFixedSize + rectBytes;

}	// SWF_IO::FileHeaderSize

// =================================================================================================

bool SWF_IO::GetTagInfo ( const RawDataBlock & swfStream, XMP_Uns32 tagOffset, SWF_IO::TagInfo * info ) {

	if ( tagOffset >= swfStream.size() ) return false;
	XMP_Uns32 spaceLeft = swfStream.size() - tagOffset;
	XMP_Uns8 headerSize = 2;
	if ( spaceLeft < headerSize ) return false;	// The minimum empty tag is a 2 byte header.
	
	XMP_Uns16 tagHeader = GetUns16LE ( &swfStream[tagOffset] );

	info->tagID = tagHeader >> 6;
	info->tagOffset = tagOffset;
	info->contentLength = tagHeader & SWF_IO::TagLengthMask;
	
	if ( info->contentLength != SWF_IO::TagLengthMask ) {
		info->hasLongHeader = false;
	} else {
		headerSize = 6;
		if ( spaceLeft < headerSize ) return false;	// Make sure there is room for the extended length.
		info->contentLength = GetUns32LE ( &swfStream[tagOffset+2] );
		info->hasLongHeader = true;
	}
	
	if ( (spaceLeft - headerSize) < info->contentLength ) return false;
	
	return true;

}	// SWF_IO::GetTagInfo

// =================================================================================================

static inline XMP_Uns32 TagHeaderSize ( const SWF_IO::TagInfo & info ) {

	XMP_Uns8 headerSize = 2;
	if ( info.hasLongHeader ) headerSize = 6;
	return headerSize;

}	// TagHeaderSize

// =================================================================================================

XMP_Uns32 SWF_IO::FullTagLength ( const SWF_IO::TagInfo & info ) {
	
	return TagHeaderSize ( info ) + info.contentLength;

}	// SWF_IO::FullTagLength

// =================================================================================================

XMP_Uns32 SWF_IO::ContentOffset ( const SWF_IO::TagInfo & info ) {

	return info.tagOffset + TagHeaderSize ( info );

}	// SWF_IO::ContentOffset

// =================================================================================================

XMP_Uns32 SWF_IO::NextTagOffset ( const SWF_IO::TagInfo & info ) {
	
	return info.tagOffset + FullTagLength ( info );

}	// SWF_IO::NextTagOffset

// =================================================================================================

static inline void AppendData ( RawDataBlock * dataOut, XMP_Uns8 * buffer, size_t count ) {

	size_t prevSize = dataOut->size();	// ! Don't save a pointer, there might be a reallocation.
	dataOut->insert ( dataOut->end(), count, 0 );	// Add space to the RawDataBlock.
	memcpy ( &((*dataOut)[prevSize]), buffer, count );

}	// AppendData

// =================================================================================================

XMP_Int64 SWF_IO::DecompressFileToMemory ( XMP_IO * fileIn, RawDataBlock * dataOut ) {

	fileIn->Rewind();
	dataOut->clear();
	
	static const size_t bufferSize = 64*1024;
	XMP_Uns8 bufferIn  [ bufferSize ];
	XMP_Uns8 bufferOut [ bufferSize ];

	int err;
	z_stream zipState;
	memset ( &zipState, 0, sizeof(zipState) );
	err = inflateInit ( &zipState );
	XMP_Enforce ( err == Z_OK );
	
	XMP_Int32 ioCount;
	XMP_Int64 offsetIn;
	const XMP_Int64 lengthIn = fileIn->Length();
	XMP_Enforce ( ((XMP_Int64)SWF_IO::HeaderPrefixSize <= lengthIn) && (lengthIn <= SWF_IO::MaxExpandedSize) );
	
	// Set the uncompressed part of the header. Save the expanded size from the file.
	
	fileIn->ReadAll ( bufferIn, SWF_IO::HeaderPrefixSize );
	offsetIn = SWF_IO::HeaderPrefixSize;
	XMP_Uns32 expectedFullSize = GetUns32LE ( &bufferIn[4] );

	AppendData ( dataOut, bufferIn, SWF_IO::HeaderPrefixSize );	// Copy the compressed stream's prefix.
	PutUns32LE ( SWF_IO::ExpandedSignature, &(*dataOut)[0] );	// Change the signature.
	(*dataOut)[3] = bufferIn[3];	// Keep the SWF version.
	
	// Read the input file, feed it to the decompression engine, writing as needed.

	zipState.next_out  = &bufferOut[0];	// Initial output conditions. Must be set before the input loop!
	zipState.avail_out = bufferSize;
	
	while ( offsetIn < lengthIn ) {
	
		// Read the next chunk of input.
		ioCount = fileIn->Read ( bufferIn, bufferSize );
		XMP_Enforce ( ioCount > 0 );
		offsetIn += ioCount;
		zipState.next_in  = &bufferIn[0];
		zipState.avail_in = ioCount;
		
		// Process all of this input, writing as needed.
		
		err = Z_OK;
		while ( (zipState.avail_in > 0) && (err == Z_OK) ) {

			XMP_Assert ( zipState.avail_out > 0 );	// Sanity check for output buffer space.
			err = inflate ( &zipState, Z_NO_FLUSH );
			XMP_Enforce ( (err == Z_OK) || (err == Z_STREAM_END) );

			if ( zipState.avail_out == 0 ) {
				AppendData ( dataOut, bufferOut, bufferSize );
				zipState.next_out  = &bufferOut[0];
				zipState.avail_out = bufferSize;
			}

		}
	
	}
	
	// Finish the decompression and write the final output.

	do {

		ioCount = bufferSize - zipState.avail_out;	// Make sure there is room for inflate to do more.
		if ( ioCount > 0 ) {
			AppendData ( dataOut, bufferOut, ioCount );
			zipState.next_out  = &bufferOut[0];
			zipState.avail_out = bufferSize;
		}

		err = inflate ( &zipState, Z_NO_FLUSH );
		XMP_Enforce ( (err == Z_OK) || (err == Z_STREAM_END) || (err == Z_BUF_ERROR) );

	} while ( err == Z_OK );

	ioCount = bufferSize - zipState.avail_out;	// Write any final output.
	if ( ioCount > 0 ) {
		AppendData ( dataOut, bufferOut, ioCount );
		zipState.next_out  = &bufferOut[0];
		zipState.avail_out = bufferSize;
	}

	// Done. Make sure the file header has the true decompressed size.
	
	XMP_Int64 lengthOut = zipState.total_out + SWF_IO::HeaderPrefixSize;
	if ( lengthOut != expectedFullSize ) PutUns32LE ( (XMP_Uns32)lengthOut, &((*dataOut)[4]) );
	inflateEnd ( &zipState );
	return lengthOut;
	
}	// SWF_IO::DecompressFileToMemory

// =================================================================================================

XMP_Int64 SWF_IO::CompressMemoryToFile ( const RawDataBlock & dataIn, XMP_IO*  fileOut ) {

	fileOut->Rewind();
	fileOut->Truncate ( 0 );
	
	static const size_t bufferSize = 64*1024;
	XMP_Uns8 bufferOut [ bufferSize ];

	int err;
	z_stream zipState;
	memset ( &zipState, 0, sizeof(zipState) );
	err = deflateInit ( &zipState, Z_DEFAULT_COMPRESSION );
	XMP_Enforce ( err == Z_OK );
	
	XMP_Int32 ioCount;
	const size_t lengthIn = dataIn.size();
	XMP_Enforce ( SWF_IO::HeaderPrefixSize <= lengthIn );
	
	// Write the uncompressed part of the file header.
	
	PutUns32LE ( SWF_IO::CompressedSignature, &bufferOut[0] );
	bufferOut[3] = dataIn[3];	// Copy the SWF version.
	PutUns32LE ( lengthIn, &bufferOut[4] );
	fileOut->Write ( bufferOut, SWF_IO::HeaderPrefixSize );
	
	// Feed the input to the compression engine in one step, write the output as available.

	zipState.next_in   = (Bytef*)&dataIn[SWF_IO::HeaderPrefixSize];
	zipState.avail_in  = lengthIn - SWF_IO::HeaderPrefixSize;
	zipState.next_out  = &bufferOut[0];
	zipState.avail_out = bufferSize;
	
	while ( zipState.avail_in > 0 ) {

		XMP_Assert ( zipState.avail_out > 0 );	// Sanity check for output buffer space.
		err = deflate ( &zipState, Z_NO_FLUSH );
		XMP_Enforce ( err == Z_OK );

		if ( zipState.avail_out == 0 ) {
			fileOut->Write ( bufferOut, bufferSize );
			zipState.next_out  = &bufferOut[0];
			zipState.avail_out = bufferSize;
		}

	}
	
	// Finish the compression and write the final output.

	do {

		err = deflate ( &zipState, Z_FINISH );
		XMP_Enforce ( (err == Z_OK) || (err == Z_STREAM_END) );
		ioCount = bufferSize - zipState.avail_out;	// See if there is output to write.

		if ( ioCount > 0 ) {
			fileOut->Write ( bufferOut, ioCount );
			zipState.next_out  = &bufferOut[0];
			zipState.avail_out = bufferSize;
		}

	} while ( err != Z_STREAM_END );

	// Done.
	
	XMP_Int64 lengthOut = zipState.total_out;
	deflateEnd ( &zipState );
	return lengthOut;
	
}	// SWF_IO::CompressMemoryToFile

// =================================================================================================

#if 0	// ! Not used, but save it for later transfer to a general ZIP utility file.

XMP_Int64 SWF_IO::DecompressFileToFile ( XMP_IO * fileIn, XMP_IO * fileOut ) {

	fileIn->Rewind();
	fileOut->Rewind();
	fileOut->Truncate ( 0 );
	
	static const size_t bufferSize = 64*1024;
	XMP_Uns8 bufferIn  [ bufferSize ];
	XMP_Uns8 bufferOut [ bufferSize ];

	int err;
	z_stream zipState;
	memset ( zipState, 0, sizeof(zipState) );
	err = inflateInit ( &zipState );
	XMP_Enforce ( err == Z_OK );
	
	XMP_Int32 ioCount;
	XMP_Int64 offsetIn;
	const XMP_Int64 lengthIn = fileIn->Length();
	XMP_Enforce ( (lengthIn >= SWF_IO::HeaderPrefixSize) && (lengthIn <= SWF_IO::MaxExpandedSize) );
	
	// Copy the uncompressed part of the file header. Save the expanded size from the header.
	
	fileIn->ReadAll ( bufferIn, SWF_IO::HeaderPrefixSize );
	fileOut.Write ( bufferIn, SWF_IO::HeaderPrefixSize );
	offsetIn = SWF_IO::HeaderPrefixSize;
	
	XMP_Uns32 expectedFullSize = GetUns32LE ( &bufferIn[4] );
	
	// Read the input file, feed it to the decompression engine, writing as needed.

	zipState.next_out  = &bufferOut[0];	// Initial output conditions. Must be set before the input loop!
	zipState.avail_out = bufferSize;
	
	while ( offsetIn < lengthIn ) {
	
		// Read the next chunk of input.
		ioCount = fileIn->Read ( bufferIn, bufferSize );
		XMP_Enforce ( ioCount > 0 );
		offsetIn += ioCount;
		zipState.next_in  = &bufferIn[0];
		zipState.avail_in = ioCount;
		
		// Process all of this input, writing as needed.
		
		err = Z_OK;
		while ( (zipState.avail_in > 0) && (err == Z_OK) ) {

			XMP_Assert ( zipState.avail_out > 0 );	// Sanity check for output buffer space.
			err = inflate ( &zipState, Z_NO_FLUSH );
			XMP_Enforce ( (err == Z_OK) || (err == Z_STREAM_END) );

			if ( zipState.avail_out == 0 ) {
				fileOut->write ( bufferOut, bufferSize );
				zipState.next_out  = &bufferOut[0];
				zipState.avail_out = bufferSize;
			}

		}
	
	}
	
	// Finish the decompression and write the final output.

	do {

		ioCount = bufferSize - zipState.avail_out;	// Make sure there is room for inflate.
		if ( ioCount > 0 ) {
			fileOut->write ( bufferOut, ioCount );
			zipState.next_out  = &bufferOut[0];
			zipState.avail_out = bufferSize;
		}

		err = inflate ( &zipState, Z_NO_FLUSH );
		XMP_Enforce ( (err == Z_OK) || (err == Z_STREAM_END) || (err == Z_BUF_ERROR) );
		ioCount = bufferSize - zipState.avail_out;	// See if there is output to write.
		if ( ioCount > 0 ) {
			fileOut->write ( bufferOut, ioCount );
			zipState.next_out  = &bufferOut[0];
			zipState.avail_out = bufferSize;
		}

	} while ( err == Z_OK );

	ioCount = bufferSize - zipState.avail_out;	// Write any final output.
	if ( ioCount > 0 ) {
		fileOut->write ( bufferOut, ioCount );
		zipState.next_out  = &bufferOut[0];
		zipState.avail_out = bufferSize;
	}

	// Done. Make sure the file header has the true decompressed size.
	
	XMP_Int64 lengthOut = zipState.total_out;
	
	if ( lengthOut != expectedFullSize ) {
		PutUns32LE ( &bufferOut[0], lengthOut );
		fileOut->Seek ( 4, kXMP_SeekFromStart );
		fileOut.Write ( &bufferOut[0], 4 );
		fileOut->Seek ( 0, kXMP_SeekFromEnd );
	}

	inflateEnd ( &zipState );
	return lengthOut;
	
}	// SWF_IO::DecompressFileToFile

#endif

// =================================================================================================

#if 0	// ! Not used, but save it for later transfer to a general ZIP utility file.

XMP_Int64 SWF_IO::CompressFileToFile ( XMP_IO * fileIn, XMP_IO * fileOut ) {

	fileIn->Rewind();
	fileOut->Rewind();
	fileOut->Truncate ( 0 );
	
	static const size_t bufferSize = 64*1024;
	XMP_Uns8 bufferIn  [ bufferSize ];
	XMP_Uns8 bufferOut [ bufferSize ];

	int err;
	z_stream zipState;
	memset ( zipState, 0, sizeof(zipState) );
	err = deflateInit ( &zipState, Z_DEFAULT_COMPRESSION );
	XMP_Enforce ( err == Z_OK );
	
	XMP_Int32 ioCount;
	XMP_Int64 offsetIn;
	const XMP_Int64 lengthIn = fileIn->Length();
	XMP_Enforce ( (lengthIn >= SWF_IO::HeaderPrefixSize) && (lengthIn <= SWF_IO::MaxExpandedSize) );
	
	// Write the uncompressed part of the file header.
	
	fileIn->ReadAll ( bufferIn, SWF_IO::HeaderPrefixSize );
	offsetIn = SWF_IO::HeaderPrefixSize;
	
	PutUns32LE ( SWF_IO::CompressedSignature, &bufferOut[0] );
	bufferOut[3] = bufferIn[3];	// Copy the SWF version.
	PutUns32LE ( &bufferOut[4], lengthIn );
	fileOut.Write ( bufferOut, SWF_IO::HeaderPrefixSize );
	
	// Read the input file, feed it to the compression engine, writing as needed.

	zipState.next_out  = &bufferOut[0];	// Initial output conditions. Must be set before the input loop!
	zipState.avail_out = bufferSize;
	
	while ( offsetIn < lengthIn ) {
	
		// Read the next chunk of input.
		ioCount = fileIn->Read ( bufferIn, bufferSize );
		XMP_Enforce ( ioCount > 0 );
		offsetIn += ioCount;
		zipState.next_in  = &bufferIn[0];
		zipState.avail_in = ioCount;
		
		// Process all of this input, writing as needed. Yes, we need a loop. Compression means less
		// output than input, but a previous read has probably left partial compression results.
		
		while ( zipState.avail_in > 0 ) {

			XMP_Assert ( zipState.avail_out > 0 );	// Sanity check for output buffer space.
			err = deflate ( &zipState, Z_NO_FLUSH );
			XMP_Enforce ( err == Z_OK );

			if ( zipState.avail_out == 0 ) {
				fileOut->write ( bufferOut, bufferSize );
				zipState.next_out  = &bufferOut[0];
				zipState.avail_out = bufferSize;
			}

		}
	
	}
	
	// Finish the compression and write the final output.

	do {

		err = deflate ( &zipState, Z_FINISH );
		XMP_Enforce ( (err == Z_OK) || (err == Z_STREAM_END) );
		ioCount = bufferSize - zipState.avail_out;	// See if there is output to write.

		if ( ioCount > 0 ) {
			fileOut->write ( bufferOut, ioCount );
			zipState.next_out  = &bufferOut[0];
			zipState.avail_out = bufferSize;
		}

	} while ( err != Z_STREAM_END );

	// Done.
	
	XMP_Int64 lengthOut = zipState.total_out;
	deflateEnd ( &zipState );
	return lengthOut;
	
}	// SWF_IO::CompressFileToFile

#endif
