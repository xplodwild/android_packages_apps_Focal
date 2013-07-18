// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _WAVEReconcile_h_
#define _WAVEReconcile_h_

#include "XMPFiles/source/NativeMetadataSupport/IReconcile.h"

class IMetadata;

namespace IFF_RIFF
{

class INFOMetadata;
class BEXTMetadata;

class WAVEReconcile : public IReconcile
{
public:
	~WAVEReconcile() {};

	/** 
	* @see IReconcile::importToXMP
	* Legacy values are always imported.
	* If the values are not UTF-8 they will be converted to UTF-8 except in ServerMode
	*/
	XMP_Bool importToXMP( SXMPMeta& outXMP, const MetadataSet& inMetaData );

	/** 
	* @see IReconcile::exportFromXMP
	* XMP values are always exported to Legacy as UTF-8 encoded
	*/
	XMP_Bool exportFromXMP( MetadataSet& outMetaData, SXMPMeta& inXMP );

private:
	// Encode a string of raw data bytes into a HexString (w/o spaces, i.e. "DEADBEEF").
	// Only used for UMID Bext field, which has fixed size of 64, so 64 chars are expected in the buffer!
	// No insertation/acceptance of whitespace/linefeeds. No output/tolerance of lowercase.
	// returns true, if *all* characters returned are zero (or if 0 bytes are returned).
	static bool encodeToHexString ( const XMP_Uns8* input, std::string& output );

	/**
	 * Decode a hex string to raw data bytes.
	 * Input must be all uppercase and w/o any whitespace, strictly (0-9A-Z)* (i.e. "DEADBEEF0099AABC")
	 * No insertation/acceptance of whitespace/linefeeds.
	 * bNo use/tolerance of lowercase.
	 * Number of bytes in the encoded String must be even.
	 * returns true if everything went well, false if illegal (non 0-9A-F) character encountered
	 */
	static bool decodeFromHexString ( std::string input, std::string &output);

	/**
	* convert a 4 character string to XPM_Uns32 (FOURCC)
	*/
	static bool stringToFOURCC ( std::string input, XMP_Uns32 &output );
};

}

#endif
