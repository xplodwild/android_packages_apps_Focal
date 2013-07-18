#ifndef __ISOBaseMedia_Support_hpp__
#define __ISOBaseMedia_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"

// =================================================================================================
/// \file ISOBaseMedia_Support.hpp
/// \brief XMPFiles support for the ISO Base Media File Format.
///
/// \note These classes are for use only when directly compiled and linked. They should not be
/// packaged in a DLL by themselves. They do not provide any form of C++ ABI protection.
// =================================================================================================

namespace ISOMedia {

	enum {
		k_ftyp = 0x66747970UL, 	// File header Box, no version/flags.

		k_mp41 = 0x6D703431UL, 	// Compatible brand codes
		k_mp42 = 0x6D703432UL,
		k_f4v  = 0x66347620UL,
		k_avc1 = 0x61766331UL,
		k_qt   = 0x71742020UL,

		k_moov = 0x6D6F6F76UL, 	// Container Box, no version/flags.
		k_mvhd = 0x6D766864UL, 	// Data FullBox, has version/flags.
		k_hdlr = 0x68646C72UL,
		k_udta = 0x75647461UL, 	// Container Box, no version/flags.
		k_cprt = 0x63707274UL, 	// Data FullBox, has version/flags.
		k_uuid = 0x75756964UL, 	// Data Box, no version/flags.
		k_free = 0x66726565UL, 	// Free space Box, no version/flags.
		k_mdat = 0x6D646174UL, 	// Media data Box, no version/flags.

		k_trak = 0x7472616BUL,	// Types for the QuickTime timecode track.
		k_tkhd = 0x746B6864UL,
		k_edts = 0x65647473UL,
		k_elst = 0x656C7374UL,
		k_mdia = 0x6D646961UL,
		k_mdhd = 0x6D646864UL,
		k_tmcd = 0x746D6364UL,
		k_mhlr = 0x6D686C72UL,
		k_minf = 0x6D696E66UL,
		k_stbl = 0x7374626CUL,
		k_stsd = 0x73747364UL,
		k_stsc = 0x73747363UL,
		k_stco = 0x7374636FUL,
		k_co64 = 0x636F3634UL,
		k_dinf = 0x64696E66UL,
		k_dref = 0x64726566UL,
		k_alis = 0x616C6973UL,

		k_meta = 0x6D657461UL, 	// Types for the iTunes metadata boxes.
		k_ilst = 0x696C7374UL,
		k_mdir = 0x6D646972UL,
		k_mean = 0x6D65616EUL,
		k_name = 0x6E616D65UL,
		k_data = 0x64617461UL,
		k_hyphens = 0x2D2D2D2DUL,

		k_skip = 0x736B6970UL, 	// Additional classic QuickTime top level boxes.
		k_wide = 0x77696465UL,
		k_pnot = 0x706E6F74UL,

		k_XMP_ = 0x584D505FUL 	// The QuickTime variant XMP box.
	};

	static XMP_Uns32 k_xmpUUID [4] = { MakeUns32BE ( 0xBE7ACFCBUL ),
									   MakeUns32BE ( 0x97A942E8UL ),
									   MakeUns32BE ( 0x9C719994UL ),
									   MakeUns32BE ( 0x91E3AFACUL ) };

	struct BoxInfo {
		XMP_Uns32 boxType;		// In memory as native endian!
		XMP_Uns32 headerSize;	// Normally 8 or 16, less than 8 if available space is too small.
		XMP_Uns64 contentSize;	// Always the real size, never 0 for "to EoF".
		BoxInfo() : boxType(0), headerSize(0), contentSize(0) {};
	};

	// Get basic info about a box in memory, returning a pointer to the following box.
	const XMP_Uns8 * GetBoxInfo ( const XMP_Uns8 * boxPtr, const XMP_Uns8 * boxLimit,
								  BoxInfo * info, bool throwErrors = false );

	// Get basic info about a box in a file, returning the offset of the following box. The I/O
	// pointer is left at the start of the box's content. Returns the offset of the following box.
	XMP_Uns64 GetBoxInfo ( XMP_IO* fileRef, const XMP_Uns64 boxOffset, const XMP_Uns64 boxLimit,
						   BoxInfo * info, bool doSeek = true, bool throwErrors = false );

//	XMP_Uns32 CountChildBoxes ( XMP_IO* fileRef, const XMP_Uns64 childOffset, const XMP_Uns64 childLimit );

}	// namespace ISO_Media

// =================================================================================================

#endif	// __ISOBaseMedia_Support_hpp__
