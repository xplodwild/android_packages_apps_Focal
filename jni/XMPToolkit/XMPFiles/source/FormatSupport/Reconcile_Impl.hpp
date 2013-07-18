#ifndef __Reconcile_Impl_hpp__
#define __Reconcile_Impl_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "XMPFiles/source/FormatSupport/ReconcileLegacy.hpp"
#include "third-party/zuid/interfaces/MD5.h"

// =================================================================================================
/// \file Reconcile_Impl.hpp
/// \brief Implementation utilities for the legacy metadata reconciliation support.
///
// =================================================================================================

typedef XMP_Uns8 MD5_Digest[16];	// ! Should be in MD5.h.

enum {
	kDigestMissing = -1,	// A partial import is done, existing XMP is left alone.
	kDigestDiffers =  0,	// A full import is done, existing XMP is deleted or replaced.
	kDigestMatches = +1		// No importing is done.
};

namespace ReconcileUtils {

	// *** These ought to be with the Unicode conversions.

	static const char * kHexDigits = "0123456789ABCDEF";

	bool IsASCII      ( const void * _textPtr, size_t textLen );
	bool IsUTF8       ( const void * _textPtr, size_t textLen );

	void UTF8ToLocal  ( const void * _utf8Ptr, size_t utf8Len, std::string * local );
	void UTF8ToLatin1 ( const void * _utf8Ptr, size_t utf8Len, std::string * latin1 );
	void LocalToUTF8  ( const void * _localPtr, size_t localLen, std::string * utf8 );
	void Latin1ToUTF8 ( const void * _latin1Ptr, size_t latin1Len, std::string * utf8 );
	
	// 
	// Checks if the input string is UTF-8 encoded. If not, it tries to convert it to UTF-8
	// This is only done, if Server Mode is not active!
	// @param input the native input string
	// @return The input if it is already UTF-8, the converted input 
	//			or an empty string if no conversion is possible because of ServerMode
	//
	void NativeToUTF8 ( const std::string & input, std::string & output );

	#if XMP_WinBuild
		void UTF8ToWinEncoding ( UINT codePage, const XMP_Uns8 * utf8Ptr, size_t utf8Len, std::string * host );
		void WinEncodingToUTF8 ( UINT codePage, const XMP_Uns8 * hostPtr, size_t hostLen, std::string * utf8 );
	#elif XMP_MacBuild
		void UTF8ToMacEncoding ( XMP_Uns16 macScript, XMP_Uns16 macLang, const XMP_Uns8 * utf8Ptr, size_t utf8Len, std::string * host );
		void MacEncodingToUTF8 ( XMP_Uns16 macScript, XMP_Uns16 macLang, const XMP_Uns8 * hostPtr, size_t hostLen, std::string * utf8 );
	#endif

};	// ReconcileUtils

namespace PhotoDataUtils {

	int CheckIPTCDigest ( const void * newPtr, const XMP_Uns32 newLen, const void * oldDigest );
	void SetIPTCDigest  ( void * iptcPtr, XMP_Uns32 iptcLen, PSIR_Manager * psir );

	bool GetNativeInfo ( const TIFF_Manager & exif, XMP_Uns8 ifd, XMP_Uns16 id, TIFF_Manager::TagInfo * info );
	size_t GetNativeInfo ( const IPTC_Manager & iptc, XMP_Uns8 id, int digestState,
						   bool haveXMP, IPTC_Manager::DataSetInfo * info );
	
	bool IsValueDifferent ( const TIFF_Manager::TagInfo & exifInfo,
							const std::string & xmpValue, std::string * exifValue );
	bool IsValueDifferent ( const IPTC_Manager & newIPTC, const IPTC_Manager & oldIPTC, XMP_Uns8 id );

	void ImportPSIR ( const PSIR_Manager & psir, SXMPMeta * xmp, int iptcDigestState );

	void Import2WayIPTC ( const IPTC_Manager & iptc, SXMPMeta * xmp, int iptcDigestState );
	void Import2WayExif ( const TIFF_Manager & exif, SXMPMeta * xmp, int iptcDigestState );

	void Import3WayItems ( const TIFF_Manager & exif, const IPTC_Manager & iptc, SXMPMeta * xmp, int iptcDigestState );

	void ExportPSIR ( const SXMPMeta & xmp, PSIR_Manager * psir );
	void ExportIPTC ( const SXMPMeta & xmp, IPTC_Manager * iptc );
	void ExportExif ( SXMPMeta * xmp, TIFF_Manager * exif );
	
	// These need to be exposed for use in Import3WayItem:

	void ImportIPTC_Simple ( const IPTC_Manager & iptc, SXMPMeta * xmp,
							 XMP_Uns8 id, const char * xmpNS, const char * xmpProp );
	
	void ImportIPTC_LangAlt ( const IPTC_Manager & iptc, SXMPMeta * xmp,
							  XMP_Uns8 id, const char * xmpNS, const char * xmpProp );
	
	void ImportIPTC_Array ( const IPTC_Manager & iptc, SXMPMeta * xmp,
							XMP_Uns8 id, const char * xmpNS, const char * xmpProp );
	
	void ImportIPTC_Date ( XMP_Uns8 dateID, const IPTC_Manager & iptc, SXMPMeta * xmp );

};	// PhotoDataUtils

#endif	// __Reconcile_Impl_hpp__
