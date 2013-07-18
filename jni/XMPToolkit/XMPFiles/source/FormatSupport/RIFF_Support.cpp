// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
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

// must have access to handler class fields...
#include "XMPFiles/source/FormatSupport/RIFF.hpp"
#include "XMPFiles/source/FileHandlers/RIFF_Handler.hpp"
#include "XMPFiles/source/FormatSupport/RIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

#define MIN(a, b)       ((a) < (b) ? (a) : (b))

using namespace RIFF;
namespace RIFF {

// The minimum BEXT chunk size should be 610 (incl. 8 byte header/size field)
XMP_Int32 MIN_BEXT_SIZE = 610; // = > 8 + ( 256+32+32+10+8+4+4+2+64+190+0 )

// An assumed secure max value of 100 MB.
XMP_Int32 MAX_BEXT_SIZE = 100 * 1024 * 1024;

// CR8R, PrmL have fixed sizes
XMP_Int32 CR8R_SIZE = 0x5C;
XMP_Int32 PRML_SIZE = 0x122;

static const char* sHexChars =    "0123456789ABCDEF";

// Encode a string of raw data bytes into a HexString (w/o spaces, i.e. "DEADBEEF").
// No insertation/acceptance of whitespace/linefeeds. No output/tolerance of lowercase.
// returns true, if *all* characters returned are zero (or if 0 bytes are returned).
static bool EncodeToHexString ( XMP_StringPtr   rawStr,
						      XMP_StringLen   rawLen,
						      std::string* encodedStr )
{
	bool allZero = true; // assume for now

	if ( (rawStr == 0) && (rawLen != 0) )
		XMP_Throw ( "EncodeToHexString: null rawStr", kXMPErr_BadParam );
	if ( encodedStr == 0 )
		XMP_Throw ( "EncodeToHexString: null encodedStr", kXMPErr_BadParam );

	encodedStr->erase();
	if ( rawLen == 0 ) return allZero;
	encodedStr->reserve ( rawLen * 2 );

	for( XMP_Uns32 i = 0; i < rawLen; i++ )
	{
		// first, second nibble
		XMP_Uns8 first = rawStr[i] >> 4;
		XMP_Uns8 second = rawStr[i] & 0xF;

		if ( allZero && (( first != 0 ) || (second != 0)))
			allZero = false;

		encodedStr->append( 1, sHexChars[first] );
		encodedStr->append( 1, sHexChars[second] );
	}

	return allZero;
}	// EncodeToHexString

// -------------------------------------------------------------------------------------------------
// DecodeFromHexString
// ----------------
//
// Decode a hex string to raw data bytes.
// * Input must be all uppercase and w/o any whitespace, strictly (0-9A-Z)* (i.e. "DEADBEEF0099AABC")
// * No insertation/acceptance of whitespace/linefeeds.
// * bNo use/tolerance of lowercase.
// * Number of bytes in the encoded String must be even.
// * returns true if everything went well, false if illegal (non 0-9A-F) character encountered

static bool DecodeFromHexString ( XMP_StringPtr  encodedStr,
							    XMP_StringLen  encodedLen,
							    std::string*   rawStr )
{
	if ( (encodedLen % 2) != 0 )
		return false;
	rawStr->erase();
	if ( encodedLen == 0 ) return true;
	rawStr->reserve ( encodedLen / 2 );

	for( XMP_Uns32 i = 0; i < encodedLen; )
	{
		XMP_Uns8 upperNibble = encodedStr[i];
		if ( (upperNibble < 48) || ( (upperNibble > 57 ) && ( upperNibble < 65 ) ) || (upperNibble > 70) )
			return false;
		if ( upperNibble >= 65 )
			upperNibble -= 7; // shift A-F area adjacent to 0-9
		upperNibble -= 48; // 'shift' to a value [0..15]
		upperNibble = ( upperNibble << 4 );
		i++;

		XMP_Uns8 lowerNibble = encodedStr[i];
		if ( (lowerNibble < 48) || ( (lowerNibble > 57 ) && ( lowerNibble < 65 ) ) || (lowerNibble > 70) )
			return false;
		if ( lowerNibble >= 65 )
			lowerNibble -= 7; // shift A-F area adjacent to 0-9
		lowerNibble -= 48; // 'shift' to a value [0..15]
		i++;

		rawStr->append ( 1, (upperNibble + lowerNibble) );
	}
	return true;
}	// DecodeFromHexString

// Converts input string to an ascii output string
// - terminates at first 0
// - replaces all non ascii with 0x3F ('?')
// - produces up to maxOut characters (note that several UTF-8 character bytes can 'melt' to one byte '?' in ascii.)
static XMP_StringLen convertToASCII( XMP_StringPtr input, XMP_StringLen inputLen, std::string* output, XMP_StringLen maxOutputLen )
{
	if ( (input == 0) && (inputLen != 0) )
		XMP_Throw ( "convertToASCII: null input string", kXMPErr_BadParam );
	if ( output == 0)
		XMP_Throw ( "convertToASCII: null output string", kXMPErr_BadParam );
	if ( maxOutputLen == 0)
		XMP_Throw ( "convertToASCII: zero maxOutputLen chars", kXMPErr_BadParam );

	output->reserve(inputLen);
	output->erase();

	bool isUTF8 = ReconcileUtils::IsUTF8( input, inputLen );
	XMP_StringLen outputLen = 0;

	for ( XMP_Uns32 i=0; i < inputLen; i++ )
	{
		XMP_Uns8 c = (XMP_Uns8) input[i];
		if ( c == 0 )  // early 0 termination, leave.
			break;
		if ( c > 127 ) // uft-8 multi-byte sequence.
		{
			if ( isUTF8 ) // skip all high bytes
			{
				// how many bytes in this ?
				if ( c >= 0xC2 && c <= 0xDF )
					i+=1; // 2-byte sequence
				else if ( c >= 0xE0 && c <= 0xEF )
					i+=2; // 3-byte sequence
				else if ( c >= 0xF0 && c <= 0xF4 )
					i+=3; // 4-byte sequence
				else
					continue; //invalid sequence, look for next 'low' byte ..
			} // thereafter and 'else': just append a question mark:
			output->append( 1, '?' );
		}
		else // regular valid ascii. 1 byte.
		{
			output->append( 1, input[i] );
		}
		outputLen++;
		if ( outputLen >= maxOutputLen )
			break; // (may be even or even greater due to UFT-8 multi-byte jumps)
	}

	return outputLen;
}

/**
 * ensures that native property gets returned as UTF-8 (may or mayn not already be UTF-8)
 *  - also takes care of "moot padding" (pre-mature zero termination)
 *  - propertyExists: it is important to know if there as an existing, non zero property
 *    even (in the event of serverMode) it is not actually returned, but an empty string instead.
 */
static std::string nativePropertyToUTF8 ( XMP_StringPtr cstring, XMP_StringLen maxSize, bool* propertyExists )
{
	// the value might be properly 0-terminated, prematurely or not
	// at all, hence scan through to find actual size
	XMP_StringLen size   = 0;
	for ( size = 0; size < maxSize; size++ )
	{
		if ( cstring[size] == 0 )
			break;
	}

	(*propertyExists) = ( size > 0 );

	std::string utf8("");
	if ( ReconcileUtils::IsUTF8( cstring, size ) )
		utf8 = std::string( cstring, size ); //use utf8 directly
	else
	{
		if ( ! ignoreLocalText )
		{
			#if ! UNIX_ENV // n/a anyway, since always ignoreLocalText on Unix
				ReconcileUtils::LocalToUTF8( cstring, size, &utf8 );
			#endif
		}
	}
	return utf8;
}

// reads maxSize bytes from file (not "up to", exactly fullSize)
// puts it into a string, sets respective tree property
static std::string getBextField ( const char* data, XMP_Uns32 offset, XMP_Uns32 maxSize )
{
	if (data == 0)
		XMP_Throw ( "getBextField: null data pointer", kXMPErr_BadParam );
	if ( maxSize == 0)
		XMP_Throw ( "getBextField: maxSize must be greater than 0", kXMPErr_BadParam );

	std::string r;
	convertToASCII( data+offset, maxSize, &r, maxSize );
	return r;
}

static void importBextChunkToXMP( RIFF_MetaHandler* handler, ValueChunk* bextChunk )
{
	// if there's a bext chunk, there is data...
	handler->containsXMP = true; // very important for treatment on caller level

	XMP_Enforce( bextChunk->oldSize >= MIN_BEXT_SIZE );
	XMP_Enforce( bextChunk->oldSize < MAX_BEXT_SIZE );

	const char* data = bextChunk->oldValue.data();
	std::string value;

	// register bext namespace:
	SXMPMeta::RegisterNamespace( kXMP_NS_BWF, "bext:", 0 );

	// bextDescription ------------------------------------------------
	value = getBextField( data, 0, 256 );
	if ( value.size() > 0 )
		handler->xmpObj.SetProperty( bextDescription.ns, bextDescription.prop, value.c_str() );

	// bextOriginator -------------------------------------------------
	value = getBextField( data, 256, 32 );
	if ( value.size() > 0 )
		handler->xmpObj.SetProperty( bextOriginator.ns , bextOriginator.prop, value.c_str() );

	// bextOriginatorRef ----------------------------------------------
	value = getBextField( data, 256+32, 32 );
	if ( value.size() > 0 )
		handler->xmpObj.SetProperty( bextOriginatorRef.ns , bextOriginatorRef.prop, value.c_str() );

	// bextOriginationDate --------------------------------------------
	value = getBextField( data, 256+32+32, 10 );
	if ( value.size() > 0 )
		handler->xmpObj.SetProperty( bextOriginationDate.ns , bextOriginationDate.prop, value.c_str() );

	// bextOriginationTime --------------------------------------------
	value = getBextField( data, 256+32+32+10, 8 );
	if ( value.size() > 0 )
		handler->xmpObj.SetProperty( bextOriginationTime.ns , bextOriginationTime.prop, value.c_str() );

	// bextTimeReference ----------------------------------------------
	// thanx to nice byte order, all 8 bytes can be read as one:
	XMP_Uns64 timeReferenceFull  = GetUns64LE( &(data[256+32+32+10+8 ] ) );
	value.erase();
	SXMPUtils::ConvertFromInt64( timeReferenceFull, "%llu", &value );
	handler->xmpObj.SetProperty( bextTimeReference.ns, bextTimeReference.prop, value );

	// bextVersion ----------------------------------------------------
	XMP_Uns16 bwfVersion  = GetUns16LE( &(data[256+32+32+10+8+8] ) );
	value.erase();
	SXMPUtils::ConvertFromInt( bwfVersion, "", &value );
	handler->xmpObj.SetProperty( bextVersion.ns, bextVersion.prop, value );

	// bextUMID -------------------------------------------------------
	// binary string is already in memory, must convert to hex string
	std::string umidString;
	bool allZero = EncodeToHexString( &(data[256+32+32+10+8+8+2]), 64, &umidString );
	if (! allZero )
		handler->xmpObj.SetProperty( bextUMID.ns, bextUMID.prop, umidString );

	// bextCodingHistory ----------------------------------------------
	bool hasCodingHistory = bextChunk->oldSize > MIN_BEXT_SIZE;

	if ( hasCodingHistory )
	{
		XMP_StringLen codingHistorySize = (XMP_StringLen) (bextChunk->oldSize - MIN_BEXT_SIZE);
		std::string codingHistory;
		convertToASCII( &data[MIN_BEXT_SIZE-8], codingHistorySize, &codingHistory, codingHistorySize );
		if (! codingHistory.empty() )
			handler->xmpObj.SetProperty( bextCodingHistory.ns, bextCodingHistory.prop, codingHistory );
	}
} // importBextChunkToXMP

static void importPrmLToXMP( RIFF_MetaHandler* handler, ValueChunk* prmlChunk )
{
	bool haveXMP = false;

	XMP_Enforce( prmlChunk->oldSize == PRML_SIZE );
	PrmLBoxContent rawPrmL;
	XMP_Assert ( sizeof ( rawPrmL ) == PRML_SIZE - 8 ); // double check tight packing.
	XMP_Assert ( sizeof ( rawPrmL.filePath ) == 260 );
	memcpy ( &rawPrmL, prmlChunk->oldValue.data(), sizeof (rawPrmL) );

	if ( rawPrmL.magic != 0xBEEFCAFE ) {
		Flip4 ( &rawPrmL.exportType );	// The only numeric field that we care about.
	}

	rawPrmL.filePath[259] = 0;	// Ensure a terminating nul.
	if ( rawPrmL.filePath[0] != 0 ) {
		if ( rawPrmL.filePath[0] == '/' ) {
			haveXMP = true;
			handler->xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "macAtom",
								  kXMP_NS_CreatorAtom, "posixProjectPath", rawPrmL.filePath );
		} else if ( XMP_LitNMatch ( rawPrmL.filePath, "\\\\?\\", 4 ) ) {
			haveXMP = true;
			handler->xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "windowsAtom",
								  kXMP_NS_CreatorAtom, "uncProjectPath", rawPrmL.filePath );
		}
	}

	const char * exportStr = 0;
	switch ( rawPrmL.exportType ) {
		case kExportTypeMovie  : exportStr = "movie";  break;
		case kExportTypeStill  : exportStr = "still";  break;
		case kExportTypeAudio  : exportStr = "audio";  break;
		case kExportTypeCustom : exportStr = "custom"; break;
	}
	if ( exportStr != 0 ) {
		haveXMP = true;
		handler->xmpObj.SetStructField ( kXMP_NS_DM, "projectRef", kXMP_NS_DM, "type", exportStr );
	}

	handler->containsXMP |= haveXMP; // mind the '|='
} // importCr8rToXMP

