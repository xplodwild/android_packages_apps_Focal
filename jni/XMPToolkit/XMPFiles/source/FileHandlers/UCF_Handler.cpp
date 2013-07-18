// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// ===============================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/UCF_Handler.hpp"

#include "third-party/zlib/zlib.h"

#include <time.h>

#ifdef DYNAMIC_CRC_TABLE
	#error "unexpectedly DYNAMIC_CRC_TABLE defined."
	//Must implement get_crc_table prior to any multi-threading (see notes there)
#endif

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

using namespace std;

// =================================================================================================
/// \file UCF_Handler.cpp
/// \brief UCF handler class
// =================================================================================================
const XMP_Uns16 xmpFilenameLen = 21;
const char* xmpFilename = "META-INF/metadata.xml";

// =================================================================================================
// UCF_MetaHandlerCTor
// ====================
XMPFileHandler* UCF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new UCF_MetaHandler ( parent );
}	// UCF_MetaHandlerCTor

// =================================================================================================
// UCF_CheckFormat
// ================
// * lenght must at least be 114 bytes
// * first bytes must be \x50\x4B\x03\x04 for *any* zip file
// * at offset 30 it must spell "mimetype"

#define MIN_UCF_LENGTH 114
// zip minimum considerations:
// the shortest legal zip is 100 byte:
//  30+1* bytes file header
//+ 0 byte content file (uncompressed)
//+ 46+1* bytes central directory file header
//+ 22 byte end of central directory record
//-------
//100 bytes
//
//1 byte is the shortest legal filename. anything below is no valid zip.
//
//==> the mandatory+first "mimetype" content file has a filename length of 8 bytes,
//    thus even if empty (arguably incorrect but tolerable),
//    the shortest legal UCF is 114 bytes (30 + 8 + 0 + 46 + 8 + 22 )
//    anything below is with certainty not a valid ucf.

bool UCF_CheckFormat (  XMP_FileFormat format,
						XMP_StringPtr  filePath,
						XMP_IO*    fileRef,
						XMPFiles *     parent )
{
	// *not* using buffer functionality here, all we need
	// to detect UCF securely is in the first 38 bytes...
	IgnoreParam(filePath); IgnoreParam(parent);	//suppress warnings
	XMP_Assert ( format == kXMP_UCFFile );								//standard assert

	XMP_Uns8 buffer[MIN_UCF_LENGTH];

	fileRef->Rewind();
	if ( MIN_UCF_LENGTH != fileRef->Read ( buffer, MIN_UCF_LENGTH) ) //NO requireall (->no throw), just return false
		return false;
	if ( !CheckBytes ( &buffer[0], "\x50\x4B\x03\x04", 4 ) ) // "PK 03 04"
		return false;
	// UCF spec says: there must be a content file mimetype, and be first and be uncompressed...
	if ( !CheckBytes ( &buffer[30], "mimetype", 8 ) )
		return false;

	//////////////////////////////////////////////////////////////////////////////
	//figure out mimetype, decide on writeability
	// grab mimetype
	fileRef->Seek ( 18, kXMP_SeekFromStart  );
	XMP_Uns32 mimeLength    = XIO::ReadUns32_LE ( fileRef );
	XMP_Uns32 mimeCompressedLength = XIO::ReadUns32_LE ( fileRef );	 // must be same since uncompressed

	XMP_Validate( mimeLength == mimeCompressedLength,
				  "mimetype compressed and uncompressed length differ",
				  kXMPErr_BadFileFormat );

	XMP_Validate( mimeLength != 0, "0-byte mimetype", kXMPErr_BadFileFormat );

	// determine writability based on mimetype
	fileRef->Seek ( 30 + 8, kXMP_SeekFromStart  );
	char* mimetype = new char[ mimeLength + 1 ];
	fileRef->ReadAll ( mimetype, mimeLength );
	mimetype[mimeLength] = '\0';

	bool okMimetype;

	// be lenient on extraneous CR (0xA) [non-XMP bug #16980028]
	if ( mimeLength > 0 ) //avoid potential crash (will properly fail below anyhow)
		if ( mimetype[mimeLength-1] == 0xA )
			mimetype[mimeLength-1] = '\0';

	if (
		XMP_LitMatch( mimetype, "application/vnd.adobe.xfl"    ) ||  //Flash Diesel team
		XMP_LitMatch( mimetype, "application/vnd.adobe.xfl+zip") ||  //Flash Diesel team
		XMP_LitMatch( mimetype, "application/vnd.adobe.x-mars" ) ||  //Mars plugin(labs only), Acrobat8
		XMP_LitMatch( mimetype, "application/vnd.adobe.pdfxml" ) ||  //Mars plugin(labs only), Acrobat 9
		XMP_LitMatch( mimetype, "vnd.adobe.x-asnd"             ) ||  //Adobe Sound Document (Soundbooth Team)
		XMP_LitMatch( mimetype, "application/vnd.adobe.indesign-idml-package" ) || //inCopy (inDesign) IDML Document
		XMP_LitMatch( mimetype, "application/vnd.adobe.incopy-package"        ) ||  // InDesign Document
		XMP_LitMatch( mimetype, "application/vnd.adobe.indesign-package"      ) ||  // InDesign Document
		XMP_LitMatch( mimetype, "application/vnd.adobe.collage"    ) ||  //Adobe Collage
		XMP_LitMatch( mimetype, "application/vnd.adobe.ideas"      ) ||  //Adobe Ideas
		XMP_LitMatch( mimetype, "application/vnd.adobe.proto"      ) ||  //Adobe Proto
		false ) // "sentinel"

		// *** ==> unknown are also treated as not acceptable
		okMimetype = true;
	else
		okMimetype = false;

	// not accepted (neither read nor write
	//.air  - Adobe Air Files
	//application/vnd.adobe.air-application-installer-package+zip
	//.airi - temporary Adobe Air Files
	//application/vnd.adobe.air-application-intermediate-package+zip

	delete [] mimetype;
	return okMimetype;

}	// UCF_CheckFormat

