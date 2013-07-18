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
#include "source/IOUtils.hpp"

#include "XMPFiles/source/FileHandlers/AVCHD_Handler.hpp"
#include "XMPFiles/source/FormatSupport/PackageFormat_Support.hpp"

#include "source/UnicodeConversions.hpp"
#include "third-party/zuid/interfaces/MD5.h"

using namespace std;

// AVCHD maker ID values. Panasonic has confirmed their Maker ID with us, the others come from examining
// sample data files.
#define kMakerIDPanasonic 0x103
#define kMakerIDSony 0x108
#define kMakerIDCanon 0x1011

// =================================================================================================
/// \file AVCHD_Handler.cpp
/// \brief Folder format handler for AVCHD.
///
/// This handler is for the AVCHD video format.
///
/// A typical AVCHD layout looks like:
///
///     BDMV/
///         index.bdmv
///         MovieObject.bdmv
///         PLAYLIST/
///				00000.mpls
///             00001.mpls
///         STREAM/
///				00000.m2ts
///             00001.m2ts
///         CLIPINF/
///				00000.clpi
///             00001.clpi
///         BACKUP/
///
// =================================================================================================

// =================================================================================================

// AVCHD Format. Book 1: Playback System Basic Specifications V 1.01. p. 76

struct AVCHD_blkProgramInfo
{
	XMP_Uns32	mLength;
	XMP_Uns8	mReserved1[2];
	XMP_Uns32	mSPNProgramSequenceStart;
	XMP_Uns16	mProgramMapPID;
	XMP_Uns8	mNumberOfStreamsInPS;
	XMP_Uns8	mReserved2;

	// Video stream.
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mVideoFormat;
		XMP_Uns8	mFrameRate;
		XMP_Uns8	mAspectRatio;
		XMP_Uns8	mCCFlag;
	} mVideoStream;

	// Audio stream.
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mAudioPresentationType;
		XMP_Uns8	mSamplingFrequency;
		XMP_Uns8	mAudioLanguageCode[4];
	} mAudioStream;

	// Pverlay bitmap stream.
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mOBLanguageCode[4];
	} mOverlayBitmapStream;

	// Menu bitmap stream.
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mBMLanguageCode[4];
	} mMenuBitmapStream;

};

// AVCHD Format, Panasonic proprietary PRO_PlayListMark block

struct AVCCAM_blkProPlayListMark
{
	XMP_Uns8	mPresent;
	XMP_Uns8	mProTagID;
	XMP_Uns8	mFillItem1;
	XMP_Uns16	mLength;
	XMP_Uns8	mMarkType;

	// Entry mark
	struct
	{
		XMP_Uns8	mGlobalClipID[32];
		XMP_Uns8	mStartTimeCode[4];
		XMP_Uns8	mStreamTimecodeInfo;
		XMP_Uns8	mStartBinaryGroup[4];
		XMP_Uns8	mLastUpdateTimeZone;
		XMP_Uns8	mLastUpdateDate[7];
		XMP_Uns16	mFillItem;
	} mEntryMark;

	// Shot Mark
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mShotMark;
		XMP_Uns8	mFillItem[3];
	} mShotMark;

	// Access
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mCreatorCharacterSet;
		XMP_Uns8	mCreatorLength;
		XMP_Uns8	mCreator[32];
		XMP_Uns8	mLastUpdatePersonCharacterSet;
		XMP_Uns8	mLastUpdatePersonLength;
		XMP_Uns8	mLastUpdatePerson[32];
	} mAccess;

	// Device
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns16	mMakerID;
		XMP_Uns16	mMakerModelCode;
		XMP_Uns8	mSerialNoCharacterCode;
		XMP_Uns8	mSerialNoLength;
		XMP_Uns8	mSerialNo[24];
		XMP_Uns16	mFillItem;
	} mDevice;

	// Shoot
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mShooterCharacterSet;
		XMP_Uns8	mShooterLength;
		XMP_Uns8	mShooter[32];
		XMP_Uns8	mStartDateTimeZone;
		XMP_Uns8	mStartDate[7];
		XMP_Uns8	mEndDateTimeZone;
		XMP_Uns8	mEndDate[7];
		XMP_Uns16	mFillItem;
	} mShoot;

	// Location
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mSource;
		XMP_Uns32	mGPSLatitudeRef;
		XMP_Uns32	mGPSLatitude1;
		XMP_Uns32	mGPSLatitude2;
		XMP_Uns32	mGPSLatitude3;
		XMP_Uns32	mGPSLongitudeRef;
		XMP_Uns32	mGPSLongitude1;
		XMP_Uns32	mGPSLongitude2;
		XMP_Uns32	mGPSLongitude3;
		XMP_Uns32	mGPSAltitudeRef;
		XMP_Uns32	mGPSAltitude;
		XMP_Uns8	mPlaceNameCharacterSet;
		XMP_Uns8	mPlaceNameLength;
		XMP_Uns8	mPlaceName[64];
		XMP_Uns8	mFillItem;
	} mLocation;
};

// AVCHD Format, Panasonic proprietary extension data (AVCCAM)

struct AVCCAM_Pro_PlayListInfo
{
	XMP_Uns8					mPresent;
	XMP_Uns8					mTagID;
	XMP_Uns8					mTagVersion;
	XMP_Uns16					mFillItem1;
	XMP_Uns32					mLength;
	XMP_Uns16					mNumberOfPlayListMarks;
	XMP_Uns16					mFillItem2;

	// Although a playlist may contain multiple marks, we only store the one that corresponds to
	// the clip/shot of interest.
	AVCCAM_blkProPlayListMark	mPlayListMark;
};

// AVCHD Format, Panasonic proprietary extension data (AVCCAM)

struct AVCHD_blkPanasonicPrivateData
{
	XMP_Uns8					mPresent;
	XMP_Uns16					mNumberOfData;
	XMP_Uns16					mReserved;

	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mTagID;
		XMP_Uns8	mTagVersion;
		XMP_Uns16	mTagLength;
		XMP_Uns8	mProfessionalMetaID[16];
	} mProMetaIDBlock;

	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mTagID;
		XMP_Uns8	mTagVersion;
		XMP_Uns16	mTagLength;
		XMP_Uns8	mGlobalClipID[32];
		XMP_Uns8	mStartTimecode[4];
		XMP_Uns32	mStartBinaryGroup;
	} mProClipIDBlock;

	AVCCAM_Pro_PlayListInfo		mProPlaylistInfoBlock;
};

// AVCHD Format. Book 2: Recording Extension Specifications, section 4.2.4.2. plus Panasonic extensions

struct AVCHD_blkMakersPrivateData
{
	XMP_Uns8						mPresent;
	XMP_Uns32						mLength;
	XMP_Uns32						mDataBlockStartAddress;
	XMP_Uns8						mReserved[3];
	XMP_Uns8						mNumberOfMakerEntries;
	XMP_Uns16						mMakerID;
	XMP_Uns16						mMakerModelCode;
	AVCHD_blkPanasonicPrivateData	mPanasonicPrivateData;
};

// AVCHD Format. Book 2: Recording Extension Specifications, section 4.4.2.1

struct AVCHD_blkClipInfoExt
{
	XMP_Uns32	mLength;
	XMP_Uns16	mMakerID;
	XMP_Uns16	mMakerModelCode;
};

// AVCHD Format. Book 2: Recording Extension Specifications, section 4.4.1.2

struct AVCHD_blkClipExtensionData
{
	XMP_Uns8					mPresent;
	XMP_Uns8					mTypeIndicator[4];
	XMP_Uns8					mReserved1[4];
	XMP_Uns32					mProgramInfoExtStartAddress;
	XMP_Uns32					mMakersPrivateDataStartAddress;

	AVCHD_blkClipInfoExt		mClipInfoExt;
	AVCHD_blkMakersPrivateData	mMakersPrivateData;
};

// AVCHD Format. Book 2: Recording Extension Specifications, section 4.3.3.1 -- although each playlist
// may contain a list of these, we only record the one that matches our target shot/clip.

struct AVCHD_blkPlayListMarkExt
{
	XMP_Uns32	mLength;
	XMP_Uns16	mNumberOfPlaylistMarks;
	bool		mPresent;
	XMP_Uns16	mMakerID;
	XMP_Uns16	mMakerModelCode;
	XMP_Uns8	mReserved1[3];
	XMP_Uns8	mFlags;			// bit 0: MarkWriteProtectFlag, bits 1-2: pulldown
	XMP_Uns16	mRefToMarkThumbnailIndex;
	XMP_Uns8	mBlkTimezone;
	XMP_Uns8	mRecordDataAndTime[7];
	XMP_Uns8	mMarkCharacterSet;
	XMP_Uns8	mMarkNameLength;
	XMP_Uns8	mMarkName[24];
	XMP_Uns8	mMakersInformation[16];
	XMP_Uns8	mBlkTimecode[4];
	XMP_Uns16	mReserved2;
};

// AVCHD Format. Book 2: Recording Extension Specifications, section 4.3.2.1

struct AVCHD_blkPlaylistMeta
{
	XMP_Uns32	mLength;
	XMP_Uns16	mMakerID;
	XMP_Uns16	mMakerModelCode;
	XMP_Uns32	mReserved1;
	XMP_Uns16	mRefToMenuThumbnailIndex;
	XMP_Uns8	mBlkTimezone;
	XMP_Uns8	mRecordDataAndTime[7];
	XMP_Uns8	mReserved2;
	XMP_Uns8	mPlaylistCharacterSet;
	XMP_Uns8	mPlaylistNameLength;
	XMP_Uns8	mPlaylistName[255];
};

// AVCHD Format. Book 2: Recording Extension Specifications, section 4.3.1.2

struct AVCHD_blkPlayListExtensionData
{
	XMP_Uns8					mPresent;
	char						mTypeIndicator[4];
	XMP_Uns8					mReserved[4];
	XMP_Uns32					mPlayListMarkExtStartAddress;
	XMP_Uns32					mMakersPrivateDataStartAddress;

	AVCHD_blkPlaylistMeta		mPlaylistMeta;
	AVCHD_blkPlayListMarkExt	mPlaylistMarkExt;
	AVCHD_blkMakersPrivateData	mMakersPrivateData;
};

// AVCHD Format. Book 1: Playback System Basic Specifications V 1.01. p. 38
struct AVCHD_blkExtensionData
{
	XMP_Uns32	mLength;
	XMP_Uns32	mDataBlockStartAddress;
	XMP_Uns8	mReserved[3];
	XMP_Uns8	mNumberOfDataEntries;

	struct AVCHD_blkExtDataEntry
	{
		XMP_Uns16	mExtDataType;
		XMP_Uns16	mExtDataVersion;
		XMP_Uns32	mExtDataStartAddress;
		XMP_Uns32	mExtDataLength;
	} mExtDataEntry;
};

// Simple container for the various AVCHD legacy metadata structures we care about for an AVCHD clip

struct AVCHD_LegacyMetadata
{
	AVCHD_blkProgramInfo			mProgramInfo;
	AVCHD_blkClipExtensionData		mClipExtensionData;
	AVCHD_blkPlayListExtensionData	mPlaylistExtensionData;
};

// =================================================================================================
// MakeLeafPath
// ============

static bool MakeLeafPath ( std::string * path, XMP_StringPtr root, XMP_StringPtr group,
						   XMP_StringPtr clip, XMP_StringPtr suffix, bool checkFile = false )
{
	size_t partialLen;

	*path = root;
	*path += kDirChar;
	*path += "BDMV";
	*path += kDirChar;
	*path += group;
	*path += kDirChar;
	*path += clip;
	partialLen = path->size();
	*path += suffix;

	if ( ! checkFile ) return true;
	if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

	// Convert the suffix to uppercase and try again. Even on Mac/Win, in case a remote file system is sensitive.
	for ( char* chPtr = ((char*)path->c_str() + partialLen); *chPtr != 0; ++chPtr ) {
		if ( (0x61 <= *chPtr) && (*chPtr <= 0x7A) ) *chPtr -= 0x20;
	}
	if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

	if ( XMP_LitMatch ( suffix, ".clpi" ) ) {	// Special case of ".cpi" for the clip file.

		path->erase ( partialLen );
		*path += ".cpi";
		if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

		path->erase ( partialLen );
		*path += ".CPI";
		if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

	} else if ( XMP_LitMatch ( suffix, ".mpls" ) ) {	// Special case of ".mpl" for the playlist file.

		path->erase ( partialLen );
		*path += ".mpl";
		if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

		path->erase ( partialLen );
		*path += ".MPL";
		if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

	} else if ( XMP_LitMatch ( suffix, ".m2ts" ) ) {	// Special case of ".mts" for the stream file.

		path->erase ( partialLen );
		*path += ".mts";
		if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

		path->erase ( partialLen );
		*path += ".MTS";
		if ( Host_IO::GetFileMode ( path->c_str() ) == Host_IO::kFMode_IsFile ) return true;

	}

	// Still not found, revert to the original suffix.
	path->erase ( partialLen );
	*path += suffix;
	return false;

}	// MakeLeafPath