static void importCr8rToXMP( RIFF_MetaHandler* handler, ValueChunk* cr8rChunk )
{
	bool haveXMP = false;

	XMP_Enforce( cr8rChunk->oldSize == CR8R_SIZE );
	Cr8rBoxContent rawCr8r;
	XMP_Assert ( sizeof ( rawCr8r ) == CR8R_SIZE - 8 ); // double check tight packing.
	memcpy ( &rawCr8r, cr8rChunk->oldValue.data(), sizeof (rawCr8r) );

	if ( rawCr8r.magic != 0xBEEFCAFE ) {
		Flip4 ( &rawCr8r.creatorCode );	// The only numeric fields that we care about.
		Flip4 ( &rawCr8r.appleEvent );
	}

	std::string fieldPath;

	SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "applicationCode", &fieldPath );
	if ( rawCr8r.creatorCode != 0 ) {
		haveXMP = true;
		handler->xmpObj.SetProperty_Int64 ( kXMP_NS_CreatorAtom, fieldPath.c_str(), (XMP_Int64)rawCr8r.creatorCode );	// ! Unsigned trickery.
	}

	SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "invocationAppleEvent", &fieldPath );
	if ( rawCr8r.appleEvent != 0 ) {
		haveXMP = true;
		handler->xmpObj.SetProperty_Int64 ( kXMP_NS_CreatorAtom, fieldPath.c_str(), (XMP_Int64)rawCr8r.appleEvent );	// ! Unsigned trickery.
	}

	rawCr8r.fileExt[15] = 0;	// Ensure a terminating nul.
	if ( rawCr8r.fileExt[0] != 0 ) {
		haveXMP = true;
		handler->xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "extension", rawCr8r.fileExt );
	}

	rawCr8r.appOptions[15] = 0;	// Ensure a terminating nul.
	if ( rawCr8r.appOptions[0] != 0 ) {
		haveXMP = true;
		handler->xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "invocationFlags", rawCr8r.appOptions );
	}

	rawCr8r.appName[31] = 0;	// Ensure a terminating nul.
	if ( rawCr8r.appName[0] != 0 ) {
		haveXMP = true;
		handler->xmpObj.SetProperty ( kXMP_NS_XMP, "CreatorTool", rawCr8r.appName );
	}

	handler->containsXMP |= haveXMP; // mind the '|='
} // importCr8rToXMP


