// =================================================================================================
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// Adobe patent application tracking #P435, entitled 'Unique markers to simplify embedding data of
// one format in a file with a different format', inventors: Sean Parent, Greg Gilley.
// =================================================================================================

#if WIN32
	#pragma warning ( disable : 4127 )	// conditional expression is constant
	#pragma warning ( disable : 4510 )	// default constructor could not be generated
	#pragma warning ( disable : 4610 )	// user defined constructor required
	#pragma warning ( disable : 4786 )	// debugger can't handle long symbol names
#endif

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"

#if TestRunnerBuild
	#define EnablePacketScanning 1
#else
	#include "XMPFiles/source/XMPFiles_Impl.hpp"
#endif

#include "XMPFiles/source/FormatSupport/XMPScanner.hpp"

#include <cassert>
#include <string>
#include <cstdlib>

#if DEBUG
	#include <iostream>
	#include <iomanip>
	#include <fstream>
#endif

#ifndef UseStringPushBack	// VC++ 6.x does not provide push_back for strings!
	#define UseStringPushBack	0
#endif

using namespace std;

#if EnablePacketScanning

// =================================================================================================
// =================================================================================================
// class PacketMachine
// ===================
//
// This is the packet recognizer state machine.  The top of the machine is FindNextPacket, this
// calls the specific state components and handles transitions.  The states are described by an
// array of RecognizerInfo records, indexed by the RecognizerKind enumeration.  Each RecognizerInfo
// record has a function that does that state's work, the success and failure transition states,
// and a string literal that is passed to the state function.  The literal lets a common MatchChar
// or MatchString function be used in several places.
//
// The state functions are responsible for consuming input to recognize their particular state.
// This includes intervening nulls for 16 and 32 bit character forms.  For the simplicity, things
// are treated as essentially little endian and the nulls are not actually checked.  The opening
// '<' is found with a byte-by-byte search, then the number of bytes per character is determined
// by counting the following nulls.  From then on, consuming a character means incrementing the
// buffer pointer by the number of bytes per character.  Thus the buffer pointer only points to
// the "real" bytes.  This also means that the pointer can go off the end of the buffer by a
// variable amount.  The amount of overrun is saved so that the pointer can be positioned at the
// right byte to start the next buffer.
//
// The state functions return a TriState value, eTriYes means the pattern was found, eTriNo means
// the pattern was definitely not found, eTriMaybe means that the end of the buffer was reached
// while working through the pattern.
//
// When eTriYes is returned, the fBufferPtr data member is left pointing to the "real" byte
// following the last actual byte.  Which might not be addressable memory!  This also means that
// a state function can be entered with nothing available in the buffer.  When eTriNo is returned,
// the fBufferPtr data member is left pointing to the byte that caused the failure.  The state
// machine starts over from the failure byte.
//
// The state functions must preserve their internal micro-state before returning eTriMaybe, and
// resume processing when called with the next buffer.  The fPosition data member is used to denote
// how many actual characters have been consumed.  The fNullCount data member is used to denote how
// many nulls are left before the next actual character.

// =================================================================================================
// PacketMachine
// =============

XMPScanner::PacketMachine::PacketMachine ( XMP_Int64 bufferOffset, const void * bufferOrigin, XMP_Int64 bufferLength ) :

	// Public members
	fPacketStart ( 0 ),
	fPacketLength ( 0 ),
	fBytesAttr ( -1 ),
	fCharForm ( eChar8Bit ),
	fAccess ( ' ' ),
	fBogusPacket ( false ),

	// Private members
	fBufferOffset ( bufferOffset ),
	fBufferOrigin ( (const char *) bufferOrigin ),
	fBufferPtr ( fBufferOrigin ),
	fBufferLimit ( fBufferOrigin + bufferLength ),
	fRecognizer ( eLeadInRecognizer ),
	fPosition ( 0 ),
	fBytesPerChar ( 1 ),
	fBufferOverrun ( 0 ),
	fQuoteChar ( ' ' )

{
	/*
	REVIEW NOTES : Should the buffer stuff be in a class?
	*/

	assert ( bufferOrigin != NULL );
	assert ( bufferLength != 0 );

}	// PacketMachine

// =================================================================================================
// ~PacketMachine
// ==============

XMPScanner::PacketMachine::~PacketMachine ()
{

	// An empty placeholder.

}	// ~PacketMachine

// =================================================================================================
// AssociateBuffer
// ===============

void
XMPScanner::PacketMachine::AssociateBuffer ( XMP_Int64 bufferOffset, const void * bufferOrigin, XMP_Int64 bufferLength )
{

	fBufferOffset = bufferOffset;
	fBufferOrigin = (const char *) bufferOrigin;
	fBufferPtr = fBufferOrigin + fBufferOverrun;
	fBufferLimit = fBufferOrigin + bufferLength;

}	// AssociateBuffer

// =================================================================================================
// ResetMachine
// ============

void
XMPScanner::PacketMachine::ResetMachine ()
{

	fRecognizer = eLeadInRecognizer;
	fPosition = 0;
	fBufferOverrun = 0;
	fCharForm = eChar8Bit;
	fBytesPerChar = 1;
	fAccess = ' ';
	fBytesAttr = -1;
	fBogusPacket = false;

	fAttrName.erase ( fAttrName.begin(), fAttrName.end() );
	fAttrValue.erase ( fAttrValue.begin(), fAttrValue.end() );
	fEncodingAttr.erase ( fEncodingAttr.begin(), fEncodingAttr.end() );

}	// ResetMachine

// =================================================================================================
// FindLessThan
// ============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::FindLessThan ( PacketMachine * ths, const char * which )
{

	if ( *which == 'H' ) {

		// --------------------------------------------------------------------------------
		// We're looking for the '<' of the header.  If we fail there is no packet in this
		// part of the input, so return eTriNo.

		ths->fCharForm = eChar8Bit;	// We might have just failed from a bogus 16 or 32 bit case.
		ths->fBytesPerChar = 1;

		while ( ths->fBufferPtr < ths->fBufferLimit ) {	// Don't skip nulls for the header's '<'!
			if ( *ths->fBufferPtr == '<' ) break;
			ths->fBufferPtr++;
		}

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriNo;
		ths->fBufferPtr++;
		return eTriYes;

	} else {

		// --------------------------------------------------------------------------------
		// We're looking for the '<' of the trailer.  We're already inside the packet body,
		// looking for the trailer.  So here if we fail we must return eTriMaybe so that we
		// keep looking for the trailer in the next buffer.

		const int bytesPerChar = ths->fBytesPerChar;

		while ( ths->fBufferPtr < ths->fBufferLimit ) {
			if ( *ths->fBufferPtr == '<' ) break;
			ths->fBufferPtr += bytesPerChar;
		}

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;
		ths->fBufferPtr += bytesPerChar;
		return eTriYes;

	}

}	// FindLessThan

