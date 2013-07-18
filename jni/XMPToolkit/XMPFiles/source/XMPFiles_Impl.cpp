// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XIO.hpp"

#include "source/UnicodeConversions.hpp"

using namespace std;

// Internal code should be using #if with XMP_MacBuild, XMP_WinBuild, or XMP_UNIXBuild.
// This is a sanity check in case of accidental use of *_ENV. Some clients use the poor
// practice of defining the *_ENV macro with an empty value.
#if defined ( MAC_ENV )
	#if ! MAC_ENV
		#error "MAC_ENV must be defined so that \"#if MAC_ENV\" is true"
	#endif
#elif defined ( WIN_ENV )
	#if ! WIN_ENV
		#error "WIN_ENV must be defined so that \"#if WIN_ENV\" is true"
	#endif
#elif defined ( UNIX_ENV )
	#if ! UNIX_ENV
		#error "UNIX_ENV must be defined so that \"#if UNIX_ENV\" is true"
	#endif
#endif

// =================================================================================================
/// \file XMPFiles_Impl.cpp
/// \brief ...
///
/// This file ...
///
// =================================================================================================

#if ! XMP_StaticBuild
	#include "public/include/XMP.incl_cpp"
#endif

#if XMP_WinBuild
	#pragma warning ( disable : 4290 )	// C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false' (performance warning)
#endif

bool ignoreLocalText = false;

XMP_FileFormat voidFileFormat = 0;	// Used as sink for unwanted output parameters.

#if ! XMP_StaticBuild
	XMP_PacketInfo voidPacketInfo;
	void *         voidVoidPtr    = 0;
	XMP_StringPtr  voidStringPtr  = 0;
	XMP_StringLen  voidStringLen  = 0;
	XMP_OptionBits voidOptionBits = 0;
#endif

// =================================================================================================

// Add all known mappings, multiple mappings (tif, tiff) are OK.
const FileExtMapping kFileExtMap[] =
	{ { "pdf",  kXMP_PDFFile },
	  { "ps",   kXMP_PostScriptFile },
	  { "eps",  kXMP_EPSFile },

	  { "jpg",  kXMP_JPEGFile },
	  { "jpeg", kXMP_JPEGFile },
	  { "jpx",  kXMP_JPEG2KFile },
	  { "tif",  kXMP_TIFFFile },
	  { "tiff", kXMP_TIFFFile },
	  { "dng",  kXMP_TIFFFile },	// DNG files are well behaved TIFF.
	  { "gif",  kXMP_GIFFile },
	  { "giff", kXMP_GIFFile },
	  { "png",  kXMP_PNGFile },

	  { "swf",  kXMP_SWFFile },
	  { "flv",  kXMP_FLVFile },

	  { "aif",  kXMP_AIFFFile },

	  { "mov",  kXMP_MOVFile },
	  { "avi",  kXMP_AVIFile },
	  { "cin",  kXMP_CINFile },
	  { "wav",  kXMP_WAVFile },
	  { "mp3",  kXMP_MP3File },
	  { "mp4",  kXMP_MPEG4File },
	  { "m4v",  kXMP_MPEG4File },
	  { "m4a",  kXMP_MPEG4File },
	  { "f4v",  kXMP_MPEG4File },
	  { "ses",  kXMP_SESFile },
	  { "cel",  kXMP_CELFile },
	  { "wma",  kXMP_WMAVFile },
	  { "wmv",  kXMP_WMAVFile },
	  { "mxf",  kXMP_MXFFile },
	  { "r3d",  kXMP_REDFile },

	  { "mpg",  kXMP_MPEGFile },
	  { "mpeg", kXMP_MPEGFile },
	  { "mp2",  kXMP_MPEGFile },
	  { "mod",  kXMP_MPEGFile },
	  { "m2v",  kXMP_MPEGFile },
	  { "mpa",  kXMP_MPEGFile },
	  { "mpv",  kXMP_MPEGFile },
	  { "m2p",  kXMP_MPEGFile },
	  { "m2a",  kXMP_MPEGFile },
	  { "m2t",  kXMP_MPEGFile },
	  { "mpe",  kXMP_MPEGFile },
	  { "vob",  kXMP_MPEGFile },
	  { "ms-pvr", kXMP_MPEGFile },
	  { "dvr-ms", kXMP_MPEGFile },

	  { "html", kXMP_HTMLFile },
	  { "xml",  kXMP_XMLFile },
	  { "txt",  kXMP_TextFile },
	  { "text", kXMP_TextFile },

	  { "psd",  kXMP_PhotoshopFile },
	  { "ai",   kXMP_IllustratorFile },
	  { "indd", kXMP_InDesignFile },
	  { "indt", kXMP_InDesignFile },
	  { "aep",  kXMP_AEProjectFile },
	  { "aepx", kXMP_AEProjectFile },
	  { "aet",  kXMP_AEProjTemplateFile },
	  { "ffx",  kXMP_AEFilterPresetFile },
	  { "ncor", kXMP_EncoreProjectFile },
	  { "prproj", kXMP_PremiereProjectFile },
	  { "prtl", kXMP_PremiereTitleFile },
	  { "ucf", kXMP_UCFFile },
	  { "xfl", kXMP_UCFFile },
	  { "pdfxml", kXMP_UCFFile },
	  { "mars", kXMP_UCFFile },
	  { "idml", kXMP_UCFFile },
	  { "idap", kXMP_UCFFile },
	  { "icap", kXMP_UCFFile },
	  { "", 0 } };	// ! Must be last as a sentinel.