static void importListChunkToXMP( RIFF_MetaHandler* handler, ContainerChunk* listChunk, Mapping mapping[], bool xmpHasPriority )
{
	valueMap* cm = &listChunk->childmap;
	for (int p=0; mapping[p].chunkID != 0; p++) // go through legacy chunks
	{
		valueMapIter result = cm->find(mapping[p].chunkID);
		if( result != cm->end() ) // if value found
		{
			ValueChunk* propChunk = result->second;

			bool propertyExists = false;
			std::string utf8 = nativePropertyToUTF8(
				propChunk->oldValue.c_str(),
				(XMP_StringLen)propChunk->oldValue.size(), &propertyExists );

			if ( utf8.size() > 0 ) // if property is not-empty, set Property
			{
				switch ( mapping[p].propType )
				{
					case prop_TIMEVALUE:
						if ( xmpHasPriority &&
							 handler->xmpObj.DoesStructFieldExist( mapping[p].ns, mapping[p].prop, kXMP_NS_DM, "timeValue" ))
							 break; // skip if XMP has precedence and exists
						handler->xmpObj.SetStructField( mapping[p].ns, mapping[p].prop,
										kXMP_NS_DM, "timeValue", utf8.c_str() );
						break;
					case prop_LOCALIZED_TEXT:
						if ( xmpHasPriority && handler->xmpObj.GetLocalizedText( mapping[p].ns ,
							mapping[p].prop, "" , "x-default", 0, 0, 0 ))
							break; // skip if XMP has precedence and exists
						handler->xmpObj.SetLocalizedText( mapping[p].ns , mapping[p].prop,
											"" , "x-default" , utf8.c_str() );
						if ( mapping[p].chunkID == kPropChunkINAM )
							handler->hasListInfoINAM = true; // needs to be known for special 3-way merge around dc:title
						break;
					case prop_ARRAYITEM:
						if ( xmpHasPriority &&
							 handler->xmpObj.DoesArrayItemExist( mapping[p].ns, mapping[p].prop, 1 ))
							break; // skip if XMP has precedence and exists
						handler->xmpObj.DeleteProperty( mapping[p].ns, mapping[p].prop );
						handler->xmpObj.AppendArrayItem( mapping[p].ns, mapping[p].prop, kXMP_PropValueIsArray, utf8.c_str(), kXMP_NoOptions );
						break;
					case prop_SIMPLE:
						if ( xmpHasPriority &&
							handler->xmpObj.DoesPropertyExist( mapping[p].ns, mapping[p].prop ))
							break; // skip if XMP has precedence and exists
						handler->xmpObj.SetProperty( mapping[p].ns, mapping[p].prop, utf8.c_str() );
						break;
					default:
						XMP_Throw( "internal error" , kXMPErr_InternalFailure );
				}

				handler->containsXMP = true; // very important for treatment on caller level
			}
			else if ( ! propertyExists) // otherwise remove it.
			{ // [2389942] don't, if legacy value is existing but non-retrievable (due to server mode)
				switch ( mapping[p].propType )
				{
					case prop_TIMEVALUE:
						if ( (!xmpHasPriority) && // forward deletion only if XMP has no priority
							 handler->xmpObj.DoesPropertyExist( mapping[p].ns, mapping[p].prop ))
							handler->xmpObj.DeleteProperty( mapping[p].ns, mapping[p].prop );
						break;
					case prop_LOCALIZED_TEXT:
						if ( (!xmpHasPriority) && // forward deletion only if XMP has no priority
							 handler->xmpObj.DoesPropertyExist( mapping[p].ns, mapping[p].prop ))
							handler->xmpObj.DeleteLocalizedText( mapping[p].ns, mapping[p].prop, "", "x-default" );
						break;
					case prop_ARRAYITEM:
					case prop_SIMPLE:
						if ( (!xmpHasPriority) && // forward deletion only if XMP has no priority
							 handler->xmpObj.DoesPropertyExist( mapping[p].ns, mapping[p].prop ))
							handler->xmpObj.DeleteProperty( mapping[p].ns, mapping[p].prop );
						break;
					default:
						XMP_Throw( "internal error" , kXMPErr_InternalFailure );
				}
			}
		}
	} // for
}
void importProperties( RIFF_MetaHandler* handler )
{
	bool hasDigest = handler->xmpObj.GetProperty( kXMP_NS_WAV, "NativeDigest", NULL , NULL );
	if ( hasDigest )
	{
		// remove! since it now becomse a 'new' handler file
		handler->xmpObj.DeleteProperty( kXMP_NS_WAV, "NativeDigest" );
	}

	// BWF Bext extension chunk -----------------------------------------------
	if (   handler->parent->format == kXMP_WAVFile && // applies only to WAV
		   handler->bextChunk != 0 ) //skip if no BEXT chunk found.
	{
		importBextChunkToXMP( handler, handler->bextChunk );
	}

	// PrmL chunk --------------------------------------------------------------
	if ( handler->prmlChunk != 0 && handler->prmlChunk->oldSize == PRML_SIZE )
	{
		importPrmLToXMP( handler, handler->prmlChunk );
	}

	// Cr8r chunk --------------------------------------------------------------
	if ( handler->cr8rChunk != 0 && handler->cr8rChunk->oldSize == CR8R_SIZE )
	{
		importCr8rToXMP( handler, handler->cr8rChunk );
	}

	// LIST:INFO --------------------------------------------------------------
	if ( handler->listInfoChunk != 0) //skip if no LIST:INFO chunk found.
		importListChunkToXMP( handler, handler->listInfoChunk, listInfoProps, hasDigest );

	// LIST:Tdat --------------------------------------------------------------
	if ( handler->listTdatChunk != 0)
		importListChunkToXMP( handler, handler->listTdatChunk, listTdatProps, hasDigest );

	// DISP (do last, higher priority than INAM ) -----------------------------
	bool takeXMP = false; // assume for now
	if ( hasDigest )
	{
		std::string actualLang, value;
		bool r = handler->xmpObj.GetLocalizedText( kXMP_NS_DC, "title", "" , "x-default" , &actualLang, &value, NULL );
		if ( r && (actualLang == "x-default") ) takeXMP = true;
	}

	if ( (!takeXMP) && handler->dispChunk != 0) //skip if no LIST:INFO chunk found.
	{
		std::string* value = &handler->dispChunk->oldValue;
		if ( value->size() >= 4 ) // ignore contents if file too small
		{
			XMP_StringPtr cstring = value->c_str();
			XMP_StringLen size = (XMP_StringLen) value->size();

			size -= 4; // skip first four bytes known to contain constant
			cstring += 4;

			bool propertyExists = false;
			std::string utf8 = nativePropertyToUTF8( cstring, size, &propertyExists );

			if ( utf8.size() > 0 )
			{
				handler->xmpObj.SetLocalizedText( kXMP_NS_DC, "title", "" , "x-default" , utf8.c_str() );
				handler->containsXMP = true; // very important for treatment on caller level
			}
			else
			{
				// found as part of [2389942]
				// forward deletion may only happen if no LIST:INFO/INAM is present:
				if ( ! handler->hasListInfoINAM &&
					 ! propertyExists )  // ..[2389942]part2: and if truly no legacy property
				{	                    //    (not just an unreadable one due to ServerMode).
					handler->xmpObj.DeleteProperty( kXMP_NS_DC, "title" );
				}
			}
		} // if size sufficient
	} // handler->dispChunk

} // importProperties

