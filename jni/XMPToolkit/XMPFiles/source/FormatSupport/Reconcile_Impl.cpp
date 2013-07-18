// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/UnicodeConversions.hpp"
#include "source/XIO.hpp"

#if XMP_WinBuild
#elif XMP_MacBuild
	#include <CoreServices/CoreServices.h>
#endif

// =================================================================================================
/// \file Reconcile_Impl.cpp
/// \brief Implementation utilities for the photo metadata reconciliation support.
///
// =================================================================================================

// =================================================================================================
// ReconcileUtils::IsASCII
// =======================
//
// See if a string is 7 bit ASCII.

bool ReconcileUtils::IsASCII ( const void * textPtr, size_t textLen )
{
	
	for ( const XMP_Uns8 * textPos = (XMP_Uns8*)textPtr; textLen > 0; --textLen, ++textPos ) {
		if ( *textPos >= 0x80 ) return false;
	}
	
	return true;

}	// ReconcileUtils::IsASCII

// =================================================================================================
// ReconcileUtils::IsUTF8
// ======================
//
// See if a string contains valid UTF-8. Allow nul bytes, they can appear inside of multi-part Exif
// strings. We don't use CodePoint_from_UTF8_Multi in UnicodeConversions because it throws an
// exception for non-Unicode and we don't need to actually compute the code points.

bool ReconcileUtils::IsUTF8 ( const void * textPtr, size_t textLen )
{
	const XMP_Uns8 * textPos = (XMP_Uns8*)textPtr;
	const XMP_Uns8 * textEnd = textPos + textLen;
	
	while ( textPos < textEnd ) {
	
		if ( *textPos < 0x80 ) {
		
			++textPos;	// ASCII is UTF-8, tolerate nuls.
		
		} else {
		
			// -------------------------------------------------------------------------------------
			// We've got a multibyte UTF-8 character. The first byte has the number of bytes as the
			// number of high order 1 bits. The remaining bytes must have 1 and 0 as the top 2 bits.
	
			#if 0	// *** This might be a more effcient way to count the bytes.
				static XMP_Uns8 kByteCounts[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 3, 4 };
				size_t bytesNeeded = kByteCounts [ *textPos >> 4 ];
				if ( (bytesNeeded < 2) || ((bytesNeeded == 4) && ((*textPos & 0x08) != 0)) ) return false;
				if ( (textPos + bytesNeeded) > textEnd ) return false;
			#endif
		
			size_t bytesNeeded = 0;	// Count the high order 1 bits in the first byte.
			for ( XMP_Uns8 temp = *textPos; temp > 0x7F; temp = temp << 1 ) ++bytesNeeded;
				// *** Consider CPU-specific assembly inline, e.g. cntlzw on PowerPC.
			
			if ( (bytesNeeded < 2) || (bytesNeeded > 4) || ((textPos+bytesNeeded) > textEnd) ) return false;
			
			for ( --bytesNeeded, ++textPos; bytesNeeded > 0; --bytesNeeded, ++textPos ) {
				if ( (*textPos >> 6) != 2 ) return false;
			}
		
		}
	
	}
	
	return true;	// ! Returns true for empty strings.

}	// ReconcileUtils::IsUTF8

// =================================================================================================
// UTF8ToHostEncoding
// ==================

#if XMP_WinBuild

	void ReconcileUtils::UTF8ToWinEncoding ( UINT codePage, const XMP_Uns8 * utf8Ptr, size_t utf8Len, std::string * host )
	{
	
		std::string utf16;	// WideCharToMultiByte wants native UTF-16.
		ToUTF16Native ( (UTF8Unit*)utf8Ptr, utf8Len, &utf16 );
	
		LPCWSTR utf16Ptr = (LPCWSTR) utf16.c_str();
		size_t  utf16Len = utf16.size() / 2;
	
		int hostLen = WideCharToMultiByte ( codePage, 0, utf16Ptr, (int)utf16Len, 0, 0, 0, 0 );
		host->assign ( hostLen, ' ' );	// Allocate space for the results.
	
		(void) WideCharToMultiByte ( codePage, 0, utf16Ptr, (int)utf16Len, (LPSTR)host->data(), hostLen, 0, 0 );
		XMP_Assert ( hostLen == host->size() );
	
	}	// UTF8ToWinEncoding