// =================================================================================================
// UCF_MetaHandler::UCF_MetaHandler
// ==================================

UCF_MetaHandler::UCF_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kUCF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;
}	// UCF_MetaHandler::UCF_MetaHandler

// =================================================================================================
// UCF_MetaHandler::~UCF_MetaHandler
// =====================================

UCF_MetaHandler::~UCF_MetaHandler()
{
	// nothing
}

// =================================================================================================
// UCF_MetaHandler::CacheFileData
// ===============================
//
void UCF_MetaHandler::CacheFileData()
{
	//*** abort procedures
	this->containsXMP = false;		//assume no XMP for now (beware of exceptions...)
	XMP_IO* file = this->parent->ioRef;
	XMP_PacketInfo &packetInfo = this->packetInfo;

	// clear file positioning info ---------------------------------------------------
	b=0;b2=0;x=0;x2=0;cd=0;cd2=0;cdx=0;cdx2=0;h=0;h2=0,fl=0;f2l=0;
	al=0;bl=0;xl=0;x2l=0;cdl=0;cd2l=0;cdxl=0;cdx2l=0;hl=0,z=0,z2=0,z2l=0;
	numCF=0;numCF2=0;
	wasCompressed = false;

	// -------------------------------------------------------------------------------
	fl=file ->Length();
	if ( fl < MIN_UCF_LENGTH )	XMP_Throw("file too short, can't be correct UCF",kXMPErr_Unimplemented);

	//////////////////////////////////////////////////////////////////////////////
	// find central directory before optional comment
	// things have to go bottom-up, since description headers are allowed in UCF
	// "scan backwards" until feasible field found (plus sig sanity check)
	// OS buffering should be smart enough, so not doing anything on top
	// plus almost all comments will be zero or rather short

	//no need to check anything but the 21 chars of "METADATA-INF/metadata.xml"
	char filenameToTest[22];
	filenameToTest[21]='\0';

	XMP_Int32 zipCommentLen = 0;
	for ( ; zipCommentLen <= EndOfDirectory::COMMENT_MAX; zipCommentLen++ )
	{
		file->Seek ( -zipCommentLen -2, kXMP_SeekFromEnd  );
		if ( XIO::ReadUns16_LE( file ) == zipCommentLen )	//found it?
		{
			//double check, might just look like comment length (actually be 'evil' comment)
			file ->Seek ( - EndOfDirectory::FIXED_SIZE, kXMP_SeekFromCurrent  );
			if ( XIO::ReadUns32_LE( file ) == EndOfDirectory::ID ) break; //heureka, directory ID
			// 'else': pretend nothing happended, just go on
		}
	}
	//was it a break or just not found ?
	if ( zipCommentLen > EndOfDirectory::COMMENT_MAX ) XMP_Throw( "zip broken near end or invalid comment" , kXMPErr_BadFileFormat );

	////////////////////////////////////////////////////////////////////////////
	//read central directory
	hl = zipCommentLen + EndOfDirectory::FIXED_SIZE;
	h = fl - hl;
	file ->Seek ( h , kXMP_SeekFromStart  );

	if ( XIO::ReadUns32_LE( file ) != EndOfDirectory::ID )
		XMP_Throw("directory header id not found. or broken comment",kXMPErr_BadFileFormat);
	if ( XIO::ReadUns16_LE( file ) != 0 )
		XMP_Throw("UCF must be 'first' zip volume",kXMPErr_BadFileFormat);
	if ( XIO::ReadUns16_LE( file ) != 0 )
		XMP_Throw("UCF must be single-volume zip",kXMPErr_BadFileFormat);

	numCF = XIO::ReadUns16_LE( file ); //number of content files
	if ( numCF != XIO::ReadUns16_LE( file ) )
		XMP_Throw( "per volume and total number of dirs differ" , kXMPErr_BadFileFormat );
	cdl = XIO::ReadUns32_LE( file );
	cd = XIO::ReadUns32_LE( file );
	file->Seek ( 2, kXMP_SeekFromCurrent  ); //skip comment len, needed since next LFA is kXMP_SeekFromCurrent !

	//////////////////////////////////////////////////////////////////////////////
	// check for zip64-end-of-CD-locator/ zip64-end-of-CD
	// to to central directory
	if ( cd == 0xffffffff )
	{	// deal with zip 64, otherwise continue
		XMP_Int64 tmp = file->Seek ( -(EndOfDirectory::FIXED_SIZE + Zip64Locator::TOTAL_SIZE),
				  kXMP_SeekFromCurrent ); //go to begining of zip64 locator
		//relative movement , absolute would imho only require another -zipCommentLen

		if (  Zip64Locator::ID == XIO::ReadUns32_LE(file) ) // prevent 'coincidental length' ffffffff
		{
			XMP_Validate( 0 == XIO::ReadUns32_LE(file),
					"zip64 CD disk must be 0", kXMPErr_BadFileFormat );

			z = XIO::ReadUns64_LE(file);
			XMP_Validate( z < 0xffffffffffffLL, "file in terrabyte range?", kXMPErr_BadFileFormat ); // 3* ffff, sanity test

			XMP_Uns32 totalNumOfDisks = XIO::ReadUns32_LE(file);
			/* tolerated while pkglib bug #1742179 */
			XMP_Validate( totalNumOfDisks == 0 || totalNumOfDisks == 1,
					"zip64 total num of disks must be 0", kXMPErr_BadFileFormat );

			///////////////////////////////////////////////
			/// on to end-of-CD itself
			file->Seek ( z, kXMP_SeekFromStart  );
			XMP_Validate( Zip64EndOfDirectory::ID == XIO::ReadUns32_LE(file),
				   "invalid zip64 end of CD sig", kXMPErr_BadFileFormat );

			XMP_Int64 sizeOfZip64EOD = XIO::ReadUns64_LE(file);
			file->Seek ( 12, kXMP_SeekFromCurrent  );
			//yes twice "total" and "per disk"
			XMP_Int64 tmp64 = XIO::ReadUns64_LE(file);
			XMP_Validate( tmp64 == numCF, "num of content files differs to zip64 (1)", kXMPErr_BadFileFormat );
			tmp64 = XIO::ReadUns64_LE(file);
			XMP_Validate( tmp64 == numCF, "num of content files differs to zip64 (2)", kXMPErr_BadFileFormat );
			// cd length verification
			tmp64 = XIO::ReadUns64_LE(file);
			XMP_Validate( tmp64 == cdl, "CD length differs in zip64", kXMPErr_BadFileFormat );

			cd = XIO::ReadUns64_LE(file); // wipe out invalid 0xffffffff with the real thing
			//ignoring "extensible data sector (would need fullLength - fixed length) for now
		}
	} // of zip64 fork
	/////////////////////////////////////////////////////////////////////////////
	// parse central directory
	// 'foundXMP' <=> cdx != 0

	file->Seek ( cd, kXMP_SeekFromStart  );
	XMP_Int64 cdx_suspect=0;
	XMP_Int64 cdxl_suspect=0;
	CDFileHeader curCDHeader;

	for ( XMP_Uns16 entryNum=1 ; entryNum <= numCF  ; entryNum++ )
	{
		cdx_suspect =  file->Offset(); //just suspect for now
		curCDHeader.read( file );

		if ( GetUns32LE( &curCDHeader.fields[CDFileHeader::o_sig] ) != 0x02014b50 )
			XMP_Throw("&invalid file header",kXMPErr_BadFileFormat);

		cdxl_suspect = curCDHeader.FIXED_SIZE +
			GetUns16LE(&curCDHeader.fields[CDFileHeader::o_fileNameLength]) +
			GetUns16LE(&curCDHeader.fields[CDFileHeader::o_extraFieldLength]) +
			GetUns16LE(&curCDHeader.fields[CDFileHeader::o_commentLength]);

		// we only look 21 characters, that's META-INF/metadata.xml, no \0 attached
		if ( curCDHeader.filenameLen == xmpFilenameLen /*21*/ )
			if( XMP_LitNMatch( curCDHeader.filename , "META-INF/metadata.xml", 21 ) )
				{
					cdx  = cdx_suspect;
					cdxl = cdxl_suspect;
					break;
				}
		//hop to next
		file->Seek ( cdx_suspect + cdxl_suspect , kXMP_SeekFromStart  );
	} //for-loop, iterating *all* central directory headers (also beyond found)

	if ( !cdx ) // not found xmp
	{
		// b and bl remain 0, x and xl remain 0
		// ==> a is everything before directory
		al = cd;
		return;
	}

	// from here is if-found-only
	//////////////////////////////////////////////////////////////////////////////
	//CD values needed, most serve counter-validation purposes (below) only
	// read whole object (incl. all 3 fields) again properly
	//  to get extra Fields, etc
	file->Seek ( cdx, kXMP_SeekFromStart  );
	xmpCDHeader.read( file );

	XMP_Validate( xmpFilenameLen == GetUns16LE( &xmpCDHeader.fields[CDFileHeader::o_fileNameLength]),
		"content file length not ok", kXMPErr_BadFileFormat );

	XMP_Uns16 CD_compression		= GetUns16LE( &xmpCDHeader.fields[CDFileHeader::o_compression] );
	XMP_Validate(( CD_compression == 0 || CD_compression == 0x08),
		"illegal compression, must be flate or none", kXMPErr_BadFileFormat );
	XMP_Uns16 CD_flags				= GetUns16LE( &xmpCDHeader.fields[CDFileHeader::o_flags] );
	XMP_Uns32 CD_crc				= GetUns32LE( &xmpCDHeader.fields[CDFileHeader::o_crc32] );

	// parse (actual, non-CD!) file header ////////////////////////////////////////////////
	x  = xmpCDHeader.offsetLocalHeader;
	file ->Seek ( x , kXMP_SeekFromStart );
	xmpFileHeader.read( file );
	xl = xmpFileHeader.sizeHeader() + xmpCDHeader.sizeCompressed;

	//values needed
	XMP_Uns16 fileNameLength	= GetUns16LE( &xmpFileHeader.fields[FileHeader::o_fileNameLength] );
	XMP_Uns16 extraFieldLength	= GetUns16LE( &xmpFileHeader.fields[FileHeader::o_extraFieldLength] );
	XMP_Uns16 compression       = GetUns16LE( &xmpFileHeader.fields[FileHeader::o_compression] );
	XMP_Uns32 sig				= GetUns32LE( &xmpFileHeader.fields[FileHeader::o_sig] );
	XMP_Uns16 flags				= GetUns16LE( &xmpFileHeader.fields[FileHeader::o_flags] );
	XMP_Uns32 sizeCompressed    = GetUns32LE( &xmpFileHeader.fields[FileHeader::o_sizeCompressed] );
	XMP_Uns32 sizeUncompressed  = GetUns32LE( &xmpFileHeader.fields[FileHeader::o_sizeUncompressed] );
	XMP_Uns32 crc				= GetUns32LE( &xmpFileHeader.fields[FileHeader::o_crc32] );

	// check filename
	XMP_Validate( fileNameLength == 21, "filename size contradiction" , kXMPErr_BadFileFormat );
	XMP_Enforce ( xmpFileHeader.filename != 0 );
	XMP_Validate( !memcmp( "META-INF/metadata.xml", xmpFileHeader.filename , xmpFilenameLen ) , "filename is cf header is not META-INF/metadata.xml" , kXMPErr_BadFileFormat );

	// deal with data descriptor if needed
	if ( flags & FileHeader::kdataDescriptorFlag )
	{
		if ( sizeCompressed!=0 || sizeUncompressed!=0 || crc!=0 )	XMP_Throw("data descriptor must mean 3x zero",kXMPErr_BadFileFormat);
		file->Seek ( xmpCDHeader.sizeCompressed + fileNameLength + xmpCDHeader.extraFieldLen, kXMP_SeekFromCurrent ); //skip actual data to get to descriptor
		crc = XIO::ReadUns32_LE( file );
		if ( crc == 0x08074b50 ) //data descriptor may or may not have signature (see spec)
		{
			crc = XIO::ReadUns32_LE( file ); //if it does, re-read
		}
		sizeCompressed = XIO::ReadUns32_LE( file );
		sizeUncompressed = XIO::ReadUns32_LE( file );
		// *** cater for zip64 plus 'streamed' data-descriptor stuff
	}

	// more integrity checks (post data descriptor handling)
	if ( sig != 0x04034b50 )						XMP_Throw("invalid content file header",kXMPErr_BadFileFormat);
	if ( compression != CD_compression )			XMP_Throw("compression contradiction",kXMPErr_BadFileFormat);
	if ( sizeUncompressed != xmpCDHeader.sizeUncompressed )	XMP_Throw("contradicting uncompressed lengths",kXMPErr_BadFileFormat);
	if ( sizeCompressed != xmpCDHeader.sizeCompressed ) XMP_Throw("contradicting compressed lengths",kXMPErr_BadFileFormat);
	if ( sizeUncompressed == 0 )					XMP_Throw("0-byte uncompressed size", kXMPErr_BadFileFormat );

	////////////////////////////////////////////////////////////////////
	// packet Info
	this->packetInfo.charForm = stdCharForm;
	this->packetInfo.writeable = false;
	this->packetInfo.offset = kXMPFiles_UnknownOffset; // checksum!, hide position to not give funny ideas
	this->packetInfo.length = kXMPFiles_UnknownLength;

	////////////////////////////////////////////////////////////////////
	// prepare packet (compressed or not)
	this->xmpPacket.erase();
	this->xmpPacket.reserve( sizeUncompressed );
	this->xmpPacket.append( sizeUncompressed, ' ' );
	XMP_StringPtr packetStr = XMP_StringPtr ( xmpPacket.c_str() );	// only set after reserving the space!

	// go to packet offset
	file->Seek ( x + xmpFileHeader.FIXED_SIZE + fileNameLength + extraFieldLength , kXMP_SeekFromStart);

	// compression fork --------------------------------------------------
	switch (compression)
	{
	case 0x8: // FLATE
		{
			wasCompressed = true;
			XMP_Uns32 bytesRead = 0;
			XMP_Uns32 bytesWritten = 0; // for writing into packetString
			const unsigned int CHUNK = 16384;

			int ret;
			unsigned int have; //added type
			z_stream strm;
			unsigned char in[CHUNK];
			unsigned char out[CHUNK];
			// does need this intermediate stage, no direct compressio to packetStr possible,
			// since also partially filled buffers must be picked up. That's how it works.
			// in addition: internal zlib variables might have 16 bit limits...

			/* allocate inflate state */
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;

			/* must use windowBits = -15, for raw inflate, no zlib header */
			ret = inflateInit2(&strm,-MAX_WBITS);

			if (ret != Z_OK)
				XMP_Throw("zlib error ",kXMPErr_ExternalFailure);

			/* decompress until deflate stream ends or end of file */
			do {
				// must take care here not to read too much, thus whichever is smaller:
				XMP_Int32 bytesRemaining = sizeCompressed - bytesRead;
				if ( (XMP_Int32)CHUNK < bytesRemaining ) bytesRemaining = (XMP_Int32)CHUNK;
				strm.avail_in=file ->ReadAll ( in , bytesRemaining  );
				bytesRead += strm.avail_in; // NB: avail_in is "unsigned_int", so might be 16 bit (not harmfull)

				if (strm.avail_in == 0) break;
				strm.next_in = in;

				do {
					strm.avail_out = CHUNK;
					strm.next_out = out;
					ret = inflate(&strm, Z_NO_FLUSH);
					XMP_Assert( ret != Z_STREAM_ERROR );	/* state not clobbered */
					switch (ret)
					{
					case Z_NEED_DICT:
						(void)inflateEnd(&strm);
						XMP_Throw("zlib error: Z_NEED_DICT",kXMPErr_ExternalFailure);
					case Z_DATA_ERROR:
						(void)inflateEnd(&strm);
						XMP_Throw("zlib error: Z_DATA_ERROR",kXMPErr_ExternalFailure);
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						XMP_Throw("zlib error: Z_MEM_ERROR",kXMPErr_ExternalFailure);
					}

					have = CHUNK - strm.avail_out;
					memcpy( (unsigned char*) packetStr + bytesWritten , out , have );
					bytesWritten += have;

				} while (strm.avail_out == 0);

				/* it's done when inflate() says it's done */
			} while (ret != Z_STREAM_END);

			/* clean up and return */
			(void)inflateEnd(&strm);
			if (ret != Z_STREAM_END)
				XMP_Throw("zlib error ",kXMPErr_ExternalFailure);
			break;
		}
	case 0x0:	// no compression - read directly into the right place
		{
			wasCompressed = false;
			XMP_Enforce( file->ReadAll ( (char*)packetStr, sizeUncompressed ) );
			break;
		}
	default:
		{
			XMP_Throw("illegal zip compression method (not none, not flate)",kXMPErr_BadFileFormat);
		}
	}
	this->containsXMP = true; // do this last, after all possible failure/execptions
}