////////////////////////////////////////////////////////////////////////////////
// EXPORT
////////////////////////////////////////////////////////////////////////////////

void relocateWronglyPlacedXMPChunk( RIFF_MetaHandler* handler )
{
	XMP_IO* file = handler->parent->ioRef;
	RIFF::containerVect *rc = &handler->riffChunks;
	RIFF::ContainerChunk* lastChunk = rc->at( rc->size()-1 );

	// 1) XMPPacket
	// needChunk exists but is not in lastChunk ?
	if (
		 handler->xmpChunk != 0 &&		// XMP Chunk existing?
		 (XMP_Uns32)rc->size() > 1 &&	// more than 1 top-level chunk (otherwise pointless)
         lastChunk->getChild( handler->xmpChunk ) == lastChunk->children.end() // not already in last chunk?
	    )
	{
		RIFF::ContainerChunk* cur;
		chunkVectIter child;
		XMP_Int32 chunkNo;

		// find and relocate to last chunk:
		for ( chunkNo = (XMP_Int32)rc->size()-2 ; chunkNo >= 0; chunkNo-- ) // ==> start with second-last chunk
		{
			cur = rc->at(chunkNo);
			child = cur->getChild( handler->xmpChunk );
			if ( child != cur->children.end() ) // found?
				break;
		} // for

		if ( chunkNo < 0 ) // already in place? nothing left to do.
			return;

		lastChunk->children.push_back( *child ); // nb: order matters!
		cur->replaceChildWithJunk( *child, false );
		cur->hasChange = true; // [2414649] initialize early-on i.e: here
	} // if
} // relocateWronglyPlacedXMPChunk

