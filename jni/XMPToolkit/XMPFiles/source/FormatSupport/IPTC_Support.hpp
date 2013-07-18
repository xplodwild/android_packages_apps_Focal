#ifndef __IPTC_Support_hpp__
#define __IPTC_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include <map>

#include "public/include/XMP_Const.h"
#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/EndianUtils.hpp"

// =================================================================================================
/// \file IPTC_Support.hpp
/// \brief XMPFiles support for IPTC (IIM) DataSets.
///
/// This header provides IPTC (IIM) DataSet support specific to the needs of XMPFiles. This is not
/// intended for general purpose IPTC processing. There is a small tree of derived classes, 1
/// virtual base class and 2 concrete leaf classes:
/// \code
/// 	IPTC_Manager - The root virtual base class.
///			IPTC_Reader - A derived concrete leaf class for memory-based read-only access.
///			IPTC_Writer - A derived concrete leaf class for memory-based read-write access.
/// \endcode
///
/// \c IPTC_Manager declares all of the public methods except for specialized constructors in the
/// leaf classes. The read-only classes throw an XMP_Error exception for output methods like
/// \c SetDataSet. They return appropriate values for "safe" methods, \c IsChanged will return false
/// for example.
///
/// The IPTC DataSet organization differs from TIFF tags and Photoshop image resources in allowing
/// muultiple occurrences for some IDs. The C++ STL multimap is a natural data structure for IPTC.
///
/// Support is only provided for DataSet 1:90 to decide if local or UTF-8 text encoding is used, and
/// the following text valued DataSets: 2:05, 2:10, 2:15, 2:20, 2:25, 2:40, 2:55, 2:80, 2:85, 2:90,
/// 2:95, 2:101, 2:103, 2:105, 2:110, 2:115, 2:116, 2:120, and 2:122. DataSet 2:00 is ignored when
/// reading but always written.
///
/// \note Unlike the TIFF_Manager and PSIR_Manager class trees, IPTC_Manager only provides in-memory
/// implementations. The total size of IPTC data is small enough to make this reasonable.
///
/// \note These classes are for use only when directly compiled and linked. They should not be
/// packaged in a DLL by themselves. They do not provide any form of C++ ABI protection.
// =================================================================================================


// =================================================================================================
// =================================================================================================

enum {	// List of recognized 2:* IIM DataSets. The names are from IIMv4 and IPTC4XMP.
	kIPTC_ObjectType        =   3,
	kIPTC_IntellectualGenre =   4,
	kIPTC_Title             =   5,
	kIPTC_EditStatus        =   7,
	kIPTC_EditorialUpdate   =   8,
	kIPTC_Urgency           =  10,
	kIPTC_SubjectCode       =  12,
	kIPTC_Category          =  15,
	kIPTC_SuppCategory      =  20,
	kIPTC_FixtureIdentifier =  22,
	kIPTC_Keyword           =  25,
	kIPTC_ContentLocCode    =  26,
	kIPTC_ContentLocName    =  27,
	kIPTC_ReleaseDate       =  30,
	kIPTC_ReleaseTime       =  35,
	kIPTC_ExpDate           =  37,
	kIPTC_ExpTime           =  38,
	kIPTC_Instructions      =  40,
	kIPTC_ActionAdvised     =  42,
	kIPTC_RefService        =  45,
	kIPTC_RefDate           =  47,
	kIPTC_RefNumber         =  50,
	kIPTC_DateCreated       =  55,
	kIPTC_TimeCreated       =  60,
	kIPTC_DigitalCreateDate =  62,
	kIPTC_DigitalCreateTime =  63,
	kIPTC_OriginProgram     =  65,
	kIPTC_ProgramVersion    =  70,
	kIPTC_ObjectCycle       =  75,
	kIPTC_Creator           =  80,
	kIPTC_CreatorJobtitle   =  85,
	kIPTC_City              =  90,
	kIPTC_Location          =  92,
	kIPTC_State             =  95,
	kIPTC_CountryCode       = 100,
	kIPTC_Country           = 101,
	kIPTC_JobID             = 103,
	kIPTC_Headline          = 105,
	kIPTC_Provider          = 110,
	kIPTC_Source            = 115,
	kIPTC_CopyrightNotice   = 116,
	kIPTC_Contact           = 118,
	kIPTC_Description       = 120,
	kIPTC_DescriptionWriter = 122,
	kIPTC_RasterizedCaption = 125,
	kIPTC_ImageType         = 130,
	kIPTC_ImageOrientation  = 131,
	kIPTC_LanguageID        = 135,
	kIPTC_AudioType         = 150,
	kIPTC_AudioSampleRate   = 151,
	kIPTC_AudioSampleRes    = 152,
	kIPTC_AudioDuration     = 153,
	kIPTC_AudioOutcue       = 154,
	kIPTC_PreviewFormat     = 200,
	kIPTC_PreviewFormatVers = 201,
	kIPTC_PreviewData       = 202
};

