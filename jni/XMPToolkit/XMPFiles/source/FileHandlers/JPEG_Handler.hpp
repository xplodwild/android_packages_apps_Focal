#ifndef __JPEG_Handler_hpp__
#define __JPEG_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! Must be the first #include!

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"

// =================================================================================================
/// \file JPEG_Handler.hpp
/// \brief File format handler for JPEG.
///
/// This header ...
///
// =================================================================================================

// *** Could derive from Basic_Handler - buffer file tail in a temp file.

extern XMPFileHandler * JPEG_MetaHandlerCTor ( XMPFiles * parent );

extern bool JPEG_CheckFormat ( XMP_FileFormat format,
							   XMP_StringPtr  filePath,
                               XMP_IO *       fileRef,
                               XMPFiles *     parent );

static const XMP_OptionBits kJPEG_HandlerFlags = (kXMPFiles_CanInjectXMP |
                                                  kXMPFiles_CanExpand |
                                                  kXMPFiles_CanRewrite |
                                                  kXMPFiles_PrefersInPlace |
                                                  kXMPFiles_CanReconcile |
                                                  kXMPFiles_AllowsOnlyXMP |
                                                  kXMPFiles_ReturnsRawPacket |
                                                  kXMPFiles_AllowsSafeUpdate);

class JPEG_MetaHandler : public XMPFileHandler
{
public:

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile    ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	struct GUID_32 {	// A hack to get an assignment operator for an array.
		char data [32];
		void operator= ( const GUID_32 & in )
		{
			memcpy ( this->data, in.data, sizeof(this->data) );	// AUDIT: Use of sizeof(this->data) is safe.
		};
		bool operator< ( const GUID_32 & right ) const
		{
			return (memcmp ( this->data, right.data, sizeof(this->data) ) < 0);
		};
		bool operator== ( const GUID_32 & right ) const
		{
			return (memcmp ( this->data, right.data, sizeof(this->data) ) == 0);
		};
	};

	JPEG_MetaHandler ( XMPFiles * parent );
	virtual ~JPEG_MetaHandler();

private:

	JPEG_MetaHandler() : exifMgr(0), psirMgr(0), iptcMgr(0), skipReconcile(false) {};	// Hidden on purpose.

	std::string exifContents;
	std::string psirContents;

	TIFF_Manager * exifMgr;	// The Exif manager will be created by ProcessTNail or ProcessXMP.
	PSIR_Manager * psirMgr;	// Need to use pointers so we can properly select between read-only and
	IPTC_Manager * iptcMgr;	//	read-write modes of usage.

	bool skipReconcile;	// ! Used between UpdateFile and WriteFile.

	typedef std::map < GUID_32, std::string > ExtendedXMPMap;

	ExtendedXMPMap extendedXMP;	// ! Only contains those with complete data.
//	void CacheExtendedXMP ( ExtendedXMPInfo * extXMP, XMP_Uns8 * buffer, size_t bufferLen );

};	// JPEG_MetaHandler

// =================================================================================================

#endif /* __JPEG_Handler_hpp__ */