#elif XMP_MacBuild

	void ReconcileUtils::UTF8ToMacEncoding ( XMP_Uns16 macScript, XMP_Uns16 macLang, const XMP_Uns8 * utf8Ptr, size_t utf8Len, std::string * host )
	{
		OSStatus err;
		
		TextEncoding destEncoding;
		if ( macLang == langUnspecified ) macLang = kTextLanguageDontCare;
		err = UpgradeScriptInfoToTextEncoding ( macScript, macLang, kTextRegionDontCare, 0, &destEncoding );
		if ( err != noErr ) XMP_Throw ( "UpgradeScriptInfoToTextEncoding failed", kXMPErr_ExternalFailure );
		
		UnicodeMapping mappingInfo;
		mappingInfo.mappingVersion  = kUnicodeUseLatestMapping;
		mappingInfo.otherEncoding   = GetTextEncodingBase ( destEncoding );
		mappingInfo.unicodeEncoding = CreateTextEncoding ( kTextEncodingUnicodeDefault,
														   kUnicodeNoSubset, kUnicodeUTF8Format );
		
		UnicodeToTextInfo converterInfo;
		err = CreateUnicodeToTextInfo ( &mappingInfo, &converterInfo );
		if ( err != noErr ) XMP_Throw ( "CreateUnicodeToTextInfo failed", kXMPErr_ExternalFailure );
		
		try {	// ! Need to call DisposeUnicodeToTextInfo before exiting.
		
			OptionBits convFlags = kUnicodeUseFallbacksMask | 
								   kUnicodeLooseMappingsMask |  kUnicodeDefaultDirectionMask;
			ByteCount bytesRead, bytesWritten;
	
			enum { kBufferLen = 1000 };	// Ought to be enough in practice, without using too much stack.
			char buffer [kBufferLen];
			
			host->reserve ( utf8Len );	// As good a guess as any.
		
			while ( utf8Len > 0 ) {
				// Ignore all errors from ConvertFromUnicodeToText. It returns info like "output 
				// buffer full" or "use substitution" as errors.
				err = ConvertFromUnicodeToText ( converterInfo, utf8Len, (UniChar*)utf8Ptr, convFlags,
												 0, 0, 0, 0, kBufferLen, &bytesRead, &bytesWritten, buffer );
				if ( bytesRead == 0 ) break;	// Make sure forward progress happens.
				host->append ( &buffer[0], bytesWritten );
				utf8Ptr += bytesRead;
				utf8Len -= bytesRead;
			}
		
			DisposeUnicodeToTextInfo ( &converterInfo );
	
		} catch ( ... ) {
	
			DisposeUnicodeToTextInfo ( &converterInfo );
			throw;
	
		}
	
	}	// UTF8ToMacEncoding

#elif XMP_UNIXBuild

	// ! Does not exist, must not be called, for Generic UNIX builds.

#endif

// =================================================================================================
// ReconcileUtils::UTF8ToLocal
// ===========================

void ReconcileUtils::UTF8ToLocal ( const void * _utf8Ptr, size_t utf8Len, std::string * local )
{
	const XMP_Uns8* utf8Ptr = (XMP_Uns8*)_utf8Ptr;

	local->erase();
	
	if ( ReconcileUtils::IsASCII ( utf8Ptr, utf8Len ) ) {
		local->assign ( (const char *)utf8Ptr, utf8Len );
		return;
	}
	
	#if XMP_WinBuild
	
		UTF8ToWinEncoding ( CP_ACP, utf8Ptr, utf8Len, local );
	
	#elif XMP_MacBuild
	
		UTF8ToMacEncoding ( smSystemScript, kTextLanguageDontCare, utf8Ptr, utf8Len, local );
	
	#elif XMP_UNIXBuild
	
		XMP_Throw ( "Generic UNIX does not have conversions between local and Unicode", kXMPErr_Unavailable );
	
	#endif

}	// ReconcileUtils::UTF8ToLocal

// =================================================================================================
// ReconcileUtils::UTF8ToLatin1
// ============================