// =================================================================================================
// AVCHD_CheckFormat
// =================
//
// This version checks for the presence of a top level BPAV directory, and the required files and
// directories immediately within it. The CLIPINF, PLAYLIST, and STREAM subfolders are required, as
// are the index.bdmv and MovieObject.bdmv files.
//
// The state of the string parameters depends on the form of the path passed by the client. If the
// client passed a logical clip path, like ".../MyMovie/00001", the parameters are:
//   rootPath   - ".../MyMovie"
//   gpName     - empty
//   parentName - empty
//   leafName   - "00001"
// If the client passed a full file path, like ".../MyMovie/BDMV/CLIPINF/00001.clpi", they are:
//   rootPath   - ".../MyMovie"
//   gpName     - "BDMV"
//   parentName - "CLIPINF" or "PALYLIST" or "STREAM"
//   leafName   - "00001"

// ! The common code has shifted the gpName, parentName, and leafName strings to upper case. It has
// ! also made sure that for a logical clip path the rootPath is an existing folder, and that the
// ! file exists for a full file path.

// ! Using explicit '/' as a separator when creating paths, it works on Windows.

// ! Sample files show that the ".bdmv" extension can sometimes be ".bdm". Allow either.

bool AVCHD_CheckFormat ( XMP_FileFormat format,
						 const std::string & rootPath,
						 const std::string & gpName,
						 const std::string & parentName,
						 const std::string & leafName,
						 XMPFiles * parent )
{
	if ( gpName.empty() != parentName.empty() ) return false;	// Must be both empty or both non-empty.

	if ( ! gpName.empty() ) {
		if ( gpName != "BDMV" ) return false;
		if ( (parentName != "CLIPINF") && (parentName != "PLAYLIST") && (parentName != "STREAM") ) return false;
	}

	// Check the rest of the required general structure. Look for both ".bdmv" and ".bmd" extensions.

	std::string bdmvPath ( rootPath );
	bdmvPath += kDirChar;
	bdmvPath += "BDMV";

	if ( Host_IO::GetChildMode ( bdmvPath.c_str(), "CLIPINF" ) != Host_IO::kFMode_IsFolder ) return false;
	if ( Host_IO::GetChildMode ( bdmvPath.c_str(), "PLAYLIST" ) != Host_IO::kFMode_IsFolder ) return false;
	if ( Host_IO::GetChildMode ( bdmvPath.c_str(), "STREAM" ) != Host_IO::kFMode_IsFolder ) return false;

	if ( (Host_IO::GetChildMode ( bdmvPath.c_str(), "index.bdmv" ) != Host_IO::kFMode_IsFile) &&
		 (Host_IO::GetChildMode ( bdmvPath.c_str(), "index.bdm" ) != Host_IO::kFMode_IsFile) &&
		 (Host_IO::GetChildMode ( bdmvPath.c_str(), "INDEX.BDMV" ) != Host_IO::kFMode_IsFile) &&	// Some usage is all caps.
		 (Host_IO::GetChildMode ( bdmvPath.c_str(), "INDEX.BDM" ) != Host_IO::kFMode_IsFile) ) return false;

	if ( (Host_IO::GetChildMode ( bdmvPath.c_str(), "MovieObject.bdmv" ) != Host_IO::kFMode_IsFile) &&
		 (Host_IO::GetChildMode ( bdmvPath.c_str(), "MovieObj.bdm" ) != Host_IO::kFMode_IsFile) &&
		 (Host_IO::GetChildMode ( bdmvPath.c_str(), "MOVIEOBJECT.BDMV" ) != Host_IO::kFMode_IsFile) &&	// Some usage is all caps.
		 (Host_IO::GetChildMode ( bdmvPath.c_str(), "MOVIEOBJ.BDM" ) != Host_IO::kFMode_IsFile) ) return false;


	// Make sure the .clpi file exists.
	std::string tempPath;
	bool foundClpi = MakeLeafPath ( &tempPath, rootPath.c_str(), "CLIPINF", leafName.c_str(), ".clpi", true /* checkFile */ );
	if ( ! foundClpi ) return false;

	// And now save the pseudo path for the handler object.
	tempPath = rootPath;
	tempPath += kDirChar;
	tempPath += leafName;
	size_t pathLen = tempPath.size() + 1;	// Include a terminating nul.
	parent->tempPtr = malloc ( pathLen );
	if ( parent->tempPtr == 0 ) XMP_Throw ( "No memory for AVCHD clip info", kXMPErr_NoMemory );
	memcpy ( parent->tempPtr, tempPath.c_str(), pathLen );

	return true;

}	// AVCHD_CheckFormat

// =================================================================================================

static void* CreatePseudoClipPath ( const std::string & clientPath ) {

	// Used to create the clip pseudo path when the CheckFormat function is skipped.
	
	std::string pseudoPath = clientPath;

	size_t pathLen;
	void* tempPtr = 0;
	
	if ( Host_IO::Exists ( pseudoPath.c_str() ) ) {
	
		// The client passed a physical path. The logical clip name is the leaf name, with the
		// extension removed. There are no extra suffixes on AVCHD files. The movie root path ends
		// two levels up.
		
		std::string clipName, ignored;
		
		XIO::SplitLeafName ( &pseudoPath, &clipName );	// Extract the logical clip name.
		XIO::SplitFileExtension ( &clipName, &ignored );

		XIO::SplitLeafName ( &pseudoPath, &ignored );	// Remove the 2 intermediate folder levels.
		XIO::SplitLeafName ( &pseudoPath, &ignored );
		
		pseudoPath += kDirChar;
		pseudoPath += clipName;
	
	}

	pathLen = pseudoPath.size() + 1;	// Include a terminating nul.
	tempPtr = malloc ( pathLen );
	if ( tempPtr == 0 ) XMP_Throw ( "No memory for AVCHD clip info", kXMPErr_NoMemory );
	memcpy ( tempPtr, pseudoPath.c_str(), pathLen );
	
	return tempPtr;

}	// CreatePseudoClipPath

// =================================================================================================
// ReadAVCHDProgramInfo
// ====================

static bool ReadAVCHDProgramInfo ( XMPFiles_IO & cpiFile, AVCHD_blkProgramInfo& avchdProgramInfo )
{
	avchdProgramInfo.mLength = XIO::ReadUns32_BE ( &cpiFile );
	cpiFile.ReadAll ( avchdProgramInfo.mReserved1, 2 );
	avchdProgramInfo.mSPNProgramSequenceStart = XIO::ReadUns32_BE ( &cpiFile );
	avchdProgramInfo.mProgramMapPID = XIO::ReadUns16_BE ( &cpiFile );
	cpiFile.ReadAll ( &avchdProgramInfo.mNumberOfStreamsInPS, 1 );
	cpiFile.ReadAll ( &avchdProgramInfo.mReserved2, 1 );

	XMP_Uns16 streamPID = 0;
	for ( int i=0; i<avchdProgramInfo.mNumberOfStreamsInPS; ++i ) {

		XMP_Uns8	length = 0;
		XMP_Uns8	streamCodingType = 0;

		streamPID = XIO::ReadUns16_BE ( &cpiFile );
		cpiFile.ReadAll ( &length, 1 );

		XMP_Int64 pos = cpiFile.Offset();

		cpiFile.ReadAll ( &streamCodingType, 1 );

		switch ( streamCodingType ) {

			case 0x1B	: // Video stream case.
				{
					XMP_Uns8 videoFormatAndFrameRate;
					cpiFile.ReadAll ( &videoFormatAndFrameRate, 1 );
					avchdProgramInfo.mVideoStream.mVideoFormat	= videoFormatAndFrameRate >> 4;    // hi 4 bits
					avchdProgramInfo.mVideoStream.mFrameRate	= videoFormatAndFrameRate & 0x0f;  // lo 4 bits

					XMP_Uns8 aspectRatioAndReserved = 0;
					cpiFile.ReadAll ( &aspectRatioAndReserved, 1 );
					avchdProgramInfo.mVideoStream.mAspectRatio	 = aspectRatioAndReserved >> 4; // hi 4 bits

					XMP_Uns8 ccFlag = 0;
					cpiFile.ReadAll ( &ccFlag, 1 );
					avchdProgramInfo.mVideoStream.mCCFlag = ccFlag;

					avchdProgramInfo.mVideoStream.mPresent = 1;
				}
				break;

			case 0x80   : // Fall through.
			case 0x81	: // Audio stream case.
				{
					XMP_Uns8 audioPresentationTypeAndFrequency = 0;
					cpiFile.ReadAll ( &audioPresentationTypeAndFrequency, 1 );

					avchdProgramInfo.mAudioStream.mAudioPresentationType = audioPresentationTypeAndFrequency >> 4;    // hi 4 bits
					avchdProgramInfo.mAudioStream.mSamplingFrequency	 = audioPresentationTypeAndFrequency & 0x0f;  // lo 4 bits

					cpiFile.ReadAll ( avchdProgramInfo.mAudioStream.mAudioLanguageCode, 3 );
					avchdProgramInfo.mAudioStream.mAudioLanguageCode[3] = 0;

					avchdProgramInfo.mAudioStream.mPresent = 1;
				}
				break;

			case 0x90	: // Overlay bitmap stream case.
				cpiFile.ReadAll ( &avchdProgramInfo.mOverlayBitmapStream.mOBLanguageCode, 3 );
				avchdProgramInfo.mOverlayBitmapStream.mOBLanguageCode[3] = 0;
				avchdProgramInfo.mOverlayBitmapStream.mPresent = 1;
				break;

			case 0x91   : // Menu bitmap stream.
				cpiFile.ReadAll ( &avchdProgramInfo.mMenuBitmapStream.mBMLanguageCode, 3 );
				avchdProgramInfo.mMenuBitmapStream.mBMLanguageCode[3] = 0;
				avchdProgramInfo.mMenuBitmapStream.mPresent = 1;
				break;

			default :
				break;

		}

		cpiFile.Seek ( pos + length, kXMP_SeekFromStart );

	}

	return true;
}

// =================================================================================================
// ReadAVCHDExtensionData
// ======================

static bool ReadAVCHDExtensionData ( XMPFiles_IO & cpiFile, AVCHD_blkExtensionData& extensionDataHeader )
{
	extensionDataHeader.mLength = XIO::ReadUns32_BE ( &cpiFile );

	if ( extensionDataHeader.mLength == 0 ) {
		// Nothing to read
		return true;
	}

	extensionDataHeader.mDataBlockStartAddress = XIO::ReadUns32_BE ( &cpiFile );
	cpiFile.ReadAll ( extensionDataHeader.mReserved, 3 );
	cpiFile.ReadAll ( &extensionDataHeader.mNumberOfDataEntries, 1 );

	if ( extensionDataHeader.mNumberOfDataEntries != 1 ) {
		// According to AVCHD Format. Book1. v. 1.01. p 38, "This field shall be set to 1 in this format."
		return false;
	}

	extensionDataHeader.mExtDataEntry.mExtDataType = XIO::ReadUns16_BE ( &cpiFile );
	extensionDataHeader.mExtDataEntry.mExtDataVersion = XIO::ReadUns16_BE ( &cpiFile );
	extensionDataHeader.mExtDataEntry.mExtDataStartAddress = XIO::ReadUns32_BE ( &cpiFile );
	extensionDataHeader.mExtDataEntry.mExtDataLength = XIO::ReadUns32_BE ( &cpiFile );

	if ( extensionDataHeader.mExtDataEntry.mExtDataType != 0x1000 ) {
		// According to AVCHD Format. Book1. v. 1.01. p 38, "If the metadata is for an AVCHD application,
		// this value shall be set to 'Ox1OOO'."
		return false;
	}

	return true;
}

// =================================================================================================
// ReadAVCCAMProMetaID
// ===================
//
// Read Panasonic's proprietary PRO_MetaID block

static bool ReadAVCCAMProMetaID ( XMPFiles_IO & cpiFile, XMP_Uns8 tagID, AVCHD_blkPanasonicPrivateData& extensionDataHeader )
{
	extensionDataHeader.mPresent = 1;
	extensionDataHeader.mProMetaIDBlock.mPresent = 1;
	extensionDataHeader.mProMetaIDBlock.mTagID = tagID;
	cpiFile.ReadAll ( &extensionDataHeader.mProMetaIDBlock.mTagVersion, 1);
	extensionDataHeader.mProMetaIDBlock.mTagLength = XIO::ReadUns16_BE ( &cpiFile );
	cpiFile.ReadAll ( &extensionDataHeader.mProMetaIDBlock.mProfessionalMetaID, 16);

	return true;
}