// writes to buffer up to max size,
// 0 termination only if shorter than maxSize
// converts down to ascii
static void setBextField ( std::string* value, XMP_Uns8* data, XMP_Uns32 offset, XMP_Uns32 maxSize )
{
	XMP_Validate( value != 0, "setBextField: null value string pointer", kXMPErr_BadParam );
	XMP_Validate( data != 0, "setBextField: null data value", kXMPErr_BadParam );
	XMP_Validate( maxSize > 0, "setBextField: maxSize must be greater than 0", kXMPErr_BadParam );

	std::string ascii;
	XMP_StringLen actualSize = convertToASCII( value->data(), (XMP_StringLen) value->size() , &ascii , maxSize );
	strncpy( (char*)(data + offset), ascii.data(), actualSize );
}

// add bwf-bext related data to bext chunk, create if not existing yet.
// * in fact, since bext is fully fixed and known, there can be no unknown subchunks worth keeping:
//    * prepare bext chunk in buffer
//    * value changed/created if needed only, otherways remove chunk
// * remove bext-mapped properties from xmp (non-redundant storage)
// note: ValueChunk**: adress of pointer to allow changing the pointer itself (i.e. chunk creation)
static void exportXMPtoBextChunk( RIFF_MetaHandler* handler, ValueChunk** bextChunk )
{
	// register bext namespace ( if there was no import, this is news, otherwise harmless moot)
	SXMPMeta::RegisterNamespace( kXMP_NS_BWF, "bext:", 0 );

	bool chunkUsed = false; // assume for now
	SXMPMeta* xmp = &handler->xmpObj;

	// prepare buffer, need to know CodingHistory size as the only variable
	XMP_Int32 bextBufferSize = MIN_BEXT_SIZE - 8; // -8 because of header
	std::string value;
	if ( xmp->GetProperty( bextCodingHistory.ns, bextCodingHistory.prop, &value, kXMP_NoOptions ))
	{
		bextBufferSize += ((XMP_StringLen)value.size()) + 1 ; // add to size (and a trailing zero)
	}

	// create and clear buffer
	XMP_Uns8* buffer = new XMP_Uns8[bextBufferSize];
	for (XMP_Int32 i = 0; i < bextBufferSize; i++ )
		buffer[i] = 0;

	// grab props, write into buffer, remove from XMP ///////////////////////////
	// bextDescription ------------------------------------------------
	if ( xmp->GetProperty( bextDescription.ns, bextDescription.prop, &value, kXMP_NoOptions ) )
	{
		setBextField( &value, (XMP_Uns8*) buffer, 0, 256 );
		xmp->DeleteProperty( bextDescription.ns, bextDescription.prop)					;
		chunkUsed = true;
	}
	// bextOriginator -------------------------------------------------
	if ( xmp->GetProperty( bextOriginator.ns , bextOriginator.prop, &value, kXMP_NoOptions ) )
	{
		setBextField( &value, (XMP_Uns8*) buffer, 256, 32 );
		xmp->DeleteProperty( bextOriginator.ns , bextOriginator.prop );
		chunkUsed = true;
	}
	// bextOriginatorRef ----------------------------------------------
	if ( xmp->GetProperty( bextOriginatorRef.ns , bextOriginatorRef.prop, &value, kXMP_NoOptions ) )
	{
		setBextField( &value, (XMP_Uns8*) buffer, 256+32, 32 );
		xmp->DeleteProperty( bextOriginatorRef.ns , bextOriginatorRef.prop );
		chunkUsed = true;
	}
	// bextOriginationDate --------------------------------------------
	if ( xmp->GetProperty( bextOriginationDate.ns , bextOriginationDate.prop, &value, kXMP_NoOptions ) )
	{
		setBextField( &value, (XMP_Uns8*) buffer, 256+32+32, 10 );
		xmp->DeleteProperty( bextOriginationDate.ns , bextOriginationDate.prop );
		chunkUsed = true;
	}
	// bextOriginationTime --------------------------------------------
	if ( xmp->GetProperty( bextOriginationTime.ns , bextOriginationTime.prop, &value, kXMP_NoOptions ) )
	{
		setBextField( &value, (XMP_Uns8*) buffer, 256+32+32+10, 8 );
		xmp->DeleteProperty( bextOriginationTime.ns , bextOriginationTime.prop );
		chunkUsed = true;
	}
	// bextTimeReference ----------------------------------------------
	// thanx to friendly byte order, all 8 bytes can be written in one go:
	if ( xmp->GetProperty( bextTimeReference.ns, bextTimeReference.prop, &value, kXMP_NoOptions ) )
	{
		try
		{
			XMP_Int64 v = SXMPUtils::ConvertToInt64( value.c_str() );
			PutUns64LE( v, &(buffer[256+32+32+10+8] ));
			chunkUsed = true;
		}
		catch (XMP_Error& e)
		{
			if ( e.GetID() != kXMPErr_BadParam )
				throw e;         // re-throw on any other error
		}  // 'else' tolerate ( time reference remains 0x00000000 )
		// valid or not, do not store redundantly:
		xmp->DeleteProperty( bextTimeReference.ns, bextTimeReference.prop );
	}

	// bextVersion ----------------------------------------------------
	// set version=1, no matter what.
	PutUns16LE( 1, &(buffer[256+32+32+10+8+8]) );
	xmp->DeleteProperty( bextVersion.ns, bextVersion.prop );

	// bextUMID -------------------------------------------------------
	if ( xmp->GetProperty( bextUMID.ns, bextUMID.prop, &value, kXMP_NoOptions ) )
	{
		std::string rawStr;

		if ( !DecodeFromHexString( value.data(), (XMP_StringLen) value.size(), &rawStr ) )
		{
			delete [] buffer; // important.
			XMP_Throw ( "EncodeFromHexString: illegal umid string. Must contain an even number of 0-9 and uppercase A-F chars.", kXMPErr_BadParam );
		}

		// if UMID is smaller/longer than 64 byte for any reason,
		// truncate/do a partial write (just like for any other bext property)

		memcpy( (char*) &(buffer[256+32+32+10+8+8+2]), rawStr.data(), MIN( 64, rawStr.size() ) );
		xmp->DeleteProperty( bextUMID.ns, bextUMID.prop );
		chunkUsed = true;
	}

	// bextCodingHistory ----------------------------------------------
	if ( xmp->GetProperty( bextCodingHistory.ns, bextCodingHistory.prop, &value, kXMP_NoOptions ) )
	{
		std::string ascii;
		convertToASCII( value.data(), (XMP_StringLen) value.size() , &ascii, (XMP_StringLen) value.size() );
		strncpy( (char*) &(buffer[MIN_BEXT_SIZE-8]), ascii.data(), ascii.size() );
		xmp->DeleteProperty( bextCodingHistory.ns, bextCodingHistory.prop );
		chunkUsed = true;
	}

	// always delete old, recreate if needed
	if ( *bextChunk != 0 )
	{
		(*bextChunk)->parent->replaceChildWithJunk( *bextChunk );
		(*bextChunk) = 0; // clear direct Chunk pointer
	}

	if ( chunkUsed)
		*bextChunk = new ValueChunk( handler->riffChunks.at(0), std::string( (char*)buffer, bextBufferSize ), kChunk_bext );

	delete [] buffer; // important.
}

