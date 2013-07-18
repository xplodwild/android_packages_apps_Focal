#ifndef __MP3_Handler_hpp__
#define __MP3_Handler_hpp__	1

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

using namespace std;
using namespace ID3_Support;

extern XMPFileHandler * MP3_MetaHandlerCTor ( XMPFiles * parent );

extern bool MP3_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
			                  XMP_IO*    fileRef,
			                  XMPFiles *     parent );

static const XMP_OptionBits kMP3_HandlerFlags = (kXMPFiles_CanInjectXMP |
												 kXMPFiles_CanExpand |
												 kXMPFiles_PrefersInPlace |
												 kXMPFiles_AllowsOnlyXMP |
												 kXMPFiles_ReturnsRawPacket|
												 kXMPFiles_CanReconcile);

class MP3_MetaHandler : public XMPFileHandler
{
public:
	MP3_MetaHandler ( XMPFiles * parent );
	~MP3_MetaHandler();

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	void ProcessXMP();

private:
	////////////////////////////////////////////////////////////////////////////////////
	// instance vars
	XMP_Int64 oldTagSize;		// the entire tag, including padding, including 10B header
	XMP_Int64 oldPadding;		// number of padding bytes
	XMP_Int64 oldFramesSize;	// actual space needed by frames := oldTagSize - 10 - oldPadding

	XMP_Int64 newTagSize;
	XMP_Int64 newPadding;
	XMP_Int64 newFramesSize;

	// decision making:
	bool tagIsDirty;	// true, if any legacy properties changed.
	bool mustShift;		// entire tag to rewrite? (or possibly just XMP?)


	XMP_Uns8 majorVersion, minorVersion; // Version Number post ID3v2, i.e. 3 0 ==> ID3v2.3.0
	bool hasID3Tag; //incoming file has an ID3 tag?
	bool hasFooter;
	//bool legacyChanged; // tag rewrite certainly needed?

	ID3Header id3Header;

	XMP_Int64 extHeaderSize;
	bool hasExtHeader;

	// the frames
	// * all to be kept till write-out
	// * parsed/understood only if needed
	// * vector used to free memory in handler destructor
	std::vector<ID3_Support::ID3v2Frame*> framesVector;

	// ID3v1 - treated as a single object
	ID3v1Tag id3v1Tag;

	// * also kept in a map for better import<->export access
	//    * only keeps legacy 'relevant frames' (i.e. no abused COMM frames)
	//    * only keeps last relevant frame
	//       * earlier 'relevant frames' will be deleted. This map also helps in this
	// * key shall be the FrameID, always interpreted as BE
	std::map<XMP_Uns32,ID3_Support::ID3v2Frame*> framesMap;

};	// MP3_MetaHandler

// =================================================================================================

#endif /* __MP3_Handler_hpp__ */
