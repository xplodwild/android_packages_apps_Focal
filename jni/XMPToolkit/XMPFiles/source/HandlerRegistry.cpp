// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "source/XIO.hpp"

#include "XMPFiles/source/HandlerRegistry.h"

#if EnablePluginManager
	#include "XMPFiles/source/PluginHandler/XMPAtoms.h"
#endif

#if EnablePhotoHandlers
	#include "XMPFiles/source/FileHandlers/JPEG_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/PSD_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/TIFF_Handler.hpp"
#endif

#if EnableDynamicMediaHandlers
	#include "XMPFiles/source/FileHandlers/AIFF_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/ASF_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/FLV_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/MP3_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/MPEG2_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/MPEG4_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/P2_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/WAVE_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/RIFF_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/SonyHDV_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/SWF_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/XDCAM_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/XDCAMEX_Handler.hpp"
#endif

#if EnableMiscHandlers
	#include "XMPFiles/source/FileHandlers/InDesign_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/PNG_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/PostScript_Handler.hpp"
	#include "XMPFiles/source/FileHandlers/UCF_Handler.hpp"
#endif

//#if EnablePacketScanning
//#include "XMPFiles/source/FileHandlers/Scanner_Handler.hpp"
//#endif

using namespace Common;

#if EnablePluginManager
	using namespace XMP_PLUGIN;
#endif

// =================================================================================================

#if EnableDynamicMediaHandlers

static const char * kP2ContentChildren[] = { "CLIP", "VIDEO", "AUDIO", "ICON", "VOICE", "PROXY", 0 };

static inline bool CheckP2ContentChild ( const std::string & folderName )
{
	for ( int i = 0; kP2ContentChildren[i] != 0; ++i ) {
		if ( folderName == kP2ContentChildren[i] ) return true;
	}
	return false;
}

#endif

// =================================================================================================

//
// Static init
//
HandlerRegistry* HandlerRegistry::sInstance = 0;

/*static*/ HandlerRegistry& HandlerRegistry::getInstance()
{
	if ( sInstance == 0 ) sInstance = new HandlerRegistry();
	return *sInstance;
}

/*static*/ void HandlerRegistry::terminate()
{
	delete sInstance;
	sInstance = 0;
}

HandlerRegistry::HandlerRegistry()
{
	mFolderHandlers		= new XMPFileHandlerTable;
	mNormalHandlers		= new XMPFileHandlerTable;
	mOwningHandlers		= new XMPFileHandlerTable;
	mReplacedHandlers	= new XMPFileHandlerTable;
}

HandlerRegistry::~HandlerRegistry()
{
	delete mFolderHandlers;
	delete mNormalHandlers;
	delete mOwningHandlers;
	delete mReplacedHandlers;
}

// =================================================================================================