// =================================================================================================
// UCF_MetaHandler::ProcessXMP
// ============================

void UCF_MetaHandler::ProcessXMP()
{
	// we have no legacy, CacheFileData did all that was needed
	// ==> default implementation is fine
	XMPFileHandler::ProcessXMP();
}

// =================================================================================================
// UCF_MetaHandler::UpdateFile
// =============================

// TODO: xmp packet with data descriptor

void UCF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	//sanity
	XMP_Enforce( (x!=0) == (cdx!=0) );
	if (!cdx)
		xmpCDHeader.setXMPFilename();	//if new, set filename (impacts length, thus before computation)
	if ( ! this->needsUpdate )
		return;

	// ***
	if ( doSafeUpdate ) XMP_Throw ( "UCF_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );

	XMP_IO* file = this->parent->ioRef;

	// final may mean compressed or not, whatever is to-be-embedded
	uncomprPacketStr = xmpPacket.c_str();
	uncomprPacketLen = (XMP_StringLen) xmpPacket.size();
	finalPacketStr = uncomprPacketStr;		// will be overriden if compressedXMP==true
	finalPacketLen = uncomprPacketLen;
	std::string compressedPacket;	// moot if non-compressed, still here for scope reasons (having to keep a .c_str() alive)

	if ( !x ) // if new XMP...
	{
		xmpFileHeader.clear();
		xmpFileHeader.setXMPFilename();
		// ZIP64 TODO: extra Fields, impact on cdxl2 and x2l
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// COMPRESSION DECISION

	// for large files compression is bad:
	// a) size of XMP becomes irrelevant on large files ==> why worry over compression ?
	// b) more importantly: no such thing as padding possible, compression ==  ever changing sizes
	//    => never in-place rewrites, *ugly* performance impact on large files
	inPlacePossible = false; //assume for now

	if ( !x )	// no prior XMP? -> decide on filesize
		compressXMP = ( fl > 1024*50 /* 100 kB */ ) ? false : true;
	else
		compressXMP = wasCompressed;	// don't change a thing

	if ( !wasCompressed && !compressXMP &&
		( GetUns32LE( &xmpFileHeader.fields[FileHeader::o_sizeUncompressed] ) == uncomprPacketLen ))
	{
			inPlacePossible = true;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////
	// COMPRESS XMP
	if ( compressXMP )
	{
		const unsigned int CHUNK = 16384;
		int ret, flush;
		unsigned int have;
		z_stream strm;
		unsigned char out[CHUNK];

		/* allocate deflate state */
		strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;
		if ( deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8 /*memlevel*/, Z_DEFAULT_STRATEGY) )
			XMP_Throw("zlib error ",kXMPErr_ExternalFailure);

		//write at once, since we got it in mem anyway:
		strm.avail_in = uncomprPacketLen;
		flush = Z_FINISH; // that's all, folks
		strm.next_in = (unsigned char*) uncomprPacketStr;

		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;

			ret = deflate(&strm, flush);    /* no bad return value (!=0 acceptable) */
			XMP_Enforce(ret != Z_STREAM_ERROR); /* state not clobbered */
			//fwrite(buffer,size,count,file)
			have = CHUNK - strm.avail_out;
			compressedPacket.append( (const char*) out,  have);
		} while (strm.avail_out == 0);

		if (ret != Z_STREAM_END)
			XMP_Throw("zlib stream incomplete ",kXMPErr_ExternalFailure);
		XMP_Enforce(strm.avail_in == 0);		// all input will be used
		(void)deflateEnd(&strm);				//clean up (do prior to checks)

		finalPacketStr = compressedPacket.c_str();
		finalPacketLen = (XMP_StringLen)compressedPacket.size();
	}

	PutUns32LE ( uncomprPacketLen,	&xmpFileHeader.fields[FileHeader::o_sizeUncompressed] );
	PutUns32LE ( finalPacketLen,	&xmpFileHeader.fields[FileHeader::o_sizeCompressed]   );
	PutUns16LE ( compressXMP ? 8:0,	&xmpFileHeader.fields[FileHeader::o_compression]      );

	////////////////////////////////////////////////////////////////////////////////////////////////
	// CRC (always of uncompressed data)
	XMP_Uns32 crc = crc32( 0 , (Bytef*)uncomprPacketStr, uncomprPacketLen );
	PutUns32LE( crc, &xmpFileHeader.fields[FileHeader::o_crc32] );

	////////////////////////////////////////////////////////////////////////////////////////////////
	// TIME calculation for timestamp
	// will be applied both to xmp content file and CD header
	XMP_Uns16 lastModTime, lastModDate;
	XMP_DateTime time;
	SXMPUtils::CurrentDateTime( &time );

	if ( (time.year - 1900) < 80)
	{
		lastModTime = 0; // 1.1.1980 00:00h
		lastModDate = 21;
	}

	// typedef unsigned short  ush; //2 bytes
	lastModDate = (XMP_Uns16) (((time.year) - 1980 ) << 9  | ((time.month) << 5) | time.day);
	lastModTime = ((XMP_Uns16)time.hour << 11) | ((XMP_Uns16)time.minute << 5) | ((XMP_Uns16)time.second >> 1);

	PutUns16LE ( lastModDate,	&xmpFileHeader.fields[FileHeader::o_lastmodDate]      );
	PutUns16LE ( lastModTime,	&xmpFileHeader.fields[FileHeader::o_lastmodTime]      );

	////////////////////////////////////////////////////////////////////////////////////////////////
	// adjustments depending on 4GB Border,
	//   decisions on in-place update
	//   so far only z, zl have been determined

	// Zip64 related assurances, see (15)
	XMP_Enforce(!z2);
	XMP_Enforce(h+hl == fl );

	////////////////////////////////////////////////////////////////////////////////////////////////
	// COMPUTE MISSING VARIABLES
	// A - based on xmp existence
	//
	// already known: x, xl, cd
	// most left side vars,
	//
	// finalPacketStr, finalPacketLen

	if ( x ) // previous xmp?
	{
		al = x;
		b  = x + xl;
		bl = cd - b;
	}
	else
	{
		al = cd;
		//b,bl left at zero
	}

	if ( inPlacePossible )
	{ // leave xmp right after A
		x2 = al;
		x2l = xmpFileHeader.sizeTotalCF(); //COULDDO: assert (x2l == xl)
		if (b) b2 = x2 + x2l; // b follows x as last content part
		cd2 = b2 + bl;	// CD follows B2
	}
	else
	{ // move xmp to end
		if (b) b2 = al; // b follows
		// x follows as last content part (B existing or not)
		x2 = al + bl;
		x2l = xmpFileHeader.sizeTotalCF();
		cd2 = x2 + x2l;	// CD follows X
	}

	/// create new XMP header ///////////////////////////////////////////////////
	// written into actual fields + generation of extraField at .write()-time...
	// however has impact on .size() computation  --  thus enter before cdx2l computation
	xmpCDHeader.sizeUncompressed = uncomprPacketLen;
	xmpCDHeader.sizeCompressed = finalPacketLen;
	xmpCDHeader.offsetLocalHeader = x2;
	PutUns32LE ( crc,				&xmpCDHeader.fields[CDFileHeader::o_crc32] );
	PutUns16LE ( compressXMP ? 8:0,	&xmpCDHeader.fields[CDFileHeader::o_compression] );
	PutUns16LE ( lastModDate,		&xmpCDHeader.fields[CDFileHeader::o_lastmodDate] );
	PutUns16LE ( lastModTime,		&xmpCDHeader.fields[CDFileHeader::o_lastmodTime] );

	// for
	if ( inPlacePossible )
	{
		cdx2 = cdx;	//same, same
		writeOut( file, file, false, true );
		return;
	}

	////////////////////////////////////////////////////////////////////////
	// temporarily store (those few, small) trailing things that might not survive the move around:
	file->Seek ( cd, kXMP_SeekFromStart );	// seek to central directory
	cdEntries.clear(); //mac precaution

	//////////////////////////////////////////////////////////////////////////////
	// parse headers
	//   * stick together output header list
	cd2l = 0; //sum up below

	CDFileHeader tempHeader;
	for( XMP_Uns16 pos=1 ; pos <= numCF ; pos++ )
	{
		if ( (cdx) && (file->Offset() == cdx) )
		{
			tempHeader.read( file ); //read, even if not use, to advance file pointer
		}
		else
		{
			tempHeader.read( file );
			// adjust b2 offset for files that were behind the xmp:
			//        may (if xmp moved to back)
			//        or may not (inPlace Update) make a difference
			if ( (x) && ( tempHeader.offsetLocalHeader > x) ) // if xmp existed before and this was a file behind it
				tempHeader.offsetLocalHeader += b2 - b;
			cd2l += tempHeader.size(); // prior offset change might have impact
			cdEntries.push_back( tempHeader );
		}
	}

	//push in XMP packet as last one (new or not)
	cdEntries.push_back( xmpCDHeader );
	cdx2l = xmpCDHeader.size();
	cd2l += cdx2l; // true, no matter which order

	//OLD  cd2l = : cdl - cdxl + cdx2l;	// (NB: cdxl might be 0)
	numCF2 = numCF + ( (cdx)?0:1 );	//xmp packet for the first time? -> add one more CF

	XMP_Validate( numCF2 > 0, "no content files", kXMPErr_BadFileFormat );
	XMP_Validate( numCF2 <= 0xFFFE, "max number of 0xFFFE entries reached", kXMPErr_BadFileFormat );

	cdx2 = cd2 + cd2l - cdx2l;	// xmp content entry comes last (since beyond inPlace Update)

	// zip64 decision
	if ( ( cd2 + cd2l + hl ) > 0xffffffff )	// predict non-zip size ==> do we need a zip-64?
	{
		z2 = cd2 + cd2l;
		z2l = Zip64EndOfDirectory::FIXED_SIZE + Zip64Locator::TOTAL_SIZE;
	}

	// header and output length,
	h2 = cd2 + cd2l + z2l; // (z2l might be 0)
	f2l = h2 + hl;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// read H (endOfCD), correct offset
	file->Seek ( h, kXMP_SeekFromStart );

	endOfCD.read( file );
	if ( cd2 <= 0xffffffff )
		PutUns32LE( (XMP_Int32) cd2 , &endOfCD.fields[ endOfCD.o_CdOffset ] );
	else
		PutUns32LE( 0xffffffff , &endOfCD.fields[ endOfCD.o_CdOffset ] );
	PutUns16LE( numCF2, &endOfCD.fields[ endOfCD.o_CdNumEntriesDisk   ] );
	PutUns16LE( numCF2, &endOfCD.fields[ endOfCD.o_CdNumEntriesTotal  ] );

	XMP_Enforce( cd2l <= 0xffffffff ); // _size_ of directory itself certainly under 4GB
	PutUns32LE( (XMP_Uns32)cd2l,   &endOfCD.fields[ endOfCD.o_CdSize ] );

	////////////////////////////////////////////////////////////////////////////////////////////////
	// MOVING
	writeOut( file, file, false, false );

	this->needsUpdate = false; //do last for safety reasons
}	// UCF_MetaHandler::UpdateFile