// Files known to contain XMP but have no smart handling, here or elsewhere.
const char * kKnownScannedFiles[] =
	{ "gif",	// GIF, public format but no smart handler.
	  "ai",		// Illustrator, actually a PDF file.
	  "ait",	// Illustrator template, actually a PDF file.
	  "svg",	// SVG, an XML file.
	  "aet",	// After Effects template project file.
	  "ffx",	// After Effects filter preset file.
	  "aep",	// After Effects project file in proprietary format
	  "aepx",	// After Effects project file in XML format
	  "inx",	// InDesign interchange, an XML file.
	  "inds",	// InDesign snippet, an XML file.
	  "inpk",	// InDesign package for GoLive, a text file (not XML).
	  "incd",	// InCopy story, an XML file.
	  "inct",	// InCopy template, an XML file.
	  "incx",	// InCopy interchange, an XML file.
	  "fm",		// FrameMaker file, proprietary format.
	  "book",	// FrameMaker book, proprietary format.
	  "icml",	// an inCopy (inDesign) format
	  "icmt",	// an inCopy (inDesign) format
	  "idms",	// an inCopy (inDesign) format
	  0 };		// ! Keep a 0 sentinel at the end.


// Extensions that XMPFiles never handles.
const char * kKnownRejectedFiles[] =
	{
		// RAW files
		"cr2", "erf", "fff", "dcr", "kdc", "mos", "mfw", "mef",
		"raw", "nef", "orf", "pef", "arw", "sr2", "srf", "sti",
		"3fr", "rwl", "crw", "sraw", "mos", "mrw", "nrw", "rw2",
		"c3f",
		// UCF subformats
		"air",
		// Others
	  0 };	// ! Keep a 0 sentinel at the end.

// =================================================================================================

// =================================================================================================

// =================================================================================================
// GetPacketCharForm
// =================
//
// The first character must be U+FEFF or ASCII, typically '<' for an outermost element, initial
// processing instruction, or XML declaration. The second character can't be U+0000.
// The possible input sequences are:
//   Cases with U+FEFF
//      EF BB BF -- - UTF-8
//      FE FF -- -- - Big endian UTF-16
//      00 00 FE FF - Big endian UTF 32
//      FF FE 00 00 - Little endian UTF-32
//      FF FE -- -- - Little endian UTF-16
//   Cases with ASCII
//      nn mm -- -- - UTF-8 -
//      00 00 00 nn - Big endian UTF-32
//      00 nn -- -- - Big endian UTF-16
//      nn 00 00 00 - Little endian UTF-32
//      nn 00 -- -- - Little endian UTF-16

