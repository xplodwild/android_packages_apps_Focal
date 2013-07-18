#ifndef __XIO_hpp__
#define __XIO_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "source/Host_IO.hpp"
#include "source/EndianUtils.hpp"

#include <string>

// =================================================================================================
// Support for I/O
// ===============

namespace XIO {

	// =============================================================================================
	// Internal utilities layered on top of the abstract XMP_IO and XMPFiles_IO classes.
	// =================================================================================

	void SplitLeafName ( std::string * path, std::string * leafName );
	void SplitFileExtension ( std::string * path, std::string * fileExt );
	
	void ReplaceTextFile ( XMP_IO* textFile, const std::string & newContent, bool doSafeUpdate );

	extern void Copy ( XMP_IO* sourceFile, XMP_IO* destFile, XMP_Int64 length,
					   XMP_AbortProc abortProc = 0, void* abortArg = 0 );

	extern void Move ( XMP_IO* sourceFile, XMP_Int64 sourceOffset,
					   XMP_IO* destFile, XMP_Int64 destOffset,
					   XMP_Int64 length, XMP_AbortProc abortProc = 0, void* abortArg = 0 );

	static inline bool CheckFileSpace ( XMP_IO* file, XMP_Int64 length )
	{
		XMP_Int64 remaining = file->Length() - file->Offset();
		return (length <= remaining);
	}
	
	// *** Need to absorb more of the utilities like FolderInfo, GetFileMode.

	// =============================================================================================
	// Inline utilities for endian-oriented reads and writes of numerbers.
	// ===================================================================

	// -------------------------------------
	// Unsigned forms that do the real work.

	static inline XMP_Uns8 ReadUns8 ( XMP_IO* file )
	{
		XMP_Uns8 value;
		file->ReadAll ( &value, 1 );
		return value;
	}

	static inline XMP_Uns16 ReadUns16_BE ( XMP_IO* file )
	{
		XMP_Uns16 value;
		file->ReadAll ( &value, 2 );
		return MakeUns16BE ( value );
	}

	static inline XMP_Uns16 ReadUns16_LE ( XMP_IO* file )
	{
		XMP_Uns16 value;
		file->ReadAll ( &value, 2 );
		return MakeUns16LE ( value );
	}

	static inline XMP_Uns32 ReadUns32_BE ( XMP_IO* file )
	{
		XMP_Uns32 value;
		file->ReadAll ( &value, 4 );
		return MakeUns32BE ( value );
	}

	static inline XMP_Uns32 ReadUns32_LE ( XMP_IO* file )
	{
		XMP_Uns32 value;
		file->ReadAll ( &value, 4 );
		return MakeUns32LE ( value );
	}

	static inline XMP_Uns64 ReadUns64_BE ( XMP_IO* file )
	{
		XMP_Uns64 value;
		file->ReadAll ( &value, 8 );
		return MakeUns64BE ( value );
	}

	static inline XMP_Uns64 ReadUns64_LE ( XMP_IO* file )
	{
		XMP_Uns64 value;
		file->ReadAll ( &value, 8 );
		return MakeUns64LE ( value );
	}

	static inline XMP_Uns8 PeekUns8 ( XMP_IO* file )
	{
		XMP_Uns8 value = XIO::ReadUns8 ( file );
		file->Seek ( -1, kXMP_SeekFromCurrent );
		return value;
	}

	static inline XMP_Uns16 PeekUns16_BE ( XMP_IO* file )
	{
		XMP_Uns16 value = XIO::ReadUns16_BE ( file );
		file->Seek ( -2, kXMP_SeekFromCurrent );
		return value;
	}

	static inline XMP_Uns16 PeekUns16_LE ( XMP_IO* file )
	{
		XMP_Uns16 value = XIO::ReadUns16_LE ( file );
		file->Seek ( -2, kXMP_SeekFromCurrent );
		return value;
	}

	static inline XMP_Uns32 PeekUns32_BE ( XMP_IO* file )
	{
		XMP_Uns32 value = XIO::ReadUns32_BE ( file );
		file->Seek ( -4, kXMP_SeekFromCurrent );
		return value;
	}

	static inline XMP_Uns32 PeekUns32_LE ( XMP_IO* file )
	{
		XMP_Uns32 value = XIO::ReadUns32_LE ( file );
		file->Seek ( -4, kXMP_SeekFromCurrent );
		return value;
	}

	static inline XMP_Uns64 PeekUns64_BE ( XMP_IO* file )
	{
		XMP_Uns64 value = XIO::ReadUns64_BE ( file );
		file->Seek ( -8, kXMP_SeekFromCurrent );
		return value;
	}