void HandlerRegistry::initialize()
{

	bool allOK = true;	// All of the linked-in handler registrations must work, do one test at the end.
	
	// -----------------------------------------
	// Register the directory-oriented handlers.

#if EnableDynamicMediaHandlers
	allOK &= this->registerFolderHandler ( kXMP_P2File, kP2_HandlerFlags, P2_CheckFormat, P2_MetaHandlerCTor );
	allOK &= this->registerFolderHandler ( kXMP_SonyHDVFile, kSonyHDV_HandlerFlags, SonyHDV_CheckFormat, SonyHDV_MetaHandlerCTor );
	allOK &= this->registerFolderHandler ( kXMP_XDCAM_FAMFile, kXDCAM_HandlerFlags, XDCAM_CheckFormat, XDCAM_MetaHandlerCTor );
	allOK &= this->registerFolderHandler ( kXMP_XDCAM_SAMFile, kXDCAM_HandlerFlags, XDCAM_CheckFormat, XDCAM_MetaHandlerCTor );
	allOK &= this->registerFolderHandler ( kXMP_XDCAM_EXFile, kXDCAMEX_HandlerFlags, XDCAMEX_CheckFormat, XDCAMEX_MetaHandlerCTor );
#endif

	// ------------------------------------------------------------------------------------------
	// Register the file-oriented handlers that don't want to open and close the file themselves.

#if EnablePhotoHandlers
	allOK &= this->registerNormalHandler ( kXMP_JPEGFile, kJPEG_HandlerFlags, JPEG_CheckFormat, JPEG_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_PhotoshopFile, kPSD_HandlerFlags, PSD_CheckFormat, PSD_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_TIFFFile, kTIFF_HandlerFlags, TIFF_CheckFormat, TIFF_MetaHandlerCTor );
#endif

#if EnableDynamicMediaHandlers
	allOK &= this->registerNormalHandler ( kXMP_WMAVFile, kASF_HandlerFlags, ASF_CheckFormat, ASF_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_MP3File, kMP3_HandlerFlags, MP3_CheckFormat, MP3_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_WAVFile, kWAVE_HandlerFlags, WAVE_CheckFormat, WAVE_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_AVIFile, kRIFF_HandlerFlags, RIFF_CheckFormat, RIFF_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_SWFFile, kSWF_HandlerFlags, SWF_CheckFormat, SWF_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_MPEG4File, kMPEG4_HandlerFlags, MPEG4_CheckFormat, MPEG4_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_MOVFile, kMPEG4_HandlerFlags, MPEG4_CheckFormat, MPEG4_MetaHandlerCTor );	// ! Yes, MPEG-4 includes MOV.
	allOK &= this->registerNormalHandler ( kXMP_FLVFile, kFLV_HandlerFlags, FLV_CheckFormat, FLV_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_AIFFFile, kAIFF_HandlerFlags, AIFF_CheckFormat, AIFF_MetaHandlerCTor );
#endif

#if EnableMiscHandlers
	allOK &= this->registerNormalHandler ( kXMP_InDesignFile, kInDesign_HandlerFlags, InDesign_CheckFormat, InDesign_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_PNGFile, kPNG_HandlerFlags, PNG_CheckFormat, PNG_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_UCFFile, kUCF_HandlerFlags, UCF_CheckFormat, UCF_MetaHandlerCTor );
	// ! EPS and PostScript have the same handler, EPS is a proper subset of PostScript.
	allOK &= this->registerNormalHandler ( kXMP_EPSFile, kPostScript_HandlerFlags, PostScript_CheckFormat, PostScript_MetaHandlerCTor );
	allOK &= this->registerNormalHandler ( kXMP_PostScriptFile, kPostScript_HandlerFlags, PostScript_CheckFormat, PostScript_MetaHandlerCTor );
#endif

	// ------------------------------------------------------------------------------------
	// Register the file-oriented handlers that need to open and close the file themselves.

#if EnableDynamicMediaHandlers
	allOK &= this->registerOwningHandler ( kXMP_MPEGFile, kMPEG2_HandlerFlags, MPEG2_CheckFormat, MPEG2_MetaHandlerCTor );
	allOK &= this->registerOwningHandler ( kXMP_MPEG2File, kMPEG2_HandlerFlags, MPEG2_CheckFormat, MPEG2_MetaHandlerCTor );
#endif

	if ( ! allOK ) XMP_Throw ( "Failure initializing linked-in file handlers", kXMPErr_InternalFailure );

}	// HandlerRegistry::initialize

// =================================================================================================

bool HandlerRegistry::registerFolderHandler( XMP_FileFormat			format,
										     XMP_OptionBits			flags,
										     CheckFolderFormatProc	checkProc,
										     XMPFileHandlerCTor		handlerCTor,
											 bool					replaceExisting /*= false*/ )
{
	XMP_Assert ( format != kXMP_UnknownFile );

	XMP_Assert ( flags & kXMPFiles_HandlerOwnsFile );
	XMP_Assert ( flags & kXMPFiles_FolderBasedFormat );
	XMP_Assert ( (flags & kXMPFiles_CanInjectXMP) ? (flags & kXMPFiles_CanExpand) : 1 );
	
	if ( replaceExisting ) 
	{
		//
		// Remember previous file handler for this format.
		// Reject if there is already a replacement.
		//
		if( mReplacedHandlers->find( format ) == mReplacedHandlers->end() )
		{
			XMPFileHandlerInfo* standardHandler = this->getHandlerInfo( format );

			if( standardHandler != NULL )
			{
				mReplacedHandlers->insert( mReplacedHandlers->end(), XMPFileHandlerTablePair( format, *standardHandler ) );
			}
			else
			{
				// skip registration if there is nothing to replace
				return false;
			}
		}
		else
		{
			// skip registration if there is already a replacing handler registered for this format
			return false;
		}

		// remove existing handler
		this->removeHandler ( format );
	} 
	else 
	{
		// skip registration if there is already a handler registered for this format
		if ( this->getFormatInfo ( format ) ) return false;
	}

	//
	// register handler
	//
	XMPFileHandlerInfo handlerInfo ( format, flags, checkProc, handlerCTor );
	mFolderHandlers->insert ( mFolderHandlers->end(), XMPFileHandlerTablePair ( format, handlerInfo ) );

	return true;

}	// HandlerRegistry::registerFolderHandler

// =================================================================================================

