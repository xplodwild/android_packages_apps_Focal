#ifndef __Basic_Handler_hpp__
#define __Basic_Handler_hpp__	1

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

// =================================================================================================
/// \file Basic_Handler.hpp
///
/// \brief Base class for handlers that support a simple file model allowing insertion and expansion
/// of XMP, but probably not reconciliation with other forms of metadata. Reconciliation would have
/// to be done within the I/O model presented here.
///
/// \note Any specific derived handler might not be able to do insertion, but all must support
/// expansion. If a handler can't do either it should be derived from Trivial_Handler. Common code
/// must check the actual canInject flag where appropriate.
///
/// The model for a basic handler divides the file into 6 portions:
///
/// \li The front of the file. This portion can be arbitrarily large. Files over 4GB are supported.
/// Adding or expanding the XMP must not require expanding this portion of the file. The XMP offset
/// or length might be written into reserved space in this section though.
///
/// \li A prefix for the XMP section. The prefix and suffix for the XMP "section" are the format
/// specific portions that surround the raw XMP packet. They must be generated on the fly, even when
/// updating existing XMP with or without expansion. Their length must not depend on the XMP packet.
///
/// \li The XMP packet, as created by SXMPMeta::SerializeToBuffer. The size must be less than 2GB.
///
/// \li A suffix for the XMP section.
///
/// \li Trailing file content. This portion can be arbitarily large. It must be possible to remove
/// the XMP, move this portion of the file forward, then reinsert the XMP after this portion. This
/// is actually how the XMP is expanded. There must not be any embedded file offsets in this part,
/// this content must not change if the XMP changes size.
///
/// \li The back of the file. This portion must have modest size, and/or be generated on the fly.
/// When inserting XMP, part of this may be buffered in RAM (hence the modest size requirement), the
/// XMP section is written, then this portion is rewritten. There must not be any embedded file
/// offsets in this part, this content must not change if the XMP changes size.
///
/// \note There is no general promise here about crash-safe I/O. An update to an existing file might
/// have invalid partial state, for example while moving the trailing content portion forward if the
/// XMP increases in size or even rewriting existing XMP in-place. Crash-safe updates are managed at
/// a higher level of XMPFiles, using a temporary file and final swap of file content.
///
// =================================================================================================

static const XMP_OptionBits kBasic_HandlerFlags = (kXMPFiles_CanInjectXMP |
                                                   kXMPFiles_CanExpand |
                                                   kXMPFiles_CanRewrite |
                                                   kXMPFiles_PrefersInPlace |
                                                   kXMPFiles_AllowsOnlyXMP |
                                                   kXMPFiles_ReturnsRawPacket |
                                                   kXMPFiles_AllowsSafeUpdate);

class Basic_MetaHandler : public XMPFileHandler
{
public:

	Basic_MetaHandler() :
		xmpFileOffset(0), xmpFileSize(0), xmpPrefixSize(0), xmpSuffixSize(0), trailingContentSize(0) {};
	~Basic_MetaHandler();

	virtual void CacheFileData() = 0;	// Sets offset for insertion if no XMP yet.

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

protected:

	// Write a cached or fixed prefix or suffix for the XMP. The file is passed because it could be
	// either the original file or a safe-update temp file.
	virtual void WriteXMPPrefix ( XMP_IO* fileRef ) = 0;	// ! Must have override in actual handlers!
	virtual void WriteXMPSuffix ( XMP_IO* fileRef ) = 0;	// ! Must have override in actual handlers!

	// Note that the XMP is being removed or inserted. The file is passed because it could be either
	// the original file or a safe-update temp file.
	virtual void NoteXMPRemoval ( XMP_IO* fileRef ) = 0;	// ! Must have override in actual handlers!
	virtual void NoteXMPInsertion ( XMP_IO* fileRef ) = 0;	// ! Must have override in actual handlers!

	// Capture or restore the tail portion of the file. The file is passed because it could be either
	// the original file or a safe-update temp file.
	virtual void CaptureFileEnding ( XMP_IO* fileRef ) = 0;	// ! Must have override in actual handlers!
	virtual void RestoreFileEnding ( XMP_IO* fileRef ) = 0;	// ! Must have override in actual handlers!

	// Move the trailing content portion forward. Excludes "back" of the file. The file is passed
	// because it could be either the original file or a safe-update temp file.
	void ShuffleTrailingContent ( XMP_IO* fileRef );	// Has a common implementation.

	XMP_Uns64 xmpFileOffset;	// The offset of the XMP in the file.
	XMP_Uns32 xmpFileSize;	// The size of the XMP in the file.
		// ! The packetInfo offset and length are updated by PutXMP, before the file is updated!

	XMP_Uns32 xmpPrefixSize;	// The size of the existing header for the XMP section.
	XMP_Uns32 xmpSuffixSize;	// The size of the existing trailer for the XMP section.

	XMP_Uns64 trailingContentSize;	// The size of the existing trailing content. Excludes "back" of the file.

};	// Basic_MetaHandler

// =================================================================================================

#endif /* __Basic_Handler_hpp__ */