	static inline XMP_Uns64 PeekUns64_LE ( XMP_IO* file )
	{
		XMP_Uns64 value = XIO::ReadUns64_LE ( file );
		file->Seek ( -8, kXMP_SeekFromCurrent );
		return value;
	}

	static inline void WriteUns8 ( XMP_IO* file, XMP_Uns8 value )
	{
		file->Write ( &value, 1 );
	}

	static inline void WriteUns16_BE ( XMP_IO* file, XMP_Uns16 value )
	{
		XMP_Uns16 v = MakeUns16BE ( value );
		file->Write ( &v, 2 );
	}

	static inline void WriteUns16_LE ( XMP_IO* file, XMP_Uns16 value )
	{
		XMP_Uns16 v = MakeUns16LE ( value );
		file->Write ( &v, 2 );
	}

	static inline void WriteUns32_BE ( XMP_IO* file, XMP_Uns32 value )
	{
		XMP_Uns32 v = MakeUns32BE ( value );
		file->Write ( &v, 4 );
	}

	static inline void WriteUns32_LE ( XMP_IO* file, XMP_Uns32 value )
	{
		XMP_Uns32 v = MakeUns32LE ( value );
		file->Write ( &v, 4 );
	}

	static inline void WriteUns64_BE ( XMP_IO* file, XMP_Uns64 value )
	{
		XMP_Uns64 v = MakeUns64BE ( value );
		file->Write ( &v, 8 );
	}

	static inline void WriteUns64_LE ( XMP_IO* file, XMP_Uns64 value )
	{
		XMP_Uns64 v = MakeUns64LE ( value );
		file->Write ( &v, 8 );
	}

	// ------------------------------------------------------------------
	// Signed forms that just cast to unsigned and use the unsigned call.

	static inline XMP_Int8 ReadInt8 ( XMP_IO* file )
	{
		return (XMP_Int8) XIO::ReadUns8 ( file );
	}

	static inline XMP_Int16 ReadInt16_BE ( XMP_IO* file )
	{
		return (XMP_Int16) XIO::ReadUns16_BE ( file );
	}

	static inline XMP_Int16 ReadInt16_LE ( XMP_IO* file )
	{
		return (XMP_Int16) XIO::ReadUns16_LE ( file );
	}

	static inline XMP_Int32 ReadInt32_BE ( XMP_IO* file )
	{
		return (XMP_Int32) XIO::ReadUns32_BE ( file );
	}

	static inline XMP_Int32 ReadInt32_LE ( XMP_IO* file )
	{
		return (XMP_Int32) XIO::ReadUns32_LE ( file );
	}

	static inline XMP_Int64 ReadInt64_BE ( XMP_IO* file )
	{
		return (XMP_Int64) XIO::ReadUns64_BE ( file );
	}

	static inline XMP_Int64 ReadInt64_LE ( XMP_IO* file )
	{
		return (XMP_Int64) XIO::ReadUns64_LE ( file );
	}

	static inline XMP_Int8 PeekInt8 ( XMP_IO* file )
	{
		return (XMP_Int8) XIO::PeekUns8 ( file );
	}

	static inline XMP_Int16 PeekInt16_BE ( XMP_IO* file )
	{
		return (XMP_Int16) XIO::PeekUns16_BE ( file );
	}

	static inline XMP_Int16 PeekInt16_LE ( XMP_IO* file )
	{
		return (XMP_Int16) XIO::PeekUns16_LE ( file );
	}

	static inline XMP_Int32 PeekInt32_BE ( XMP_IO* file )
	{
		return (XMP_Int32) XIO::PeekUns32_BE ( file );
	}

	static inline XMP_Int32 PeekInt32_LE ( XMP_IO* file )
	{
		return (XMP_Int32) XIO::PeekUns32_LE ( file );
	}

	static inline XMP_Int64 PeekInt64_BE ( XMP_IO* file )
	{
		return (XMP_Int64) XIO::PeekUns64_BE ( file );
	}

	static inline XMP_Int64 PeekInt64_LE ( XMP_IO* file )
	{
		return (XMP_Int64) XIO::PeekUns64_LE ( file );
	}

	static inline void WriteInt8 ( XMP_IO* file, XMP_Int8 value )
	{
		XIO::WriteUns8 ( file, (XMP_Uns8)value );
	}

	static inline void WriteInt16_BE ( XMP_IO* file, XMP_Int16 value )
	{
		XIO::WriteUns16_BE ( file, (XMP_Uns16)value );
	}

	static inline void WriteInt16_LE ( XMP_IO* file, XMP_Int16 value )
	{
		XIO::WriteUns16_LE ( file, (XMP_Uns16)value );
	}

	static inline void WriteInt32_BE ( XMP_IO* file, XMP_Int32 value )
	{
		XIO::WriteUns32_BE ( file, (XMP_Uns32)value );
	}

