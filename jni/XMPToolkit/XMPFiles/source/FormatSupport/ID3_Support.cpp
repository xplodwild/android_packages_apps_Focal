// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/FormatSupport/ID3_Support.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

#include "source/UnicodeConversions.hpp"
#include "source/XIO.hpp"

#include <vector>

#define MIN(a,b)	((a) < (b) ? (a) : (b))

namespace ID3_Support {

// =================================================================================================

ID3GenreMap* kMapID3GenreCodeToName = 0;	// Map from a code like "21" or "RX" to the full name.
ID3GenreMap* kMapID3GenreNameToCode = 0;	// Map from the full name to a code like "21" or "RX".

static size_t numberedGenreCount = 0;	// Set in InitializeGlobals, used in ID3v1Tag::read and write.

struct GenreInfo { const char * code; const char * name; };

static const GenreInfo kAbbreviatedGenres[] = {	// ID3 v3 or v4 genre abbreviations.
	{ "RX", "Remix" },
	{ "CR", "Cover" },
	{ 0, 0 }
};

static const GenreInfo kNumberedGenres[] = {	// Numeric genre codes from ID3 v1, complete range of 0..125.
	{   "0", "Blues" },
	{   "1", "Classic Rock" },
	{   "2", "Country" },
	{   "3", "Dance" },
	{   "4", "Disco" },
	{   "5", "Funk" },
	{   "6", "Grunge" },
	{   "7", "Hip-Hop" },
	{   "8", "Jazz" },
	{   "9", "Metal" },
	{  "10", "New Age" },
	{  "11", "Oldies" },
	{  "12", "Other" },
	{  "13", "Pop" },
	{  "14", "R&B" },
	{  "15", "Rap" },
	{  "16", "Reggae" },
	{  "17", "Rock" },
	{  "18", "Techno" },
	{  "19", "Industrial" },
	{  "20", "Alternative" },
	{  "21", "Ska" },
	{  "22", "Death Metal" },
	{  "23", "Pranks" },
	{  "24", "Soundtrack" },
	{  "25", "Euro-Techno" },
	{  "26", "Ambient" },
	{  "27", "Trip-Hop" },
	{  "28", "Vocal" },
	{  "29", "Jazz+Funk" },
	{  "30", "Fusion" },
	{  "31", "Trance" },
	{  "32", "Classical" },
	{  "33", "Instrumental" },
	{  "34", "Acid" },
	{  "35", "House" },
	{  "36", "Game" },
	{  "37", "Sound Clip" },
	{  "38", "Gospel" },
	{  "39", "Noise" },
	{  "40", "AlternRock" },
	{  "41", "Bass" },
	{  "42", "Soul" },
	{  "43", "Punk" },
	{  "44", "Space" },
	{  "45", "Meditative" },
	{  "46", "Instrumental Pop" },
	{  "47", "Instrumental Rock" },
	{  "48", "Ethnic" },
	{  "49", "Gothic" },
	{  "50", "Darkwave" },
	{  "51", "Techno-Industrial" },
	{  "52", "Electronic" },
	{  "53", "Pop-Folk" },
	{  "54", "Eurodance" },
	{  "55", "Dream" },
	{  "56", "Southern Rock" },
	{  "57", "Comedy" },
	{  "58", "Cult" },
	{  "59", "Gangsta" },
	{  "60", "Top 40" },
	{  "61", "Christian Rap" },
	{  "62", "Pop/Funk" },
	{  "63", "Jungle" },
	{  "64", "Native American" },
	{  "65", "Cabaret" },
	{  "66", "New Wave" },
	{  "67", "Psychadelic" },
	{  "68", "Rave" },
	{  "69", "Showtunes" },
	{  "70", "Trailer" },
	{  "71", "Lo-Fi" },
	{  "72", "Tribal" },
	{  "73", "Acid Punk" },
	{  "74", "Acid Jazz" },
	{  "75", "Polka" },
	{  "76", "Retro" },
	{  "77", "Musical" },
	{  "78", "Rock & Roll" },
	{  "79", "Hard Rock" },
	{  "80", "Folk" },
	{  "81", "Folk-Rock" },
	{  "82", "National Folk" },
	{  "83", "Swing" },
	{  "84", "Fast Fusion" },
	{  "85", "Bebob" },
	{  "86", "Latin" },
	{  "87", "Revival" },
	{  "88", "Celtic" },
	{  "89", "Bluegrass" },
	{  "90", "Avantgarde" },
	{  "91", "Gothic Rock" },
	{  "92", "Progressive Rock" },
	{  "93", "Psychedelic Rock" },
	{  "94", "Symphonic Rock" },
	{  "95", "Slow Rock" },
	{  "96", "Big Band" },
	{  "97", "Chorus" },
	{  "98", "Easy Listening" },
	{  "99", "Acoustic" },
	{ "100", "Humour" },
	{ "101", "Speech" },
	{ "102", "Chanson" },
	{ "103", "Opera" },
	{ "104", "Chamber Music" },
	{ "105", "Sonata" },
	{ "106", "Symphony" },
	{ "107", "Booty Bass" },
	{ "108", "Primus" },
	{ "109", "Porn Groove" },
	{ "110", "Satire" },
	{ "111", "Slow Jam" },
	{ "112", "Club" },
	{ "113", "Tango" },
	{ "114", "Samba" },
	{ "115", "Folklore" },
	{ "116", "Ballad" },
	{ "117", "Power Ballad" },
	{ "118", "Rhythmic Soul" },
	{ "119", "Freestyle" },
	{ "120", "Duet" },
	{ "121", "Punk Rock" },
	{ "122", "Drum Solo" },
	{ "123", "A capella" },	// ! Should be Acapella, keep space for compatibility with old code.
	{ "124", "Euro-House" },
	{ "125", "Dance Hall" },
	{ 0, 0 }
};

// =================================================================================================

bool InitializeGlobals()
{

	kMapID3GenreCodeToName = new ID3GenreMap;
	if ( kMapID3GenreCodeToName == 0 ) return false;
	kMapID3GenreNameToCode = new ID3GenreMap;
	if ( kMapID3GenreNameToCode == 0 ) return false;
	
	ID3GenreMap::value_type newValue;
	
	size_t i;
	
	for ( i = 0; kNumberedGenres[i].code != 0; ++i ) {
		XMP_Assert ( (long)i == strtol ( kNumberedGenres[i].code, 0, 10 ) );
		ID3GenreMap::value_type code2Name ( kNumberedGenres[i].code, kNumberedGenres[i].name );
		kMapID3GenreCodeToName->insert ( kMapID3GenreCodeToName->end(), code2Name );
		ID3GenreMap::value_type name2Code ( kNumberedGenres[i].name, kNumberedGenres[i].code );
		kMapID3GenreNameToCode->insert ( kMapID3GenreNameToCode->end(), name2Code );
	}
	
	numberedGenreCount = i;	// Used in ID3v1Tag::read and write.
	
	for ( i = 0; kAbbreviatedGenres[i].code != 0; ++i ) {
		ID3GenreMap::value_type code2Name ( kAbbreviatedGenres[i].code, kAbbreviatedGenres[i].name );
		kMapID3GenreCodeToName->insert ( kMapID3GenreCodeToName->end(), code2Name );
		ID3GenreMap::value_type name2Code ( kAbbreviatedGenres[i].name, kAbbreviatedGenres[i].code );
		kMapID3GenreNameToCode->insert ( kMapID3GenreNameToCode->end(), name2Code );
	}

	return true;

}	// InitializeGlobals

// =================================================================================================

void TerminateGlobals()
{
	delete kMapID3GenreCodeToName;
	delete kMapID3GenreNameToCode;
	kMapID3GenreCodeToName = kMapID3GenreNameToCode = 0;
}

// =================================================================================================
// GenreUtils
// =================================================================================================

const char * GenreUtils::FindGenreName ( const std::string & code )
{
	// Lookup a genre code and return its name if known, otherwise 0.
	
	const char * name = 0;
	ID3GenreMap::iterator mapPos = kMapID3GenreCodeToName->find ( code.c_str() );
	if ( mapPos != kMapID3GenreCodeToName->end() ) name = mapPos->second;
	return name;

}

// =================================================================================================
	
const char * GenreUtils::FindGenreCode ( const std::string & name )
{
	// Lookup a genre name and return its code if known, otherwise 0.
	
	const char * code = 0;
	ID3GenreMap::iterator mapPos = kMapID3GenreNameToCode->find ( name.c_str() );
	if ( mapPos != kMapID3GenreNameToCode->end() ) code = mapPos->second;
	return code;

}

// =================================================================================================

static void StripOutsideSpaces ( std::string * value )
{
	size_t length = value->size();
	size_t first, last;
	
	for ( first = 0; ((first < length) && ((*value)[first] == ' ')); ++first ) {}
	if ( first == length ) { value->erase(); return; }
	XMP_Assert ( (first < length) && ((*value)[first] != ' ') );
	
	for ( last = length-1; ((last > first) && ((*value)[last] == ' ')); --last ) {}
	if ( (first == 0) && (last == length-1) ) return;
	
	size_t newLen = last - first + 1;
	if ( newLen < length ) *value = value->substr ( first, newLen );
	
}

// =================================================================================================

void GenreUtils::ConvertGenreToXMP ( const char * id3Genre, std::string * xmpGenre )
{
	// If the first character of TCON is not '(' then the entire TCON value is taken as the genre
	// name and the suffix is empty.
	//
	// If the first character of TCON is '(' then the string up to ')' (or the end) is taken as the
	// coded genre name. The rest of the TCON value after ')' is taken as the suffix.
	//
	// If the coded name is known then the corresponsing full name is used as the genre name, with
	// no parens.
	//
	// If the coded name is not known then the coded name with parens is used as the genre name.
	//
	// The value of xmpDM:genre begins with the genre name. If the suffix is not empty we append
	// "; " and the suffix. The known coded genre names currently do not use semicolon.
	//
	// Keeping the parens when importing unknown coded names might seem odd. But it preserves the
	// ID3 syntax when exporting. Otherwise we would import "(XX)" and export "XX". We don't add
	// parens all the time on export, that would import "Blues/R&B" and export "(Blues/R&B)".

	xmpGenre->erase();
	size_t id3Length = strlen ( id3Genre );
	if ( id3Length == 0 ) return;
	
	if ( id3Genre[0] != '(' ) {
		// No left paren, take the whole TCON value as the XMP value.
		xmpGenre->assign ( id3Genre, id3Length );
		StripOutsideSpaces ( xmpGenre );
		return;
	}
	
	// The first character of TCON is '(', process the coded part and the suffix.
	
	size_t codeEnd;
	std::string genreCode, suffix;

	for ( codeEnd = 1; ((codeEnd < id3Length) && (id3Genre[codeEnd] != ')')); ++codeEnd ) {}
	genreCode.assign ( &id3Genre[1], codeEnd-1 );
	if ( codeEnd < id3Length ) suffix.assign ( &id3Genre[codeEnd+1], id3Length-codeEnd-1 );

	StripOutsideSpaces ( &genreCode );
	StripOutsideSpaces ( &suffix );

	if ( genreCode.empty() ) {

		(*xmpGenre) = suffix;	// Degenerate case of "()suffix", treat as if "suffix".

	} else {

		const char * fullName = FindGenreName ( genreCode );

		if ( fullName != 0 ) {
			(*xmpGenre) = fullName;
		} else {
			(*xmpGenre) = '(';
			(*xmpGenre) += genreCode;
			(*xmpGenre) += ')';
		}

		if ( ! suffix.empty() ) {
			(*xmpGenre) += "; ";
			(*xmpGenre) += suffix;
		}

	}

}

// =================================================================================================

void GenreUtils::ConvertGenreToID3 ( const char * xmpGenre, std::string * id3Genre )
{
	// The genre name is the xmpDM:genre value up to ';', with spaces at the front or back removed.
	// The suffix is everything after ';', also with spaces at the front or back removed.
	//
	// If the genre name is known, it is replaced by the coded name in parens.
	//
	// The TCON value is the genre name plus the suffix. If the genre name does not end in ')' then
	// a space is inserted.
	
	id3Genre->erase();
	size_t xmpLength = strlen ( xmpGenre );
	if ( xmpLength == 0 ) return;
	
	size_t nameEnd;
	std::string genreName, suffix;
	
	for ( nameEnd = 0; ((nameEnd < xmpLength) && (xmpGenre[nameEnd] != ';')); ++nameEnd ) {}
	genreName.assign ( xmpGenre, nameEnd );
	if ( nameEnd < xmpLength ) suffix.assign ( &xmpGenre[nameEnd+1], xmpLength-nameEnd-1 );

	StripOutsideSpaces ( &genreName );
	StripOutsideSpaces ( &suffix );
	
	if ( genreName.empty() ) {

		(*id3Genre) = suffix;	// Degenerate case of "; suffix", treat as if "suffix".

	} else {

		const char * codedName = FindGenreCode ( genreName );
		if ( codedName != 0 ) {
			genreName = '(';
			genreName += codedName;
			genreName += ')';
		}
		
		(*id3Genre) = genreName;
		if ( ! suffix.empty() ) {
			if ( genreName[genreName.size()-1] != ')' ) (*id3Genre) += ' ';
			(*id3Genre) += suffix;
		}

	}

}

// =================================================================================================
// ID3Header
// =================================================================================================

bool ID3Header::read ( XMP_IO* file )
{

	XMP_Assert ( sizeof(fields) == kID3_TagHeaderSize );
	file->ReadAll ( this->fields, kID3_TagHeaderSize );

	if ( ! CheckBytes ( &this->fields[ID3Header::o_id], "ID3", 3 ) ) {
		// chuck in default contents:
		const static char defaultHeader[kID3_TagHeaderSize] = { 'I', 'D', '3', 3, 0, 0, 0, 0, 0, 0 };
		memcpy ( this->fields, defaultHeader, kID3_TagHeaderSize );
		return false; // no header found (o.k.) thus stick with new, default header constructed above
	}

	XMP_Uns8 major = this->fields[o_vMajor];
	XMP_Uns8 minor = this->fields[o_vMinor];
	XMP_Validate ( ((2 <= major) && (major <= 4)), "Invalid ID3 major version", kXMPErr_BadFileFormat );

	return true;

}

// =================================================================================================

void ID3Header::write ( XMP_IO* file, XMP_Int64 tagSize )
{

	XMP_Assert ( ((XMP_Int64)kID3_TagHeaderSize <= tagSize) && (tagSize < 256*1024*1024) );	// 256 MB limit due to synching.

	XMP_Uns32 synchSize = int32ToSynch ( (XMP_Uns32)tagSize - kID3_TagHeaderSize );
	PutUns32BE ( synchSize, &this->fields[ID3Header::o_size] );
	file->Write ( this->fields, kID3_TagHeaderSize );

}

// =================================================================================================
// ID3v2Frame
// =================================================================================================

#define frameDefaults	id(0), flags(0), content(0), contentSize(0), active(true), changed(false)

ID3v2Frame::ID3v2Frame() : frameDefaults
{
	XMP_Assert ( sizeof(fields) == kV23_FrameHeaderSize );	// Only need to do this in one place.
	memset ( this->fields, 0, kV23_FrameHeaderSize );
}

// =================================================================================================

ID3v2Frame::ID3v2Frame ( XMP_Uns32 id ) : frameDefaults
{
	memset ( this->fields, 0, kV23_FrameHeaderSize );
	this->id = id;
	PutUns32BE ( id, &this->fields[o_id] );
}

// =================================================================================================

void ID3v2Frame::release()
{
	if ( this->content != 0 ) delete this->content;
	this->content = 0;
	this->contentSize = 0;
}

// =================================================================================================

void ID3v2Frame::setFrameValue ( const std::string& rawvalue, bool needDescriptor,
											  bool utf16, bool isXMPPRIVFrame, bool needEncodingByte )
{

	std::string value;

	if ( isXMPPRIVFrame ) {

		XMP_Assert ( (! needDescriptor) && (! utf16) );

		value.append ( "XMP\0", 4 );
		value.append ( rawvalue );
		value.append ( "\0", 1  ); // final zero byte

	} else {

		if ( needEncodingByte ) {
			if ( utf16 ) {
				value.append ( "\x1", 1  );
			} else {
				value.append ( "\x0", 1  );
			}
		}

		if ( needDescriptor ) value.append ( "eng", 3 );

		if ( utf16 ) {

			if ( needDescriptor ) value.append ( "\xFF\xFE\0\0", 4 );

			value.append ( "\xFF\xFE", 2 );
			std::string utf16str;
			ToUTF16 ( (XMP_Uns8*) rawvalue.c_str(), rawvalue.size(), &utf16str, false );
			value.append ( utf16str );
			value.append ( "\0\0", 2 );

		} else {

			std::string convertedValue;
			ReconcileUtils::UTF8ToLatin1 ( rawvalue.c_str(), rawvalue.size(), &convertedValue );

			if ( needDescriptor ) value.append ( "\0", 1 );
			value.append ( convertedValue );
			value.append ( "\0", 1  );

		}

	}

	this->changed = true;
	this->release();

	this->contentSize = (XMP_Int32) value.size();
	XMP_Validate ( (this->contentSize < 20*1024*1024), "XMP Property exceeds 20MB in size", kXMPErr_InternalFailure );
	this->content = new char [ this->contentSize ];
	memcpy ( this->content, value.c_str(), this->contentSize );

}	// ID3v2Frame::setFrameValue

// =================================================================================================

XMP_Int64 ID3v2Frame::read ( XMP_IO* file, XMP_Uns8 majorVersion )
{
	XMP_Assert ( (2 <= majorVersion) && (majorVersion <= 4) );

	this->release(); // ensures/allows reuse of 'curFrame'
	XMP_Int64 start = file->Offset();
	
	if ( majorVersion > 2 ) {
		file->ReadAll ( this->fields, kV23_FrameHeaderSize );
	} else {
		// Read the 6 byte v2.2 header into the 10 byte form.
		memset ( this->fields, 0, kV23_FrameHeaderSize );	// Clear all of the bytes.
		file->ReadAll ( &this->fields[o_id], 3 );		// Leave the low order byte as zero.
		file->ReadAll ( &this->fields[o_size+1], 3 );	// Read big endian UInt24.
	}

	this->id = GetUns32BE ( &this->fields[o_id] );

	if ( this->id == 0 ) {
		file->Seek ( start, kXMP_SeekFromStart );	// Zero ID must mean nothing but padding.
		return 0;
	}

	this->flags = GetUns16BE ( &this->fields[o_flags] );
	XMP_Validate ( (0 == (this->flags & 0xEE)), "invalid lower bits in frame flags", kXMPErr_BadFileFormat );

	//*** flag handling, spec :429ff aka line 431ff  (i.e. Frame should be discarded)
	//  compression and all of that..., unsynchronisation
	this->contentSize = GetUns32BE ( &this->fields[o_size] );
	if ( majorVersion == 4 ) this->contentSize = synchToInt32 ( this->contentSize );

	XMP_Validate ( (this->contentSize >= 0), "negative frame size", kXMPErr_BadFileFormat );
	XMP_Validate ( (this->contentSize < 20*1024*1024), "single frame exceeds 20MB", kXMPErr_BadFileFormat );

	this->content = new char [ this->contentSize ];

	file->ReadAll ( this->content, this->contentSize );
	return file->Offset() - start;

}	// ID3v2Frame::read

// =================================================================================================

void ID3v2Frame::write ( XMP_IO* file, XMP_Uns8 majorVersion )
{
	XMP_Assert ( (2 <= majorVersion) && (majorVersion <= 4) );

	if ( majorVersion < 4 ) {
		PutUns32BE ( this->contentSize, &this->fields[o_size] );
	} else {
		PutUns32BE ( int32ToSynch ( this->contentSize ), &this->fields[o_size] );
	}

	if ( majorVersion > 2 ) {
		file->Write ( this->fields, kV23_FrameHeaderSize );
	} else {
		file->Write ( &this->fields[o_id], 3 );
		file->Write ( &this->fields[o_size+1], 3 );
	}

	file->Write ( this->content, this->contentSize );

}	// ID3v2Frame::write

// =================================================================================================
		
bool ID3v2Frame::advancePastCOMMDescriptor ( XMP_Int32& pos )
{

		if ( (this->contentSize - pos) <= 3 ) return false; // silent error, no room left behing language tag
		if ( ! CheckBytes ( &this->content[pos], "eng", 3 ) ) return false; // not an error, but leave all non-eng tags alone...

		pos += 3; // skip lang tag
		if ( pos >= this->contentSize ) return false; // silent error

		while ( pos < this->contentSize ) {
			if ( this->content[pos++] == 0x00 ) break;
		}
		if ( (pos < this->contentSize) && (this->content[pos] == 0x00) ) pos++;

		if ( (pos == 5) && (this->contentSize == 6) && (GetUns16BE(&this->content[4]) == 0x0031) ) {
			return false;
		}

		if ( pos > 4 ) {
			std::string descriptor = std::string ( &this->content[4], pos-1 );
			if ( 0 == descriptor.substr(0,4).compare( "iTun" ) ) {	// begins with engiTun ?
				return false; // leave alone, then
			}
		}

		return true; //o.k., descriptor skipped, time for the real thing.

}	// ID3v2Frame::advancePastCOMMDescriptor

// =================================================================================================

bool ID3v2Frame::getFrameValue ( XMP_Uns8 majorVersion, XMP_Uns32 logicalID, std::string* utf8string )
{

	XMP_Assert ( (this->content != 0) && (this->contentSize >= 0) && (this->contentSize < 20*1024*1024) );

	if ( this->contentSize == 0 ) {
		utf8string->erase();
		return true; // ...it is "of interest", even if empty contents.
	}

	XMP_Int32 pos = 0;
	XMP_Uns8 encByte = 0;
	// WCOP does not have an encoding byte, for all others: use [0] as EncByte, advance pos
	if ( logicalID != 0x57434F50 ) {
		encByte = this->content[0];
		pos++;
	}

	// mode specific forks, COMM or USLT
	bool commMode = ( (logicalID == 0x434F4D4D) || (logicalID == 0x55534C54) );

	switch ( encByte ) {

		case 0: //ISO-8859-1, 0-terminated
		{
			if ( commMode && (! advancePastCOMMDescriptor ( pos )) ) return false; // not a frame of interest!

			char* localPtr  = &this->content[pos];
			size_t localLen = this->contentSize - pos;
			ReconcileUtils::Latin1ToUTF8 ( localPtr, localLen, utf8string );
			break;

		}

		case 1: // Unicode, v2.4: UTF-16 (undetermined Endianess), with BOM, terminated 0x00 00
		case 2: // UTF-16BE without BOM, terminated 0x00 00
		{

			if ( commMode && (! advancePastCOMMDescriptor ( pos )) ) return false; // not a frame of interest!

			std::string tmp ( this->content, this->contentSize );
			bool bigEndian = true;	// assume for now (if no BOM follows)

			if ( GetUns16BE ( &this->content[pos] ) == 0xFEFF ) {
				pos += 2;
				bigEndian = true;
			} else if ( GetUns16BE ( &this->content[pos] ) == 0xFFFE ) {
				pos += 2;
				bigEndian = false;
			}

			FromUTF16 ( (UTF16Unit*)&this->content[pos], ((this->contentSize - pos)) / 2, utf8string, bigEndian );
			break;

		}

		case 3: // UTF-8 unicode, terminated \0
		{
			if ( commMode && (! advancePastCOMMDescriptor ( pos )) ) return false; // not a frame of interest!
		
			if ( (GetUns32BE ( &this->content[pos]) & 0xFFFFFF00 ) == 0xEFBBBF00 ) {
				pos += 3;	// swallow any BOM, just in case
			}

			utf8string->assign ( &this->content[pos], (this->contentSize - pos) );
			break;
		}

		default:
			XMP_Throw ( "unknown text encoding", kXMPErr_BadFileFormat ); //COULDDO assume latin-1 or utf-8 as best-effort
			break;

	}

	return true;

}	// ID3v2Frame::getFrameValue

// =================================================================================================
// ID3v1Tag
// =================================================================================================

bool ID3v1Tag::read ( XMP_IO* file, SXMPMeta* meta )
{
	// Returns true if ID3v1 (or v1.1) exists, otherwise false, sets XMP properties en route.

	if ( file->Length() <= 128 ) return false;  // ensure sufficient room
	file->Seek ( -128, kXMP_SeekFromEnd );

	XMP_Uns32 tagID = XIO::ReadInt32_BE ( file );
	tagID = tagID & 0xFFFFFF00; // wipe 4th byte
	if ( tagID != 0x54414700 ) return false; // must be "TAG"
	file->Seek ( -1, kXMP_SeekFromCurrent  ); //rewind 1

	XMP_Uns8 buffer[31]; // nothing is bigger here, than 30 bytes (offsets [0]-[29])
	buffer[30] = 0;		 // wipe last byte
	std::string utf8string;

	file->ReadAll ( buffer, 30 );
	std::string title ( (char*) buffer ); //security: guaranteed to 0-terminate after 30 bytes
	if ( ! title.empty() ) {
		ReconcileUtils::Latin1ToUTF8 ( title.c_str(), title.size(), &utf8string );
		meta->SetLocalizedText ( kXMP_NS_DC, "title", "", "x-default", utf8string.c_str() );
	}

	file->ReadAll ( buffer, 30 );
	std::string artist( (char*) buffer );
	if ( ! artist.empty() ) {
		ReconcileUtils::Latin1ToUTF8 ( artist.c_str(), artist.size(), &utf8string );
		meta->SetProperty ( kXMP_NS_DM, "artist", utf8string.c_str() );
	}

	file->ReadAll ( buffer, 30 );
	std::string album( (char*) buffer );
	if ( ! album.empty() ) {
		ReconcileUtils::Latin1ToUTF8 ( album.c_str(), album.size(), &utf8string );
		meta->SetProperty ( kXMP_NS_DM, "album", utf8string.c_str() );
	}

	file->ReadAll ( buffer, 4 );
	buffer[4]=0; // ensure 0-term
	std::string year( (char*) buffer );
	if ( ! year.empty() ) {	// should be moot for a year, but let's be safe:
		ReconcileUtils::Latin1ToUTF8 ( year.c_str(), year.size(), &utf8string );
		meta->SetProperty ( kXMP_NS_XMP, "CreateDate",  utf8string.c_str() );
	}

	file->ReadAll ( buffer, 30 );
	std::string comment( (char*) buffer );
	if ( ! comment.empty() ) {
		ReconcileUtils::Latin1ToUTF8 ( comment.c_str(), comment.size(), &utf8string );
		meta->SetProperty ( kXMP_NS_DM, "logComment", utf8string.c_str() );
	}

	if ( buffer[28] == 0 ) {
		XMP_Uns8 trackNo = buffer[29];
		if ( trackNo > 0 ) {
			std::string trackStr;
			meta->SetProperty_Int ( kXMP_NS_DM, "trackNumber", trackNo );
		}
	}

	XMP_Uns8 genreNo = XIO::ReadUns8 ( file );
	if ( genreNo < numberedGenreCount ) {
		meta->SetProperty ( kXMP_NS_DM, "genre", kNumberedGenres[genreNo].name );
	} else {
		char buffer[4];	// AUDIT: Big enough for UInt8.
		snprintf ( buffer, 4, "%d", genreNo );
		XMP_Assert ( strlen(buffer) == 3 );	// Should be in the range 126..255.
		meta->SetProperty ( kXMP_NS_DM, "genre", buffer );
	}

	return true; // ID3Tag found

}	// ID3v1Tag::read

// =================================================================================================

static inline bool GetDecimalUns32 ( const char * str, XMP_Uns32 * bin )
{
	XMP_Assert ( bin != 0 );
	if ( (str == 0) || (str[0] == 0) ) return false;
	
	*bin = 0;
	for ( size_t i = 0; str[i] != 0; ++i ) {
		char ch = str[i];
		if ( (ch < '0') || (ch > '9') ) return false;
		*bin = (*bin * 10) + (ch - '0');
	}
	
	return true;

}

// =================================================================================================

void ID3v1Tag::write ( XMP_IO* file, SXMPMeta* meta )
{

	std::string zeros ( 128, '\0' );
	std::string utf8, latin1;

	file->Seek ( -128, kXMP_SeekFromEnd );
	file->Write ( zeros.data(), 128 );

	file->Seek ( -128, kXMP_SeekFromEnd );
	XIO::WriteUns8 ( file, 'T' );
	XIO::WriteUns8 ( file, 'A' );
	XIO::WriteUns8 ( file, 'G' );

	if ( meta->GetLocalizedText ( kXMP_NS_DC, "title", "", "x-default", 0, &utf8, 0 ) ) {
		file->Seek ( (-128 + 3), kXMP_SeekFromEnd );
		ReconcileUtils::UTF8ToLatin1 ( utf8.c_str(), utf8.size(), &latin1 );
		file->Write ( latin1.c_str(), MIN ( 30, (XMP_Int32)latin1.size() ) );
	}

	if ( meta->GetProperty ( kXMP_NS_DM, "artist", &utf8, 0 ) ) {
		file->Seek ( (-128 + 33), kXMP_SeekFromEnd );
		ReconcileUtils::UTF8ToLatin1 ( utf8.c_str(), utf8.size(), &latin1 );
		file->Write ( latin1.c_str(), MIN ( 30, (XMP_Int32)latin1.size() ) );
	}

	if ( meta->GetProperty ( kXMP_NS_DM, "album", &utf8, 0 ) ) {
		file->Seek ( (-128 + 63), kXMP_SeekFromEnd );
		ReconcileUtils::UTF8ToLatin1 ( utf8.c_str(), utf8.size(), &latin1 );
		file->Write ( latin1.c_str(), MIN ( 30, (XMP_Int32)latin1.size() ) );
	}

	if ( meta->GetProperty ( kXMP_NS_XMP, "CreateDate", &utf8, 0 ) ) {
		XMP_DateTime dateTime;
		SXMPUtils::ConvertToDate( utf8, &dateTime );
		if ( dateTime.hasDate ) {
			SXMPUtils::ConvertFromInt ( dateTime.year, "", &latin1 );
			file->Seek ( (-128 + 93), kXMP_SeekFromEnd );
			file->Write ( latin1.c_str(), MIN ( 4, (XMP_Int32)latin1.size() ) );
		}
	}

	if ( meta->GetProperty ( kXMP_NS_DM, "logComment", &utf8, 0 ) ) {
		file->Seek ( (-128 + 97), kXMP_SeekFromEnd );
		ReconcileUtils::UTF8ToLatin1 ( utf8.c_str(), utf8.size(), &latin1 );
		file->Write ( latin1.c_str(), MIN ( 30, (XMP_Int32)latin1.size() ) );
	}

	if ( meta->GetProperty ( kXMP_NS_DM, "genre", &utf8, 0 ) ) {

		// Write the first genre code as a UInt8.
		size_t nameEnd;
		std::string name;
		
		for ( nameEnd = 0; ((nameEnd < utf8.size()) && (utf8[nameEnd] != ';')); ++nameEnd ) {}
		name.assign ( utf8.c_str(), nameEnd );
		const char * code = GenreUtils::FindGenreCode ( name );

		if ( code != 0 ) {
			XMP_Uns32 value;
			bool ok = GetDecimalUns32 ( code, &value );
			if ( ok && (value <= 255) ) {
				file->Seek ( (-128 + 127), kXMP_SeekFromEnd );
				XIO::WriteUns8 ( file, (XMP_Uns8)value );
			}
		}

	}

	if ( meta->GetProperty ( kXMP_NS_DM, "trackNumber", &utf8, kXMP_NoOptions ) ) {

		XMP_Uns8 trackNo = 0;
		try {
			trackNo = (XMP_Uns8) SXMPUtils::ConvertToInt ( utf8.c_str() );
			file->Seek ( (-128 + 125), kXMP_SeekFromEnd );
			XIO::WriteUns8 ( file, 0 ); // ID3v1.1 extension
			XIO::WriteUns8 ( file, trackNo );
		} catch ( ... ) {
			// forgive, just don't set this one.
		}

	}

}	// ID3v1Tag::write

// =================================================================================================

};	// namespace ID3_Support