void ReconcileUtils::UTF8ToLatin1 ( const void * _utf8Ptr, size_t utf8Len, std::string * latin1 )
{
	const XMP_Uns8* utf8Ptr = (XMP_Uns8*)_utf8Ptr;
	const XMP_Uns8* utf8End = utf8Ptr + utf8Len;

	latin1->erase();
	latin1->reserve ( utf8Len );	// As good a guess as any, at least enough, exact for ASCII.
	
	bool inBadRun = false;
	
	while ( utf8Ptr < utf8End ) {
	
		if ( *utf8Ptr <= 0x7F ) {

			(*latin1) += (char)*utf8Ptr;	// Have an ASCII character.
			inBadRun = false;
			++utf8Ptr;

		} else if ( utf8Ptr == (utf8End - 1) ) {

			inBadRun = false;
			++utf8Ptr;	// Ignore a bad end to the UTF-8.

		} else {

			XMP_Assert ( (utf8End - utf8Ptr) >= 2 );
			XMP_Uns16 ch16 = GetUns16BE ( utf8Ptr );	// A Latin-1 80..FF is 2 UTF-8 bytes.

			if ( (0xC280 <= ch16) && (ch16 <= 0xC2BF) ) {

				(*latin1) += (char)(ch16 & 0xFF);	// UTF-8 C280..C2BF are Latin-1 80..BF.
				inBadRun = false;
				utf8Ptr += 2;

			} else if ( (0xC380 <= ch16) && (ch16 <= 0xC3BF) ) {

				(*latin1) += (char)((ch16 & 0xFF) + 0x40);	// UTF-8 C380..C3BF are Latin-1 C0..FF.
				inBadRun = false;
				utf8Ptr += 2;

			} else {

				if ( ! inBadRun ) {
					inBadRun = true;
					(*latin1) += "(?)";	// Mark the run of out of scope UTF-8.
				}
				
				++utf8Ptr;	// Skip the presumably well-formed UTF-8 character.
				while ( (utf8Ptr < utf8End) && ((*utf8Ptr & 0xC0) == 0x80) ) ++utf8Ptr;

			}

		}
	
	}
	
	XMP_Assert ( utf8Ptr == utf8End );

}	// ReconcileUtils::UTF8ToLatin1

// =================================================================================================
// HostEncodingToUTF8
// ==================

#if XMP_WinBuild

	void ReconcileUtils::WinEncodingToUTF8 ( UINT codePage, const XMP_Uns8 * hostPtr, size_t hostLen, std::string * utf8 )
	{

		int utf16Len = MultiByteToWideChar ( codePage, 0, (LPCSTR)hostPtr, (int)hostLen, 0, 0 );
		std::vector<UTF16Unit> utf16 ( utf16Len, 0 );	// MultiByteToWideChar returns native UTF-16.

		(void) MultiByteToWideChar ( codePage, 0, (LPCSTR)hostPtr, (int)hostLen, (LPWSTR)&utf16[0], utf16Len );
		FromUTF16Native ( &utf16[0], (int)utf16Len, utf8 );
	
	}	// WinEncodingToUTF8

#elif XMP_MacBuild

	void ReconcileUtils::MacEncodingToUTF8 ( XMP_Uns16 macScript, XMP_Uns16 macLang, const XMP_Uns8 * hostPtr, size_t hostLen, std::string * utf8 )
	{
		OSStatus err;
		
		TextEncoding srcEncoding;
		if ( macLang == langUnspecified ) macLang = kTextLanguageDontCare;
		err = UpgradeScriptInfoToTextEncoding ( macScript, macLang, kTextRegionDontCare, 0, &srcEncoding );
		if ( err != noErr ) XMP_Throw ( "UpgradeScriptInfoToTextEncoding failed", kXMPErr_ExternalFailure );
		
		UnicodeMapping mappingInfo;
		mappingInfo.mappingVersion  = kUnicodeUseLatestMapping;
		mappingInfo.otherEncoding   = GetTextEncodingBase ( srcEncoding );
		mappingInfo.unicodeEncoding = CreateTextEncoding ( kTextEncodingUnicodeDefault,
														   kUnicodeNoSubset, kUnicodeUTF8Format );
		
		TextToUnicodeInfo converterInfo;
		err = CreateTextToUnicodeInfo ( &mappingInfo, &converterInfo );
		if ( err != noErr ) XMP_Throw ( "CreateTextToUnicodeInfo failed", kXMPErr_ExternalFailure );
		
		try {	// ! Need to call DisposeTextToUnicodeInfo before exiting.
		
			ByteCount bytesRead, bytesWritten;

			enum { kBufferLen = 1000 };	// Ought to be enough in practice, without using too much stack.
			char buffer [kBufferLen];
			
			utf8->reserve ( hostLen );	// As good a guess as any.
		
			while ( hostLen > 0 ) {
				// Ignore all errors from ConvertFromTextToUnicode. It returns info like "output 
				// buffer full" or "use substitution" as errors.
				err = ConvertFromTextToUnicode ( converterInfo, hostLen, hostPtr, kNilOptions,
												 0, 0, 0, 0, kBufferLen, &bytesRead, &bytesWritten, (UniChar*)buffer );
				if ( bytesRead == 0 ) break;	// Make sure forward progress happens.
				utf8->append ( &buffer[0], bytesWritten );
				hostPtr += bytesRead;
				hostLen -= bytesRead;
			}
		
			DisposeTextToUnicodeInfo ( &converterInfo );

		} catch ( ... ) {

			DisposeTextToUnicodeInfo ( &converterInfo );
			throw;

		}
	
	}	// MacEncodingToUTF8