	static inline void WriteInt32_LE ( XMP_IO* file, XMP_Int32 value )
	{
		XIO::WriteUns32_LE ( file, (XMP_Uns32)value );
	}

	static inline void WriteInt64_BE ( XMP_IO* file, XMP_Int64 value )
	{
		XIO::WriteUns64_BE ( file, (XMP_Uns64)value );
	}

	static inline void WriteInt64_LE ( XMP_IO* file, XMP_Int64 value )
	{
		XIO::WriteUns64_LE ( file, (XMP_Uns64)value );
	}

};

// =================================================================================================

// **********************************************************************************************
// I/O buffer used in JPEG, PSD, and TIFF handlers. Discredited, a mistake, do not use elsewhere.
// **********************************************************************************************

// -------------------------------------------------------------------------------------------------
// CheckFileSpace and RefillBuffer
// -------------------------------
//
// There is always a problem in file scanning of managing what you want to check against what is
// available in a buffer, trying to keep the logic understandable and minimize data movement. The
// CheckFileSpace and RefillBuffer functions are used here for a standard scanning model.
//
// The format scanning routines have an outer, "infinite" loop that looks for file markers. There
// is a local (on stack) buffer, a pointer to the current position in the buffer, and a pointer for
// the end of the buffer. The end pointer is just past the end of the buffer, "bufPtr == bufLimit"
// means you are out of data. The outer loop ends when the necessary markers are found or we reach
// the end of the file.
//
// The filePos is the file offset of the start of the current buffer. This is maintained so that
// we can tell where the packet is in the file, part of the info returned to the client.
//
// At each check CheckFileSpace is used to make sure there is enough data in the buffer for the
// check to be made. It refills the buffer if necessary, preserving the unprocessed data, setting
// bufPtr and bufLimit appropriately. If we are too close to the end of the file to make the check
// a failure status is returned.

enum { kIOBufferSize = 128*1024 };

struct IOBuffer {
	XMP_Int64 filePos;
	XMP_Uns8* ptr;
	XMP_Uns8* limit;
	size_t    len;
	XMP_Uns8  data [kIOBufferSize];
	IOBuffer() : filePos(0), ptr(&data[0]), limit(ptr), len(0) {};
};

static inline void FillBuffer ( XMP_IO* fileRef, XMP_Int64 fileOffset, IOBuffer* ioBuf )
{
	ioBuf->filePos = fileRef->Seek ( fileOffset, kXMP_SeekFromStart );
	if ( ioBuf->filePos != fileOffset ) {
		throw XMP_Error ( kXMPErr_ExternalFailure, "Seek failure in FillBuffer" );
	}
	ioBuf->len = fileRef->Read ( &ioBuf->data[0], kIOBufferSize );
	ioBuf->ptr = &ioBuf->data[0];
	ioBuf->limit = ioBuf->ptr + ioBuf->len;
}

static inline void MoveToOffset ( XMP_IO* fileRef, XMP_Int64 fileOffset, IOBuffer* ioBuf )
{
	if ( (ioBuf->filePos <= fileOffset) && (fileOffset < (XMP_Int64)(ioBuf->filePos + ioBuf->len)) ) {
		size_t bufOffset = (size_t)(fileOffset - ioBuf->filePos);
		ioBuf->ptr = &ioBuf->data[bufOffset];
	} else {
		FillBuffer ( fileRef, fileOffset, ioBuf );
	}
}

static inline void RefillBuffer ( XMP_IO* fileRef, IOBuffer* ioBuf )
{
	// Refill including part of the current data, seek back to the new buffer origin and read.
	ioBuf->filePos += (ioBuf->ptr - &ioBuf->data[0]);	// ! Increment before the read.
	size_t bufTail = ioBuf->limit - ioBuf->ptr;	// We'll re-read the tail portion of the buffer.
	if ( bufTail > 0 ) ioBuf->filePos = fileRef->Seek ( -((XMP_Int64)bufTail), kXMP_SeekFromCurrent );
	ioBuf->len = fileRef->Read ( &ioBuf->data[0], kIOBufferSize );
	ioBuf->ptr = &ioBuf->data[0];
	ioBuf->limit = ioBuf->ptr + ioBuf->len;
}

static inline bool CheckFileSpace ( XMP_IO* fileRef, IOBuffer* ioBuf, size_t neededLen )
{
	if ( size_t(ioBuf->limit - ioBuf->ptr) < size_t(neededLen) ) {	// ! Avoid VS.Net compare warnings.
		RefillBuffer ( fileRef, ioBuf );
	}
	return (size_t(ioBuf->limit - ioBuf->ptr) >= size_t(neededLen));
}

#endif	// __XIO_hpp__