static inline void SetBufferedString ( char * dest, const std::string source, size_t limit )
{
	memset ( dest, 0, limit );
	size_t count = source.size();
	if ( count >= limit ) count = limit - 1;	// Ensure a terminating nul.
	memcpy ( dest, source.c_str(), count );
}

static void exportXMPtoCr8rChunk ( RIFF_MetaHandler* handler, ValueChunk** cr8rChunk )
{
	const SXMPMeta & xmp = handler->xmpObj;

	// Make sure an existing Cr8r chunk has the proper fixed length.
	bool haveOldCr8r = (*cr8rChunk != 0);
	if ( haveOldCr8r && ((*cr8rChunk)->oldSize != sizeof(Cr8rBoxContent)+8) ) {
		(*cr8rChunk)->parent->replaceChildWithJunk ( *cr8rChunk );	// Wrong length, the existing chunk must be bad.
		(*cr8rChunk) = 0;
		haveOldCr8r = false;
	}

	bool haveNewCr8r = false;
	std::string creatorCode, appleEvent, fileExt, appOptions, appName;

	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "applicationCode", &creatorCode, 0 );
	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "invocationAppleEvent", &appleEvent, 0 );
	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "extension", &fileExt, 0 );
	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "invocationFlags", &appOptions, 0 );
	haveNewCr8r |= xmp.GetProperty ( kXMP_NS_XMP, "CreatorTool", &appName, 0 );

	if ( ! haveNewCr8r ) {	// Get rid of an existing Cr8r chunk if there is no new XMP.
		if ( haveOldCr8r ) {
			(*cr8rChunk)->parent->replaceChildWithJunk ( *cr8rChunk );
			*cr8rChunk = 0;
		}
		return;
	}

	if ( ! haveOldCr8r ) {
		*cr8rChunk = new ValueChunk ( handler->lastChunk, std::string(), kChunk_Cr8r );
	}

	std::string strValue;
	strValue.assign ( (sizeof(Cr8rBoxContent) - 1), '\0' );	// ! Use size-1 because SetValue appends a trailing 0 byte.
	(*cr8rChunk)->SetValue ( strValue );	// ! Just get the space available.
	XMP_Assert ( (*cr8rChunk)->newValue.size() == sizeof(Cr8rBoxContent) );
	(*cr8rChunk)->hasChange = true;

	Cr8rBoxContent * newCr8r = (Cr8rBoxContent*) (*cr8rChunk)->newValue.data();

	if ( ! haveOldCr8r ) {

		newCr8r->magic = MakeUns32LE ( 0xBEEFCAFE );
		newCr8r->size = MakeUns32LE ( sizeof(Cr8rBoxContent) );
		newCr8r->majorVer = MakeUns16LE ( 1 );

	} else {

		const Cr8rBoxContent * oldCr8r = (Cr8rBoxContent*) (*cr8rChunk)->oldValue.data();
		memcpy ( newCr8r, oldCr8r, sizeof(Cr8rBoxContent) );
		if ( GetUns32LE ( &newCr8r->magic ) != 0xBEEFCAFE ) {	// Make sure we write LE numbers.
			Flip4 ( &newCr8r->magic );
			Flip4 ( &newCr8r->size );
			Flip2 ( &newCr8r->majorVer );
			Flip2 ( &newCr8r->minorVer );
			Flip4 ( &newCr8r->creatorCode );
			Flip4 ( &newCr8r->appleEvent );
		}

	}

	if ( ! creatorCode.empty() ) {
		newCr8r->creatorCode = MakeUns32LE ( (XMP_Uns32) strtoul ( creatorCode.c_str(), 0, 0 ) );
	}

	if ( ! appleEvent.empty() ) {
		newCr8r->appleEvent = MakeUns32LE ( (XMP_Uns32) strtoul ( appleEvent.c_str(), 0, 0 ) );
	}

	if ( ! fileExt.empty() ) SetBufferedString ( newCr8r->fileExt, fileExt, sizeof ( newCr8r->fileExt ) );
	if ( ! appOptions.empty() ) SetBufferedString ( newCr8r->appOptions, appOptions, sizeof ( newCr8r->appOptions ) );
	if ( ! appName.empty() ) SetBufferedString ( newCr8r->appName, appName, sizeof ( newCr8r->appName ) );

}