bool HandlerRegistry::registerNormalHandler( XMP_FileFormat			format,
										     XMP_OptionBits			flags,
										     CheckFileFormatProc	checkProc,
											 XMPFileHandlerCTor		handlerCTor,
											 bool					replaceExisting /*= false*/ )
{
	XMP_Assert ( format != kXMP_UnknownFile );

	XMP_Assert ( ! (flags & kXMPFiles_HandlerOwnsFile) );
	XMP_Assert ( ! (flags & kXMPFiles_FolderBasedFormat) );
	XMP_Assert ( (flags & kXMPFiles_CanInjectXMP) ? (flags & kXMPFiles_CanExpand) : 1 );

	if ( replaceExisting ) 
	{
		//
		// Remember previous file handler for this format.
		// Reject if there is already a replacement.
		//
		if( mReplacedHandlers->find( format ) == mReplacedHandlers->end() )
		{
			XMPFileHandlerInfo* standardHandler = this->getHandlerInfo( format );

			if( standardHandler != NULL )
			{
				mReplacedHandlers->insert( mReplacedHandlers->end(), XMPFileHandlerTablePair( format, *standardHandler ) );
			}
			else
			{
				// skip registration if there is nothing to replace
				return false;
			}
		}
		else
		{
			// skip registration if there is already a replacing handler registered for this format
			return false;
		}

		// remove existing handler
		this->removeHandler ( format );
	} 
	else 
	{
		// skip registration if there is already a handler registered for this format
		if ( this->getFormatInfo ( format ) ) return false;
	}

	//
	// register handler
	//
	XMPFileHandlerInfo handlerInfo ( format, flags, checkProc, handlerCTor );
	mNormalHandlers->insert ( mNormalHandlers->end(), XMPFileHandlerTablePair ( format, handlerInfo ) );
	return true;

}	// HandlerRegistry::registerNormalHandler

// =================================================================================================

bool HandlerRegistry::registerOwningHandler( XMP_FileFormat			format,
										     XMP_OptionBits			flags,
										     CheckFileFormatProc	checkProc,
											 XMPFileHandlerCTor		handlerCTor,
											 bool					replaceExisting /*= false*/ )
{
	XMP_Assert ( format != kXMP_UnknownFile );

	XMP_Assert ( flags & kXMPFiles_HandlerOwnsFile );
	XMP_Assert ( ! (flags & kXMPFiles_FolderBasedFormat) );
	XMP_Assert ( (flags & kXMPFiles_CanInjectXMP) ? (flags & kXMPFiles_CanExpand) : 1 );

	if ( replaceExisting ) 
	{
		//
		// Remember previous file handler for this format.
		// Reject if there is already a replacement.
		//
		if( mReplacedHandlers->find( format ) == mReplacedHandlers->end() )
		{
			XMPFileHandlerInfo* standardHandler = this->getHandlerInfo( format );

			if( standardHandler != NULL )
			{
				mReplacedHandlers->insert( mReplacedHandlers->end(), XMPFileHandlerTablePair( format, *standardHandler ) );
			}
			else
			{
				// skip registration if there is nothing to replace
				return false;
			}
		}
		else
		{
			// skip registration if there is already a replacing handler registered for this format
			return false;
		}

		// remove existing handler
		this->removeHandler ( format );
	} 
	else 
	{
		// skip registration if there is already a handler registered for this format
		if ( this->getFormatInfo ( format ) ) return false;
	}

	//
	// register handler
	//
	XMPFileHandlerInfo handlerInfo ( format, flags, checkProc, handlerCTor );
	mOwningHandlers->insert ( mOwningHandlers->end(), XMPFileHandlerTablePair ( format, handlerInfo ) );
	return true;

}	// HandlerRegistry::registerOwningHandler

// =================================================================================================

void HandlerRegistry::removeHandler ( XMP_FileFormat format ) {

	XMPFileHandlerTablePos handlerPos;

	handlerPos = mFolderHandlers->find ( format );
	if ( handlerPos != mFolderHandlers->end() ) {
		mFolderHandlers->erase ( handlerPos );
		XMP_Assert ( ! this->getFormatInfo ( format ) );
		return;
	}

	handlerPos = mNormalHandlers->find ( format );
	if ( handlerPos != mNormalHandlers->end() ) {
		mNormalHandlers->erase ( handlerPos );
		XMP_Assert ( ! this->getFormatInfo ( format ) );
		return;
	}

	handlerPos = mOwningHandlers->find ( format );
	if ( handlerPos != mOwningHandlers->end() ) {
		mOwningHandlers->erase ( handlerPos );
		XMP_Assert ( ! this->getFormatInfo ( format ) );
		return;
	}

}	// HandlerRegistry::removeHandler

// =================================================================================================

XMP_FileFormat HandlerRegistry::getFileFormat( const std::string & fileExt, bool addIfNotFound /*= false*/ )
{
	if ( ! fileExt.empty() ) {
		for ( int i=0; kFileExtMap[i].format != 0; ++i ) {
			if ( fileExt == kFileExtMap[i].ext ) return kFileExtMap[i].format;
		}
	}

	#if EnablePluginManager
		return ResourceParser::getPluginFileFormat ( fileExt, addIfNotFound );
	#else
		return kXMP_UnknownFile;
	#endif
}

// =================================================================================================