// =================================================================================================
// ReadAVCCAMProClipInfo
// =====================
//
// Read Panasonic's proprietary PRO_ClipInfo block.

static bool ReadAVCCAMProClipInfo ( XMPFiles_IO & cpiFile, XMP_Uns8 tagID, AVCHD_blkPanasonicPrivateData& extensionDataHeader )
{
	extensionDataHeader.mPresent = 1;
	extensionDataHeader.mProClipIDBlock.mPresent = 1;
	extensionDataHeader.mProClipIDBlock.mTagID = tagID;
	cpiFile.ReadAll ( &extensionDataHeader.mProClipIDBlock.mTagVersion, 1);
	extensionDataHeader.mProClipIDBlock.mTagLength = XIO::ReadUns16_BE ( &cpiFile );
	cpiFile.ReadAll ( &extensionDataHeader.mProClipIDBlock.mGlobalClipID, 32);
	cpiFile.ReadAll ( &extensionDataHeader.mProClipIDBlock.mStartTimecode, 4 );
	extensionDataHeader.mProClipIDBlock.mStartBinaryGroup = XIO::ReadUns32_BE ( &cpiFile );

	return true;
}

// =================================================================================================
// ReadAVCCAM_blkPRO_ShotMark
// ==========================
//
// Read Panasonic's proprietary PRO_ShotMark block.

static bool ReadAVCCAM_blkPRO_ShotMark ( XMPFiles_IO & mplFile, AVCCAM_blkProPlayListMark& proMark )
{
	proMark.mShotMark.mPresent = 1;
	mplFile.ReadAll ( &proMark.mShotMark.mShotMark, 1);
	mplFile.ReadAll ( &proMark.mShotMark.mFillItem, 3);

	return true;
}

// =================================================================================================
// ReadAVCCAM_blkPRO_Access
// ========================
//
// Read Panasonic's proprietary PRO_Access block.

static bool ReadAVCCAM_blkPRO_Access ( XMPFiles_IO & mplFile, AVCCAM_blkProPlayListMark& proMark )
{
	proMark.mAccess.mPresent = 1;
	mplFile.ReadAll ( &proMark.mAccess.mCreatorCharacterSet, 1 );
	mplFile.ReadAll ( &proMark.mAccess.mCreatorLength, 1 );
	mplFile.ReadAll ( &proMark.mAccess.mCreator, 32 );
	mplFile.ReadAll ( &proMark.mAccess.mLastUpdatePersonCharacterSet, 1 );
	mplFile.ReadAll ( &proMark.mAccess.mLastUpdatePersonLength, 1 );
	mplFile.ReadAll ( &proMark.mAccess.mLastUpdatePerson, 32 );

	return true;
}

// =================================================================================================
// ReadAVCCAM_blkPRO_Device
// ========================
//
// Read Panasonic's proprietary PRO_Device block.

static bool ReadAVCCAM_blkPRO_Device ( XMPFiles_IO & mplFile, AVCCAM_blkProPlayListMark& proMark )
{
	proMark.mDevice.mPresent = 1;
	proMark.mDevice.mMakerID = XIO::ReadUns16_BE ( &mplFile );
	proMark.mDevice.mMakerModelCode = XIO::ReadUns16_BE ( &mplFile );
	mplFile.ReadAll ( &proMark.mDevice.mSerialNoCharacterCode, 1 );
	mplFile.ReadAll ( &proMark.mDevice.mSerialNoLength, 1 );
	mplFile.ReadAll ( &proMark.mDevice.mSerialNo, 24 );
	mplFile.ReadAll ( &proMark.mDevice.mFillItem, 2 );

	return true;
}

// =================================================================================================
// ReadAVCCAM_blkPRO_Shoot
// =======================
//
// Read Panasonic's proprietary PRO_Shoot block.

static bool ReadAVCCAM_blkPRO_Shoot ( XMPFiles_IO & mplFile, AVCCAM_blkProPlayListMark& proMark )
{
	proMark.mShoot.mPresent = 1;
	mplFile.ReadAll ( &proMark.mShoot.mShooterCharacterSet, 1 );
	mplFile.ReadAll ( &proMark.mShoot.mShooterLength, 1 );
	mplFile.ReadAll ( &proMark.mShoot.mShooter, 32 );
	mplFile.ReadAll ( &proMark.mShoot.mStartDateTimeZone, 1 );
	mplFile.ReadAll ( &proMark.mShoot.mStartDate, 7 );
	mplFile.ReadAll ( &proMark.mShoot.mEndDateTimeZone, 1 );
	mplFile.ReadAll ( &proMark.mShoot.mEndDate, 7 );
	mplFile.ReadAll ( &proMark.mShoot.mFillItem, 2 );

	return true;
}

// =================================================================================================
// ReadAVCCAM_blkPRO_Location
// ==========================
//
// Read Panasonic's proprietary PRO_Location block.

static bool ReadAVCCAM_blkPRO_Location ( XMPFiles_IO & mplFile, AVCCAM_blkProPlayListMark& proMark )
{
	proMark.mLocation.mPresent = 1;
	mplFile.ReadAll ( &proMark.mLocation.mSource, 1 );
	proMark.mLocation.mGPSLatitudeRef = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLatitude1 = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLatitude2 = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLatitude3 = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLongitudeRef = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLongitude1 = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLongitude2 = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSLongitude3 = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSAltitudeRef = XIO::ReadUns32_BE ( &mplFile );
	proMark.mLocation.mGPSAltitude = XIO::ReadUns32_BE ( &mplFile );
	mplFile.ReadAll ( &proMark.mLocation.mPlaceNameCharacterSet, 1 );
	mplFile.ReadAll ( &proMark.mLocation.mPlaceNameLength, 1 );
	mplFile.ReadAll ( &proMark.mLocation.mPlaceName, 64 );
	mplFile.ReadAll ( &proMark.mLocation.mFillItem, 1 );

	return true;
}

// =================================================================================================
// ReadAVCCAMProPlaylistInfo
// =========================
//
// Read Panasonic's proprietary PRO_PlayListInfo block.

static bool ReadAVCCAMProPlaylistInfo ( XMPFiles_IO & mplFile,
										XMP_Uns8 tagID,
										XMP_Uns16 playlistMarkID,
										AVCHD_blkPanasonicPrivateData& extensionDataHeader )
{
	AVCCAM_Pro_PlayListInfo& playlistBlock = extensionDataHeader.mProPlaylistInfoBlock;

	playlistBlock.mTagID = tagID;
	mplFile.ReadAll ( &playlistBlock.mTagVersion, 1);
	mplFile.ReadAll ( &playlistBlock.mFillItem1, 2);
	playlistBlock.mLength = XIO::ReadUns32_BE ( &mplFile );
	playlistBlock.mNumberOfPlayListMarks = XIO::ReadUns16_BE ( &mplFile );
	mplFile.ReadAll ( &playlistBlock.mFillItem2, 2);

	if ( playlistBlock.mNumberOfPlayListMarks == 0 ) return true;

	extensionDataHeader.mPresent = 1;

	XMP_Uns64 blockStart = 0;

	for ( int i = 0; i < playlistBlock.mNumberOfPlayListMarks; ++i ) {
		AVCCAM_blkProPlayListMark& currMark = playlistBlock.mPlayListMark;

		mplFile.ReadAll ( &currMark.mProTagID, 1);
		mplFile.ReadAll ( &currMark.mFillItem1, 1);
		currMark.mLength = XIO::ReadUns16_BE ( &mplFile );
		blockStart = mplFile.Offset();
		mplFile.ReadAll ( &currMark.mMarkType, 1 );

		if ( ( currMark.mProTagID == 0x40 ) && ( currMark.mMarkType == 0x01 ) ) {
			mplFile.ReadAll ( &currMark.mEntryMark.mGlobalClipID, 32);

			// skip marks for different clips
			if ( i == playlistMarkID ) {
				playlistBlock.mPresent = 1;
				currMark.mPresent = 1;
				mplFile.ReadAll ( &currMark.mEntryMark.mStartTimeCode, 4);
				mplFile.ReadAll ( &currMark.mEntryMark.mStreamTimecodeInfo, 1);
				mplFile.ReadAll ( &currMark.mEntryMark.mStartBinaryGroup, 4);
				mplFile.ReadAll ( &currMark.mEntryMark.mLastUpdateTimeZone, 1);
				mplFile.ReadAll ( &currMark.mEntryMark.mLastUpdateDate, 7);
				mplFile.ReadAll ( &currMark.mEntryMark.mFillItem, 2);

				XMP_Uns64 currPos = mplFile.Offset();
				XMP_Uns8 blockTag = 0;
				XMP_Uns8 blockFill;
				XMP_Uns16 blockLength = 0;

				while ( currPos < ( blockStart + currMark.mLength ) ) {
					mplFile.ReadAll ( &blockTag, 1);
					mplFile.ReadAll ( &blockFill, 1);
					blockLength = XIO::ReadUns16_BE ( &mplFile );
					currPos += 4;

					switch ( blockTag ) {
						case 0x20:
							if ( ! ReadAVCCAM_blkPRO_ShotMark ( mplFile, currMark ) ) return false;
							break;


						case 0x21:
							if ( ! ReadAVCCAM_blkPRO_Access ( mplFile, currMark ) ) return false;
							break;

						case 0x22:
							if ( ! ReadAVCCAM_blkPRO_Device ( mplFile, currMark ) ) return false;
							break;

						case 0x23:
							if ( ! ReadAVCCAM_blkPRO_Shoot ( mplFile, currMark ) ) return false;
							break;

						case 0x24:
							if (! ReadAVCCAM_blkPRO_Location ( mplFile, currMark ) ) return false;
							break;

						default : break;
					}

					currPos += blockLength;
					mplFile.Seek ( currPos, kXMP_SeekFromStart );
				}
			}
		}

		mplFile.Seek ( blockStart + currMark.mLength, kXMP_SeekFromStart );
	}

	return true;
}

// =================================================================================================
// ReadAVCCAMMakersPrivateData
// ===========================
//
// Read Panasonic's implementation of an AVCCAM "Maker's Private Data" block. Panasonic calls their
// extensions "AVCCAM."

static bool ReadAVCCAMMakersPrivateData ( XMPFiles_IO & fileRef,
										  XMP_Uns16 playlistMarkID,
										  AVCHD_blkPanasonicPrivateData& avccamPrivateData )
{
	const XMP_Uns64 blockStart = fileRef.Offset();

	avccamPrivateData.mNumberOfData = XIO::ReadUns16_BE ( &fileRef );
	fileRef.ReadAll ( &avccamPrivateData.mReserved, 2 );

	for (int i = 0; i < avccamPrivateData.mNumberOfData; ++i) {
		const XMP_Uns8 tagID = XIO::ReadUns8 ( &fileRef );

		switch ( tagID ) {
			case 0xe0:	ReadAVCCAMProMetaID ( fileRef, tagID, avccamPrivateData );
				break;
			case 0xe2:	ReadAVCCAMProClipInfo( fileRef, tagID, avccamPrivateData );
				break;
			case 0xf0:	ReadAVCCAMProPlaylistInfo( fileRef, tagID, playlistMarkID, avccamPrivateData );
				break;

			default:
				// Ignore any blocks we don't now or care about
				break;
		}
	}

	return true;
}

// =================================================================================================
// ReadAVCHDMakersPrivateData
// ==========================
//
// AVCHD Format. Book 2: Recording Extension Specifications, section 4.2.4.2.