#elif XMP_UNIXBuild

	// ! Does not exist, must not be called, for Generic UNIX builds.

#endif

// =================================================================================================
// ReconcileUtils::LocalToUTF8
// ===========================

void ReconcileUtils::LocalToUTF8 ( const void * _localPtr, size_t localLen, std::string * utf8 )
{
	const XMP_Uns8* localPtr = (XMP_Uns8*)_localPtr;

	utf8->erase();
	
	if ( ReconcileUtils::IsASCII ( localPtr, localLen ) ) {
		utf8->assign ( (const char *)localPtr, localLen );
		return;
	} 
	
	#if XMP_WinBuild

		WinEncodingToUTF8 ( CP_ACP, localPtr, localLen, utf8 );
		
	#elif XMP_MacBuild
	
		MacEncodingToUTF8 ( smSystemScript, kTextLanguageDontCare, localPtr, localLen, utf8 );

	#elif XMP_UNIXBuild
	
		XMP_Throw ( "Generic UNIX does not have conversions between local and Unicode", kXMPErr_Unavailable );
	
	#endif

}	// ReconcileUtils::LocalToUTF8

// =================================================================================================
// ReconcileUtils::Latin1ToUTF8
// ============================

void ReconcileUtils::Latin1ToUTF8 ( const void * _latin1Ptr, size_t latin1Len, std::string * utf8 )
{
	const XMP_Uns8* latin1Ptr = (XMP_Uns8*)_latin1Ptr;
	const XMP_Uns8* latin1End = latin1Ptr + latin1Len;

	utf8->erase();
	utf8->reserve ( latin1Len );	// As good a guess as any, exact for ASCII.
	
	for ( ; latin1Ptr < latin1End; ++latin1Ptr ) {

		XMP_Uns8 ch8 = *latin1Ptr;
		
		if ( ch8 <= 0x7F ) {
			(*utf8) += (char)ch8;	// Have an ASCII character.
		} else if ( ch8 <= 0xBF ) {
			(*utf8) += 0xC2;	// Latin-1 80..BF are UTF-8 C280..C2BF.
			(*utf8) += (char)ch8;
		} else {
			(*utf8) += 0xC3;	// Latin-1 C0..FF are UTF-8 C380..C3BF.
			(*utf8) += (char)(ch8 - 0x40);
		}
	
	}
	
}	// ReconcileUtils::Latin1ToUTF8


// =================================================================================================
// ReconcileUtils::NativeToUTF8
// ============================

void ReconcileUtils::NativeToUTF8( const std::string & input, std::string & output )
{
	output.erase();
	// IF it is not UTF-8 
	if( ! ReconcileUtils::IsUTF8( input.c_str(), input.length() ) )
	{	
		// And ServerMode is not active
		if( ! ignoreLocalText )
		{
			// Convert it to UTF-8
			ReconcileUtils::LocalToUTF8( input.c_str(), input.length(), &output );
		}
	}
	else // If it is already UTF-8
	{
		output = input;
	}
}	// ReconcileUtils::NativeToUTF8