// =================================================================================================
// UCF_MetaHandler::WriteTempFile
// ==============================
void UCF_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	IgnoreParam ( tempRef );
	XMP_Throw ( "UCF_MetaHandler::WriteTempFile: TO BE IMPLEMENTED", kXMPErr_Unimplemented );
}

// =================================================================================================
// own approach to unify Update and WriteFile:
// ============================

void UCF_MetaHandler::writeOut( XMP_IO* sourceFile, XMP_IO* targetFile, bool isRewrite, bool isInPlace)
{
	// isInPlace is only possible when it's not a complete rewrite
	XMP_Enforce( (!isInPlace) || (!isRewrite) );

	/////////////////////////////////////////////////////////
	// A
	if (isRewrite)	//move over A block
		XIO::Move( sourceFile , 0 , targetFile, 0 , al );

	/////////////////////////////////////////////////////////
	// B / X (not necessarily in this order)
	if ( !isInPlace ) // B does not change a thing (important optimization)
	{
		targetFile ->Seek ( b2 , kXMP_SeekFromStart  );
		XIO::Move( sourceFile , b , targetFile, b2 , bl );
	}

	targetFile ->Seek ( x2 , kXMP_SeekFromStart  );
	xmpFileHeader.write( targetFile );
	targetFile->Write ( finalPacketStr, finalPacketLen  );
	//TODO: cover reverse case / inplace ...

	/////////////////////////////////////////////////////////
	// CD
	// No Seek here on purpose.
	// This assert must still be valid

	// if inPlace, the only thing that needs still correction is the CRC in CDX:
	if ( isInPlace )
	{
		XMP_Uns32 crc;	//TEMP, not actually needed
		crc = GetUns32LE( &xmpFileHeader.fields[FileHeader::o_crc32] );

		// go there,
		// do the job (take value directly from (non-CD-)fileheader),
		// end of story.
		targetFile ->Seek ( cdx2 + CDFileHeader::o_crc32 , kXMP_SeekFromStart  );
		targetFile->Write ( &xmpFileHeader.fields[FileHeader::o_crc32], 4 );

		return;
	}

	targetFile ->Seek ( cd2 , kXMP_SeekFromStart  );

	std::vector<CDFileHeader>::iterator iter;
	int tmptmp=1;
	for( iter = cdEntries.begin(); iter != cdEntries.end(); iter++ ) {
		CDFileHeader* p=&(*iter);
		XMP_Int64 before = targetFile->Offset();
		p->write( targetFile );
		XMP_Int64 total = targetFile->Offset() - before;
		XMP_Int64 tmpSize = p->size();
		tmptmp++;
	}

	/////////////////////////////////////////////////////////
	// Z
	if ( z2 )  // yes, that simple
	{
		XMP_Assert( z2 == targetFile->Offset());
		targetFile ->Seek ( z2 , kXMP_SeekFromStart  );

		//no use in copying, always construct from scratch
		Zip64EndOfDirectory zip64EndOfDirectory( cd2, cd2l, numCF2) ;
		Zip64Locator zip64Locator( z2 );

		zip64EndOfDirectory.write( targetFile );
		zip64Locator.write( targetFile );
	}

	/////////////////////////////////////////////////////////
	// H
	XMP_Assert( h2 == targetFile->Offset());
	endOfCD.write( targetFile );

	XMP_Assert( f2l == targetFile->Offset());
	if ( f2l< fl)
		targetFile->Truncate ( f2l );	//file may have shrunk
}