XMPFileHandlerInfo*	HandlerRegistry::getHandlerInfo( XMP_FileFormat format )
{
	XMPFileHandlerTablePos handlerPos;

	handlerPos = mFolderHandlers->find( format );

	if( handlerPos != mFolderHandlers->end() ) 
	{
		return &(handlerPos->second);
	}

	handlerPos = mNormalHandlers->find ( format );

	if( handlerPos != mNormalHandlers->end() ) 
	{
		return &(handlerPos->second);
	}

	handlerPos = mOwningHandlers->find ( format );

	if( handlerPos != mOwningHandlers->end() ) 
	{
		return &(handlerPos->second);
	}

	return NULL;
}

// =================================================================================================

XMPFileHandlerInfo*	HandlerRegistry::getStandardHandlerInfo( XMP_FileFormat format )
{
	XMPFileHandlerTablePos handlerPos = mReplacedHandlers->find( format );

	if( handlerPos != mReplacedHandlers->end() )
	{
		return &(handlerPos->second);
	}
	else
	{
		return this->getHandlerInfo( format );
	}
}

// =================================================================================================

bool HandlerRegistry::isReplaced( XMP_FileFormat format )
{
	return ( mReplacedHandlers->find( format ) != mReplacedHandlers->end() );
}

// =================================================================================================

bool HandlerRegistry::getFormatInfo( XMP_FileFormat format, XMP_OptionBits* flags /*= 0*/ )
{
	if ( flags == 0 ) flags = &voidOptionBits;

	XMPFileHandlerInfo*	handler = this->getHandlerInfo( format );

	if( handler != NULL )
	{
		*flags = handler->flags;
	}

	return ( handler != NULL );
}	// HandlerRegistry::getFormatInfo

// =================================================================================================

XMPFileHandlerInfo* HandlerRegistry::pickDefaultHandler ( XMP_FileFormat format, const std::string & fileExt )
{
	if ( format == kXMP_UnknownFile ) format = this->getFileFormat ( fileExt );
	if ( format == kXMP_UnknownFile ) return 0;

	XMPFileHandlerTablePos handlerPos;

	handlerPos = mNormalHandlers->find ( format );
	if ( handlerPos != mNormalHandlers->end() ) return &handlerPos->second;

	handlerPos = mOwningHandlers->find ( format );
	if ( handlerPos != mOwningHandlers->end() ) return &handlerPos->second;

	handlerPos = mFolderHandlers->find ( format );
	if ( handlerPos != mFolderHandlers->end() ) return &handlerPos->second;

	return 0;
}

// =================================================================================================