// =================================================================================================
// MatchString
// ===========

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::MatchString ( PacketMachine * ths, const char * literal )
{
	const int			bytesPerChar	= ths->fBytesPerChar;
	const char *		litPtr			= literal + ths->fPosition;
	const XMP_Int32		charsToGo		= (XMP_Int32) strlen ( literal ) - ths->fPosition;
	int					charsDone		= 0;

	while ( (charsDone < charsToGo) && (ths->fBufferPtr < ths->fBufferLimit) ) {
		if ( *litPtr != *ths->fBufferPtr ) return eTriNo;
		charsDone++;
		litPtr++;
		ths->fBufferPtr += bytesPerChar;
	}

	if ( charsDone == charsToGo ) return eTriYes;
	ths->fPosition += charsDone;
	return eTriMaybe;

}	// MatchString

// =================================================================================================
// MatchChar
// =========

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::MatchChar ( PacketMachine * ths, const char * literal )
{
	const int	bytesPerChar	= ths->fBytesPerChar;

	if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

	const char currChar = *ths->fBufferPtr;
	if ( currChar != *literal ) return eTriNo;
	ths->fBufferPtr += bytesPerChar;
	return eTriYes;

}	// MatchChar

// =================================================================================================
// MatchOpenQuote
// ==============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::MatchOpenQuote ( PacketMachine * ths, const char * /* unused */ )
{
	const int	bytesPerChar	= ths->fBytesPerChar;

	if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

	const char currChar = *ths->fBufferPtr;
	if ( (currChar != '\'') && (currChar != '"') ) return eTriNo;
	ths->fQuoteChar = currChar;
	ths->fBufferPtr += bytesPerChar;
	return eTriYes;

}	// MatchOpenQuote

// =================================================================================================
// MatchCloseQuote
// ===============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::MatchCloseQuote ( PacketMachine * ths, const char * /* unused */ )
{

	return MatchChar ( ths, &ths->fQuoteChar );

}	// MatchCloseQuote

// =================================================================================================
// CaptureAttrName
// ===============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::CaptureAttrName ( PacketMachine * ths, const char * /* unused */ )
{
	const int	bytesPerChar	= ths->fBytesPerChar;
	char		currChar;

	if ( ths->fPosition == 0 ) {	// Get the first character in the name.

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

		currChar = *ths->fBufferPtr;
		if ( ths->fAttrName.size() == 0 ) {
			if ( ! ( ( ('a' <= currChar) && (currChar <= 'z') ) ||
					 ( ('A' <= currChar) && (currChar <= 'Z') ) ||
					 (currChar == '_') || (currChar == ':') ) ) {
				return eTriNo;
			}
		}

		ths->fAttrName.erase ( ths->fAttrName.begin(), ths->fAttrName.end() );
		#if UseStringPushBack
			ths->fAttrName.push_back ( currChar );
		#else
			ths->fAttrName.insert ( ths->fAttrName.end(), currChar );
		#endif
		ths->fBufferPtr += bytesPerChar;

	}

	while ( ths->fBufferPtr < ths->fBufferLimit ) {	// Get the remainder of the name.

		currChar = *ths->fBufferPtr;
		if ( ! ( ( ('a' <= currChar) && (currChar <= 'z') ) ||
				 ( ('A' <= currChar) && (currChar <= 'Z') ) ||
				 ( ('0' <= currChar) && (currChar <= '9') ) ||
				 (currChar == '-') || (currChar == '.') || (currChar == '_') || (currChar == ':') ) ) {
			break;
		}

		#if UseStringPushBack
			ths->fAttrName.push_back ( currChar );
		#else
			ths->fAttrName.insert ( ths->fAttrName.end(), currChar );
		#endif
		ths->fBufferPtr += bytesPerChar;

	}

	if ( ths->fBufferPtr < ths->fBufferLimit ) return eTriYes;
	ths->fPosition = (long) ths->fAttrName.size();	// The name might span into the next buffer.
	return eTriMaybe;

}	// CaptureAttrName

// =================================================================================================
// CaptureAttrValue
// ================
//
// Recognize the equal sign and the quoted string value, capture the value along the way.

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::CaptureAttrValue ( PacketMachine * ths, const char * /* unused */ )
{
	const int	bytesPerChar	= ths->fBytesPerChar;
	char		currChar		= 0;
	TriState	result			= eTriMaybe;

	if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

	switch ( ths->fPosition ) {

		case 0 :	// The name should haved ended at the '=', nulls already skipped.

			if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;
			if ( *ths->fBufferPtr != '=' ) return eTriNo;
			ths->fBufferPtr += bytesPerChar;
			ths->fPosition = 1;
			// fall through OK because MatchOpenQuote will check the buffer limit and nulls ...

		case 1 :	// Look for the open quote.

			result = MatchOpenQuote ( ths, NULL );
			if ( result != eTriYes ) return result;
			ths->fPosition = 2;
			// fall through OK because the buffer limit and nulls are checked below ...

		default :	// Look for the close quote, capturing the value along the way.

			assert ( ths->fPosition == 2 );

			const char quoteChar = ths->fQuoteChar;

			while ( ths->fBufferPtr < ths->fBufferLimit ) {
				currChar = *ths->fBufferPtr;
				if ( currChar == quoteChar ) break;
				#if UseStringPushBack
					ths->fAttrValue.push_back ( currChar );
				#else
					ths->fAttrValue.insert ( ths->fAttrValue.end(), currChar );
				#endif
				ths->fBufferPtr += bytesPerChar;
			}

			if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;
			assert ( currChar == quoteChar );
			ths->fBufferPtr += bytesPerChar;	// Advance past the closing quote.
			return eTriYes;

	}

}	// CaptureAttrValue