static bool ReadAVCHDMakersPrivateData ( XMPFiles_IO & mplFile,
										 XMP_Uns16 playlistMarkID,
										 AVCHD_blkMakersPrivateData& avchdLegacyData )
{
	const XMP_Uns64 blockStart = mplFile.Offset();

	avchdLegacyData.mLength = XIO::ReadUns32_BE ( &mplFile );

	if ( avchdLegacyData.mLength == 0 ) return false;

	avchdLegacyData.mPresent = 1;
	avchdLegacyData.mDataBlockStartAddress = XIO::ReadUns32_BE ( &mplFile );
	mplFile.ReadAll ( &avchdLegacyData.mReserved, 3 );
	mplFile.ReadAll ( &avchdLegacyData.mNumberOfMakerEntries, 1 );

	if ( avchdLegacyData.mNumberOfMakerEntries == 0 ) return true;

	XMP_Uns16 makerID;
	XMP_Uns16 makerModelCode;
	XMP_Uns32 mpdStartAddress;
	XMP_Uns32 mpdLength;

	for ( int i = 0; i < avchdLegacyData.mNumberOfMakerEntries; ++i ) {
		makerID = XIO::ReadUns16_BE ( &mplFile );
		makerModelCode = XIO::ReadUns16_BE ( &mplFile );
		mpdStartAddress = XIO::ReadUns32_BE ( &mplFile );
		mpdLength = XIO::ReadUns32_BE ( &mplFile );

		// We only have documentation for Panasonic's Maker's Private Data blocks, so we'll ignore everyone else's
		if ( makerID == kMakerIDPanasonic ) {
			avchdLegacyData.mMakerID = makerID;
			avchdLegacyData.mMakerModelCode = makerModelCode;
			mplFile.Seek ( blockStart + mpdStartAddress, kXMP_SeekFromStart );

			if (! ReadAVCCAMMakersPrivateData ( mplFile, playlistMarkID, avchdLegacyData.mPanasonicPrivateData ) ) return false;
		}
	}

	return true;
}

// =================================================================================================
// ReadAVCHDClipExtensionData
// ==========================

static bool ReadAVCHDClipExtensionData ( XMPFiles_IO & cpiFile, AVCHD_blkClipExtensionData& avchdExtensionData )
{
	const XMP_Int64 extensionBlockStart = cpiFile.Offset();
	AVCHD_blkExtensionData extensionDataHeader;

	if ( ! ReadAVCHDExtensionData ( cpiFile, extensionDataHeader ) ) {
		 return false;
	}

	if ( extensionDataHeader.mLength == 0 ) {
		return true;
	}

	const XMP_Int64 dataBlockStart = extensionBlockStart + extensionDataHeader.mDataBlockStartAddress;

	cpiFile.Seek ( dataBlockStart, kXMP_SeekFromStart );
	cpiFile.ReadAll ( avchdExtensionData.mTypeIndicator, 4 );

	if ( strncmp ( reinterpret_cast<const char*>( avchdExtensionData.mTypeIndicator ), "CLEX", 4 ) != 0 ) return false;

	avchdExtensionData.mPresent = 1;
	cpiFile.ReadAll ( avchdExtensionData.mReserved1, 4 );
	avchdExtensionData.mProgramInfoExtStartAddress = XIO::ReadUns32_BE ( &cpiFile );
	avchdExtensionData.mMakersPrivateDataStartAddress = XIO::ReadUns32_BE ( &cpiFile );

	// read Clip info extension
	cpiFile.Seek ( dataBlockStart + 40, kXMP_SeekFromStart );
	avchdExtensionData.mClipInfoExt.mLength = XIO::ReadUns32_BE ( &cpiFile );
	avchdExtensionData.mClipInfoExt.mMakerID = XIO::ReadUns16_BE ( &cpiFile );
	avchdExtensionData.mClipInfoExt.mMakerModelCode = XIO::ReadUns16_BE ( &cpiFile );

	if ( avchdExtensionData.mMakersPrivateDataStartAddress == 0 )  return true;

	if ( avchdExtensionData.mClipInfoExt.mMakerID == kMakerIDPanasonic ) {
		// Read Maker's Private Data block -- we only have Panasonic's definition for their AVCCAM models
		// at this point, so we'll ignore the block if its from a different manufacturer.
		cpiFile.Seek ( dataBlockStart + avchdExtensionData.mMakersPrivateDataStartAddress, kXMP_SeekFromStart );

		if ( ! ReadAVCHDMakersPrivateData ( cpiFile, 0, avchdExtensionData.mMakersPrivateData ) ) {
			return false;
		}
	}

	return true;
}

// =================================================================================================
// AVCHD_PlaylistContainsClip
// ==========================
//
// Returns true of the specified AVCHD playlist block references the specified clip, or false if not.

static bool AVCHD_PlaylistContainsClip ( XMPFiles_IO & mplFile, XMP_Uns16& playItemID, const std::string& strClipName )
{
	// Read clip header. ( AVCHD Format. Book1. v. 1.01. p 45 )
	struct AVCHD_blkPlayList
	{
		XMP_Uns32	mLength;
		XMP_Uns16	mReserved;
		XMP_Uns16	mNumberOfPlayItems;
		XMP_Uns16	mNumberOfSubPaths;
	};

	AVCHD_blkPlayList blkPlayList;
	blkPlayList.mLength = XIO::ReadUns32_BE ( &mplFile );
	mplFile.ReadAll ( &blkPlayList.mReserved, 2 );
	blkPlayList.mNumberOfPlayItems = XIO::ReadUns16_BE ( &mplFile );
	blkPlayList.mNumberOfSubPaths = XIO::ReadUns16_BE ( &mplFile );

	// Search the play items. ( AVCHD Format. Book1. v. 1.01. p 47 )
	struct AVCHD_blkPlayItem
	{
		XMP_Uns16	mLength;
		char		mClipInformationFileName[5];
		// Note: remaining fields omitted because we don't care about them
	};

	AVCHD_blkPlayItem currPlayItem;
	XMP_Uns64 blockStart = 0;
	for ( playItemID = 0; playItemID < blkPlayList.mNumberOfPlayItems; ++playItemID ) {
		currPlayItem.mLength = XIO::ReadUns16_BE ( &mplFile );

		// mLength is measured from the end of mLength, not the start of the block ( AVCHD Format. Book1. v. 1.01. p 47 )
		blockStart = mplFile.Offset();
		mplFile.ReadAll ( currPlayItem.mClipInformationFileName, 5 );

		if ( strncmp ( strClipName.c_str(), currPlayItem.mClipInformationFileName, 5 ) == 0 ) return true;

		mplFile.Seek ( blockStart + currPlayItem.mLength, kXMP_SeekFromStart );
	}

	return false;
}

// =================================================================================================
// ReadAVCHDPlaylistMetadataBlock
// ==============================

static bool ReadAVCHDPlaylistMetadataBlock ( XMPFiles_IO & mplFile,
											 AVCHD_blkPlaylistMeta& avchdLegacyData )
{
	avchdLegacyData.mLength = XIO::ReadUns32_BE ( &mplFile );

	if ( avchdLegacyData.mLength < sizeof ( AVCHD_blkPlaylistMeta ) ) return false;

	avchdLegacyData.mMakerID = XIO::ReadUns16_BE ( &mplFile );
	avchdLegacyData.mMakerModelCode = XIO::ReadUns16_BE ( &mplFile );
	mplFile.ReadAll ( &avchdLegacyData.mReserved1, 4 );
	avchdLegacyData.mRefToMenuThumbnailIndex = XIO::ReadUns16_BE ( &mplFile );
	mplFile.ReadAll ( &avchdLegacyData.mBlkTimezone, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mRecordDataAndTime, 7 );
	mplFile.ReadAll ( &avchdLegacyData.mReserved2, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mPlaylistCharacterSet, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mPlaylistNameLength, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mPlaylistName, avchdLegacyData.mPlaylistNameLength );

	return true;
}

// =================================================================================================
// ReadAVCHDPlaylistMarkExtension
// ==============================

static bool ReadAVCHDPlaylistMarkExtension ( XMPFiles_IO & mplFile,
											 XMP_Uns16 playlistMarkID,
											 AVCHD_blkPlayListMarkExt& avchdLegacyData )
{
	avchdLegacyData.mLength = XIO::ReadUns32_BE ( &mplFile );

	if ( avchdLegacyData.mLength == 0 ) return false;

	avchdLegacyData.mNumberOfPlaylistMarks = XIO::ReadUns16_BE ( &mplFile );

	if ( avchdLegacyData.mNumberOfPlaylistMarks <= playlistMarkID ) return true;

	// Number of bytes in blkMarkExtension, AVCHD Book 2, section 4.3.3.1
	const XMP_Uns64 markExtensionSize = 66;

	// Entries in the mark extension block correspond one-to-one with entries in
	// blkPlaylistMark, so we'll only read the one that corresponds to the
	// chosen clip.
	const XMP_Uns64 markOffset = markExtensionSize * playlistMarkID;

	avchdLegacyData.mPresent = 1;
	mplFile.Seek ( markOffset, kXMP_SeekFromCurrent );
	avchdLegacyData.mMakerID = XIO::ReadUns16_BE ( &mplFile );
	avchdLegacyData.mMakerModelCode = XIO::ReadUns16_BE ( &mplFile );
	mplFile.ReadAll ( &avchdLegacyData.mReserved1, 3 );
	mplFile.ReadAll ( &avchdLegacyData.mFlags, 1 );
	avchdLegacyData.mRefToMarkThumbnailIndex = XIO::ReadUns16_BE ( &mplFile );
	mplFile.ReadAll ( &avchdLegacyData.mBlkTimezone, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mRecordDataAndTime, 7 );
	mplFile.ReadAll ( &avchdLegacyData.mMarkCharacterSet, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mMarkNameLength, 1 );
	mplFile.ReadAll ( &avchdLegacyData.mMarkName, 24 );
	mplFile.ReadAll ( &avchdLegacyData.mMakersInformation, 16 );
	mplFile.ReadAll ( &avchdLegacyData.mBlkTimecode, 4 );
	mplFile.ReadAll ( &avchdLegacyData.mReserved2, 2 );

	return true;
}

// =================================================================================================
// ReadAVCHDPlaylistMarkID
// =======================
//
// Read the playlist mark block to find the ID of the playlist mark that matches the specified
// playlist item.

static bool ReadAVCHDPlaylistMarkID ( XMPFiles_IO & mplFile,
									  XMP_Uns16 playItemID,
									  XMP_Uns16& markID )
{
	XMP_Uns32 length = XIO::ReadUns32_BE ( &mplFile );
	XMP_Uns16 numberOfPlayListMarks = XIO::ReadUns16_BE ( &mplFile );

	if ( length == 0 ) return false;

	XMP_Uns8 reserved;
	XMP_Uns8 markType;
	XMP_Uns16 refToPlayItemID;

	for ( int i = 0; i < numberOfPlayListMarks; ++i ) {
		mplFile.ReadAll ( &reserved, 1 );
		mplFile.ReadAll ( &markType, 1 );
		refToPlayItemID = XIO::ReadUns16_BE ( &mplFile );

		if ( ( markType == 0x01 ) && ( refToPlayItemID == playItemID ) ) {
			markID = i;
			return true;
		}

		mplFile.Seek ( 10, kXMP_SeekFromCurrent );
	}

	return false;
}

// =================================================================================================
// ReadAVCHDPlaylistExtensionData
// ==============================

static bool ReadAVCHDPlaylistExtensionData ( XMPFiles_IO & mplFile,
											 AVCHD_LegacyMetadata& avchdLegacyData,
											 XMP_Uns16 playlistMarkID )
{
	const XMP_Int64 extensionBlockStart = mplFile.Offset();
	AVCHD_blkExtensionData extensionDataHeader;

	if ( ! ReadAVCHDExtensionData ( mplFile, extensionDataHeader ) ) {
		 return false;
	}

	if ( extensionDataHeader.mLength == 0 ) {
		return true;
	}

	const XMP_Int64 dataBlockStart = extensionBlockStart + extensionDataHeader.mDataBlockStartAddress;
	AVCHD_blkPlayListExtensionData& extensionData = avchdLegacyData.mPlaylistExtensionData;
	const int reserved2Len = 24;

	mplFile.Seek ( dataBlockStart, kXMP_SeekFromStart );
	mplFile.ReadAll ( extensionData.mTypeIndicator, 4 );

	if ( strncmp ( extensionData.mTypeIndicator, "PLEX", 4 ) != 0 ) return false;

	extensionData.mPresent = true;
	mplFile.ReadAll ( extensionData.mReserved, 4 );
	extensionData.mPlayListMarkExtStartAddress = XIO::ReadUns32_BE ( &mplFile );
	extensionData.mMakersPrivateDataStartAddress = XIO::ReadUns32_BE ( &mplFile );
	mplFile.Seek ( reserved2Len, kXMP_SeekFromCurrent );

	if ( ! ReadAVCHDPlaylistMetadataBlock ( mplFile, extensionData.mPlaylistMeta ) ) return false;

	mplFile.Seek ( dataBlockStart + extensionData.mPlayListMarkExtStartAddress, kXMP_SeekFromStart );

	if ( ! ReadAVCHDPlaylistMarkExtension ( mplFile, playlistMarkID, extensionData.mPlaylistMarkExt ) ) return false;

	if ( extensionData.mMakersPrivateDataStartAddress > 0 ) {

		// return true here because all the data is already read successfully except the maker's private data and more
		// specifically of panasonic. So if the relevant panasonic data is not present we just skip it.
		// Assumption here is that if its not present in ClipExtension then it will not be in Playlist extension
		if ( ! avchdLegacyData.mClipExtensionData.mMakersPrivateData.mPanasonicPrivateData.mPresent ) return true;

		mplFile.Seek ( dataBlockStart + extensionData.mMakersPrivateDataStartAddress, kXMP_SeekFromStart );
		
		// Here private data was found.If the data was panasonic private data and we were unable to read it ,
		// Return false 
		if ( ! ReadAVCHDMakersPrivateData ( mplFile, playlistMarkID, extensionData.mMakersPrivateData ) ) return false;

	}

	return true;
}