static XMP_Uns8 GetPacketCharForm ( XMP_StringPtr packetStr, XMP_StringLen packetLen )
{
	XMP_Uns8   charForm = kXMP_CharUnknown;
	XMP_Uns8 * unsBytes = (XMP_Uns8*)packetStr;	// ! Make sure comparisons are unsigned.

	if ( packetLen < 2 ) return kXMP_Char8Bit;

	if ( packetLen < 4 ) {

		// These cases are based on the first 2 bytes:
		//   00 nn Big endian UTF-16
		//   nn 00 Little endian UTF-16
		//   FE FF Big endian UTF-16
		//   FF FE Little endian UTF-16
		//   Otherwise UTF-8

		if ( packetStr[0] == 0 ) return kXMP_Char16BitBig;
		if ( packetStr[1] == 0 ) return kXMP_Char16BitLittle;
		if ( CheckBytes ( packetStr, "\xFE\xFF", 2 ) ) return kXMP_Char16BitBig;
		if ( CheckBytes ( packetStr, "\xFF\xFE", 2 ) ) return kXMP_Char16BitLittle;
		return kXMP_Char8Bit;

	}

	// If we get here the packet is at least 4 bytes, could be any form.

	if ( unsBytes[0] == 0 ) {

		// These cases are:
		//   00 nn -- -- - Big endian UTF-16
		//   00 00 00 nn - Big endian UTF-32
		//   00 00 FE FF - Big endian UTF 32

		if ( unsBytes[1] != 0 ) {
			charForm = kXMP_Char16BitBig;			// 00 nn
		} else {
			if ( (unsBytes[2] == 0) && (unsBytes[3] != 0) ) {
				charForm = kXMP_Char32BitBig;		// 00 00 00 nn
			} else if ( (unsBytes[2] == 0xFE) && (unsBytes[3] == 0xFF) ) {
				charForm = kXMP_Char32BitBig;		// 00 00 FE FF
			}
		}

	} else {

		// These cases are:
		//   FE FF -- -- - Big endian UTF-16, FE isn't valid UTF-8
		//   FF FE 00 00 - Little endian UTF-32, FF isn't valid UTF-8
		//   FF FE -- -- - Little endian UTF-16
		//   nn mm -- -- - UTF-8, includes EF BB BF case
		//   nn 00 00 00 - Little endian UTF-32
		//   nn 00 -- -- - Little endian UTF-16

		if ( unsBytes[0] == 0xFE ) {
			if ( unsBytes[1] == 0xFF ) charForm = kXMP_Char16BitBig;	// FE FF
		} else if ( unsBytes[0] == 0xFF ) {
			if ( unsBytes[1] == 0xFE ) {
				if ( (unsBytes[2] == 0) && (unsBytes[3] == 0) ) {
					charForm = kXMP_Char32BitLittle;	// FF FE 00 00
				} else {
					charForm = kXMP_Char16BitLittle;	// FF FE
				}
			}
		} else if ( unsBytes[1] != 0 ) {
			charForm = kXMP_Char8Bit;					// nn mm
		} else {
			if ( (unsBytes[2] == 0) && (unsBytes[3] == 0) ) {
				charForm = kXMP_Char32BitLittle;		// nn 00 00 00
			} else {
				charForm = kXMP_Char16BitLittle;		// nn 00
			}
		}

	}

	//	XMP_Assert ( charForm != kXMP_CharUnknown );
	return charForm;

}	// GetPacketCharForm

// =================================================================================================
// FillPacketInfo
// ==============
//
// If a packet wrapper is present, the the packet string is roughly:
//   <?xpacket begin= ...?>
//   <outer-XML-element>
//     ... more XML ...
//   </outer-XML-element>
//   ... whitespace padding ...
//   <?xpacket end='.'?>

// The 8-bit form is 14 bytes, the 16-bit form is 28 bytes, the 32-bit form is 56 bytes.
#define k8BitTrailer  "<?xpacket end="
#define k16BitTrailer "<\0?\0x\0p\0a\0c\0k\0e\0t\0 \0e\0n\0d\0=\0"
#define k32BitTrailer "<\0\0\0?\0\0\0x\0\0\0p\0\0\0a\0\0\0c\0\0\0k\0\0\0e\0\0\0t\0\0\0 \0\0\0e\0\0\0n\0\0\0d\0\0\0=\0\0\0"
static XMP_StringPtr kPacketTrailiers[3] = { k8BitTrailer, k16BitTrailer, k32BitTrailer };