// =================================================================================================
// RecordStart
// ===========
//
// Note that this routine looks at bytes, not logical characters.  It has to figure out how many
// bytes per character there are so that the other recognizers can skip intervening nulls.

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::RecordStart ( PacketMachine * ths, const char * /* unused */ )
{

	while ( true ) {

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

		const char currByte = *ths->fBufferPtr;

		switch ( ths->fPosition ) {

			case 0 :	// Record the length.
				assert ( ths->fCharForm == eChar8Bit );
				assert ( ths->fBytesPerChar == 1 );
				ths->fPacketStart = ths->fBufferOffset + ((ths->fBufferPtr - 1) - ths->fBufferOrigin);
				ths->fPacketLength = 0;
				ths->fPosition = 1;
				// ! OK to fall through here, we didn't consume a byte in this step.

			case 1 :	// Look for the first null byte.
				if ( currByte != 0 ) return eTriYes;	// No nulls found.
				ths->fCharForm = eChar16BitBig;			// Assume 16 bit big endian for now.
				ths->fBytesPerChar = 2;
				ths->fBufferPtr++;
				ths->fPosition = 2;
				break;	// ! Don't fall through, have to check for the end of the buffer between each byte.

			case 2 :	// One null was found, look for a second.
				if ( currByte != 0 ) return eTriYes;	// Just one null found.
				ths->fBufferPtr++;
				ths->fPosition = 3;
				break;

			case 3 :	// Two nulls were found, look for a third.
				if ( currByte != 0 ) return eTriNo;	// Just two nulls is not valid.
				ths->fCharForm = eChar32BitBig;		// Assume 32 bit big endian for now.
				ths->fBytesPerChar = 4;
				ths->fBufferPtr++;
				return eTriYes;
				break;

		}

	}

}	// RecordStart

// =================================================================================================
// RecognizeBOM
// ============
//
// Recognizing the byte order marker is a surprisingly messy thing to do.  It can't be done by the
// normal string matcher, there are no intervening nulls.  There are 4 transitions after the opening
// quote, the closing quote or one of the three encodings.  For the actual BOM there are then 1 or 2
// following bytes that depend on which of the encodings we're in.  Not to mention that the buffer
// might end at any point.
//
// The intervening null count done earlier determined 8, 16, or 32 bits per character, but not the
// big or little endian nature for the 16/32 bit cases.  The BOM must be present for the 16 and 32
// bit cases in order to determine the endian mode.  There are six possible byte sequences for the
// quoted BOM string, ignoring the differences for quoting with ''' versus '"'.
//
// Keep in mind that for the 16 and 32 bit cases there will be nulls for the quote.  In the table
// below the symbol <quote> means just the one byte containing the ''' or '"'.  The nulls for the
// quote character are explicitly shown.
//
//	<quote> <quote>					- 1: No BOM, this must be an 8 bit case.
//	<quote> \xEF \xBB \xBF <quote>	- 1.12-13: The 8 bit form.
//
//	<quote> \xFE \xFF \x00 <quote>	- 1.22-23: The 16 bit, big endian form
//	<quote> \x00 \xFF \xFE <quote>	- 1.32-33: The 16 bit, little endian form.
//
//	<quote> \x00 \x00 \xFE \xFF \x00 \x00 \x00 <quote>	- 1.32.43-45.56-57: The 32 bit, big endian form.
//	<quote> \x00 \x00 \x00 \xFF \xFE \x00 \x00 <quote>	- 1.32.43.54-57: The 32 bit, little endian form.

enum {
	eBOM_8_1		= 0xEF,
	eBOM_8_2		= 0xBB,
	eBOM_8_3		= 0xBF,
	eBOM_Big_1		= 0xFE,
	eBOM_Big_2		= 0xFF,
	eBOM_Little_1	= eBOM_Big_2,
	eBOM_Little_2	= eBOM_Big_1
};

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::RecognizeBOM ( PacketMachine * ths, const char * /* unused */ )
{
	const int	bytesPerChar	= ths->fBytesPerChar;

	while ( true ) {	// Handle one character at a time, the micro-state (fPosition) changes for each.

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

		const unsigned char currChar = *ths->fBufferPtr;	// ! The BOM bytes look like integers bigger than 127.

		switch ( ths->fPosition ) {

			case  0 :	// Look for the opening quote.
				if ( (currChar != '\'') && (currChar != '"') ) return eTriNo;
				ths->fQuoteChar = currChar;
				ths->fBufferPtr++;
				ths->fPosition = 1;
				break;	// ! Don't fall through, have to check for the end of the buffer between each byte.

			case 1 :	// Look at the byte immediately following the opening quote.
				if ( currChar == ths->fQuoteChar ) {	// Closing quote, no BOM character, must be 8 bit.
					if ( ths->fCharForm != eChar8Bit ) return eTriNo;
					ths->fBufferPtr += bytesPerChar;	// Skip the nulls after the closing quote.
					return eTriYes;
				} else if ( currChar == eBOM_8_1 ) {	// Start of the 8 bit form.
					if ( ths->fCharForm != eChar8Bit ) return eTriNo;
					ths->fBufferPtr++;
					ths->fPosition = 12;
				} else if ( currChar == eBOM_Big_1 ) {	// Start of the 16 bit big endian form.
					if ( ths->fCharForm != eChar16BitBig ) return eTriNo;
					ths->fBufferPtr++;
					ths->fPosition = 22;
				} else if ( currChar == 0 ) {	// Start of the 16 bit little endian or either 32 bit form.
					if ( ths->fCharForm == eChar8Bit ) return eTriNo;
					ths->fBufferPtr++;
					ths->fPosition = 32;
				} else {
					return eTriNo;
				}
				break;

			case 12 :	// Look for the second byte of the 8 bit form.
				if ( currChar != eBOM_8_2 ) return eTriNo;
				ths->fPosition = 13;
				ths->fBufferPtr++;
				break;

			case 13 :	// Look for the third byte of the 8 bit form.
				if ( currChar != eBOM_8_3 ) return eTriNo;
				ths->fPosition = 99;
				ths->fBufferPtr++;
				break;

			case 22 :	// Look for the second byte of the 16 bit big endian form.
				if ( currChar != eBOM_Big_2 ) return eTriNo;
				ths->fPosition = 23;
				ths->fBufferPtr++;
				break;

			case 23 :	// Look for the null before the closing quote of the 16 bit big endian form.
				if ( currChar != 0 ) return eTriNo;
				ths->fBufferPtr++;
				ths->fPosition = 99;
				break;

			case 32 :	// Look at the second byte of the 16 bit little endian or either 32 bit form.
				if ( currChar == eBOM_Little_1 ) {
					ths->fPosition = 33;
				} else if ( currChar == 0 ) {
					ths->fPosition = 43;
				} else {
					return eTriNo;
				}
				ths->fBufferPtr++;
				break;

			case 33 :	// Look for the third byte of the 16 bit little endian form.
				if ( ths->fCharForm != eChar16BitBig ) return eTriNo;	// Null count before assumed big endian.
				if ( currChar != eBOM_Little_2 ) return eTriNo;
				ths->fCharForm = eChar16BitLittle;
				ths->fPosition = 99;
				ths->fBufferPtr++;
				break;

			case 43 :	// Look at the third byte of either 32 bit form.
				if ( ths->fCharForm != eChar32BitBig ) return eTriNo;	// Null count before assumed big endian.
				if ( currChar == eBOM_Big_1 ) {
					ths->fPosition = 44;
				} else if ( currChar == 0 ) {
					ths->fPosition = 54;
				} else {
					return eTriNo;
				}
				ths->fBufferPtr++;
				break;

			case 44 :	// Look for the fourth byte of the 32 bit big endian form.
				if ( currChar != eBOM_Big_2 ) return eTriNo;
				ths->fPosition = 45;
				ths->fBufferPtr++;
				break;

			case 45 :	// Look for the first null before the closing quote of the 32 bit big endian form.
				if ( currChar != 0 ) return eTriNo;
				ths->fPosition = 56;
				ths->fBufferPtr++;
				break;

			case 54 :	// Look for the fourth byte of the 32 bit little endian form.
				ths->fCharForm = eChar32BitLittle;
				if ( currChar != eBOM_Little_1 ) return eTriNo;
				ths->fPosition = 55;
				ths->fBufferPtr++;
				break;

			case 55 :	// Look for the fifth byte of the 32 bit little endian form.
				if ( currChar != eBOM_Little_2 ) return eTriNo;
				ths->fPosition = 56;
				ths->fBufferPtr++;
				break;

			case 56 :	// Look for the next to last null before the closing quote of the 32 bit forms.
				if ( currChar != 0 ) return eTriNo;
				ths->fPosition = 57;
				ths->fBufferPtr++;
				break;

			case 57 :	// Look for the last null before the closing quote of the 32 bit forms.
				if ( currChar != 0 ) return eTriNo;
				ths->fPosition = 99;
				ths->fBufferPtr++;
				break;

			default :	// Look for the closing quote.
				assert ( ths->fPosition == 99 );
				if ( currChar != ths->fQuoteChar ) return eTriNo;
				ths->fBufferPtr += bytesPerChar;	// Skip the nulls after the closing quote.
				return eTriYes;
				break;

		}

	}

}	// RecognizeBOM