XMPFileHandlerInfo* HandlerRegistry::selectSmartHandler( XMPFiles* session, XMP_StringPtr clientPath, XMP_FileFormat format, XMP_OptionBits openFlags )
{

	// The normal case for selectSmartHandler is when OpenFile is given a string file path. All of
	// the stages described below have slight special cases when OpenFile is given an XMP_IO object
	// for client-managed I/O. In that case the only handlers considered are those for embedded XMP
	// that do not need to own the file.
	//
	// There are 4 stages in finding a handler, ending at the first success:
	//   1. If the client passes in a format, try that handler.
	//   2. Try all of the folder-oriented handlers.
	//   3. Try a file-oriented handler based on the file extension.
	//   4. Try all of the file-oriented handlers.
	//
	// The most common case is almost certainly #3, so we want to get there quickly. Most of the
	// time the client won't pass in a format, so #1 takes no time. The folder-oriented handler
	// checks are preceded by minimal folder checks. These checks are meant to be fast in the
	// failure case. The folder-oriented checks have to go before the general file-oriented checks
	// because the client path might be to one of the inner files, and we might have a file-oriented
	// handler for that kind of file, but we want to recognize the clip. More details are below.
	//
	// In brief, the folder-oriented formats use shallow trees with specific folder names and
	// highly stylized file names. The user thinks of the tree as a collection of clips, each clip
	// is stored as multiple files for video, audio, metadata, etc. The folder-oriented stage has
	// to be first because there can be files in the structure that are also covered by a
	// file-oriented handler.
	//
	// In the file-oriented case, the CheckProc should do as little as possible to determine the
	// format, based on the actual file content. If that is not possible, use the format hint. The
	// initial CheckProc calls (steps 1 and 3) has the presumed format in this->format, the later
	// calls (step 4) have kXMP_UnknownFile there.
	//
	// The folder-oriented checks need to be well optimized since the formats are relatively rare,
	// but have to go first and could require multiple file system calls to identify. We want to
	// get to the first file-oriented guess as quickly as possible, that is the real handler most of
	// the time.
	//
	// The folder-oriented handlers are for things like P2 and XDCAM that use files distributed in a
	// well defined folder structure. Using a portion of P2 as an example:
	//	.../MyMovie
	//		CONTENTS
	//			CLIP
	//				0001AB.XML
	//				0002CD.XML
	//			VIDEO
	//				0001AB.MXF
	//				0002CD.MXF
	//			VOICE
	//				0001AB.WAV
	//				0002CD.WAV
	//
	// The user thinks of .../MyMovie as the container of P2 stuff, in this case containing 2 clips
	// called 0001AB and 0002CD. The exact folder structure and file layout differs, but the basic
	// concepts carry across all of the folder-oriented handlers.
	//
	// The client path can be a conceptual clip path like .../MyMovie/0001AB, or a full path to any
	// of the contained files. For file paths we have to behave the same as the implied conceptual
	// path, e.g. we don't want .../MyMovie/CONTENTS/VOICE/0001AB.WAV to invoke the WAV handler.
	// There might also be a mapping from user friendly names to clip names (e.g. Intro to 0001AB).
	// If so that is private to the handler and does not affect this code.
	//
	// In order to properly handle the file path input we have to look for the folder-oriented case
	// before any of the file-oriented cases. And since these are relatively rare, hence fail most of
	// the time, we have to get in and out fast in the not handled case. That is what we do here.
	//
	// The folder-oriented processing done here is roughly:
	//
	// 1. Get the state of the client path: does-not-exist, is-file, is-folder, is-other.
	// 2. Reject is-folder and is-other, they can't possibly be a valid case.
	// 3. For does-not-exist:
	//	3a. Split the client path into a leaf component and root path.
	//	3b. Make sure the root path names an existing folder.
	//	3c. Make sure the root folder has a viable top level child folder (e.g. CONTENTS).
	// 4. For is-file:
	//	4a. Split the client path into a root path, grandparent folder, parent folder, and leaf name.
	//	4b. Make sure the parent or grandparent has a viable name (e.g. CONTENTS).
	// 5. Try the registered folder handlers.
	//
	// For the common case of "regular" files, we should only get as far as 3b. This is just 1 file
	// system call to get the client path state and some string processing.

	bool readOnly = XMP_OptionIsClear( openFlags, kXMPFiles_OpenForUpdate );

	Host_IO::FileMode clientMode;
	std::string rootPath;
	std::string leafName;
	std::string fileExt;
	std::string emptyStr;

	XMPFileHandlerInfo* handlerInfo	= 0;
	bool foundHandler				= false;
	
	if ( openFlags & kXMPFiles_ForceGivenHandler ) {
		// We're being told to blindly use the handler for the given format and nothing else.
		return this->pickDefaultHandler ( format, emptyStr );	// Picks based on just the format.
	}

	if ( session->UsesClientIO() ) {

		XMP_Assert ( session->ioRef != 0 );
		clientMode = Host_IO::kFMode_IsFile;

	} else {

		clientMode = Host_IO::GetFileMode( clientPath );
		if ( (clientMode == Host_IO::kFMode_IsFolder) || (clientMode == Host_IO::kFMode_IsOther) ) return 0;

		rootPath = clientPath;
		XIO::SplitLeafName ( &rootPath, &leafName );
		
		if ( leafName.empty() ) return 0;

		if ( clientMode == Host_IO::kFMode_IsFile ) {
			// Only extract the file extension for existing files. Non-existing files can only be
			// logical clip names, and they don't have file extensions.
			XIO::SplitFileExtension ( &leafName, &fileExt );
		}

	}

	session->format	= kXMP_UnknownFile;	// Make sure it is preset for later checks.
	session->openFlags = openFlags;

	// If the client passed in a format, try that first.

	if( format != kXMP_UnknownFile ) 
	{
		handlerInfo = this->pickDefaultHandler( format, emptyStr );	// Picks based on just the format.

		if( handlerInfo != 0 ) 
		{
			if( ( session->ioRef == 0 ) && (! ( handlerInfo->flags & kXMPFiles_HandlerOwnsFile ) ) ) 
			{
				session->ioRef = XMPFiles_IO::New_XMPFiles_IO( clientPath, readOnly, &session->errorCallback);
				if ( session->ioRef == 0 ) return 0;
			}
			
			session->format = format;	// ! Hack to tell the CheckProc session is an initial call.

			if( handlerInfo->flags & kXMPFiles_FolderBasedFormat ) 
			{
#if 0
				std::string gpName, parentName;
				if ( clientMode == Host_IO::kFMode_IsFile ) {
					XIO::SplitLeafName( &rootPath, &parentName );
					XIO::SplitLeafName( &rootPath, &gpName );
					if ( format != kXMP_XDCAM_FAMFile ) MakeUpperCase( &gpName );	// ! Save the original case for XDCAM-FAM.
					MakeUpperCase( &parentName );
				}
				CheckFolderFormatProc CheckProc = (CheckFolderFormatProc) (handlerInfo->checkProc);
				foundHandler = CheckProc ( handlerInfo->format, rootPath, gpName, parentName, leafName, session );
#else
				// *** Don't try here yet. These are messy, needing existence checking and path processing.
				// *** CheckFolderFormatProc CheckProc = (CheckFolderFormatProc) (handlerInfo->checkProc);
				// *** foundHandler = CheckProc ( handlerInfo->format, rootPath, gpName, parentName, leafName, session );
				// *** Don't let OpenStrictly cause an early exit:
				if( openFlags & kXMPFiles_OpenStrictly ) openFlags ^= kXMPFiles_OpenStrictly;
#endif
			} 
			else 
			{
				bool tryThisHandler = true;

				if( session->UsesClientIO() ) 
				{
					if ( (handlerInfo->flags & kXMPFiles_UsesSidecarXMP) ||
						(handlerInfo->flags & kXMPFiles_HandlerOwnsFile) ) tryThisHandler = false;
				}

				if( tryThisHandler ) 
				{
					CheckFileFormatProc CheckProc = (CheckFileFormatProc) (handlerInfo->checkProc);
					foundHandler = CheckProc ( format, clientPath, session->ioRef, session );
				}
			}

			XMP_Assert( foundHandler || (session->tempPtr == 0) );
			
			if ( foundHandler ) return handlerInfo;
			
			handlerInfo = 0;	// ! Clear again for later use.
		}

		if ( openFlags & kXMPFiles_OpenStrictly ) return 0;

	}

#if EnableDynamicMediaHandlers	// All of the folder handlers are for dynamic media.

	// Try the folder handlers if appropriate.

	if( session->UsesLocalIO() ) 
	{
		XMP_Assert ( handlerInfo == 0 );
		XMP_Assert ( (clientMode == Host_IO::kFMode_IsFile) || (clientMode == Host_IO::kFMode_DoesNotExist) );

		std::string gpName, parentName;

		if( clientMode == Host_IO::kFMode_DoesNotExist ) 
		{
			// 3. For does-not-exist:
			//	3a. Split the client path into a leaf component and root path.
			//	3b. Make sure the root path names an existing folder.
			//	3c. Make sure the root folder has a viable top level child folder.

			// ! This does "return 0" on failure, the file does not exist so a normal file handler can't apply.

			if ( Host_IO::GetFileMode ( rootPath.c_str() ) != Host_IO::kFMode_IsFolder ) return 0;
			
			session->format = checkTopFolderName ( rootPath );
			
			if ( session->format == kXMP_UnknownFile ) return 0;

			handlerInfo = this->tryFolderHandlers( session->format, rootPath, gpName, parentName, leafName, session );	// ! Parent and GP are empty.

			return handlerInfo;	// ! Return found handler or 0.
		}

		XMP_Assert ( clientMode == Host_IO::kFMode_IsFile );

		// 4. For is-file:
		//	4a. Split the client path into root, grandparent, parent, and leaf.
		//	4b. Make sure the parent or grandparent has a viable name.

		// ! Don't "return 0" on failure, this has to fall through to the normal file handlers.

		XIO::SplitLeafName( &rootPath, &parentName );
		XIO::SplitLeafName( &rootPath, &gpName );
		std::string origGPName ( gpName );	// ! Save the original case for XDCAM-FAM.
		MakeUpperCase( &parentName );
		MakeUpperCase( &gpName );

		session->format = checkParentFolderNames( rootPath, gpName, parentName, leafName );

		if( session->format != kXMP_UnknownFile ) 
		{
			if( (session->format == kXMP_XDCAM_FAMFile) &&
			    ( (parentName == "CLIP") || (parentName == "EDIT") || (parentName == "SUB") ) ) 
			{
					// ! The standard says Clip/Edit/Sub, but we just shifted to upper case.
					gpName = origGPName;	// ! XDCAM-FAM has just 1 level of inner folder, preserve the "MyMovie" case.
			}

			handlerInfo = tryFolderHandlers ( session->format, rootPath, gpName, parentName, leafName, session );
			if ( handlerInfo != 0 ) return handlerInfo;

		}
	}

#endif	// EnableDynamicMediaHandlers

	// Try an initial file-oriented handler based on the extension.

	if( session->UsesLocalIO() ) 
	{
		handlerInfo = pickDefaultHandler ( kXMP_UnknownFile, fileExt );	// Picks based on just the extension.

		if( handlerInfo != 0 ) 
		{
			if( (session->ioRef == 0) && (! (handlerInfo->flags & kXMPFiles_HandlerOwnsFile)) ) 
			{
				session->ioRef = XMPFiles_IO::New_XMPFiles_IO ( clientPath, readOnly, &session->errorCallback);
				if ( session->ioRef == 0 ) return 0;
			} 
			else if( (session->ioRef != 0) && (handlerInfo->flags & kXMPFiles_HandlerOwnsFile) ) 
			{
				delete session->ioRef;	// Close is implicit in the destructor.
				session->ioRef = 0;
			}
			
			session->format = handlerInfo->format;	// ! Hack to tell the CheckProc this is an initial call.
			CheckFileFormatProc CheckProc = (CheckFileFormatProc) (handlerInfo->checkProc);
			foundHandler = CheckProc ( handlerInfo->format, clientPath, session->ioRef, session );
			XMP_Assert ( foundHandler || (session->tempPtr == 0) );
			
			if ( foundHandler ) return handlerInfo;
		}
	}

	// Search the handlers that don't want to open the file themselves.

	if( session->ioRef == 0 ) 
	{
		session->ioRef = XMPFiles_IO::New_XMPFiles_IO ( clientPath, readOnly, &session->errorCallback );
		if ( session->ioRef == 0 ) return 0;
	}
	
	XMPFileHandlerTablePos handlerPos = mNormalHandlers->begin();

	for( ; handlerPos != mNormalHandlers->end(); ++handlerPos ) 
	{
		session->format = kXMP_UnknownFile;	// ! Hack to tell the CheckProc this is not an initial call.
		handlerInfo = &handlerPos->second;
		CheckFileFormatProc CheckProc = (CheckFileFormatProc) (handlerInfo->checkProc);
		foundHandler = CheckProc ( handlerInfo->format, clientPath, session->ioRef, session );
		XMP_Assert ( foundHandler || (session->tempPtr == 0) );
		if ( foundHandler ) return handlerInfo;
	}

	// Search the handlers that do want to open the file themselves.

	if( session->UsesLocalIO() ) 
	{
		delete session->ioRef;	// Close is implicit in the destructor.
		session->ioRef = 0;
		handlerPos = mOwningHandlers->begin();

		for( ; handlerPos != mOwningHandlers->end(); ++handlerPos ) 
		{
			session->format = kXMP_UnknownFile;	// ! Hack to tell the CheckProc this is not an initial call.
			handlerInfo = &handlerPos->second;
			CheckFileFormatProc CheckProc = (CheckFileFormatProc) (handlerInfo->checkProc);
			foundHandler = CheckProc ( handlerInfo->format, clientPath, session->ioRef, session );
			XMP_Assert ( foundHandler || (session->tempPtr == 0) );
			if ( foundHandler ) return handlerInfo;
		}
	}

	// Failed to find a smart handler.

	return 0;

}	// HandlerRegistry::selectSmartHandler