enum {	// Forms of mapping legacy IPTC to XMP. Order is significant, see PhotoDataUtils::Import2WayIPTC!
	kIPTC_MapSimple,	// The XMP is simple, the last DataSet occurrence is kept.
	kIPTC_MapLangAlt,	// The XMP is a LangAlt x-default item, the last DataSet occurrence is kept.
	kIPTC_MapArray,		// The XMP is an unordered array, all DataSets are kept.
	kIPTC_MapSpecial,	// The mapping requires DataSet specific code.
	kIPTC_Map3Way,		// Has a 3 way mapping between Exif, IPTC, and XMP. 
	kIPTC_UnmappedText,	// A text DataSet that is not mapped to XMP.
	kIPTC_UnmappedBin	// A binary DataSet that is not mapped to XMP.
};

struct DataSetCharacteristics {
	XMP_Uns8 dsNum;
	XMP_Uns8 mapForm;
	size_t   maxLen;
	XMP_StringPtr xmpNS;
	XMP_StringPtr xmpProp;
};

extern const DataSetCharacteristics kKnownDataSets[];

struct IntellectualGenreMapping {
	XMP_StringPtr refNum;	// The reference number as a 3 digit string.
	XMP_StringPtr name;		// The intellectual genre name.
};

extern const IntellectualGenreMapping kIntellectualGenreMappings[];

// =================================================================================================
// IPTC_Manager
// ============

class IPTC_Manager {
public:

	// ---------------------------------------------------------------------------------------------
	// Types and constants.

	struct DataSetInfo {
		XMP_Uns8   recNum, dsNum;
		XMP_Uns32  dataLen;
		XMP_Uns8 * dataPtr;	// ! The data is read-only. Raw data pointer, beware of character encoding.
		DataSetInfo() : recNum(0), dsNum(0), dataLen(0), dataPtr(0) {};
		DataSetInfo ( XMP_Uns8 _recNum, XMP_Uns8 _dsNum, XMP_Uns32 _dataLen, XMP_Uns8 * _dataPtr )
			: recNum(_recNum), dsNum(_dsNum), dataLen(_dataLen), dataPtr(_dataPtr) {};
	};

	// ---------------------------------------------------------------------------------------------
	// Parse a binary IPTC (IIM) block.
	
	void ParseMemoryDataSets ( const void* data, XMP_Uns32 length, bool copyData = true );
	
	// ---------------------------------------------------------------------------------------------
	// Get the information about a 2:xx DataSet. Returns the number of occurrences. The "which"
	// parameter selects the occurrence, they are numbered from 0 to count-1. Returns 0 if which is
	// too large.

	size_t GetDataSet ( XMP_Uns8 dsNum, DataSetInfo* info, size_t which = 0 ) const;
	
	// ---------------------------------------------------------------------------------------------
	// Get the value of a text 2:xx DataSet as UTF-8. The returned pointer must be treated as
	// read-only. Calls GetDataSet then does a local to UTF-8 conversion if necessary.
	
	size_t GetDataSet_UTF8 ( XMP_Uns8 dsNum, std::string * utf8Str, size_t which = 0 ) const;
	
	// ---------------------------------------------------------------------------------------------
	// Set the value of a text 2:xx DataSet from a UTF-8 string. Does a UTF-8 to local conversion if
	// necessary. If the encoding mode is currently local and this value has round-trip loss, then
	// the encoding mode will be changed to UTF-8 and all existing values will be converted.
	// Modifies an existing occurrence if "which" is within range. Adds an occurrence if which
	// equals the current count, or which is -1 and repeats are allowed. Throws an exception if
	// which is too large. The dataPtr provides the raw data, text must be in the right encoding.