// =================================================================================================
// RecordHeadAttr
// ==============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::RecordHeadAttr ( PacketMachine * ths, const char * /* unused */ )
{

	if ( ths->fAttrName == "encoding" ) {

		assert ( ths->fEncodingAttr.empty() );
		ths->fEncodingAttr = ths->fAttrValue;

	} else if ( ths->fAttrName == "bytes" ) {

		long	value	= 0;
		int		count	= (int) ths->fAttrValue.size();
		int		i;

		assert ( ths->fBytesAttr == -1 );

		if ( count > 0 ) {	// Allow bytes='' to be the same as no bytes attribute.

			for ( i = 0; i < count; i++ ) {
				const char	currChar	= ths->fAttrValue[i];
				if ( ('0' <= currChar) && (currChar <= '9') ) {
					value = (value * 10) + (currChar - '0');
				} else {
					ths->fBogusPacket = true;
					value = -1;
					break;
				}
			}
			ths->fBytesAttr = value;

			if ( CharFormIs16Bit ( ths->fCharForm ) ) {
				if ( (ths->fBytesAttr & 1) != 0 ) ths->fBogusPacket = true;
			} else if ( CharFormIs32Bit ( ths->fCharForm ) ) {
				if ( (ths->fBytesAttr & 3) != 0 ) ths->fBogusPacket = true;
			}

		}

	}

	ths->fAttrName.erase ( ths->fAttrName.begin(), ths->fAttrName.end() );
	ths->fAttrValue.erase ( ths->fAttrValue.begin(), ths->fAttrValue.end() );

	return eTriYes;

}	// RecordHeadAttr

// =================================================================================================
// CaptureAccess
// =============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::CaptureAccess ( PacketMachine * ths, const char * /* unused */ )
{
	const int	bytesPerChar	= ths->fBytesPerChar;

	while ( true ) {

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

		const char currChar = *ths->fBufferPtr;

		switch ( ths->fPosition ) {

			case  0 :	// Look for the opening quote.
				if ( (currChar != '\'') && (currChar != '"') ) return eTriNo;
				ths->fQuoteChar = currChar;
				ths->fBufferPtr += bytesPerChar;
				ths->fPosition = 1;
				break;	// ! Don't fall through, have to check for the end of the buffer between each byte.

			case  1 :	// Look for the 'r' or 'w'.
				if ( (currChar != 'r') && (currChar != 'w') ) return eTriNo;
				ths->fAccess = currChar;
				ths->fBufferPtr += bytesPerChar;
				ths->fPosition = 2;
				break;

			default :	// Look for the closing quote.
				assert ( ths->fPosition == 2 );
				if ( currChar != ths->fQuoteChar ) return eTriNo;
				ths->fBufferPtr += bytesPerChar;
				return eTriYes;
				break;

		}

	}

}	// CaptureAccess

// =================================================================================================
// RecordTailAttr
// ==============

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::RecordTailAttr ( PacketMachine * ths, const char * /* unused */ )
{

	// There are no known "general" attributes for the packet trailer.

	ths->fAttrName.erase ( ths->fAttrName.begin(), ths->fAttrName.end() );
	ths->fAttrValue.erase ( ths->fAttrValue.begin(), ths->fAttrValue.end() );

	return eTriYes;

}	// RecordTailAttr