// =================================================================================================

#if EnableDynamicMediaHandlers

XMPFileHandlerInfo* HandlerRegistry::tryFolderHandlers( XMP_FileFormat format,
													    const std::string & rootPath,
														const std::string & gpName,
														const std::string & parentName,
														const std::string & leafName,
														XMPFiles * parentObj )
{
	bool foundHandler = false;
	XMPFileHandlerInfo * handlerInfo = 0;
	XMPFileHandlerTablePos handlerPos;

	// We know we're in a possible context for a folder-oriented handler, so try them.

	if( format != kXMP_UnknownFile ) 
	{

		// Have an explicit format, pick that or nothing.
		handlerPos = mFolderHandlers->find ( format );
	
		if( handlerPos != mFolderHandlers->end() ) 
		{
			handlerInfo = &handlerPos->second;
			CheckFolderFormatProc CheckProc = (CheckFolderFormatProc) (handlerInfo->checkProc);
			foundHandler = CheckProc ( handlerInfo->format, rootPath, gpName, parentName, leafName, parentObj );
			XMP_Assert ( foundHandler || (parentObj->tempPtr == 0) );
		}
	} 
	else 
	{
		// Try all of the folder handlers.
		for( handlerPos = mFolderHandlers->begin(); handlerPos != mFolderHandlers->end(); ++handlerPos ) 
		{
			handlerInfo = &handlerPos->second;
			CheckFolderFormatProc CheckProc = (CheckFolderFormatProc) (handlerInfo->checkProc);
			foundHandler = CheckProc ( handlerInfo->format, rootPath, gpName, parentName, leafName, parentObj );
			XMP_Assert ( foundHandler || (parentObj->tempPtr == 0) );
			if ( foundHandler ) break;	// ! Exit before incrementing handlerPos.
		}
	}

	if ( ! foundHandler ) handlerInfo = 0;

	return handlerInfo;
}