static void exportXMPtoListChunk( XMP_Uns32 id, XMP_Uns32 containerType,
						   RIFF_MetaHandler* handler, ContainerChunk** listChunk, Mapping mapping[])
{
	// note: ContainerChunk**: adress of pointer to allow changing the pointer itself (i.e. chunk creation)
	SXMPMeta* xmp = &handler->xmpObj;
	bool listChunkIsNeeded = false; // assume for now

	// ! The NUL is optional in WAV to avoid a parsing bug in Audition 3 - can't handle implicit pad byte.
	bool optionalNUL = (handler->parent->format == kXMP_WAVFile);

	for ( int p=0; mapping[p].chunkID != 0; ++p ) {	// go through all potential property mappings

		bool propExists = false;
		std::string value, actualLang;

		switch ( mapping[p].propType ) {

			// get property. if existing, remove from XMP (to avoid redundant storage)
			case prop_TIMEVALUE:
				propExists = xmp->GetStructField ( mapping[p].ns, mapping[p].prop, kXMP_NS_DM, "timeValue", &value, 0 );
				break;

			case prop_LOCALIZED_TEXT:
				propExists = xmp->GetLocalizedText ( mapping[p].ns, mapping[p].prop, "", "x-default", &actualLang, &value, 0);
				if ( actualLang != "x-default" ) propExists = false; // no "x-default" => nothing to reconcile !
				break;

			case prop_ARRAYITEM:
				propExists = xmp->GetArrayItem ( mapping[p].ns, mapping[p].prop, 1, &value, 0 );
				break;

			case prop_SIMPLE:
				propExists = xmp->GetProperty ( mapping[p].ns, mapping[p].prop, &value, 0 );
				break;

			default:
				XMP_Throw ( "internal error", kXMPErr_InternalFailure );

		}

		if ( ! propExists ) {

			if ( *listChunk != 0 ) (*listChunk)->removeValue ( mapping[p].chunkID );

		} else {

			listChunkIsNeeded = true;
			if ( *listChunk == 0 ) *listChunk = new ContainerChunk ( handler->riffChunks[0], id, containerType );

			valueMap* cm = &(*listChunk)->childmap;
			valueMapIter result = cm->find( mapping[p].chunkID );
			ValueChunk* propChunk = 0;

			if ( result != cm->end() ) {
				propChunk = result->second;
			} else {
				propChunk = new ValueChunk ( *listChunk, std::string(), mapping[p].chunkID );
			}

			propChunk->SetValue ( value.c_str(), optionalNUL );

		}

	} // for each property

	if ( (! listChunkIsNeeded) && (*listChunk != 0) && ((*listChunk)->children.size() == 0) ) {
		(*listChunk)->parent->replaceChildWithJunk ( *listChunk );
		(*listChunk) = 0; // reset direct Chunk pointer
	}

}