// =================================================================================================
// ReadAVCHDLegacyClipFile
// =======================
//
// Read the legacy metadata stored in an AVCHD .CPI file.

static bool ReadAVCHDLegacyClipFile ( const std::string& strPath, AVCHD_LegacyMetadata& avchdLegacyData )
{
	bool success = false;

	try {

		Host_IO::FileRef hostRef = Host_IO::Open ( strPath.c_str(), Host_IO::openReadOnly );
		if ( hostRef == Host_IO::noFileRef ) return false;	// The open failed.
		XMPFiles_IO cpiFile ( hostRef, strPath.c_str(), Host_IO::openReadOnly );

		memset ( &avchdLegacyData, 0, sizeof(AVCHD_LegacyMetadata) );

		// Read clip header. ( AVCHD Format. Book1. v. 1.01. p 64 )
		struct AVCHD_ClipInfoHeader
		{
			char		mTypeIndicator[4];
			char		mTypeIndicator2[4];
			XMP_Uns32	mSequenceInfoStartAddress;
			XMP_Uns32	mProgramInfoStartAddress;
			XMP_Uns32	mCPIStartAddress;
			XMP_Uns32	mClipMarkStartAddress;
			XMP_Uns32	mExtensionDataStartAddress;
			XMP_Uns8	mReserved[12];
		};

		// Read the AVCHD header.
		AVCHD_ClipInfoHeader avchdHeader;
		cpiFile.ReadAll ( avchdHeader.mTypeIndicator,  4 );
		cpiFile.ReadAll ( avchdHeader.mTypeIndicator2, 4 );

		if ( strncmp ( avchdHeader.mTypeIndicator, "HDMV", 4 ) != 0 ) return false;
		if ( strncmp ( avchdHeader.mTypeIndicator2, "0100", 4 ) != 0 ) return false;

		avchdHeader.mSequenceInfoStartAddress	= XIO::ReadUns32_BE ( &cpiFile );
		avchdHeader.mProgramInfoStartAddress	= XIO::ReadUns32_BE ( &cpiFile );
		avchdHeader.mCPIStartAddress			= XIO::ReadUns32_BE ( &cpiFile );
		avchdHeader.mClipMarkStartAddress		= XIO::ReadUns32_BE ( &cpiFile );
		avchdHeader.mExtensionDataStartAddress	= XIO::ReadUns32_BE ( &cpiFile );
		cpiFile.ReadAll ( avchdHeader.mReserved, 12 );

		// Seek to the program header. (AVCHD Format. Book1. v. 1.01. p 77 )
		cpiFile.Seek ( avchdHeader.mProgramInfoStartAddress, kXMP_SeekFromStart );

		// Read the program info block
		success = ReadAVCHDProgramInfo ( cpiFile, avchdLegacyData.mProgramInfo );

		if ( success && ( avchdHeader.mExtensionDataStartAddress != 0 ) ) {
			// Seek to the program header. (AVCHD Format. Book1. v. 1.01. p 77 )
			cpiFile.Seek ( avchdHeader.mExtensionDataStartAddress, kXMP_SeekFromStart );
			success = ReadAVCHDClipExtensionData ( cpiFile, avchdLegacyData.mClipExtensionData );
		}

	} catch ( ... ) {

		return false;

	}

	return success;

}	// ReadAVCHDLegacyClipFile

// =================================================================================================
// ReadAVCHDLegacyPlaylistFile
// ===========================
//
// Read the legacy metadata stored in an AVCHD .MPL file.

static bool ReadAVCHDLegacyPlaylistFile ( const std::string& mplPath,
										  const std::string& strClipName,
										  AVCHD_LegacyMetadata& avchdLegacyData )
{

#if 1
	bool success = false;

	try {

		Host_IO::FileRef hostRef = Host_IO::Open ( mplPath.c_str(), Host_IO::openReadOnly );
		if ( hostRef == Host_IO::noFileRef ) return false;	// The open failed.
		XMPFiles_IO mplFile ( hostRef, mplPath.c_str(), Host_IO::openReadOnly );

		// Read playlist header. ( AVCHD Format. Book1. v. 1.01. p 43 )
		struct AVCHD_PlaylistFileHeader {
			char		mTypeIndicator[4];
			char		mTypeIndicator2[4];
			XMP_Uns32	mPlaylistStartAddress;
			XMP_Uns32	mPlaylistMarkStartAddress;
			XMP_Uns32	mExtensionDataStartAddress;
		};

		// Read the AVCHD playlist file header.
		AVCHD_PlaylistFileHeader avchdHeader;
		mplFile.ReadAll ( avchdHeader.mTypeIndicator,  4 );
		mplFile.ReadAll ( avchdHeader.mTypeIndicator2,  4 );

		if ( strncmp ( avchdHeader.mTypeIndicator, "MPLS", 4 ) != 0 ) return false;
		if ( strncmp ( avchdHeader.mTypeIndicator2, "0100", 4 ) != 0 ) return false;

		avchdHeader.mPlaylistStartAddress		= XIO::ReadUns32_BE ( &mplFile );
		avchdHeader.mPlaylistMarkStartAddress	= XIO::ReadUns32_BE ( &mplFile );
		avchdHeader.mExtensionDataStartAddress	= XIO::ReadUns32_BE ( &mplFile );

		if ( avchdHeader.mExtensionDataStartAddress == 0 ) return false;

		// Seek to the start of the Playlist block. (AVCHD Format. Book1. v. 1.01. p 45 )
		mplFile.Seek ( avchdHeader.mPlaylistStartAddress, kXMP_SeekFromStart );

		XMP_Uns16 playItemID = 0xFFFF;
		XMP_Uns16 playlistMarkID = 0xFFFF;

		if ( AVCHD_PlaylistContainsClip ( mplFile, playItemID, strClipName ) ) {
			mplFile.Seek ( avchdHeader.mPlaylistMarkStartAddress, kXMP_SeekFromStart );
			if ( ! ReadAVCHDPlaylistMarkID ( mplFile, playItemID, playlistMarkID ) ) return false;
			mplFile.Seek ( avchdHeader.mExtensionDataStartAddress, kXMP_SeekFromStart );
			success = ReadAVCHDPlaylistExtensionData ( mplFile, avchdLegacyData, playlistMarkID );
		}

	} catch ( ... ) {

		success = false;

	}

	return success;

#else

	bool success = false;
	std::string mplPath;
	char playlistName [10];
	const int rootPlaylistNum = atoi(strClipName.c_str());

	// Find the corresponding .MPL file -- because of clip spanning the .MPL name may not match the .CPI name for
	// a given clip -- we need to open .MPL files and look for one that contains a reference to the clip name. To speed
	// up the search we'll start with the playlist with the same number/name as the clip and search backwards. Assuming
	// this directory was generated by a camera, the clip numbers will increase sequentially across the playlist files,
	// though one playlist file may reference more than one clip.
	for ( int i = rootPlaylistNum; i >= 0; --i ) {

		sprintf ( playlistName, "%05d", i );

		if ( MakeLeafPath ( &mplPath, strRootPath.c_str(), "PLAYLIST", playlistName, ".mpl", true /* checkFile */ ) ) {

			try {

				Host_IO::FileRef hostRef = Host_IO::Open ( mplPath.c_str(), Host_IO::openReadOnly );
				if ( hostRef == Host_IO::noFileRef ) return false;	// The open failed.
				XMPFiles_IO mplFile ( hostRef, mplPath.c_str(), Host_IO::openReadOnly );

				// Read playlist header. ( AVCHD Format. Book1. v. 1.01. p 43 )
				struct AVCHD_PlaylistFileHeader
				{
					char		mTypeIndicator[4];
					char		mTypeIndicator2[4];
					XMP_Uns32	mPlaylistStartAddress;
					XMP_Uns32	mPlaylistMarkStartAddress;
					XMP_Uns32	mExtensionDataStartAddress;
				};

				// Read the AVCHD playlist file header.
				AVCHD_PlaylistFileHeader avchdHeader;
				mplFile.ReadAll ( avchdHeader.mTypeIndicator,  4 );
				mplFile.ReadAll ( avchdHeader.mTypeIndicator2, 4 );

				if ( strncmp ( avchdHeader.mTypeIndicator, "MPLS", 4 ) != 0 ) return false;
				if ( strncmp ( avchdHeader.mTypeIndicator2, "0100", 4 ) != 0 ) return false;

				avchdHeader.mPlaylistStartAddress		= XIO::ReadUns32_BE ( &mplFile );
				avchdHeader.mPlaylistMarkStartAddress	= XIO::ReadUns32_BE ( &mplFile );
				avchdHeader.mExtensionDataStartAddress	= XIO::ReadUns32_BE ( &mplFile );

				if ( avchdHeader.mExtensionDataStartAddress == 0 ) return false;

				// Seek to the start of the Playlist block. (AVCHD Format. Book1. v. 1.01. p 45 )
				mplFile.Seek ( avchdHeader.mPlaylistStartAddress, kXMP_SeekFromStart );

				XMP_Uns16 playItemID = 0xFFFF;
				XMP_Uns16 playlistMarkID = 0xFFFF;

				if ( AVCHD_PlaylistContainsClip ( mplFile, playItemID, strClipName ) ) {
					mplFile.Seek ( avchdHeader.mPlaylistMarkStartAddress, kXMP_SeekFromStart );

					if ( ! ReadAVCHDPlaylistMarkID ( mplFile, playItemID, playlistMarkID ) ) return false;

					mplFile.Seek ( avchdHeader.mExtensionDataStartAddress, kXMP_SeekFromStart );
					success = ReadAVCHDPlaylistExtensionData ( mplFile, avchdLegacyData, playlistMarkID );
				}

			} catch ( ... ) {

				return false;

			}
		}

	}

	return success;

#endif

}	// ReadAVCHDLegacyPlaylistFile

// =================================================================================================
// FindAVCHDLegacyPlaylistFile
// ===========================
//
// Find and read the legacy metadata stored in an AVCHD .MPL file.

static bool FindAVCHDLegacyPlaylistFile ( const std::string& strRootPath,
										  const std::string& strClipName,
										  AVCHD_LegacyMetadata& avchdLegacyData,
										  std::string &mplPath )
{
	bool success = false;

	// Find the corresponding .MPL file -- because of clip spanning the .MPL name may not match the
	// .CPI name for a given clip -- we need to open .MPL files and look for one that contains a
	// reference to the clip name. To speed up the search we'll start with the playlist with the
	// same number/name as the clip, and if that fails look into other playlist files in the
	// directory. One playlist file may reference more than one clip.

	if ( MakeLeafPath ( &mplPath, strRootPath.c_str(), "PLAYLIST", strClipName.c_str(), ".mpl", true /* checkFile */ ) ) {
		success = ReadAVCHDLegacyPlaylistFile ( mplPath, strClipName, avchdLegacyData );
	}

	if ( ! success ) {

		std::string playlistPath = strRootPath;
		playlistPath += kDirChar;
		playlistPath += "BDMV";
		playlistPath += kDirChar;
		playlistPath += "PLAYLIST";
		playlistPath += kDirChar;

		std::string childName;

		if ( Host_IO::GetFileMode ( playlistPath.c_str() ) == Host_IO::kFMode_IsFolder ) {

			Host_IO::AutoFolder af;
			af.folder = Host_IO::OpenFolder ( playlistPath.c_str() );
			if ( af.folder == Host_IO::noFolderRef ) return false;

			while ( (! success) && Host_IO::GetNextChild ( af.folder, &childName ) &&
					(childName.find(".mpl") || childName.find(".MPL")) ) {
				mplPath = playlistPath + childName;
				if ( Host_IO::GetFileMode ( mplPath.c_str() ) == Host_IO::kFMode_IsFile ) {
					success = ReadAVCHDLegacyPlaylistFile ( mplPath, strClipName, avchdLegacyData );
				}

			}
			
			af.Close();

		}

	}

	return success;

}	// FindAVCHDLegacyPlaylistFile

// =================================================================================================
// ReadAVCHDLegacyMetadata
// =======================
//
// Read the legacy metadata stored in an AVCHD .CPI file.