#endif

// =================================================================================================

#if EnableDynamicMediaHandlers

/*static*/ XMP_FileFormat HandlerRegistry::checkTopFolderName( const std::string & rootPath )
{
	// This is called when the input path to XMPFiles::OpenFile does not name an existing file (or
	// existing anything). We need to quickly decide if this might be a logical path for a folder
	// handler. See if the root contains the top content folder for any of the registered folder
	// handlers. This check does not have to be precise, the handler will do that. This does have to
	// be fast.
	//
	// Since we don't have many folder handlers, this is simple hardwired code.

	std::string childPath = rootPath;
	childPath += kDirChar;
	size_t baseLen = childPath.size();

	// P2 .../MyMovie/CONTENTS/<group>/... - only check for CONTENTS/CLIP
	childPath += "CONTENTS";
	childPath += kDirChar;
	childPath += "CLIP";
	if ( Host_IO::GetFileMode ( childPath.c_str() ) == Host_IO::kFMode_IsFolder ) return kXMP_P2File;
	childPath.erase ( baseLen );

	// XDCAM-FAM .../MyMovie/<group>/... - only check for Clip and MEDIAPRO.XML
	childPath += "Clip";	// ! Yes, mixed case.
	if ( Host_IO::GetFileMode ( childPath.c_str() ) == Host_IO::kFMode_IsFolder ) {
		childPath.erase ( baseLen );
		childPath += "MEDIAPRO.XML";
		if ( Host_IO::GetFileMode ( childPath.c_str() ) == Host_IO::kFMode_IsFile ) return kXMP_XDCAM_FAMFile;
	}
	childPath.erase ( baseLen );

	// XDCAM-SAM .../MyMovie/PROAV/<group>/... - only check for PROAV/CLPR
	childPath += "PROAV";
	childPath += kDirChar;
	childPath += "CLPR";
	if ( Host_IO::GetFileMode ( childPath.c_str() ) == Host_IO::kFMode_IsFolder ) return kXMP_XDCAM_SAMFile;
	childPath.erase ( baseLen );

	// XDCAM-EX .../MyMovie/BPAV/<group>/... - check for BPAV/CLPR
	childPath += "BPAV";
	childPath += kDirChar;
	childPath += "CLPR";
	if ( Host_IO::GetFileMode ( childPath.c_str() ) == Host_IO::kFMode_IsFolder ) return kXMP_XDCAM_EXFile;
	childPath.erase ( baseLen );

	// Sony HDV .../MyMovie/VIDEO/HVR/<file>.<ext> - check for VIDEO/HVR
	childPath += "VIDEO";
	childPath += kDirChar;
	childPath += "HVR";
	if ( Host_IO::GetFileMode ( childPath.c_str() ) == Host_IO::kFMode_IsFolder ) return kXMP_SonyHDVFile;
	childPath.erase ( baseLen );

	return kXMP_UnknownFile;

}	// CheckTopFolderName