// =================================================================================================
// CheckPacketEnd
// ==============
//
// Check for trailing padding and record the packet length.  We have trailing padding if the bytes
// attribute is present and has a value greater than the current length.

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::CheckPacketEnd ( PacketMachine * ths, const char * /* unused */ )
{
	const int	bytesPerChar	= ths->fBytesPerChar;

	if ( ths->fPosition == 0 ) {	// First call, decide if there is trailing padding.

		const XMP_Int64 currLen64 = (ths->fBufferOffset + (ths->fBufferPtr - ths->fBufferOrigin)) - ths->fPacketStart;
		if ( currLen64 > 0x7FFFFFFF ) throw std::runtime_error ( "Packet length exceeds 2GB-1" );
		const XMP_Int32 currLength = (XMP_Int32)currLen64;

		if ( (ths->fBytesAttr != -1) && (ths->fBytesAttr != currLength) ) {
			if ( ths->fBytesAttr < currLength ) {
				ths->fBogusPacket = true;	// The bytes attribute value is too small.
			} else {
				ths->fPosition = ths->fBytesAttr - currLength;
				if ( (ths->fPosition % ths->fBytesPerChar) != 0 ) {
					ths->fBogusPacket = true;	// The padding is not a multiple of the character size.
					ths->fPosition = (ths->fPosition / ths->fBytesPerChar) * ths->fBytesPerChar;
				}
			}
		}

	}

	while ( ths->fPosition > 0 ) {

		if ( ths->fBufferPtr >= ths->fBufferLimit ) return eTriMaybe;

		const char currChar = *ths->fBufferPtr;

		if ( (currChar != ' ') && (currChar != '\t') && (currChar != '\n') && (currChar != '\r') ) {
			ths->fBogusPacket = true;	// The padding is not whitespace.
			break;						// Stop the packet here.
		}

		ths->fPosition -= bytesPerChar;
		ths->fBufferPtr += bytesPerChar;

	}

	const XMP_Int64 currLen64 = (ths->fBufferOffset + (ths->fBufferPtr - ths->fBufferOrigin)) - ths->fPacketStart;
	if ( currLen64 > 0x7FFFFFFF ) throw std::runtime_error ( "Packet length exceeds 2GB-1" );
	ths->fPacketLength = (XMP_Int32)currLen64;
	return eTriYes;

}	// CheckPacketEnd

// =================================================================================================
// CheckFinalNulls
// ===============
//
// Do some special case processing for little endian characters.  We have to make sure the presumed
// nulls after the last character actually exist, i.e. that the stream does not end too soon.  Note
// that the prior character scanning has moved the buffer pointer to the address following the last
// byte of the last character.  I.e. we're already past the presumed nulls, so we can't check their
// content.  All we can do is verify that the stream does not end too soon.
//
// Doing this check is simple yet subtle.  If we're still in the current buffer then the trailing
// bytes obviously exist.  If we're exactly at the end of the buffer then the bytes also exist.
// The only question is when we're actually past this buffer, partly into the next buffer.  This is
// when "ths->fBufferPtr > ths->fBufferLimit" on entry.  For that case we have to wait until we've
// actually seen enough extra bytes of input.
//
// Since the normal buffer processing is already adjusting for this partial character overrun, all
// that needs to be done here is wait until "ths->fBufferPtr <= ths->fBufferLimit" on entry.  In
// other words, if we're presently too far, ths->fBufferPtr will be adjusted by the amount of the
// overflow the next time XMPScanner::Scan is called.  This might still be too far, so just keep
// waiting for enough data to pass by.
//
// Note that there is a corresponding special case for big endian characters, we must decrement the
// starting offset by the number of leading nulls.  But we don't do that here, we leave it to the
// outer code.  This is because the leading nulls might have been at the exact end of a previous
// buffer, in which case we have to also decrement the length of that raw data snip.

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::CheckFinalNulls ( PacketMachine * ths, const char * /* unused */ )
{

	if ( (ths->fCharForm != eChar8Bit) && CharFormIsLittleEndian ( ths->fCharForm ) ) {
		if ( ths->fBufferPtr > ths->fBufferLimit ) return eTriMaybe;
	}

	return eTriYes;

}	// CheckFinalNulls

// =================================================================================================
// SetNextRecognizer
// =================

void
XMPScanner::PacketMachine::SetNextRecognizer ( RecognizerKind nextRecognizer )
{

	fRecognizer = nextRecognizer;
	fPosition = 0;

}	// SetNextRecognizer

// =================================================================================================
// FindNextPacket
// ==============

// *** When we start validating intervening nulls for 2 and 4 bytes characters, throw an exception
// *** for errors.  Don't return eTriNo, that might skip at an optional point.

XMPScanner::PacketMachine::TriState
XMPScanner::PacketMachine::FindNextPacket ()
{

	TriState	status;

	#define kPacketHead		"?xpacket begin="
	#define kPacketID		"W5M0MpCehiHzreSzNTczkc9d"
	#define kPacketTail		"?xpacket end="

	static const RecognizerInfo	recognizerTable [eRecognizerCount]	= {		// ! Would be safer to assign these explicitly.

		// proc				successNext					failureNext					literal

		{ NULL,				eFailureRecognizer,			eFailureRecognizer,			NULL},			// eFailureRecognizer
		{ NULL,				eSuccessRecognizer,			eSuccessRecognizer,			NULL},			// eSuccessRecognizer

		{ FindLessThan,		eHeadStartRecorder,			eFailureRecognizer,			"H" },			// eLeadInRecognizer
		{ RecordStart,	 	eHeadStartRecognizer,		eLeadInRecognizer,			NULL },			// eHeadStartRecorder
		{ MatchString, 		eBOMRecognizer,				eLeadInRecognizer,			kPacketHead },	// eHeadStartRecognizer

		{ RecognizeBOM, 	eIDTagRecognizer,			eLeadInRecognizer,			NULL },			// eBOMRecognizer

		{ MatchString, 		eIDOpenRecognizer,			eLeadInRecognizer,			" id=" },		// eIDTagRecognizer
		{ MatchOpenQuote,	eIDValueRecognizer,			eLeadInRecognizer,			NULL },			// eIDOpenRecognizer
		{ MatchString, 		eIDCloseRecognizer,			eLeadInRecognizer,			kPacketID },	// eIDValueRecognizer
		{ MatchCloseQuote,	eAttrSpaceRecognizer_1,		eLeadInRecognizer,			NULL },			// eIDCloseRecognizer

		{ MatchChar, 		eAttrNameRecognizer_1,		eHeadEndRecognizer,			" " },			// eAttrSpaceRecognizer_1
		{ CaptureAttrName,	eAttrValueRecognizer_1,		eLeadInRecognizer,			NULL },			// eAttrNameRecognizer_1
		{ CaptureAttrValue,	eAttrValueRecorder_1,		eLeadInRecognizer,			NULL },			// eAttrValueRecognizer_1
		{ RecordHeadAttr,	eAttrSpaceRecognizer_1,		eLeadInRecognizer,			NULL },			// eAttrValueRecorder_1

		{ MatchString, 		eBodyRecognizer,			eLeadInRecognizer,			"?>" },			// eHeadEndRecognizer

		{ FindLessThan,		eTailStartRecognizer,		eBodyRecognizer,			"T"},			// eBodyRecognizer

		{ MatchString, 		eAccessValueRecognizer,		eBodyRecognizer,			kPacketTail },	// eTailStartRecognizer
		{ CaptureAccess,	eAttrSpaceRecognizer_2,		eBodyRecognizer,			NULL },			// eAccessValueRecognizer

		{ MatchChar, 		eAttrNameRecognizer_2,		eTailEndRecognizer,			" " },			// eAttrSpaceRecognizer_2
		{ CaptureAttrName,	eAttrValueRecognizer_2,		eBodyRecognizer,			NULL },			// eAttrNameRecognizer_2
		{ CaptureAttrValue,	eAttrValueRecorder_2,		eBodyRecognizer,			NULL },			// eAttrValueRecognizer_2
		{ RecordTailAttr,	eAttrSpaceRecognizer_2,		eBodyRecognizer,			NULL },			// eAttrValueRecorder_2

		{ MatchString, 		ePacketEndRecognizer,		eBodyRecognizer,			"?>" },			// eTailEndRecognizer
		{ CheckPacketEnd,	eCloseOutRecognizer,		eBodyRecognizer,			"" },			// ePacketEndRecognizer
		{ CheckFinalNulls,	eSuccessRecognizer,			eBodyRecognizer,			"" }			// eCloseOutRecognizer

	};

	while ( true ) {

		switch ( fRecognizer ) {

			case eFailureRecognizer :
				return eTriNo;

			case eSuccessRecognizer :
				return eTriYes;

			default :

				// -------------------------------------------------------------------
				// For everything else, the normal cases, use the state machine table.

				const RecognizerInfo *	thisState	= &recognizerTable [fRecognizer];

				status = thisState->proc ( this, thisState->literal );

				switch ( status ) {

					case eTriNo :
						SetNextRecognizer ( thisState->failureNext );
						continue;

					case eTriYes :
						SetNextRecognizer ( thisState->successNext );
						continue;

					case eTriMaybe :
						fBufferOverrun = (unsigned char)(fBufferPtr - fBufferLimit);
						return eTriMaybe;	// Keep this recognizer intact, to be resumed later.

				}

		}	// switch ( fRecognizer ) { ...

	}	// while ( true ) { ...

}	// FindNextPacket

