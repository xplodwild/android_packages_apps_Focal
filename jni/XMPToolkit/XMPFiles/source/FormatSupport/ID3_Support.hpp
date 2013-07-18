#ifndef __ID3_Support_hpp__
#define __ID3_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#if XMP_WinBuild
	#define stricmp _stricmp
#else
	static int stricmp ( const char * left, const char * right )	// Case insensitive ASCII compare.
	{
		char chL = *left;	// ! Allow for 0 passes in the loop (one string is empty).
		char chR = *right;	// ! Return -1 for stricmp ( "a", "Z" ).

		for ( ; (*left != 0) && (*right != 0); ++left, ++right ) {
			chL = *left;
			chR = *right;
			if ( chL == chR ) continue;
			if ( ('A' <= chL) && (chL <= 'Z') ) chL |= 0x20;
			if ( ('A' <= chR) && (chR <= 'Z') ) chR |= 0x20;
			if ( chL != chR ) break;
		}

		if ( chL == chR ) return 0;
		if ( chL < chR ) return -1;
		return 1;
	}
#endif

// =================================================================================================

namespace ID3_Support {

	// =============================================================================================

	inline XMP_Int32 synchToInt32 ( XMP_Uns32 rawDataBE ) {
		XMP_Validate ( (0 == (rawDataBE & 0x80808080)), "input not synchsafe", kXMPErr_InternalFailure );
		XMP_Int32 r = (rawDataBE & 0x0000007F) + ((rawDataBE >> 1) & 0x00003F80) +
					  ((rawDataBE >> 2) & 0x001FC000) + ((rawDataBE >> 3) & 0x0FE00000);
		return r;
	}

	inline XMP_Uns32 int32ToSynch ( XMP_Int32 value ) {
		XMP_Validate ( (0 <= 0x0FFFFFFF), "value too big", kXMPErr_InternalFailure );
		XMP_Uns32 r = (value & 0x0000007F) + ((value & 0x00003F80) << 1) +
					  ((value & 0x001FC000) << 2) + ((value & 0x0FE00000) << 3);
		return r;
	}

	// =============================================================================================
	
	bool InitializeGlobals();	// Initialize and terminate the known genre maps.
	void TerminateGlobals();
	
	// =============================================================================================

	namespace GenreUtils {	
		
		void ConvertGenreToXMP ( const char * id3Genre, std::string * xmpGenre );
		void ConvertGenreToID3 ( const char * xmpGenre, std::string * id3Genre );

		// Internal utilities, exposed for unit testing:
		const char * FindGenreName ( const std::string & code );
		const char * FindGenreCode ( const std::string & name );

	};

	// =============================================================================================

	class ID3Header {	// Minimal support to read and write the ID3 header.
	public:

		const static size_t o_id     = 0;
		const static size_t o_vMajor = 3;
		const static size_t o_vMinor = 4;
		const static size_t o_flags  = 5;
		const static size_t o_size   = 6;

		const static size_t kID3_TagHeaderSize = 10;	// This is the same in v2.2, v2.3, and v2.4.
		char fields [kID3_TagHeaderSize];

		~ID3Header() {};

		// Read the v2 header into the fields buffer and check the version.
		bool read ( XMP_IO* file );
		
		// Set the size and write the the v2 header from the fields buffer.
		void write ( XMP_IO* file, XMP_Int64 tagSize );

	};

	// =============================================================================================

	class ID3v2Frame {
	public:
	
		// Applies to ID3 v2.2, v2.3, and v2.4. The metadata values are mostly the same, v2.2 has
		// smaller frame headers and only supports UTF-16 Unicode.

		const static XMP_Uns16 o_id = 0;
		const static XMP_Uns16 o_size = 4; // size after unsync, excludes frame header.
		const static XMP_Uns16 o_flags = 8;

		const static int kV23_FrameHeaderSize = 10;	// The header for v2.3 and v2.4.
		const static int kV22_FrameHeaderSize = 6;	// The header for v2.2.
		char fields [kV23_FrameHeaderSize];

		XMP_Uns32 id;
		XMP_Uns16 flags;
		
		char* content;
		XMP_Int32 contentSize; // size of variable content, right as its stored in o_size

		bool active; //default: true. flag is lowered, if another frame with replaces this one as "last meaningful frame of its kind"
		bool changed; //default: false. flag is raised, if setString() is used

		ID3v2Frame();
		ID3v2Frame ( XMP_Uns32 id );
		
		ID3v2Frame ( const ID3v2Frame& orig ) {
			XMP_Throw ( "ID3v2Frame copy constructor not implemented", kXMPErr_InternalFailure );
		}
		
		~ID3v2Frame() { this->release(); }

		void release();
		
		void setFrameValue ( const std::string& rawvalue, bool needDescriptor = false,
							 bool utf16 = false, bool isXMPPRIVFrame = false, bool needEncodingByte = true );
		
		XMP_Int64 read ( XMP_IO* file, XMP_Uns8 majorVersion );
		void write ( XMP_IO* file, XMP_Uns8 majorVersion );

		// two types of COMM frames should be preserved but otherwise ignored
		// * a 6-field long header just having
		//      encoding(1 byte),lang(3 bytes) and 0x00 31 (no descriptor, "1" as content")
		//      perhaps only used to indicate client language
		// * COMM frames whose description begins with engiTun, these are iTunes flags
		//
		// returns true: job done as expted
		//         false: do not use this frame, but preserve (i.e. iTunes marker COMM frame)
		bool advancePastCOMMDescriptor ( XMP_Int32& pos );

		// returns the frame content as a proper UTF8 string
		//    * w/o the initial encoding byte
		//    * dealing with BOM's
		//
		// @returns: by value: character string with the value
		//			as return value: false if frame is "not of intereset" despite a generally
		//                            "interesting" frame ID, these are
		//                                * iTunes-'marker' COMM frame
		bool getFrameValue ( XMP_Uns8 majorVersion, XMP_Uns32 logicalID, std::string* utf8string );

	};

	// =============================================================================================

	class ID3v1Tag {	// Support for the fixed length v1 tag found at the end of the file.
	public:

		const static XMP_Uns16 o_tag     =   0; // must be "TAG"
		const static XMP_Uns16 o_title   =   3;
		const static XMP_Uns16 o_artist  =  33;
		const static XMP_Uns16 o_album   =  63;
		const static XMP_Uns16 o_year    =  93;
		const static XMP_Uns16 o_comment =  97;
		const static XMP_Uns16 o_zero    = 125; // must be zero for trackNo to be valid
		const static XMP_Uns16 o_trackNo = 126; // trackNo
		const static XMP_Uns16 o_genre   = 127; // last byte: index, or 255

		const static int kV1_TagSize = 128;

		bool read ( XMP_IO* file, SXMPMeta* meta );
		void write ( XMP_IO* file, SXMPMeta* meta );

	};

}

#endif	// __ID3_Support_hpp__