#endif

// =================================================================================================

#if EnableDynamicMediaHandlers

/*static*/ XMP_FileFormat HandlerRegistry::checkParentFolderNames( const std::string& rootPath,
																   const std::string& gpName,
																   const std::string& parentName, 
																   const std::string& leafName )
{
	IgnoreParam ( parentName );

	// This is called when the input path to XMPFiles::OpenFile names an existing file. We need to
	// quickly decide if this might be inside a folder-handler's structure. See if the containing
	// folders might match any of the registered folder handlers. This check does not have to be
	// precise, the handler will do that. This does have to be fast.
	//
	// Since we don't have many folder handlers, this is simple hardwired code. Note that the caller
	// has already shifted the names to upper case.

	// P2  .../MyMovie/CONTENTS/<group>/<file>.<ext> - check CONTENTS and <group>
	if ( (gpName == "CONTENTS") && CheckP2ContentChild ( parentName ) ) return kXMP_P2File;

	// XDCAM-EX  .../MyMovie/BPAV/CLPR/<clip>/<file>.<ext> - check for BPAV/CLPR
	// ! This must be checked before XDCAM-SAM because both have a "CLPR" grandparent.
	if ( gpName == "CLPR" ) {
		std::string tempPath, greatGP;
		tempPath = rootPath;
		XIO::SplitLeafName ( &tempPath, &greatGP );
		MakeUpperCase ( &greatGP );
		if ( greatGP == "BPAV" ) return kXMP_XDCAM_EXFile;
	}

	// XDCAM-FAM  .../MyMovie/<group>/<file>.<ext> - check that <group> is CLIP, or EDIT, or SUB
	// ! The standard says Clip/Edit/Sub, but the caller has already shifted to upper case.
	if ( (parentName == "CLIP") || (parentName == "EDIT") || (parentName == "SUB") ) return kXMP_XDCAM_FAMFile;

	// XDCAM-SAM  .../MyMovie/PROAV/<group>/<clip>/<file>.<ext> - check for PROAV and CLPR or EDTR
	if ( (gpName == "CLPR") || (gpName == "EDTR") ) {
		std::string tempPath, greatGP;
		tempPath = rootPath;
		XIO::SplitLeafName ( &tempPath, &greatGP );
		MakeUpperCase ( &greatGP );
		if ( greatGP == "PROAV" ) return kXMP_XDCAM_SAMFile;
	}

	// Sony HDV  .../MyMovie/VIDEO/HVR/<file>.<ext> - check for VIDEO and HVR
	if ( (gpName == "VIDEO") && (parentName == "HVR") ) return kXMP_SonyHDVFile;

	return kXMP_UnknownFile;

}	// CheckParentFolderNames

#endif
