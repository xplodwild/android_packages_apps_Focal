#ifndef __MPEG4_Handler_hpp__
#define __MPEG4_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include "XMPFiles/source/FormatSupport/MOOV_Support.hpp"
#include "XMPFiles/source/FormatSupport/QuickTime_Support.hpp"

//  ================================================================================================
/// \file MPEG4_Handler.hpp
/// \brief File format handler for MPEG-4.
///
/// This header ...
///
//  ================================================================================================

extern XMPFileHandler * MPEG4_MetaHandlerCTor ( XMPFiles * parent );

extern bool MPEG4_CheckFormat ( XMP_FileFormat format,
								XMP_StringPtr  filePath,
								XMP_IO*    fileRef,
								XMPFiles *     parent );

static const XMP_OptionBits kMPEG4_HandlerFlags = ( kXMPFiles_CanInjectXMP |
													kXMPFiles_CanExpand |
													kXMPFiles_CanRewrite |
													kXMPFiles_PrefersInPlace |
													kXMPFiles_CanReconcile |
													kXMPFiles_AllowsOnlyXMP |
													kXMPFiles_ReturnsRawPacket |
													kXMPFiles_AllowsSafeUpdate |
													kXMPFiles_CanNotifyProgress
												  );

class MPEG4_MetaHandler : public XMPFileHandler
{
public:

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );


	MPEG4_MetaHandler ( XMPFiles * _parent );
	virtual ~MPEG4_MetaHandler();

	struct TimecodeTrackInfo {	// Info about a QuickTime timecode track.
		bool stsdBoxFound, isDropFrame;
		XMP_Uns32 timeScale;
		XMP_Uns32 frameDuration;
		XMP_Uns32 timecodeSample;
		XMP_Uns64 sampleOffset;	// Absolute file offset of the timecode sample, 0 if none.
		XMP_Uns32 nameOffset;	// The offset of the 'name' box relative to the 'stsd' box content.
		XMP_Uns16   macLang;	// The Mac language code of the trailing 'name' box.
		std::string macName;	// The text part of the trailing 'name' box, in macLang encoding.
		TimecodeTrackInfo()
			: stsdBoxFound(false), isDropFrame(false), timeScale(0), frameDuration(0),
			  timecodeSample(0), sampleOffset(0), nameOffset(0), macLang(0) {};
	};

private:

	MPEG4_MetaHandler() : fileMode(0), havePreferredXMP(false),
						  xmpBoxPos(0), moovBoxPos(0), xmpBoxSize(0), moovBoxSize(0) {};	// Hidden on purpose.

	bool ParseTimecodeTrack();

	void UpdateTopLevelBox ( XMP_Uns64 oldOffset, XMP_Uns32 oldSize, const XMP_Uns8 * newBox, XMP_Uns32 newSize );

	XMP_Uns8 fileMode;
	bool havePreferredXMP;
	XMP_Uns64 xmpBoxPos;	// The file offset of the XMP box (the size field, not the content).
	XMP_Uns64 moovBoxPos;	// The file offset of the 'moov' box (the size field, not the content).
	XMP_Uns32 xmpBoxSize, moovBoxSize;	// The full size of the boxes, not just the content.

	MOOV_Manager moovMgr;
	TradQT_Manager tradQTMgr;

	TimecodeTrackInfo tmcdInfo;

};	// MPEG4_MetaHandler

// =================================================================================================

#endif // __MPEG4_Handler_hpp__
