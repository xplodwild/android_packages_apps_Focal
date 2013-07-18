#ifndef __QuickTime_Support_hpp__
#define __QuickTime_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include <string>
#include <vector>
#include <map>

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/FormatSupport/ISOBaseMedia_Support.hpp"
#include "XMPFiles/source/FormatSupport/MOOV_Support.hpp"

// =================================================================================================
// =================================================================================================

// =================================================================================================
// TradQT_Manager
// ==============

// Support for selected traditional QuickTime metadata items. The supported items are the children
// of the 'moov'/'udta' box whose type begins with 0xA9, a MacRoman copyright symbol. Each of these
// is a box whose contents are a sequence of "mini boxes" analogous to XMP AltText arrays. Each mini
// box has a 16-bit size, 16-bit language code, and text. The language code values are the old
// Macintosh Script Manager langXyz codes, the text encoding is implicit, see Mac Script.h.

enum {	// List of recognized items from the QuickTime 'moov'/'udta' box.
	// These items are defined by Adobe.
	kQTilst_Reel        = 0xA952454CUL, // '©REL'
	kQTilst_Timecode    = 0xA954494DUL, // '©TIM'
	kQTilst_TimeScale   = 0xA9545343UL, // '©TSC'
	kQTilst_TimeSize    = 0xA954535AUL  // '©TSZ'
};

enum {
	kNoMacLang   = 0xFFFF,
	kNoMacScript = 0xFFFF
};

extern bool ConvertToMacLang ( const std::string & utf8Value, XMP_Uns16 macLang, std::string * macValue );
extern bool ConvertFromMacLang ( const std::string & macValue, XMP_Uns16 macLang, std::string * utf8Value );

class TradQT_Manager {
public:

	TradQT_Manager() : changed(false) {};

	bool ParseCachedBoxes ( const MOOV_Manager & moovMgr );

	bool ImportSimpleXMP  ( XMP_Uns32 id, SXMPMeta * xmp, XMP_StringPtr ns, XMP_StringPtr prop ) const;
	bool ImportLangAltXMP ( XMP_Uns32 id, SXMPMeta * xmp, XMP_StringPtr ns, XMP_StringPtr langArray ) const;

	void ExportSimpleXMP  ( XMP_Uns32 id, const SXMPMeta & xmp, XMP_StringPtr ns, XMP_StringPtr prop,
							bool createWithZeroLang = false );
	void ExportLangAltXMP ( XMP_Uns32 id, const SXMPMeta & xmp, XMP_StringPtr ns, XMP_StringPtr langArray );

	bool IsChanged() const { return this->changed; };

	void UpdateChangedBoxes ( MOOV_Manager * moovMgr );

private:

	struct ValueInfo {
		bool marked;
		XMP_Uns16 macLang;
		XMP_StringPtr xmpLang;	// ! Only set if macLang is known, i.e. the value can be converted.
		std::string   macValue;
		ValueInfo() : marked(false), macLang(kNoMacLang), xmpLang("") {};
	};
	typedef std::vector<ValueInfo> ValueVector;
	typedef ValueVector::iterator ValueInfoPos;
	typedef ValueVector::const_iterator ValueInfoCPos;

	struct ParsedBoxInfo {
		XMP_Uns32 id;
		ValueVector values;
		bool changed;
		ParsedBoxInfo() : id(0), changed(false) {};
		ParsedBoxInfo ( XMP_Uns32 _id ) : id(_id), changed(false) {};
	};

	typedef std::map < XMP_Uns32, ParsedBoxInfo > InfoMap;	// Metadata item kind and content info.
	typedef InfoMap::iterator InfoMapPos;
	typedef InfoMap::const_iterator InfoMapCPos;

	InfoMap parsedBoxes;
	bool changed;

	bool ImportLangItem ( const ValueInfo & qtItem, SXMPMeta * xmp, XMP_StringPtr ns, XMP_StringPtr langArray ) const;

};	// TradQT_Manager

#endif	// __QuickTime_Support_hpp__