	virtual void SetDataSet_UTF8 ( XMP_Uns8 dsNum, const void* utf8Ptr, XMP_Uns32 utf8Len, long which = -1 ) = 0;
	
	// ---------------------------------------------------------------------------------------------
	// Delete an existing 2:xx DataSet. Deletes all occurrences if which is -1.

	virtual void DeleteDataSet ( XMP_Uns8 dsNum, long which = -1 ) = 0;

	// ---------------------------------------------------------------------------------------------
	// Determine if any 2:xx DataSets are changed.

	virtual bool IsChanged() = 0;

	// ---------------------------------------------------------------------------------------------
	// Determine if UTF-8 or local text encoding is being used.
	
	bool UsingUTF8() const { return this->utf8Encoding; };

	// --------------------------------------------------
	// Update all DataSets to reflect the changed values.

	virtual void UpdateMemoryDataSets() = 0;

	// ---------------------------------------------------------------------------------------------
	// Get the location and size of the full IPTC block. The client must call UpdateMemoryDataSets
	// first if appropriate. The returned dataPtr must be treated as read only. It exists until the
	// IPTC_Manager destructor is called.

	XMP_Uns32 GetBlockInfo ( void** dataPtr ) const
		{ if ( dataPtr != 0 ) *dataPtr = this->iptcContent; return this->iptcLength; };

	// ---------------------------------------------------------------------------------------------

	virtual ~IPTC_Manager() { if ( this->ownedContent ) free ( this->iptcContent ); };

protected:
	
	enum { kMinDataSetSize = 5 };	// 1+1+1+2
	
	typedef std::multimap<XMP_Uns16,DataSetInfo>  DataSetMap;
	DataSetMap dataSets;	// ! All datasets are in the map, key is (record*1000 + dataset).

	XMP_Uns8* iptcContent;
	XMP_Uns32 iptcLength;

	bool changed;
	bool ownedContent;	// True if IPTC_Manager destructor needs to release the content block.
	bool utf8Encoding;	// True if text values are encoded as UTF-8.

	IPTC_Manager() : iptcContent(0), iptcLength(0),
					 changed(false), ownedContent(false), utf8Encoding(false) {};
	
	void DisposeLooseValue ( DataSetInfo & dsInfo );
	XMP_Uns8* AppendDataSet ( XMP_Uns8* dsPtr, const DataSetInfo & dsInfo );

};	// IPTC_Manager


// =================================================================================================
// =================================================================================================


// =================================================================================================
// IPTC_Reader
// ===========

class IPTC_Reader : public IPTC_Manager {
public:

	IPTC_Reader() {};
		
	void SetDataSet_UTF8 ( XMP_Uns8 dsNum, const void* utf8Ptr, XMP_Uns32 utf8Len, long which = -1 ) { NotAppropriate(); };
	
	void DeleteDataSet ( XMP_Uns8 dsNum, long which = -1 ) { NotAppropriate(); };

	bool IsChanged() { return false; };
	
	void UpdateMemoryDataSets() { NotAppropriate(); };

	virtual ~IPTC_Reader() {};
	
private:

	static inline void NotAppropriate() { XMP_Throw ( "Not appropriate for IPTC_Reader", kXMPErr_InternalFailure ); };

};	// IPTC_Reader

// =================================================================================================
// IPTC_Writer
// ===========

class IPTC_Writer : public IPTC_Manager {
public:
		
	void SetDataSet_UTF8 ( XMP_Uns8 dsNum, const void* utf8Ptr, XMP_Uns32 utf8Len, long which = -1 );
	
	void DeleteDataSet ( XMP_Uns8 dsNum, long which = -1 );

	bool IsChanged() { return changed; };
	
	void UpdateMemoryDataSets ();

	IPTC_Writer() {};

	virtual ~IPTC_Writer();

private:

	void ConvertToUTF8();
	void ConvertToLocal();

	bool CheckRoundTripLoss();

};	// IPTC_Writer

// =================================================================================================

#endif	// __IPTC_Support_hpp__