// =================================================================================================
// =================================================================================================
// class InternalSnip
// ==================

// =================================================================================================
// InternalSnip
// ============

XMPScanner::InternalSnip::InternalSnip ( XMP_Int64 offset, XMP_Int64 length )
{

	fInfo.fOffset = offset;
	fInfo.fLength = length;

}	// InternalSnip

// =================================================================================================
// InternalSnip
// ============

XMPScanner::InternalSnip::InternalSnip ( const InternalSnip & rhs ) :
	fInfo ( rhs.fInfo ),
	fMachine ( NULL )
{

	assert ( rhs.fMachine.get() == NULL );	// Don't copy a snip with a machine.
	assert ( (rhs.fInfo.fEncodingAttr == 0) || (*rhs.fInfo.fEncodingAttr == 0) ); // Don't copy a snip with an encoding.

}	// InternalSnip

// =================================================================================================
// ~InternalSnip
// =============

XMPScanner::InternalSnip::~InternalSnip ()
{
}	// ~InternalSnip


// =================================================================================================
// =================================================================================================
// class XMPScanner
// ================

// =================================================================================================
// DumpSnipList
// ============

#if DEBUG

static const char *	snipStateName [6] = { "not-seen", "pending", "raw-data", "good-packet", "partial", "bad-packet" };

void
XMPScanner::DumpSnipList ( const char * title )
{
	InternalSnipIterator currPos = fInternalSnips.begin();
	InternalSnipIterator endPos  = fInternalSnips.end();

	cout << endl << title << " snip list: " << fInternalSnips.size() << endl;

	for ( ; currPos != endPos; ++currPos ) {
		SnipInfo * currSnip = &currPos->fInfo;
		cout << '\t' << currSnip << ' ' << snipStateName[currSnip->fState] << ' '
		     << currSnip->fOffset << ".." << (currSnip->fOffset + currSnip->fLength - 1)
			 << ' ' << currSnip->fLength << ' ' << endl;
	}
}	// DumpSnipList

#endif

// =================================================================================================
// PrevSnip and NextSnip
// =====================

XMPScanner::InternalSnipIterator
XMPScanner::PrevSnip ( InternalSnipIterator snipPos )
{

	InternalSnipIterator prev = snipPos;
	return --prev;

}	// PrevSnip

XMPScanner::InternalSnipIterator
XMPScanner::NextSnip ( InternalSnipIterator snipPos )
{

	InternalSnipIterator next = snipPos;
	return ++next;

}	// NextSnip

// =================================================================================================
// XMPScanner
// ==========
//
// Initialize the scanner object with one "not seen" snip covering the whole stream.

XMPScanner::XMPScanner ( XMP_Int64 streamLength ) :

	fStreamLength ( streamLength )

{
	InternalSnip	rootSnip ( 0, streamLength );

	if ( streamLength > 0 ) fInternalSnips.push_front ( rootSnip );		// Be nice for empty files.
	// DumpSnipList ( "New XMPScanner" );

}	// XMPScanner

// =================================================================================================
// ~XMPScanner
// ===========

XMPScanner::~XMPScanner()
{

}	// ~XMPScanner

// =================================================================================================
// GetSnipCount
// ============

long
XMPScanner::GetSnipCount ()
{

	return (long)fInternalSnips.size();

}	// GetSnipCount

// =================================================================================================
// StreamAllScanned
// ================

bool
XMPScanner::StreamAllScanned ()
{
	InternalSnipIterator currPos = fInternalSnips.begin();
	InternalSnipIterator endPos  = fInternalSnips.end();

	for ( ; currPos != endPos; ++currPos ) {
		if ( currPos->fInfo.fState == eNotSeenSnip ) return false;
	}
	return true;

}	// StreamAllScanned

// =================================================================================================
// SplitInternalSnip
// =================
//
// Split the given snip into up to 3 pieces.  The new pieces are inserted before and after this one
// in the snip list.  The relOffset is the first byte to be kept, it is relative to this snip.  If
// the preceeding or following snips have the same state as this one, just shift the boundaries.
// I.e. move the contents from one snip to the other, don't create a new snip.

// *** To be thread safe we ought to lock the entire list during manipulation.  Let data scanning
// *** happen in parallel, serialize all mucking with the list.