static bool ReadAVCHDLegacyMetadata ( const std::string& strPath,
									  const std::string& strRootPath,
									  const std::string& strClipName,
									  AVCHD_LegacyMetadata& avchdLegacyData,
									  std::string&	mplFile)
{
	bool success = ReadAVCHDLegacyClipFile ( strPath, avchdLegacyData );

	if ( success && avchdLegacyData.mClipExtensionData.mPresent ) {
		success = FindAVCHDLegacyPlaylistFile ( strRootPath, strClipName, avchdLegacyData, mplFile );
	}

	return success;

}	// ReadAVCHDLegacyMetadata

// =================================================================================================
// AVCCAM_SetXMPStartTimecode
// ==========================

static void AVCCAM_SetXMPStartTimecode ( SXMPMeta& xmpObj, const XMP_Uns8* avccamTimecode, XMP_Uns8 avchdFrameRate )
{
	//	Timecode in SMPTE 12M format, according to Panasonic's documentation
	if ( *reinterpret_cast<const XMP_Uns32*>( avccamTimecode ) == 0xFFFFFFFF ) {
		// 0xFFFFFFFF means timecode not specified
		return;
	}

	const XMP_Uns8 isColor = ( avccamTimecode[0] >> 7 ) & 0x01;
	const XMP_Uns8 isDropFrame = ( avccamTimecode[0] >> 6 ) & 0x01;
	const XMP_Uns8 frameTens = ( avccamTimecode[0] >> 4 ) & 0x03;
	const XMP_Uns8 frameUnits = avccamTimecode[0] & 0x0f;
	const XMP_Uns8 secondTens = ( avccamTimecode[1] >> 4 ) & 0x07;
	const XMP_Uns8 secondUnits = avccamTimecode[1] & 0x0f;
	const XMP_Uns8 minuteTens = ( avccamTimecode[2] >> 4 ) & 0x07;
	const XMP_Uns8 minuteUnits = avccamTimecode[2] & 0x0f;
	const XMP_Uns8 hourTens = ( avccamTimecode[3] >> 4 ) & 0x03;
	const XMP_Uns8 hourUnits = avccamTimecode[3] & 0x0f;
	char tcSeparator = ':';
	const char* dmTimeFormat = NULL;
	const char* dmTimeScale = NULL;
	const char* dmTimeSampleSize = NULL;

	switch ( avchdFrameRate ) {
		case 1 :
			// 23.976i
			dmTimeFormat = "23976Timecode";
			dmTimeScale = "24000";
			dmTimeSampleSize = "1001";
			break;

		case 2 :
			// 24p
			dmTimeFormat = "24Timecode";
			dmTimeScale = "24";
			dmTimeSampleSize = "1";
			break;

		case 3 :
		case 6 :
			// 50i or 25p
			dmTimeFormat = "25Timecode";
			dmTimeScale = "25";
			dmTimeSampleSize = "1";
			break;

		case 4 :
		case 7 :
			// 29.97p or 59.94i
			if ( isDropFrame ) {
				dmTimeFormat = "2997DropTimecode";
				tcSeparator = ';';
			} else {
				dmTimeFormat = "2997NonDropTimecode";
			}

			dmTimeScale = "30000";
			dmTimeSampleSize = "1001";

			break;
	}

	if ( dmTimeFormat != NULL ) {
		char timecodeBuff [12];

		sprintf ( timecodeBuff, "%d%d%c%d%d%c%d%d%c%d%d", hourTens, hourUnits, tcSeparator,
			minuteTens, minuteUnits, tcSeparator, secondTens, secondUnits, tcSeparator, frameTens, frameUnits);

		xmpObj.SetProperty( kXMP_NS_DM, "startTimeScale", dmTimeScale, kXMP_DeleteExisting );
		xmpObj.SetProperty( kXMP_NS_DM, "startTimeSampleSize", dmTimeSampleSize, kXMP_DeleteExisting );
		xmpObj.SetStructField ( kXMP_NS_DM, "startTimecode", kXMP_NS_DM, "timeValue", timecodeBuff, 0 );
		xmpObj.SetStructField ( kXMP_NS_DM, "startTimecode", kXMP_NS_DM, "timeFormat", dmTimeFormat, 0 );
	}
}

// =================================================================================================
// AVCHD_SetXMPMakeAndModel
// ========================

static bool AVCHD_SetXMPMakeAndModel ( SXMPMeta& xmpObj, const AVCHD_blkClipExtensionData& clipExtData )
{
	if ( ! clipExtData.mPresent ) return false;

	XMP_StringPtr xmpValue = 0;

	// Set the Make. Use a hex string for unknown makes.
	{
		char hexMakeNumber [7];

		switch ( clipExtData.mClipInfoExt.mMakerID ) {
			case kMakerIDCanon : xmpValue = "Canon";			break;
			case kMakerIDPanasonic : xmpValue = "Panasonic";	break;
			case kMakerIDSony : xmpValue = "Sony";				break;
			default :
				std::sprintf ( hexMakeNumber, "0x%04x", clipExtData.mClipInfoExt.mMakerID );
				xmpValue = hexMakeNumber;

				break;
		}

		xmpObj.SetProperty ( kXMP_NS_TIFF, "Make", xmpValue, kXMP_DeleteExisting );
	}

	// Set the Model number. Use a hex string for unknown model numbers so they can still be distinguished.
	{
		char hexModelNumber [7];

		xmpValue = 0;

		switch ( clipExtData.mClipInfoExt.mMakerID  ) {
			case kMakerIDCanon :
				switch ( clipExtData.mClipInfoExt.mMakerModelCode ) {
					case 0x1000 : xmpValue = "HR10";		break;
					case 0x2000 : xmpValue = "HG10";		break;
					case 0x2001 : xmpValue = "HG21";		break;
					case 0x3000 : xmpValue = "HF100";		break;
					case 0x3003 : xmpValue = "HF S10";		break;
					default :								break;
				}
				break;

			case kMakerIDPanasonic :
				switch ( clipExtData.mClipInfoExt.mMakerModelCode ) {
					case 0x0202 : xmpValue = "HD-writer";				break;
					case 0x0400 : xmpValue = "AG-HSC1U";				break;
					case 0x0401 : xmpValue = "AG-HMC70";				break;
					case 0x0410 : xmpValue = "AG-HMC150";				break;
					case 0x0411 : xmpValue = "AG-HMC40";				break;
					case 0x0412 : xmpValue = "AG-HMC80";				break;
					case 0x0413 : xmpValue = "AG-3DA1";					break;
					case 0x0414 : xmpValue = "AG-AF100";				break;
					case 0x0450 : xmpValue = "AG-HMR10";				break;
					case 0x0451 : xmpValue = "AJ-YCX250";				break;
					case 0x0452 : xmpValue = "AG-MDR15";				break;
					case 0x0490 : xmpValue = "AVCCAM Restorer";			break;
					case 0x0491 : xmpValue = "AVCCAM Viewer";			break;
					case 0x0492 : xmpValue = "AVCCAM Viewer for Mac";	break;
					default :											break;
				}

			break;

			default : break;
		}

		if ( ( xmpValue == 0 ) && ( clipExtData.mClipInfoExt.mMakerID != kMakerIDSony ) ) {
			// Panasonic has said that if we don't have a string for the model number, they'd like to see the code
			// anyway. We'll do the same for every manufacturer except Sony, who have said that they use
			// the same model number for multiple cameras.
			std::sprintf ( hexModelNumber, "0x%04x", clipExtData.mClipInfoExt.mMakerModelCode );
			xmpValue = hexModelNumber;
		}

		if ( xmpValue != 0 ) xmpObj.SetProperty ( kXMP_NS_TIFF, "Model", xmpValue, kXMP_DeleteExisting );
	}

	return true;
}

// =================================================================================================
// AVCHD_StringFieldToXMP
// ======================

static std::string AVCHD_StringFieldToXMP ( XMP_Uns8 avchdLength,
											XMP_Uns8 avchdCharacterSet,
											const XMP_Uns8* avchdField,
											XMP_Uns8 avchdFieldSize )
{
	std::string xmpString;

	if ( avchdCharacterSet == 0x02 ) {
		// UTF-16, Big Endian
		UTF8Unit utf8Name [512];
		const XMP_Uns8 avchdMaxChars = ( avchdFieldSize / 2);
		size_t utf16Read;
		size_t utf8Written;

		// The spec doesn't say whether AVCHD length fields count bytes or characters, so we'll
		// clamp to the max number of UTF-16 characters just in case.
		const int stringLength = ( avchdLength > avchdMaxChars ) ? avchdMaxChars : avchdLength;

		UTF16BE_to_UTF8 ( reinterpret_cast<const UTF16Unit*> ( avchdField ), stringLength,
						  utf8Name, 512, &utf16Read, &utf8Written );
		xmpString.assign ( reinterpret_cast<const char*> ( utf8Name ), utf8Written );
	} else {
		// AVCHD supports many character encodings, but UTF-8 (0x01) and ASCII (0x90) are the only ones I've
		// seen in the wild at this point. We'll treat the other character sets as UTF-8 on the assumption that
		// at least a few characters will come across, and something is better than nothing.
		const int stringLength = ( avchdLength > avchdFieldSize ) ? avchdFieldSize : avchdLength;

		xmpString.assign ( reinterpret_cast<const char*> ( avchdField ), stringLength );
	}

	return xmpString;
}

// =================================================================================================
// AVCHD_SetXMPShotName
// ====================

static void AVCHD_SetXMPShotName ( SXMPMeta& xmpObj, const AVCHD_blkPlayListMarkExt& markExt, const std::string& strClipName )
{
	if ( markExt.mPresent ) {
		const std::string shotName = AVCHD_StringFieldToXMP ( markExt.mMarkNameLength, markExt.mMarkCharacterSet, markExt.mMarkName, 24 );

		if ( ! shotName.empty() ) xmpObj.SetProperty ( kXMP_NS_DC, "shotName", shotName.c_str(), kXMP_DeleteExisting );
	}
}

// =================================================================================================
// BytesToHex
// ==========

#define kHexDigits "0123456789ABCDEF"

static std::string BytesToHex ( const XMP_Uns8* inClipIDBytes, int inNumBytes )
{
	const int numChars = ( inNumBytes * 2 );
	std::string hexStr;

	hexStr.reserve(numChars);

	for ( int i = 0; i < inNumBytes; ++i ) {
		const XMP_Uns8 byte = inClipIDBytes[i];
		hexStr.push_back ( kHexDigits [byte >> 4] );
		hexStr.push_back ( kHexDigits [byte & 0xF] );
	}

	return hexStr;
}

// =================================================================================================
// AVCHD_DateFieldToXMP
// ====================
//
// AVCHD Format Book 2, section 4.2.2.2.

static std::string AVCHD_DateFieldToXMP ( XMP_Uns8 avchdTimeZone, const XMP_Uns8* avchdDateTime )
{
	const XMP_Uns8 daylightSavingsTime = ( avchdTimeZone >> 6 ) & 0x01;
	const XMP_Uns8 timezoneSign = ( avchdTimeZone >> 5 ) & 0x01;
	const XMP_Uns8 timezoneValue = ( avchdTimeZone >> 1 ) & 0x0F;
	const XMP_Uns8 halfHourFlag = avchdTimeZone & 0x01;
	int utcOffsetHours = 0;
	unsigned int utcOffsetMinutes = 0;

	// It's not entirely clear how to interpret the daylightSavingsTime flag from the documentation -- my best
	// guess is that it should only be used if trying to display local time, not the UTC-relative time that
	// XMP specifies.
	if ( timezoneValue != 0xF ) {
		utcOffsetHours = timezoneSign ? -timezoneValue : timezoneValue;
		utcOffsetMinutes = 30 * halfHourFlag;
	}

	char dateBuff [26];

	sprintf ( dateBuff,
			  "%01d%01d%01d%01d-%01d%01d-%01d%01dT%01d%01d:%01d%01d:%01d%01d%+02d:%02d",
			  (avchdDateTime[0] >> 4), (avchdDateTime[0] & 0x0F),
			  (avchdDateTime[1] >> 4), (avchdDateTime[1] & 0x0F),
			  (avchdDateTime[2] >> 4), (avchdDateTime[2] & 0x0F),
			  (avchdDateTime[3] >> 4), (avchdDateTime[3] & 0x0F),
			  (avchdDateTime[4] >> 4), (avchdDateTime[4] & 0x0F),
			  (avchdDateTime[5] >> 4), (avchdDateTime[5] & 0x0F),
			  (avchdDateTime[6] >> 4), (avchdDateTime[6] & 0x0F),
			  utcOffsetHours, utcOffsetMinutes );

	return std::string(dateBuff);
}