void exportAndRemoveProperties ( RIFF_MetaHandler* handler )
{
	SXMPMeta xmpObj = handler->xmpObj;

	exportXMPtoCr8rChunk ( handler, &handler->cr8rChunk );

	// 1/4 BWF Bext extension chunk -----------------------------------------------
	if ( handler->parent->format == kXMP_WAVFile ) {	// applies only to WAV
		exportXMPtoBextChunk ( handler, &handler->bextChunk );
	}

	// 2/4 DISP chunk
	if ( handler->parent->format == kXMP_WAVFile ) {	// create for WAVE only

		std::string actualLang, xmpValue;
		bool r = xmpObj.GetLocalizedText ( kXMP_NS_DC, "title", "" , "x-default" , &actualLang, &xmpValue, 0 );

		if ( r && ( actualLang == "x-default" ) ) {	// prop exists?

			// the 'right' DISP is lead by a 32 bit low endian 0x0001
			std::string dispValue = std::string( "\x1\0\0\0", 4 );
			dispValue.append ( xmpValue );

			if ( handler->dispChunk == 0 ) {
				handler->dispChunk = new RIFF::ValueChunk ( handler->riffChunks.at(0), std::string(), kChunk_DISP );
			}

			// ! The NUL is optional in WAV to avoid a parsing bug in Audition 3 - can't handle implicit pad byte.
			handler->dispChunk->SetValue ( dispValue, ValueChunk::kNULisOptional );

		} else {	// remove Disp Chunk..

			if ( handler->dispChunk != 0 ) {	// ..if existing
				ContainerChunk* mainChunk = handler->riffChunks.at(0);
				Chunk* needle = handler->dispChunk;
				chunkVectIter iter = mainChunk->getChild ( needle );
				if ( iter != mainChunk->children.end() ) {
					mainChunk->replaceChildWithJunk ( *iter );
					handler->dispChunk = 0;
					mainChunk->hasChange = true;
				}
			}

		}

	}

	// 3/4 LIST:INFO
	exportXMPtoListChunk ( kChunk_LIST, kType_INFO, handler, &handler->listInfoChunk, listInfoProps );

	// 4/4 LIST:Tdat
	exportXMPtoListChunk ( kChunk_LIST, kType_Tdat, handler, &handler->listTdatChunk, listTdatProps );

}

} // namespace RIFF