void
XMPScanner::SplitInternalSnip ( InternalSnipIterator snipPos, XMP_Int64 relOffset, XMP_Int64 newLength )
{

	assert ( (relOffset + newLength) > relOffset );	// Check for overflow.
	assert ( (relOffset + newLength) <= snipPos->fInfo.fLength );

	// -----------------------------------
	// First deal with the low offset end.

	if ( relOffset > 0 ) {

		InternalSnipIterator prevPos;
		if ( snipPos != fInternalSnips.begin() ) prevPos = PrevSnip ( snipPos );

		if ( (snipPos != fInternalSnips.begin()) && (snipPos->fInfo.fState == prevPos->fInfo.fState) ) {
			prevPos->fInfo.fLength += relOffset;	// Adjust the preceeding snip.
		} else {
			InternalSnip headExcess ( snipPos->fInfo.fOffset, relOffset );
			headExcess.fInfo.fState = snipPos->fInfo.fState;
			headExcess.fInfo.fOutOfOrder = snipPos->fInfo.fOutOfOrder;
			fInternalSnips.insert ( snipPos, headExcess );	// Insert the head piece before the middle piece.
		}

		snipPos->fInfo.fOffset += relOffset;	// Adjust the remainder of this snip.
		snipPos->fInfo.fLength -= relOffset;

	}

	// ----------------------------------
	// Now deal with the high offset end.

	if ( newLength < snipPos->fInfo.fLength ) {

		InternalSnipIterator nextPos    = NextSnip ( snipPos );
		const XMP_Int64      tailLength = snipPos->fInfo.fLength - newLength;

		if ( (nextPos != fInternalSnips.end()) && (snipPos->fInfo.fState == nextPos->fInfo.fState) ) {
			nextPos->fInfo.fOffset -= tailLength;		// Adjust the following snip.
			nextPos->fInfo.fLength += tailLength;
		} else {
			InternalSnip tailExcess ( (snipPos->fInfo.fOffset + newLength), tailLength );
			tailExcess.fInfo.fState = snipPos->fInfo.fState;
			tailExcess.fInfo.fOutOfOrder = snipPos->fInfo.fOutOfOrder;
			fInternalSnips.insert ( nextPos, tailExcess );		// Insert the tail piece after the middle piece.
		}

		snipPos->fInfo.fLength = newLength;

	}

}	// SplitInternalSnip

// =================================================================================================
// MergeInternalSnips
// ==================

XMPScanner::InternalSnipIterator
XMPScanner::MergeInternalSnips ( InternalSnipIterator firstPos, InternalSnipIterator secondPos )
{

	firstPos->fInfo.fLength += secondPos->fInfo.fLength;
	fInternalSnips.erase ( secondPos );
	return firstPos;

}	// MergeInternalSnips

// =================================================================================================
// Scan
// ====