// =================================================================================================
// AVCHD_MetaHandlerCTor
// =====================

XMPFileHandler * AVCHD_MetaHandlerCTor ( XMPFiles * parent )
{
	return new AVCHD_MetaHandler ( parent );

}	// AVCHD_MetaHandlerCTor

// =================================================================================================
// AVCHD_MetaHandler::AVCHD_MetaHandler
// ====================================

AVCHD_MetaHandler::AVCHD_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kAVCHD_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	// Extract the root path and clip name.

	if ( this->parent->tempPtr == 0 ) {
		// The CheckFormat call might have been skipped.
		this->parent->tempPtr = CreatePseudoClipPath ( this->parent->GetFilePath() );
	}

	this->rootPath.assign ( (char*) this->parent->tempPtr );
	free ( this->parent->tempPtr );
	this->parent->tempPtr = 0;

	XIO::SplitLeafName ( &this->rootPath, &this->clipName );

}	// AVCHD_MetaHandler::AVCHD_MetaHandler

// =================================================================================================
// AVCHD_MetaHandler::~AVCHD_MetaHandler
// =====================================

AVCHD_MetaHandler::~AVCHD_MetaHandler()
{

	if ( this->parent->tempPtr != 0 ) {
		free ( this->parent->tempPtr );
		this->parent->tempPtr = 0;
	}

}	// AVCHD_MetaHandler::~AVCHD_MetaHandler

// =================================================================================================
// AVCHD_MetaHandler::MakeClipInfoPath
// ===================================

bool AVCHD_MetaHandler::MakeClipInfoPath ( std::string * path, XMP_StringPtr suffix, bool checkFile /* = false */ ) const
{
	return MakeLeafPath ( path, this->rootPath.c_str(), "CLIPINF", this->clipName.c_str(), suffix, checkFile );
}	// AVCHD_MetaHandler::MakeClipInfoPath

// =================================================================================================
// AVCHD_MetaHandler::MakeClipStreamPath
// =====================================

bool AVCHD_MetaHandler::MakeClipStreamPath ( std::string * path, XMP_StringPtr suffix, bool checkFile /* = false */ ) const
{
	return MakeLeafPath ( path, this->rootPath.c_str(), "STREAM", this->clipName.c_str(), suffix, checkFile );
}	// AVCHD_MetaHandler::MakeClipStreamPath

// =================================================================================================
// AVCHD_MetaHandler::MakePlaylistPath
// =====================================

bool AVCHD_MetaHandler::MakePlaylistPath ( std::string * path, XMP_StringPtr suffix, bool checkFile /* = false */ ) const
{
	return MakeLeafPath ( path, this->rootPath.c_str(), "PLAYLIST", this->clipName.c_str(), suffix, checkFile );
}	// AVCHD_MetaHandler::MakePlaylistPath

// =================================================================================================
// AVCHD_MetaHandler::MakeLegacyDigest
// ===================================

void AVCHD_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
{
	std::string strClipPath;
	std::string strPlaylistPath;
	std::vector<XMP_Uns8> legacyBuff;

	bool ok = this->MakeClipInfoPath ( &strClipPath, ".clpi", true /* checkFile */ );
	if ( ! ok ) return;

	ok = this->MakePlaylistPath ( &strPlaylistPath, ".mpls", true /* checkFile */ );
	if ( ! ok ) return;

	try {
		{
			Host_IO::FileRef hostRef = Host_IO::Open ( strClipPath.c_str(), Host_IO::openReadOnly );
			if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
			XMPFiles_IO cpiFile ( hostRef, strClipPath.c_str(), Host_IO::openReadOnly );

			// Read at most the first 2k of data from the cpi file to use in the digest
			// (every CPI file I've seen is less than 1k).
			const XMP_Int64 cpiLen = cpiFile.Length();
			const XMP_Int64 buffLen = (cpiLen <= 2048) ? cpiLen : 2048;

			legacyBuff.resize ( (unsigned int) buffLen );
			cpiFile.ReadAll ( &(legacyBuff[0]),  static_cast<XMP_Int32> ( buffLen ) );
		}

		{
			Host_IO::FileRef hostRef = Host_IO::Open ( strPlaylistPath.c_str(), Host_IO::openReadOnly );
			if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
			XMPFiles_IO mplFile ( hostRef, strPlaylistPath.c_str(), Host_IO::openReadOnly );

			// Read at most the first 2k of data from the cpi file to use in the digest
			// (every playlist file I've seen is less than 1k).
			const XMP_Int64 mplLen = mplFile.Length();
			const XMP_Int64 buffLen = (mplLen <= 2048) ? mplLen : 2048;
			const XMP_Int64 clipBuffLen = legacyBuff.size();

			legacyBuff.resize ( (unsigned int) (clipBuffLen + buffLen) );
			mplFile.ReadAll ( &( legacyBuff [(unsigned int)clipBuffLen] ),  (XMP_Int32)buffLen );
		}

	} catch (...) {
		return;
	}

	MD5_CTX context;
	unsigned char digestBin [16];

	MD5Init ( &context );
	MD5Update ( &context, (XMP_Uns8*)&(legacyBuff[0]), (unsigned int) legacyBuff.size() );
	MD5Final ( digestBin, &context );

	*digestStr = BytesToHex ( digestBin, 16 );
}	// AVCHD_MetaHandler::MakeLegacyDigest

// =================================================================================================
// AVCHD_MetaHandler::GetFileModDate
// =================================

static inline bool operator< ( const XMP_DateTime & left, const XMP_DateTime & right ) {
	int compare = SXMPUtils::CompareDateTime ( left, right );
	return (compare < 0);
}