void FillPacketInfo ( const std::string & packet, XMP_PacketInfo * info )
{
	XMP_StringPtr packetStr = packet.c_str();
	XMP_StringLen packetLen = (XMP_StringLen) packet.size();
	if ( packetLen == 0 ) return;

	info->charForm = GetPacketCharForm ( packetStr, packetLen );
	XMP_StringLen charSize = XMP_GetCharSize ( info->charForm );

	// Look for a packet wrapper. For our purposes, we can be lazy and just look for the trailer PI.
	// If that is present we'll assume that a recognizable header is present. First do a bytewise
	// search for '<', then a char sized comparison for the start of the trailer. We don't really
	// care about big or little endian here. We're looking for ASCII bytes with zeroes between.
	// Shorten the range comparisons (n*charSize) by 1 to easily tolerate both big and little endian.

	XMP_StringLen padStart, padEnd;
	XMP_StringPtr packetTrailer = kPacketTrailiers [ charSize>>1 ];

	padEnd = packetLen - 1;
	for ( ; padEnd > 0; --padEnd ) if ( packetStr[padEnd] == '<' ) break;
	if ( (packetStr[padEnd] != '<') || ((packetLen - padEnd) < (18*charSize)) ) return;
	if ( ! CheckBytes ( &packetStr[padEnd], packetTrailer, (13*charSize) ) ) return;

	info->hasWrapper = true;

	char rwFlag = packetStr [padEnd + 15*charSize];
	if ( rwFlag == 'w' ) info->writeable = true;

	// Look for the start of the padding, right after the last XML end tag.

	padStart = padEnd;	// Don't do the -charSize here, might wrap below zero.
	for ( ; padStart >= charSize; padStart -= charSize ) if ( packetStr[padStart] == '>' ) break;
	if ( padStart < charSize ) return;
	padStart += charSize;	// The padding starts after the '>'.

	info->padSize = padEnd - padStart;	// We want bytes of padding, not character units.

}	// FillPacketInfo

// =================================================================================================
// ReadXMPPacket
// =============

void ReadXMPPacket ( XMPFileHandler * handler )
{
	XMP_IO* fileRef   = handler->parent->ioRef;
	std::string & xmpPacket = handler->xmpPacket;
	XMP_StringLen packetLen = handler->packetInfo.length;

	if ( packetLen == 0 ) XMP_Throw ( "ReadXMPPacket - No XMP packet", kXMPErr_BadXMP );

	xmpPacket.erase();
	xmpPacket.reserve ( packetLen );
	xmpPacket.append ( packetLen, ' ' );

	XMP_StringPtr packetStr = XMP_StringPtr ( xmpPacket.c_str() );	// Don't set until after reserving the space!

	fileRef->Seek ( handler->packetInfo.offset, kXMP_SeekFromStart );
	fileRef->ReadAll ( (char*)packetStr, packetLen );

}	// ReadXMPPacket

// =================================================================================================
// XMPFileHandler::GetFileModDate
// ==============================
//
// The base implementation is only for typical embedding handlers. It returns the modification date
// of the named file.

bool XMPFileHandler::GetFileModDate ( XMP_DateTime * modDate )
{

	XMP_OptionBits flags = this->handlerFlags;
	if ( (flags & kXMPFiles_HandlerOwnsFile) ||
		 (flags & kXMPFiles_UsesSidecarXMP) ||
		 (flags & kXMPFiles_FolderBasedFormat) ) {
		XMP_Throw ( "Base implementation of GetFileModDate only for typical embedding handlers", kXMPErr_InternalFailure );
	}
	
	if ( this->parent->GetFilePath().empty() ) {
		XMP_Throw ( "GetFileModDate cannot be used with client-provided I/O", kXMPErr_InternalFailure );
	}
	
	return Host_IO::GetModifyDate ( this->parent->GetFilePath().c_str(), modDate );

}	// XMPFileHandler::GetFileModDate

// =================================================================================================
// XMPFileHandler::FillMetadataFiles
// ==============================
//
// The base implementation is only for files having embedded metadata for which the same file will
// be returned.