void
XMPScanner::Scan ( const void * bufferOrigin, XMP_Int64 bufferOffset, XMP_Int64 bufferLength )
{
	XMP_Int64	relOffset;

	#if 0
		cout << "Scan: @ " << bufferOrigin << ", " << bufferOffset << ", " << bufferLength << endl;
	#endif

	if ( bufferLength == 0 ) return;

	// ----------------------------------------------------------------
	// These comparisons are carefully done to avoid overflow problems.

	if ( (bufferOffset >= fStreamLength) ||
		 (bufferLength > (fStreamLength - bufferOffset)) ||
		 (bufferOrigin == 0) ) {
		throw ScanError ( "Bad origin, offset, or length" );
	}

	// ----------------------------------------------------------------------------------------------
	// This buffer must be within a not-seen snip.  Find it and split it.  The first snip whose whose
	// end is beyond the buffer must be the enclosing one.

	// *** It would be friendly for rescans for out of order problems to accept any buffer postion.

	const XMP_Int64			endOffset	= bufferOffset + bufferLength - 1;
	InternalSnipIterator	snipPos	= fInternalSnips.begin();

	while ( endOffset > (snipPos->fInfo.fOffset + snipPos->fInfo.fLength - 1) ) ++ snipPos;
	if ( snipPos->fInfo.fState != eNotSeenSnip ) throw ScanError ( "Already seen" );

	relOffset = bufferOffset - snipPos->fInfo.fOffset;
	if ( (relOffset + bufferLength) > snipPos->fInfo.fLength ) throw ScanError ( "Not within existing snip" );

	SplitInternalSnip ( snipPos, relOffset, bufferLength );		// *** If sequential & prev is partial, just tack on,

	// --------------------------------------------------------
	// Merge this snip with the preceeding snip if appropriate.

	// *** When out of order I/O is supported we have to do something about buffers who's predecessor is not seen.

	if ( snipPos->fInfo.fOffset > 0 ) {
		InternalSnipIterator prevPos = PrevSnip ( snipPos );
		if ( prevPos->fInfo.fState == ePartialPacketSnip ) snipPos = MergeInternalSnips ( prevPos, snipPos );
	}

	// ----------------------------------
	// Look for packets within this snip.

	snipPos->fInfo.fState = ePendingSnip;
	PacketMachine* thisMachine = snipPos->fMachine.get();
	// DumpSnipList ( "Before scan" );

	if ( thisMachine != 0 ) {
		thisMachine->AssociateBuffer ( bufferOffset, bufferOrigin, bufferLength );
	} else {
		// *** snipPos->fMachine.reset ( new PacketMachine ( bufferOffset, bufferOrigin, bufferLength ) );		VC++ lacks reset
		#if 0
			snipPos->fMachine = auto_ptr<PacketMachine> ( new PacketMachine ( bufferOffset, bufferOrigin, bufferLength ) );
		#else
			{
				// Some versions of gcc complain about the assignment operator above.  This avoids the gcc bug.
				PacketMachine *	pm	= new PacketMachine ( bufferOffset, bufferOrigin, bufferLength );
				auto_ptr<PacketMachine>	ap ( pm );
				snipPos->fMachine = ap;
			}
		#endif
		thisMachine = snipPos->fMachine.get();
	}

	bool	bufferDone	= false;
	while ( ! bufferDone ) {

		PacketMachine::TriState	foundPacket = thisMachine->FindNextPacket();

		if ( foundPacket == PacketMachine::eTriNo ) {

			// -----------------------------------------------------------------------
			// No packet, mark the snip as raw data and get rid of the packet machine.
			// We're done with this buffer.

			snipPos->fInfo.fState = eRawInputSnip;
			#if 0
				snipPos->fMachine = auto_ptr<PacketMachine>();	// *** snipPos->fMachine.reset();	VC++ lacks reset
			#else
				{
					// Some versions of gcc complain about the assignment operator above.  This avoids the gcc bug.
					auto_ptr<PacketMachine>	ap ( 0 );
					snipPos->fMachine = ap;
				}
			#endif
			bufferDone = true;

		} else {

			// ---------------------------------------------------------------------------------------------
			// Either a full or partial packet.  First trim any excess off of the front as a raw input snip.
			// If this is a partial packet mark the snip and keep the packet machine to be resumed later.
			// We're done with this buffer, the partial packet by definition extends to the end.  If this is
			// a complete packet first extract the additional information from the packet machine.  If there
			// is leftover data split the snip and transfer the packet machine to the new trailing snip.

			if ( thisMachine->fPacketStart > snipPos->fInfo.fOffset ) {

				// There is data at the front of the current snip that must be trimmed.
				SnipState	savedState	= snipPos->fInfo.fState;
				snipPos->fInfo.fState = eRawInputSnip;	// ! So it gets propagated to the trimmed front part.
				relOffset = thisMachine->fPacketStart - snipPos->fInfo.fOffset;
				SplitInternalSnip ( snipPos, relOffset, (snipPos->fInfo.fLength - relOffset) );
				snipPos->fInfo.fState = savedState;

			}

			if ( foundPacket == PacketMachine::eTriMaybe ) {

				// We have only found a partial packet.
				snipPos->fInfo.fState = ePartialPacketSnip;
				bufferDone = true;

			} else {

				// We have found a complete packet. Extract all the info for it and split any trailing data.

				InternalSnipIterator	packetSnip	= snipPos;
				SnipState				packetState	= eValidPacketSnip;

				if ( thisMachine->fBogusPacket ) packetState = eBadPacketSnip;

				packetSnip->fInfo.fAccess = thisMachine->fAccess;
				packetSnip->fInfo.fCharForm = thisMachine->fCharForm;
				packetSnip->fInfo.fBytesAttr = thisMachine->fBytesAttr;
				packetSnip->fInfo.fEncodingAttr = thisMachine->fEncodingAttr.c_str();
				thisMachine->fEncodingAttr.erase ( thisMachine->fEncodingAttr.begin(), thisMachine->fEncodingAttr.end() );

				if ( (thisMachine->fCharForm != eChar8Bit) && CharFormIsBigEndian ( thisMachine->fCharForm ) ) {

					// ------------------------------------------------------------------------------
					// Handle a special case for big endian characters.  The packet machine works as
					// though things were little endian.  The packet starting offset points to the
					// byte containing the opening '<', and the length includes presumed nulls that
					// follow the last "real" byte.  If the characters are big endian we now have to
					// decrement the starting offset of the packet, and also decrement the length of
					// the previous snip.
					//
					// Note that we can't do this before the head trimming above in general.  The
					// nulls might have been exactly at the end of a buffer and already in the
					// previous snip.  We are doing this before trimming the tail from the raw snip
					// containing the packet.  We adjust the raw snip's size because it ends with
					// the input buffer.  We don't adjust the packet's size, it is already correct.
					//
					// The raw snip (the one before the packet) might entirely disappear.  A simple
					// example of this is when the packet is at the start of the file.

					assert ( packetSnip != fInternalSnips.begin() );	// Leading nulls were trimmed!

					if ( packetSnip != fInternalSnips.begin() ) {	// ... but let's program defensibly.

						InternalSnipIterator prevSnip  = PrevSnip ( packetSnip );
						const unsigned int nullsToAdd = ( CharFormIs16Bit ( thisMachine->fCharForm ) ? 1 : 3 );

						assert ( nullsToAdd <= prevSnip->fInfo.fLength );
						prevSnip->fInfo.fLength -= nullsToAdd;
						if ( prevSnip->fInfo.fLength == 0 ) (void) fInternalSnips.erase ( prevSnip );

						packetSnip->fInfo.fOffset	-= nullsToAdd;
						packetSnip->fInfo.fLength	+= nullsToAdd;
						thisMachine->fPacketStart	-= nullsToAdd;

					}

				}

				if ( thisMachine->fPacketLength == snipPos->fInfo.fLength ) {

					// This packet ends exactly at the end of the current snip.
					#if 0
						snipPos->fMachine = auto_ptr<PacketMachine>();	// *** snipPos->fMachine.reset();	VC++ lacks reset
					#else
						{
							// Some versions of gcc complain about the assignment operator above.  This avoids the gcc bug.
							auto_ptr<PacketMachine>	ap ( 0 );
							snipPos->fMachine = ap;
						}
					#endif
					bufferDone = true;

				} else {

					// There is trailing data to split from the just found packet.
					SplitInternalSnip ( snipPos, 0, thisMachine->fPacketLength );

					InternalSnipIterator	tailPos	= NextSnip ( snipPos );

					tailPos->fMachine = snipPos->fMachine;	// auto_ptr assignment - taking ownership
					thisMachine->ResetMachine ();

					snipPos = tailPos;

				}

				packetSnip->fInfo.fState = packetState;	// Do this last to avoid messing up the tail split.
				// DumpSnipList ( "Found a packet" );

			}

		}

	}

	// --------------------------------------------------------
	// Merge this snip with the preceeding snip if appropriate.

	// *** When out of order I/O is supported we have to check the following snip too.

	if ( (snipPos->fInfo.fOffset > 0) && (snipPos->fInfo.fState == eRawInputSnip) ) {
		InternalSnipIterator prevPos = PrevSnip ( snipPos );
		if ( prevPos->fInfo.fState == eRawInputSnip ) snipPos = MergeInternalSnips ( prevPos, snipPos );
	}

	// DumpSnipList ( "After scan" );

}	// Scan

// =================================================================================================
// Report
// ======

void
XMPScanner::Report ( SnipInfoVector& snips )
{
	const int				count	= (int)fInternalSnips.size();
	InternalSnipIterator	snipPos	= fInternalSnips.begin();

	int	s;

	// DumpSnipList ( "Report" );

	snips.erase ( snips.begin(), snips.end() );		// ! Should use snips.clear, but VC++ doesn't have it.
	snips.reserve ( count );

	for ( s = 0; s < count; s += 1 ) {
		snips.push_back ( SnipInfo ( snipPos->fInfo.fState, snipPos->fInfo.fOffset, snipPos->fInfo.fLength ) );
		snips[s] = snipPos->fInfo;	// Pick up all of the fields.
		++ snipPos;
	}

}	// Report

// =================================================================================================

#endif	// EnablePacketScanning