bool AVCHD_MetaHandler::GetFileModDate ( XMP_DateTime * modDate )
{

	// The AVCHD locations of metadata:
	//	BDMV/
	//		CLIPINF/
	//			00001.clpi
	//		PLAYLIST/
	//			00001.mpls
	//		STREAM/
	//			00001.xmp

	bool ok, haveDate = false;
	std::string fullPath;
	XMP_DateTime oneDate, junkDate;
	if ( modDate == 0 ) modDate = &junkDate;

	ok = this->MakeClipInfoPath ( &fullPath, ".clpi", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	ok = this->MakePlaylistPath ( &fullPath, ".mpls", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	ok = this->MakeClipStreamPath ( &fullPath, ".xmp", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	return haveDate;

}	// AVCHD_MetaHandler::GetFileModDate

// =================================================================================================
// AVCHD_MetaHandler::FillMetadataFiles
// ================================
void AVCHD_MetaHandler::FillMetadataFiles ( std::vector<std::string> * metadataFiles)
{
	std::string noExtPath, filePath, altPath;

	noExtPath = rootPath + kDirChar + "BDMV" + kDirChar + "STREAM" + kDirChar + clipName;
	filePath  = noExtPath + ".xmp";
	if ( ! Host_IO::Exists ( filePath.c_str() ) ) {
		altPath = noExtPath + ".XMP";
		if ( Host_IO::Exists ( altPath.c_str() ) ) filePath = altPath;
	}
	metadataFiles->push_back ( filePath );

	noExtPath = rootPath + kDirChar + "BDMV" + kDirChar + "CLIPINF" + kDirChar + clipName;
	filePath  = noExtPath + ".clpi";
	if ( ! Host_IO::Exists ( filePath.c_str() ) ) {
		altPath = noExtPath + ".CLPI";
		if ( ! Host_IO::Exists ( altPath.c_str() ) ) altPath = noExtPath + ".cpi";
		if ( ! Host_IO::Exists ( altPath.c_str() ) ) altPath = noExtPath + ".CPI";
		if ( Host_IO::Exists ( altPath.c_str() ) ) filePath = altPath;
	}
	metadataFiles->push_back ( filePath );

}	// 	FillMetadataFiles_AVCHD

// =================================================================================================
// AVCHD_MetaHandler::IsMetadataWritable
// =======================================

bool AVCHD_MetaHandler::IsMetadataWritable ( )
{
	std::vector<std::string> metadataFiles;
	FillMetadataFiles(&metadataFiles);
	std::vector<std::string>::iterator itr = metadataFiles.begin();
	// Check whether sidecar is writable, if not then check if it can be created.
	return Host_IO::Writable( itr->c_str(), true );
}// AVCHD_MetaHandler::IsMetadataWritable

// =================================================================================================
// AVCHD_MetaHandler::FillAssociatedResources
// ======================================
void AVCHD_MetaHandler::FillAssociatedResources ( std::vector<std::string> * resourceList )
{
	/// The possible associated resources:
	///   BDMV/
	///         index.bdmv
	///         MovieObject.bdmv
	///         PLAYLIST/
	///				xxxxx.mpls   
	///         STREAM/
	///				zzzzz.m2ts
	///             zzzzz.xmp
	///         CLIPINF/
	///				zzzzz.clpi
	///         BACKUP/
	//    xxxxx is a five digit playlist name
	//    zzzzz is a five digit clip name
	//
	std::string bdmvPath = rootPath + kDirChar + "BDMV" + kDirChar;
	std::string filePath, clipInfoPath;
	//Add RootPath
	filePath = rootPath + kDirChar;
	PackageFormat_Support::AddResourceIfExists( resourceList, filePath );
	// Add existing files under the folder "BDMV"
	filePath = bdmvPath + "index.bdmv";
	if ( ! PackageFormat_Support::AddResourceIfExists ( resourceList, filePath ) ) {
		filePath = bdmvPath + "INDEX.BDMV";
		if ( ! PackageFormat_Support::AddResourceIfExists ( resourceList, filePath ) ) {
			filePath = bdmvPath + "index.bdm";
			if ( ! PackageFormat_Support::AddResourceIfExists ( resourceList, filePath ) ) {
				filePath = bdmvPath + "INDEX.BDM";
				PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
			}
		}
	}
	filePath = bdmvPath + "MovieObject.bdmv";
	if ( ! PackageFormat_Support::AddResourceIfExists ( resourceList, filePath ) ) {
		filePath = bdmvPath + "MOVIEOBJECT.BDMV";
		if ( ! PackageFormat_Support::AddResourceIfExists ( resourceList, filePath ) ) {
			filePath = bdmvPath + "MovieObj.bdm";
			if ( ! PackageFormat_Support::AddResourceIfExists ( resourceList, filePath ) ) {
				filePath = bdmvPath + "MOVIEOBJ.BDM";
				PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
			}
		}
	}


	if ( MakeClipInfoPath ( &filePath, ".clpi", true /* checkFile */ ) ) {
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
		clipInfoPath = filePath;
	}
	else {
		filePath = bdmvPath + "CLIPINF" + kDirChar ;
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	}

	bool addedStreamDir=false;
	if ( MakeClipStreamPath ( &filePath, ".xmp", true /* checkFile */ ) ) {
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
		addedStreamDir = true;
	}


	if ( MakeClipStreamPath ( &filePath, ".m2ts", true /* checkFile */ ) ) {
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	}
	else if ( ! addedStreamDir ) {
		filePath = bdmvPath + "STREAM" + kDirChar ;
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	}

	AVCHD_LegacyMetadata avchdLegacyData;
	if ( ReadAVCHDLegacyMetadata ( clipInfoPath, this->rootPath, this->clipName, avchdLegacyData, filePath ) ) {
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	}	
	else {
		filePath = bdmvPath + "PLAYLIST" + kDirChar ;
		PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	}
}
	
// =================================================================================================
// AVCHD_MetaHandler::CacheFileData
// ================================

void AVCHD_MetaHandler::CacheFileData()
{
	XMP_Assert ( ! this->containsXMP );

	if ( this->parent->UsesClientIO() ) {
		XMP_Throw ( "AVCHD cannot be used with client-managed I/O", kXMPErr_InternalFailure );
	}

	// See if the clip's .XMP file exists.

	std::string xmpPath;
	bool found = this->MakeClipStreamPath ( &xmpPath, ".xmp", true /* checkFile */ );
	if ( ! found ) return;	// No XMP.
	XMP_Assert ( Host_IO::Exists ( xmpPath.c_str() ) );	// MakeClipStreamPath should ensure this.

	// Read the entire .XMP file. We know the XMP exists, New_XMPFiles_IO is supposed to return 0
	// only if the file does not exist.

	bool readOnly = XMP_OptionIsClear ( this->parent->openFlags, kXMPFiles_OpenForUpdate );

	XMP_Assert ( this->parent->ioRef == 0 );
	XMPFiles_IO* xmpFile =  XMPFiles_IO::New_XMPFiles_IO ( xmpPath.c_str(), readOnly );
	if ( xmpFile == 0 ) XMP_Throw ( "AVCHD XMP file open failure", kXMPErr_InternalFailure );
	this->parent->ioRef = xmpFile;

	XMP_Int64 xmpLen = xmpFile->Length();
	if ( xmpLen > 100*1024*1024 ) {
		XMP_Throw ( "AVCHD XMP is outrageously large", kXMPErr_InternalFailure );	// Sanity check.
	}

	this->xmpPacket.erase();
	this->xmpPacket.append ( (size_t ) xmpLen, ' ' );

	XMP_Int32 ioCount = xmpFile->ReadAll ( (void*)this->xmpPacket.data(), (XMP_Int32)xmpLen );

	this->packetInfo.offset = 0;
	this->packetInfo.length = (XMP_Int32)xmpLen;
	FillPacketInfo ( this->xmpPacket, &this->packetInfo );

	this->containsXMP = true;

}	// AVCHD_MetaHandler::CacheFileData

// =================================================================================================
// AVCHD_MetaHandler::ProcessXMP
// =============================

void AVCHD_MetaHandler::ProcessXMP()
{
	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.

	if ( this->containsXMP ) {
		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	}

	// read clip info
	AVCHD_LegacyMetadata avchdLegacyData;
	std::string strPath,mplfile;

	bool ok = this->MakeClipInfoPath ( &strPath, ".clpi", true /* checkFile */ );
	if ( ok ) ReadAVCHDLegacyMetadata ( strPath, this->rootPath, this->clipName, avchdLegacyData , mplfile);
	if ( ! ok ) return;

	const AVCHD_blkPlayListMarkExt& markExt = avchdLegacyData.mPlaylistExtensionData.mPlaylistMarkExt;
	XMP_Uns8 pulldownFlag = 0;

	if ( markExt.mPresent ) {
		const std::string dateString = AVCHD_DateFieldToXMP ( markExt.mBlkTimezone, markExt.mRecordDataAndTime );

		if ( ! dateString.empty() ) this->xmpObj.SetProperty ( kXMP_NS_DM, "shotDate", dateString.c_str(), kXMP_DeleteExisting );
		AVCHD_SetXMPShotName ( this->xmpObj, markExt, this->clipName );
		AVCCAM_SetXMPStartTimecode ( this->xmpObj, markExt.mBlkTimecode, avchdLegacyData.mProgramInfo.mVideoStream.mFrameRate );
		pulldownFlag = (markExt.mFlags >> 1) & 0x03;  // bits 1 and 2
	}

	// Video Stream. AVCHD Format v. 1.01 p. 78

	const bool has2_2pulldown = (pulldownFlag == 0x01);
	const bool has3_2pulldown = (pulldownFlag == 0x10);
	XMP_StringPtr xmpValue = 0;

	if ( avchdLegacyData.mProgramInfo.mVideoStream.mPresent ) {

		// XMP videoFrameSize.
		xmpValue = 0;
		int frameIndex = -1;
		bool isProgressiveHD = false;
		const char* frameWidth[4]	= { "720", "720", "1280", "1920" };
		const char* frameHeight[4]	= { "480", "576", "720",  "1080" };

		switch ( avchdLegacyData.mProgramInfo.mVideoStream.mVideoFormat ) {
			case 1 : frameIndex = 0;							break; // 480i
			case 2 : frameIndex = 1;							break; // 576i
			case 3 : frameIndex = 0;							break; // 480p
			case 4 : frameIndex = 3;							break; // 1080i
			case 5 : frameIndex = 2; isProgressiveHD = true;	break; // 720p
			case 6 : frameIndex = 3; isProgressiveHD = true;	break; // 1080p
			default: break;
		}

		if ( frameIndex != -1 ) {
			xmpValue = frameWidth[frameIndex];
			this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "w", xmpValue, 0 );
			xmpValue = frameHeight[frameIndex];
			this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "h", xmpValue, 0 );
			xmpValue = "pixels";
			this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "unit", xmpValue, 0 );
		}

		// XMP videoFrameRate. The logic below seems pretty tortured, but matches "Table 4-4 pulldown" on page 31 of Book 2 of the AVCHD
		// spec, if you interepret "frame_mbs_only_flag" as "isProgressiveHD", "frame-rate [Hz]" as the frame rate encoded in
		// mVideoStream.mFrameRate, and "Video Scan Type" as the desired xmp output value. The algorithm produces correct results for
		// all the AVCHD media I've tested.
		xmpValue = 0;
		if ( isProgressiveHD ) {

			switch ( avchdLegacyData.mProgramInfo.mVideoStream.mFrameRate ) {
				case 1 : xmpValue = "23.98p";								break;   // "23.976"
				case 2 : xmpValue = "24p";									break;   // "24"
				case 3 : xmpValue = "25p";									break;   // "25"
				case 4 : xmpValue = has2_2pulldown ? "29.97p" : "59.94p";	break;   // "29.97"
				case 6 : xmpValue = has2_2pulldown ? "25p" : "50p";			break;   // "50"
				case 7 :															 // "59.94"
					if ( has2_2pulldown )
						xmpValue = "29.97p";
					else
						xmpValue = has3_2pulldown ? "23.98p" : "59.94p";

					break;
				default: break;
			}

		} else {

			switch ( avchdLegacyData.mProgramInfo.mVideoStream.mFrameRate ) {
				case 3 : xmpValue = has2_2pulldown ? "25p" : "50i";			break;  // "25" (but 1080p25 is reported as 1080i25 with 2:2 pulldown...)
				case 4 :															// "29.97"
					if ( has2_2pulldown )
						xmpValue = "29.97p";
					else
						xmpValue = has3_2pulldown ? "23.98p" : "59.94i";

					break;
				default: break;
			}

		}

		if ( xmpValue != 0 ) {
			this->xmpObj.SetProperty ( kXMP_NS_DM, "videoFrameRate", xmpValue, kXMP_DeleteExisting );
		}

		this->containsXMP = true;

	}

	// Audio Stream.
	if ( avchdLegacyData.mProgramInfo.mAudioStream.mPresent ) {

		xmpValue = 0;
		switch ( avchdLegacyData.mProgramInfo.mAudioStream.mAudioPresentationType ) {
			case 1  : xmpValue = "Mono"; break;
			case 3  : xmpValue = "Stereo"; break;
			default : break;
		}
		if ( xmpValue != 0 ) {
			this->xmpObj.SetProperty ( kXMP_NS_DM, "audioChannelType", xmpValue, kXMP_DeleteExisting );
		}

		xmpValue = 0;
		switch ( avchdLegacyData.mProgramInfo.mAudioStream.mSamplingFrequency ) {
			case 1  : xmpValue = "48000"; break;
			case 4  : xmpValue = "96000"; break;
			case 5  : xmpValue = "192000"; break;
			default : break;
		}
		if ( xmpValue != 0 ) {
			this->xmpObj.SetProperty ( kXMP_NS_DM, "audioSampleRate", xmpValue, kXMP_DeleteExisting );
		}

		this->containsXMP = true;
	}

	// Proprietary vendor extensions
	if ( AVCHD_SetXMPMakeAndModel ( this->xmpObj, avchdLegacyData.mClipExtensionData ) ) this->containsXMP = true;

	this->xmpObj.SetProperty ( kXMP_NS_DM, "title", this->clipName.c_str(), kXMP_DeleteExisting );
	this->containsXMP = true;

	if ( avchdLegacyData.mClipExtensionData.mMakersPrivateData.mPresent &&
		 ( avchdLegacyData.mClipExtensionData.mClipInfoExt.mMakerID == kMakerIDPanasonic ) ) {

		const AVCHD_blkPanasonicPrivateData& panasonicClipData = avchdLegacyData.mClipExtensionData.mMakersPrivateData.mPanasonicPrivateData;

		if ( panasonicClipData.mProClipIDBlock.mPresent ) {
			const std::string globalClipIDString = BytesToHex ( panasonicClipData.mProClipIDBlock.mGlobalClipID, 32 );

			this->xmpObj.SetProperty ( kXMP_NS_DC, "identifier", globalClipIDString.c_str(), kXMP_DeleteExisting );
		}

		const AVCHD_blkPanasonicPrivateData& panasonicPlaylistData =
			avchdLegacyData.mPlaylistExtensionData.mMakersPrivateData.mPanasonicPrivateData;

		if ( panasonicPlaylistData.mProPlaylistInfoBlock.mPlayListMark.mPresent ) {
			const AVCCAM_blkProPlayListMark& playlistMark = panasonicPlaylistData.mProPlaylistInfoBlock.mPlayListMark;

			if ( playlistMark.mShotMark.mPresent ) {
				// Unlike P2, where "shotmark" is a boolean, Panasonic treats their AVCCAM shotmark as a bit field with
				// 8 user-definable bits. For now we're going to treat any bit being set as xmpDM::good == true, and all
				// bits being clear as xmpDM::good == false.
				const bool isGood = ( playlistMark.mShotMark.mShotMark != 0 );

				xmpObj.SetProperty_Bool ( kXMP_NS_DM, "good",  isGood, kXMP_DeleteExisting );
			}

			if ( playlistMark.mAccess.mPresent && ( playlistMark.mAccess.mCreatorLength > 0 ) ) {
				const std::string creatorString = AVCHD_StringFieldToXMP (
					playlistMark.mAccess.mCreatorLength, playlistMark.mAccess.mCreatorCharacterSet, playlistMark.mAccess.mCreator, 32 ) ;

				if ( ! creatorString.empty() ) {
					xmpObj.DeleteProperty ( kXMP_NS_DC, "creator" );
					xmpObj.AppendArrayItem ( kXMP_NS_DC, "creator", kXMP_PropArrayIsOrdered, creatorString.c_str() );
				}
			}

			if ( playlistMark.mDevice.mPresent && ( playlistMark.mDevice.mSerialNoLength > 0 ) ) {
				const std::string serialNoString = AVCHD_StringFieldToXMP (
					playlistMark.mDevice.mSerialNoLength, playlistMark.mDevice.mSerialNoCharacterCode, playlistMark.mDevice.mSerialNo, 24 ) ;

				if ( ! serialNoString.empty() ) xmpObj.SetProperty ( kXMP_NS_EXIF_Aux, "SerialNumber", serialNoString.c_str(), kXMP_DeleteExisting );
			}

			if ( playlistMark.mLocation.mPresent && ( playlistMark.mLocation.mPlaceNameLength > 0 ) ) {
				const std::string placeNameString = AVCHD_StringFieldToXMP (
					playlistMark.mLocation.mPlaceNameLength, playlistMark.mLocation.mPlaceNameCharacterSet, playlistMark.mLocation.mPlaceName, 64 ) ;

				if ( ! placeNameString.empty() ) xmpObj.SetProperty ( kXMP_NS_DM, "shotLocation", placeNameString.c_str(), kXMP_DeleteExisting );
			}
		}
	}

}	// AVCHD_MetaHandler::ProcessXMP

// =================================================================================================
// AVCHD_MetaHandler::UpdateFile
// =============================
//
// Note that UpdateFile is only called from XMPFiles::CloseFile, so it is OK to close the file here.

void AVCHD_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	this->needsUpdate = false;	// Make sure only called once.

	XMP_Assert ( this->parent->UsesLocalIO() );

	std::string newDigest;
	this->MakeLegacyDigest ( &newDigest );
	this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "AVCHD", newDigest.c_str(), kXMP_DeleteExisting );

	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, this->GetSerializeOptions() );

	std::string xmpPath;
	this->MakeClipStreamPath ( &xmpPath, ".xmp" );

	bool haveXMP = Host_IO::Exists ( xmpPath.c_str() );
	if ( ! haveXMP ) {
		XMP_Assert ( this->parent->ioRef == 0 );
		Host_IO::Create ( xmpPath.c_str() );
		this->parent->ioRef = XMPFiles_IO::New_XMPFiles_IO ( xmpPath.c_str(), Host_IO::openReadWrite );
		if ( this->parent->ioRef == 0 ) XMP_Throw ( "Failure opening AVCHD XMP file", kXMPErr_ExternalFailure );
	}

	XMP_IO* xmpFile = this->parent->ioRef;
	XMP_Assert ( xmpFile != 0 );
	XIO::ReplaceTextFile ( xmpFile, this->xmpPacket, (haveXMP & doSafeUpdate) );

}	// AVCHD_MetaHandler::UpdateFile

// =================================================================================================
// AVCHD_MetaHandler::WriteTempFile
// ================================

void AVCHD_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{

	// ! WriteTempFile is not supposed to be called for handlers that own the file.
	XMP_Throw ( "AVCHD_MetaHandler::WriteTempFile should not be called", kXMPErr_InternalFailure );

}	// AVCHD_MetaHandler::WriteTempFile

// =================================================================================================