void XMPFileHandler::FillMetadataFiles ( std::vector<std::string> * metadataFiles )
{
	XMP_OptionBits flags = this->handlerFlags;
	if ( (flags & kXMPFiles_HandlerOwnsFile) ||
		 (flags & kXMPFiles_UsesSidecarXMP) ||
		 (flags & kXMPFiles_FolderBasedFormat) ) {
		XMP_Throw ( "Base implementation of FillMetadataFiles only for typical embedding handlers", kXMPErr_InternalFailure );
	}
	
	if ( this->parent->GetFilePath().empty() ) {
		XMP_Throw ( "FillMetadataFiles cannot be used with client-provided I/O", kXMPErr_InternalFailure );
	}
	
	metadataFiles->push_back ( std::string ( this->parent->GetFilePath().c_str() ) );
}	// XMPFileHandler::FillMetadataFiles

// =================================================================================================
// XMPFileHandler::FillAssociatedResources
// =======================================
//
// The base implementation is only for files having embedded metadata for which the same file will
// be returned as the associated resource.
// 

void XMPFileHandler::FillAssociatedResources ( std::vector<std::string> * metadataFiles )
{
	XMP_OptionBits flags = this->handlerFlags;
	if ( (flags & kXMPFiles_HandlerOwnsFile) ||
		 (flags & kXMPFiles_UsesSidecarXMP) ||
		 (flags & kXMPFiles_FolderBasedFormat) ) {
		XMP_Throw ( "GetAssociatedResources is not implemented for this file format", kXMPErr_InternalFailure );
	}
	
	if ( this->parent->GetFilePath().empty() ) {
		XMP_Throw ( "GetAssociatedResources cannot be used with client-provided I/O", kXMPErr_InternalFailure );
	}
	
	metadataFiles->push_back ( std::string ( this->parent->GetFilePath().c_str() ) );
}	// XMPFileHandler::FillAssociatedResources

// =================================================================================================
// XMPFileHandler::IsMetadataWritable
// =======================================
//
// The base implementation is only for files having embedded metadata and it checks whether that 
// file is writable or no.Returns true if the file is writable. 
// 
bool XMPFileHandler::IsMetadataWritable ( )
{
	XMP_OptionBits flags = this->handlerFlags;
	if ( (flags & kXMPFiles_HandlerOwnsFile) ||
		 (flags & kXMPFiles_UsesSidecarXMP)  ||
		 (flags & kXMPFiles_FolderBasedFormat) ) {
		XMP_Throw ( "IsMetadataWritable is not implemented for this file format", kXMPErr_InternalFailure );
	}

	if ( this->parent->GetFilePath().empty() ) {
		XMP_Throw ( "IsMetadataWritable cannot be used with client-provided I/O", kXMPErr_InternalFailure );
	}

	try {
		return Host_IO::Writable( this->parent->GetFilePath().c_str() );
	} catch ( ... ) {

	}
	return false;
}

// =================================================================================================
// XMPFileHandler::ProcessXMP
// ==========================
//
// This base implementation just parses the XMP. If the derived handler does reconciliation then it
// must have its own implementation of ProcessXMP.

void XMPFileHandler::ProcessXMP()
{

	if ( (!this->containsXMP) || this->processedXMP ) return;

	if ( this->handlerFlags & kXMPFiles_CanReconcile ) {
		XMP_Throw ( "Reconciling file handlers must implement ProcessXMP", kXMPErr_InternalFailure );
	}

	SXMPUtils::RemoveProperties ( &this->xmpObj, 0, 0, kXMPUtil_DoAllProperties );
	this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	this->processedXMP = true;

}	// XMPFileHandler::ProcessXMP

// =================================================================================================
// XMPFileHandler::GetSerializeOptions
// ===================================
//
// This base implementation just selects compact serialization. The character form and padding/in-place
// settings are added in the common code before calling SerializeToBuffer.

XMP_OptionBits XMPFileHandler::GetSerializeOptions()
{

	return kXMP_UseCompactFormat;

}	// XMPFileHandler::GetSerializeOptions

// =================================================================================================
