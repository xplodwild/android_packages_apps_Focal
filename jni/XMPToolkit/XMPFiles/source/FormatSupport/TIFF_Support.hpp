#ifndef __TIFF_Support_hpp__
#define __TIFF_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include <map>
#include <stdlib.h>
#include <string.h>

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include "source/EndianUtils.hpp"

#include "source/Endian.h"

#if SUNOS_SPARC
        static const IEndian &IE = BigEndian::getInstance();
#else
        static const IEndian &IE = LittleEndian::getInstance();
#endif //#if SUNOS_SPARC

// =================================================================================================
/// \file TIFF_Support.hpp
/// \brief XMPFiles support for TIFF streams.
///
/// This header provides TIFF stream support specific to the needs of XMPFiles. This is not intended
/// for general purpose TIFF processing. TIFF_Manager is an abstract base class with 2 concrete
/// derived classes, TIFF_MemoryReader and TIFF_FileWriter.
///
/// TIFF_MemoryReader provides read-only support for TIFF streams that are small enough to be kept
/// entirely in memory. This allows optimizations to reduce heap usage and processing code. It is
/// sufficient for browsing access to the Exif metadata in JPEG and Photoshop files. Think of
/// TIFF_MemoryReader as "memory-based AND read-only". Since the entire TIFF stream is available,
/// GetTag will return information about any tag in the stream.
///
/// TIFF_FileWriter is for cases where updates are needed or the TIFF stream is too large to be kept
/// entirely in memory. Think of TIFF_FileWriter as "file-based OR read-write". TIFF_FileWriter only
/// maintains information for tags of interest as metadata.
///
/// The needs of XMPFiles are well defined metadata access. Only 4 IFDs are processed:
/// \li The 0th IFD, for the primary image, the first one in the outer list of IFDs.
/// \li The Exif general metadata IFD, from tag 34665 in the primary image IFD.
/// \li The Exif GPS Info metadata IFD, from tag 34853 in the primary image IFD.
/// \li The Exif Interoperability IFD, from tag 40965 in the Exif general metadata IFD.
///
/// \note These classes are for use only when directly compiled and linked. They should not be
/// packaged in a DLL by themselves. They do not provide any form of C++ ABI protection.
// =================================================================================================


// =================================================================================================
// TIFF IFD and type constants
// ===========================
//
// These aren't inside TIFF_Manager because static data members can't be initialized there.

enum {	// Constants for the recognized IFDs.
	kTIFF_PrimaryIFD    = 0,	// The primary image IFD, also called the 0th IFD.
	kTIFF_TNailIFD      = 1,	// The thumbnail image IFD also called the 1st IFD. (not used)
	kTIFF_ExifIFD       = 2,	// The Exif general metadata IFD.
	kTIFF_GPSInfoIFD    = 3,	// The Exif GPS Info IFD.
	kTIFF_InteropIFD    = 4,	// The Exif Interoperability IFD.
	kTIFF_LastRealIFD   = 4,
	kTIFF_KnownIFDCount = 5,
	kTIFF_KnownIFD      = 9	// The IFD that a tag is known to belong in.
};

enum {	// Constants for the type field of a tag, as defined by TIFF.
	kTIFF_ShortOrLongType =  0,	// ! Not part of the TIFF spec, never in a tag!
	kTIFF_ByteType        =  1,
	kTIFF_ASCIIType       =  2,
	kTIFF_ShortType       =  3,
	kTIFF_LongType        =  4,
	kTIFF_RationalType    =  5,
	kTIFF_SByteType       =  6,
	kTIFF_UndefinedType   =  7,
	kTIFF_SShortType      =  8,
	kTIFF_SLongType       =  9,
	kTIFF_SRationalType   = 10,
	kTIFF_FloatType       = 11,
	kTIFF_DoubleType      = 12,
	kTIFF_LastType        = 12
};

static const size_t kTIFF_TypeSizes[]    = { 0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };

static const bool kTIFF_IsIntegerType[]  = { 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0 };
static const bool kTIFF_IsRationalType[] = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 };
static const bool kTIFF_IsFloatType[]    = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };

static const char * kTIFF_TypeNames[] = { "ShortOrLong", "BYTE", "ASCII", "SHORT", "LONG", "RATIONAL",
										  "SBYTE", "UNDEFINED", "SSHORT", "SLONG", "SRATIONAL",
										  "FLOAT", "DOUBLE" };

enum {	// Encodings for SetTag_EncodedString.
	kTIFF_EncodeUndefined = 0,
	kTIFF_EncodeASCII     = 1,
	kTIFF_EncodeUnicode   = 2,	// UTF-16 in the endianness of the TIFF stream.
	kTIFF_EncodeJIS       = 3,	// Exif 2.2 uses JIS X 208-1990.
	kTIFF_EncodeUnknown   = 9
};

// =================================================================================================
// Recognized TIFF tags
// ====================

// -----------------------------------------------------------------------------------------------
// An enum of IDs for all of the tags as potential interest as metadata. The numberical order does
// not matter. These are mostly listed in the order of the Exif specification tables for convenience
// of checking correspondence.

enum {

	// General 0th IFD tags. Some of these can also be in the thumbnail IFD.

	// General tags from Exif 2.3 table 4:
	kTIFF_ImageWidth = 256,
	kTIFF_ImageLength = 257,
	kTIFF_BitsPerSample = 258,
	kTIFF_Compression = 259,
	kTIFF_PhotometricInterpretation = 262,
	kTIFF_Orientation = 274,
	kTIFF_SamplesPerPixel = 277,
	kTIFF_PlanarConfiguration = 284,
	kTIFF_YCbCrCoefficients = 529,
	kTIFF_YCbCrSubSampling = 530,
	kTIFF_XResolution = 282,
	kTIFF_YResolution = 283,
	kTIFF_ResolutionUnit = 296,
	kTIFF_TransferFunction = 301,
	kTIFF_WhitePoint = 318,
	kTIFF_PrimaryChromaticities = 319,
	kTIFF_YCbCrPositioning = 531,
	kTIFF_ReferenceBlackWhite = 532,
	kTIFF_DateTime = 306,
	kTIFF_ImageDescription = 270,
	kTIFF_Make = 271,
	kTIFF_Model = 272,
	kTIFF_Software = 305,
	kTIFF_Artist = 315,
	kTIFF_Copyright = 33432,
	
	// Tags defined by Adobe:
	kTIFF_XMP = 700,
	kTIFF_IPTC = 33723,
	kTIFF_PSIR = 34377,
	kTIFF_DNGVersion = 50706,
	kTIFF_DNGBackwardVersion = 50707,

	// Additional thumbnail IFD tags. We also care about 256, 257, and 259 in thumbnails.
	kTIFF_JPEGInterchangeFormat = 513,
	kTIFF_JPEGInterchangeFormatLength = 514,

	// Tags that need special handling when rewriting memory-based TIFF.
	kTIFF_StripOffsets = 273,
	kTIFF_StripByteCounts = 279,
	kTIFF_FreeOffsets = 288,
	kTIFF_FreeByteCounts = 289,
	kTIFF_TileOffsets = 324,
	kTIFF_TileByteCounts = 325,
	kTIFF_SubIFDs = 330,
	kTIFF_JPEGQTables = 519,
	kTIFF_JPEGDCTables = 520,
	kTIFF_JPEGACTables = 521,

	// Exif IFD tags defined in Exif 2.3 table 7.

	kTIFF_ExifVersion = 36864,
	kTIFF_FlashpixVersion = 40960,
	kTIFF_ColorSpace = 40961,
	kTIFF_Gamma = 42240,
	kTIFF_ComponentsConfiguration = 37121,
	kTIFF_CompressedBitsPerPixel = 37122,
	kTIFF_PixelXDimension = 40962,
	kTIFF_PixelYDimension = 40963,
	kTIFF_MakerNote = 37500, // Gets deleted when rewriting memory-based TIFF.
	kTIFF_UserComment = 37510,
	kTIFF_RelatedSoundFile = 40964,
	kTIFF_DateTimeOriginal = 36867,
	kTIFF_DateTimeDigitized = 36868,
	kTIFF_SubSecTime = 37520,
	kTIFF_SubSecTimeOriginal = 37521,
	kTIFF_SubSecTimeDigitized = 37522,
	kTIFF_ImageUniqueID = 42016,
	kTIFF_CameraOwnerName = 42032,
	kTIFF_BodySerialNumber = 42033,
	kTIFF_LensSpecification = 42034,
	kTIFF_LensMake = 42035,
	kTIFF_LensModel = 42036,
	kTIFF_LensSerialNumber = 42037,

	// Exif IFD tags defined in Exif 2.3 table 8.

	kTIFF_ExposureTime = 33434,
	kTIFF_FNumber = 33437,
	kTIFF_ExposureProgram = 34850,
	kTIFF_SpectralSensitivity = 34852,
	kTIFF_PhotographicSensitivity = 34855,	// ! Called kTIFF_ISOSpeedRatings before Exif 2.3.
	kTIFF_OECF = 34856,
	kTIFF_SensitivityType = 34864,
	kTIFF_StandardOutputSensitivity = 34865,
	kTIFF_RecommendedExposureIndex = 34866,
	kTIFF_ISOSpeed = 34867,
	kTIFF_ISOSpeedLatitudeyyy = 34868,
	kTIFF_ISOSpeedLatitudezzz = 34869,
	kTIFF_ShutterSpeedValue = 37377,
	kTIFF_ApertureValue = 37378,
	kTIFF_BrightnessValue = 37379,
	kTIFF_ExposureBiasValue = 37380,
	kTIFF_MaxApertureValue = 37381,
	kTIFF_SubjectDistance = 37382,
	kTIFF_MeteringMode = 37383,
	kTIFF_LightSource = 37384,
	kTIFF_Flash = 37385,
	kTIFF_FocalLength = 37386,
	kTIFF_SubjectArea = 37396,
	kTIFF_FlashEnergy = 41483,
	kTIFF_SpatialFrequencyResponse = 41484,
	kTIFF_FocalPlaneXResolution = 41486,
	kTIFF_FocalPlaneYResolution = 41487,
	kTIFF_FocalPlaneResolutionUnit = 41488,
	kTIFF_SubjectLocation = 41492,
	kTIFF_ExposureIndex = 41493,
	kTIFF_SensingMethod = 41495,
	kTIFF_FileSource = 41728,
	kTIFF_SceneType = 41729,
	kTIFF_CFAPattern = 41730,
	kTIFF_CustomRendered = 41985,
	kTIFF_ExposureMode = 41986,
	kTIFF_WhiteBalance = 41987,
	kTIFF_DigitalZoomRatio = 41988,
	kTIFF_FocalLengthIn35mmFilm = 41989,
	kTIFF_SceneCaptureType = 41990,
	kTIFF_GainControl = 41991,
	kTIFF_Contrast = 41992,
	kTIFF_Saturation = 41993,
	kTIFF_Sharpness = 41994,
	kTIFF_DeviceSettingDescription = 41995,
	kTIFF_SubjectDistanceRange = 41996,

	// GPS IFD tags.

	kTIFF_GPSVersionID = 0,
	kTIFF_GPSLatitudeRef = 1,
	kTIFF_GPSLatitude = 2,
	kTIFF_GPSLongitudeRef = 3,
	kTIFF_GPSLongitude = 4,
	kTIFF_GPSAltitudeRef = 5,
	kTIFF_GPSAltitude = 6,
	kTIFF_GPSTimeStamp = 7,
	kTIFF_GPSSatellites = 8,
	kTIFF_GPSStatus = 9,
	kTIFF_GPSMeasureMode = 10,
	kTIFF_GPSDOP = 11,
	kTIFF_GPSSpeedRef = 12,
	kTIFF_GPSSpeed = 13,
	kTIFF_GPSTrackRef = 14,
	kTIFF_GPSTrack = 15,
	kTIFF_GPSImgDirectionRef = 16,
	kTIFF_GPSImgDirection = 17,
	kTIFF_GPSMapDatum = 18,
	kTIFF_GPSDestLatitudeRef = 19,
	kTIFF_GPSDestLatitude = 20,
	kTIFF_GPSDestLongitudeRef = 21,
	kTIFF_GPSDestLongitude = 22,
	kTIFF_GPSDestBearingRef = 23,
	kTIFF_GPSDestBearing = 24,
	kTIFF_GPSDestDistanceRef = 25,
	kTIFF_GPSDestDistance = 26,
	kTIFF_GPSProcessingMethod = 27,
	kTIFF_GPSAreaInformation = 28,
	kTIFF_GPSDateStamp = 29,
	kTIFF_GPSDifferential = 30,
	kTIFF_GPSHPositioningError = 31,

	// Special tags that are links to other IFDs.
	
	kTIFF_ExifIFDPointer = 34665,				// Found in 0th IFD
	kTIFF_GPSInfoIFDPointer = 34853,			// Found in 0th IFD
	kTIFF_InteroperabilityIFDPointer = 40965	// Found in Exif IFD

};

// *** Temporary hack:
#define kTIFF_ISOSpeedRatings	kTIFF_PhotographicSensitivity

// ------------------------------------------------------------------
// Sorted arrays of the tags that are recognized in the various IFDs.

static const XMP_Uns16 sKnownPrimaryIFDTags[] =
{
	kTIFF_ImageWidth,					//   256
	kTIFF_ImageLength,					//   257
	kTIFF_BitsPerSample,				//   258
	kTIFF_Compression,					//   259
	kTIFF_PhotometricInterpretation,	//   262
	kTIFF_ImageDescription,				//   270
	kTIFF_Make,							//   271
	kTIFF_Model,						//   272
	kTIFF_Orientation,					//   274
	kTIFF_SamplesPerPixel,				//   277
	kTIFF_XResolution,					//   282
	kTIFF_YResolution,					//   283
	kTIFF_PlanarConfiguration,			//   284
	kTIFF_ResolutionUnit,				//   296
	kTIFF_TransferFunction,				//   301
	kTIFF_Software,						//   305
	kTIFF_DateTime,						//   306
	kTIFF_Artist,						//   315
	kTIFF_WhitePoint,					//   318
	kTIFF_PrimaryChromaticities,		//   319
	kTIFF_YCbCrCoefficients,			//   529
	kTIFF_YCbCrSubSampling,				//   530
	kTIFF_YCbCrPositioning,				//   531
	kTIFF_ReferenceBlackWhite,			//   532
	kTIFF_XMP,							//   700
	kTIFF_Copyright,					// 33432
	kTIFF_IPTC,							// 33723
	kTIFF_PSIR,							// 34377
	kTIFF_ExifIFDPointer,				// 34665
	kTIFF_GPSInfoIFDPointer,			// 34853
	kTIFF_DNGVersion,					// 50706
	kTIFF_DNGBackwardVersion,			// 50707
	0xFFFF	// Must be last as a sentinel.
};

static const XMP_Uns16 sKnownThumbnailIFDTags[] =
{
	kTIFF_ImageWidth,					// 256
	kTIFF_ImageLength,					// 257
	kTIFF_Compression,					// 259
	kTIFF_JPEGInterchangeFormat,		// 513
	kTIFF_JPEGInterchangeFormatLength,	// 514
	0xFFFF	// Must be last as a sentinel.
};

static const XMP_Uns16 sKnownExifIFDTags[] =
{
	kTIFF_ExposureTime,					// 33434
	kTIFF_FNumber,						// 33437
	kTIFF_ExposureProgram,				// 34850
	kTIFF_SpectralSensitivity,			// 34852
	kTIFF_PhotographicSensitivity,		// 34855
	kTIFF_OECF,							// 34856
	kTIFF_SensitivityType,				// 34864
	kTIFF_StandardOutputSensitivity,	// 34865
	kTIFF_RecommendedExposureIndex,		// 34866
	kTIFF_ISOSpeed,						// 34867
	kTIFF_ISOSpeedLatitudeyyy,			// 34868
	kTIFF_ISOSpeedLatitudezzz,			// 34869
	kTIFF_ExifVersion,					// 36864
	kTIFF_DateTimeOriginal,				// 36867
	kTIFF_DateTimeDigitized,			// 36868
	kTIFF_ComponentsConfiguration,		// 37121
	kTIFF_CompressedBitsPerPixel,		// 37122
	kTIFF_ShutterSpeedValue,			// 37377
	kTIFF_ApertureValue,				// 37378
	kTIFF_BrightnessValue,				// 37379
	kTIFF_ExposureBiasValue,			// 37380
	kTIFF_MaxApertureValue,				// 37381
	kTIFF_SubjectDistance,				// 37382
	kTIFF_MeteringMode,					// 37383
	kTIFF_LightSource,					// 37384
	kTIFF_Flash,						// 37385
	kTIFF_FocalLength,					// 37386
	kTIFF_SubjectArea,					// 37396
	kTIFF_UserComment,					// 37510
	kTIFF_SubSecTime,					// 37520
	kTIFF_SubSecTimeOriginal,			// 37521
	kTIFF_SubSecTimeDigitized,			// 37522
	kTIFF_FlashpixVersion,				// 40960
	kTIFF_ColorSpace,					// 40961
	kTIFF_PixelXDimension,				// 40962
	kTIFF_PixelYDimension,				// 40963
	kTIFF_RelatedSoundFile,				// 40964
	kTIFF_FlashEnergy,					// 41483
	kTIFF_SpatialFrequencyResponse,		// 41484
	kTIFF_FocalPlaneXResolution,		// 41486
	kTIFF_FocalPlaneYResolution,		// 41487
	kTIFF_FocalPlaneResolutionUnit,		// 41488
	kTIFF_SubjectLocation,				// 41492
	kTIFF_ExposureIndex,				// 41493
	kTIFF_SensingMethod,				// 41495
	kTIFF_FileSource,					// 41728
	kTIFF_SceneType,					// 41729
	kTIFF_CFAPattern,					// 41730
	kTIFF_CustomRendered,				// 41985
	kTIFF_ExposureMode,					// 41986
	kTIFF_WhiteBalance,					// 41987
	kTIFF_DigitalZoomRatio,				// 41988
	kTIFF_FocalLengthIn35mmFilm,		// 41989
	kTIFF_SceneCaptureType,				// 41990
	kTIFF_GainControl,					// 41991
	kTIFF_Contrast,						// 41992
	kTIFF_Saturation,					// 41993
	kTIFF_Sharpness,					// 41994
	kTIFF_DeviceSettingDescription,		// 41995
	kTIFF_SubjectDistanceRange,			// 41996
	kTIFF_ImageUniqueID,				// 42016
	kTIFF_CameraOwnerName,				// 42032
	kTIFF_BodySerialNumber,				// 42033
	kTIFF_LensSpecification,			// 42034
	kTIFF_LensMake,						// 42035
	kTIFF_LensModel,					// 42036
	kTIFF_LensSerialNumber,				// 42037
	kTIFF_Gamma,						// 42240
	0xFFFF	// Must be last as a sentinel.
};

static const XMP_Uns16 sKnownGPSInfoIFDTags[] =
{
	kTIFF_GPSVersionID,			//  0
	kTIFF_GPSLatitudeRef,		//  1
	kTIFF_GPSLatitude,			//  2
	kTIFF_GPSLongitudeRef,		//  3
	kTIFF_GPSLongitude,			//  4
	kTIFF_GPSAltitudeRef,		//  5
	kTIFF_GPSAltitude,			//  6
	kTIFF_GPSTimeStamp,			//  7
	kTIFF_GPSSatellites,		//  8
	kTIFF_GPSStatus,			//  9
	kTIFF_GPSMeasureMode,		// 10
	kTIFF_GPSDOP,				// 11
	kTIFF_GPSSpeedRef,			// 12
	kTIFF_GPSSpeed,				// 13
	kTIFF_GPSTrackRef,			// 14
	kTIFF_GPSTrack,				// 15
	kTIFF_GPSImgDirectionRef,	// 16
	kTIFF_GPSImgDirection,		// 17
	kTIFF_GPSMapDatum,			// 18
	kTIFF_GPSDestLatitudeRef,	// 19
	kTIFF_GPSDestLatitude,		// 20
	kTIFF_GPSDestLongitudeRef,	// 21
	kTIFF_GPSDestLongitude,		// 22
	kTIFF_GPSDestBearingRef,	// 23
	kTIFF_GPSDestBearing,		// 24
	kTIFF_GPSDestDistanceRef,	// 25
	kTIFF_GPSDestDistance,		// 26
	kTIFF_GPSProcessingMethod,	// 27
	kTIFF_GPSAreaInformation,	// 28
	kTIFF_GPSDateStamp,			// 29
	kTIFF_GPSDifferential,		// 30
	kTIFF_GPSHPositioningError,	// 31
	0xFFFF	// Must be last as a sentinel.
};

static const XMP_Uns16 sKnownInteroperabilityIFDTags[] =
{
	// ! Yes, there are none at present.
	0xFFFF	// Must be last as a sentinel.
};

// ! Make sure these are in the same order as the IFD enum!
static const XMP_Uns16* sKnownTags[kTIFF_KnownIFDCount] = { sKnownPrimaryIFDTags,
															sKnownThumbnailIFDTags,
															sKnownExifIFDTags,
															sKnownGPSInfoIFDTags,
															sKnownInteroperabilityIFDTags };


// =================================================================================================
// =================================================================================================


// =================================================================================================
// TIFF_Manager
// ============

class TIFF_Manager {	// The abstract base class.
public:

	// ---------------------------------------------------------------------------------------------
	// Types and constants

	static const XMP_Uns32 kBigEndianPrefix    = 0x4D4D002AUL;
	static const XMP_Uns32 kLittleEndianPrefix = 0x49492A00UL;

	static const size_t kEmptyTIFFLength = 8;		// Just the header.
	static const size_t kEmptyIFDLength  = 2 + 4;	// Entry count and next-IFD offset.
	static const size_t kIFDEntryLength  = 12;

	struct TagInfo {
		XMP_Uns16   id;
		XMP_Uns16   type;
		XMP_Uns32   count;
		const void* dataPtr;	// ! The data must not be changed! Stream endian format!
		XMP_Uns32   dataLen;	// The length in bytes.
		TagInfo() : id(0), type(0), count(0), dataPtr(0), dataLen(0) {};
		TagInfo ( XMP_Uns16 _id, XMP_Uns16 _type, XMP_Uns32 _count, const void* _dataPtr, XMP_Uns32 _dataLen )
			: id(_id), type(_type), count(_count), dataPtr(_dataPtr), dataLen(_dataLen) {};
	};

	typedef std::map<XMP_Uns16,TagInfo> TagInfoMap;

	struct Rational  { XMP_Uns32 num, denom; };
	struct SRational { XMP_Int32 num, denom; };

	// ---------------------------------------------------------------------------------------------
	// The IsXyzEndian methods return the external endianness of the original parsed TIFF stream.
	// The \c GetTag methods return native endian values, the \c SetTag methods take native values.
	// The original endianness is preserved in output.

	bool IsBigEndian() const { return this->bigEndian; };
	bool IsLittleEndian() const { return (! this->bigEndian); };
	bool IsNativeEndian() const { return this->nativeEndian; };

	// ---------------------------------------------------------------------------------------------
	// The TIFF_Manager only keeps explicit knowledge of up to 4 IFDs:
	// - The primary image IFD, also known as the 0th IFD. This must be present.
	// - A possible Exif general metadata IFD, found from tag 34665 in the primary image IFD.
	// - A possible Exif GPS metadata IFD, found from tag 34853 in the primary image IFD.
	// - A possible Exif Interoperability IFD, found from tag 40965 in the Exif general metadata IFD.
	//
	// Parsing will silently forget about certain aspects of ill-formed streams. If any tags are
	// repeated in an IFD, only the last is kept. Any known tags that are in the wrong IFD are
	// removed. Parsing will sort the tags into ascending order, AppendTIFF and ComposeTIFF will
	// preserve the sorted order. These fixes do not cause IsChanged to return true, that only
	// happens if the client makes explicit changes using SetTag or DeleteTag.

	virtual bool HasExifIFD() const = 0;
	virtual bool HasGPSInfoIFD() const = 0;

	// ---------------------------------------------------------------------------------------------
	// These are the basic methods to get a map of all of the tags in an IFD, to get or set a tag,
	// or to delete a tag. The dataPtr returned by \c GetTag is consided read-only, the client must
	// not change it. The ifd parameter to \c GetIFD must be for one of the recognized actual IFDs.
	// The ifd parameter for the GetTag or SetTag methods can be a specific IFD of kTIFF_KnownIFD.
	// Using the specific IFD will be slightly faster, saving a lookup in the known tag map. An
	// exception is thrown if kTIFF_KnownIFD is passed to GetTag or SetTag and the tag is not known.
	// \c SetTag replaces an existing tag regardless of type or count. \c DeleteTag deletes a tag,
	// it is a no-op if the tag does not exist. \c GetValueOffset returns the offset within the
	// parsed stream of the tag's value. It returns 0 if the tag was not in the parsed input.

	virtual bool GetIFD ( XMP_Uns8 ifd, TagInfoMap* ifdMap ) const = 0;

	virtual bool GetTag ( XMP_Uns8 ifd, XMP_Uns16 id, TagInfo* info ) const = 0;

	virtual void SetTag ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16 type, XMP_Uns32 count, const void* dataPtr ) = 0;

	virtual void DeleteTag ( XMP_Uns8 ifd, XMP_Uns16 id ) = 0;

	virtual XMP_Uns32 GetValueOffset ( XMP_Uns8 ifd, XMP_Uns16 id ) const = 0;

	// ---------------------------------------------------------------------------------------------
	// These methods are for tags whose type can be short or long, depending on the actual value.
	// \c GetTag_Integer returns false if an existing tag's type is not short, or long, or if the
	// count is not 1. \c SetTag_Integer replaces an existing tag regardless of type or count, the
	// new tag will have type short or long.

	virtual bool GetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const = 0;

	void SetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32 data );

	// ---------------------------------------------------------------------------------------------
	// These are customized forms of GetTag that verify the type and return a typed value. False is
	// returned if the type does not match or if the count is not 1.

	// *** Should also add support for ASCII multi-strings?

	virtual bool GetTag_Byte   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8* data ) const = 0;
	virtual bool GetTag_SByte  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8* data ) const = 0;
	virtual bool GetTag_Short  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16* data ) const = 0;
	virtual bool GetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16* data ) const = 0;
	virtual bool GetTag_Long   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const = 0;
	virtual bool GetTag_SLong  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32* data ) const = 0;

	virtual bool GetTag_Rational  ( XMP_Uns8 ifd, XMP_Uns16 id, Rational* data ) const = 0;
	virtual bool GetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, SRational* data ) const = 0;

	virtual bool GetTag_Float  ( XMP_Uns8 ifd, XMP_Uns16 id, float* data ) const = 0;
	virtual bool GetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double* data ) const = 0;

	virtual bool GetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr* dataPtr, XMP_StringLen* dataLen ) const = 0;

	// ---------------------------------------------------------------------------------------------

	void SetTag_Byte   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8 data );
	void SetTag_SByte  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8 data );
	void SetTag_Short  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16 data );
	void SetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16 data );
	void SetTag_Long   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32 data );
	void SetTag_SLong  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32 data );

	void SetTag_Rational  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32 num, XMP_Uns32 denom );
	void SetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32 num, XMP_Int32 denom );

	void SetTag_Float  ( XMP_Uns8 ifd, XMP_Uns16 id, float data );
	void SetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double data );

	void SetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr dataPtr );

	// ---------------------------------------------------------------------------------------------

	virtual bool GetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, std::string* utf8Str ) const = 0;
	virtual void SetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, const std::string& utf8Str, XMP_Uns8 encoding ) = 0;

	bool DecodeString ( const void * encodedPtr, size_t encodedLen, std::string* utf8Str ) const;
	bool EncodeString ( const std::string& utf8Str, XMP_Uns8 encoding, std::string* encodedStr );

	// ---------------------------------------------------------------------------------------------
	// \c IsChanged returns true if a read-write stream has changes that need to be saved. This is
	// only the case when a \c SetTag method has been called. It is not true for changes related to
	// parsing normalization such as sorting of tags. \c IsChanged returns false for read-only streams.

	virtual bool IsChanged() = 0;

	// ---------------------------------------------------------------------------------------------
	// \c IsLegacyChanged returns true if a read-write stream has changes that need to be saved to
	// tags other than the XMP (tag 700). This only the case when a \c SetTag method has been
	// called. It is not true for changes related to parsing normalization such as sorting of tags.
	// \c IsLegacyChanged returns false for read-only streams.

	virtual bool IsLegacyChanged() = 0;

	// ---------------------------------------------------------------------------------------------
	// \c UpdateMemoryStream is mainly applicable to memory-based read-write streams. It recomposes
	// the memory stream to incorporate all changes. The new length and data pointer are returned.
	// \c UpdateMemoryStream can be used with a read-only memory stream to get the raw stream info.
	//
	// \c UpdateFileStream updates file-based TIFF. The client must guarantee that the TIFF portion
	// of the file matches that from the parse in the file-based constructor. Offsets saved from that
	// parse must still be valid. The open file reference need not be the same, e.g. the client can
	// be doing a crash-safe update into a temporary copy.
	//
	// Both \c UpdateMemoryStream and \c UpdateFileStream use an update-by-append model. Changes are
	// written in-place where they fit, anything requiring growth is appended to the end and the old
	// space is abandoned. The end for memory-based TIFF is the end of the data block, the end for
	// file-based TIFF is the end of the file. This update-by-append model has the advantage of not
	// perturbing any hidden offsets, a common feature of proprietary MakerNotes.
	//
	// The condenseStream parameter to UpdateMemoryStream can be used to rewrite the full stream
	// instead of appending. This will discard any MakerNote tags and risks breaking offsets that
	// are hidden. This can be necessary though to try to make the TIFF fit in a JPEG file.

	virtual void ParseMemoryStream ( const void* data, XMP_Uns32 length, bool copyData = true ) = 0;
	virtual void ParseFileStream   ( XMP_IO* fileRef ) = 0;

	virtual void IntegrateFromPShop6 ( const void * buriedPtr, size_t buriedLen ) = 0;

	virtual XMP_Uns32 UpdateMemoryStream ( void** dataPtr, bool condenseStream = false ) = 0;
	virtual void      UpdateFileStream   ( XMP_IO* fileRef, XMP_ProgressTracker* progressTracker ) = 0;

	// ---------------------------------------------------------------------------------------------

	GetUns16_Proc  GetUns16;	// Get values from the TIFF stream.
	GetUns32_Proc  GetUns32;	// Always native endian on the outside, stream endian in the stream.
	GetFloat_Proc  GetFloat;
	GetDouble_Proc GetDouble;

	PutUns16_Proc  PutUns16;	// Put values into the TIFF stream.
	PutUns32_Proc  PutUns32;	// Always native endian on the outside, stream endian in the stream.
	PutFloat_Proc  PutFloat;
	PutDouble_Proc PutDouble;

	virtual ~TIFF_Manager() {};

	virtual void SetErrorCallback ( GenericErrorCallback * ec ) { this->errorCallbackPtr = ec; };

	virtual void NotifyClient ( XMP_ErrorSeverity severity, XMP_Error & error );

protected:

	bool bigEndian, nativeEndian;

	XMP_Uns32 CheckTIFFHeader ( const XMP_Uns8* tiffPtr, XMP_Uns32 length );
		// The pointer is to a buffer of the first 8 bytes. The length is the overall length, used
		// to check the primary IFD offset.

	TIFF_Manager();	// Force clients to use the reader or writer derived classes.

	struct RawIFDEntry {
		XMP_Uns16 id;
		XMP_Uns16 type;
		XMP_Uns32 count;
		XMP_Uns32 dataOrOffset;
	};

	GenericErrorCallback *errorCallbackPtr;

};	// TIFF_Manager


// =================================================================================================
// =================================================================================================


// =================================================================================================
// TIFF_MemoryReader
// =================

class TIFF_MemoryReader : public TIFF_Manager {	// The derived class for memory-based read-only access.
public:

	bool HasExifIFD() const { return (containedIFDs[kTIFF_ExifIFD].count != 0); };
	bool HasGPSInfoIFD() const { return (containedIFDs[kTIFF_GPSInfoIFD].count != 0); };

	bool GetIFD ( XMP_Uns8 ifd, TagInfoMap* ifdMap ) const;

	bool GetTag ( XMP_Uns8 ifd, XMP_Uns16 id, TagInfo* info ) const;

	void SetTag ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16 type, XMP_Uns32 count, const void* dataPtr ) { NotAppropriate(); };

	void DeleteTag ( XMP_Uns8 ifd, XMP_Uns16 id ) { NotAppropriate(); };

	XMP_Uns32 GetValueOffset ( XMP_Uns8 ifd, XMP_Uns16 id ) const;

	bool GetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const;

	bool GetTag_Byte   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8* data ) const;
	bool GetTag_SByte  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8* data ) const;
	bool GetTag_Short  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16* data ) const;
	bool GetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16* data ) const;
	bool GetTag_Long   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const;
	bool GetTag_SLong  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32* data ) const;

	bool GetTag_Rational  ( XMP_Uns8 ifd, XMP_Uns16 id, Rational* data ) const;
	bool GetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, SRational* data ) const;

	bool GetTag_Float  ( XMP_Uns8 ifd, XMP_Uns16 id, float* data ) const;
	bool GetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double* data ) const;

	bool GetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr* dataPtr, XMP_StringLen* dataLen ) const;

	bool GetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, std::string* utf8Str ) const;

	void SetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, const std::string& utf8Str, XMP_Uns8 encoding ) { NotAppropriate(); };

	bool IsChanged() { return false; };
	bool IsLegacyChanged() { return false; };

	void ParseMemoryStream ( const void* data, XMP_Uns32 length, bool copyData = true );
	void ParseFileStream   ( XMP_IO* fileRef ) { NotAppropriate(); };

	void IntegrateFromPShop6 ( const void * buriedPtr, size_t buriedLen ) { NotAppropriate(); };

	XMP_Uns32 UpdateMemoryStream ( void** dataPtr, bool condenseStream = false ) { if ( dataPtr != 0 ) *dataPtr = tiffStream; return tiffLength; };
	void      UpdateFileStream   ( XMP_IO* fileRef, XMP_ProgressTracker* progressTracker ) { NotAppropriate(); };

	TIFF_MemoryReader() : ownedStream(false), tiffStream(0), tiffLength(0) {};

	virtual ~TIFF_MemoryReader() { if ( this->ownedStream ) free ( this->tiffStream ); };

private:

	bool ownedStream;

	XMP_Uns8* tiffStream;
	XMP_Uns32 tiffLength;

	// Memory usage notes: TIFF_MemoryReader is for memory-based read-only usage (both apply). There
	// is no need to ever allocate separate blocks of memory, everything is used directly from the
	// TIFF stream. Data pointers are computed on the fly, the offset field is 4 bytes and pointers
	// will be 8 bytes for 64-bit platforms.

	struct TweakedIFDEntry {	// ! Most fields are in native byte order, dataOrPos is for offsets only.
		XMP_Uns16 id;
		XMP_Uns16 type;
		XMP_Uns32 bytes;
		XMP_Uns32 dataOrPos;
		TweakedIFDEntry() : id(0), type(0), bytes(0), dataOrPos(0) {};
	};

	struct TweakedIFDInfo {
		XMP_Uns16 count;
		TweakedIFDEntry* entries;
		TweakedIFDInfo() : count(0), entries(0) {};
	};

	TweakedIFDInfo containedIFDs[kTIFF_KnownIFDCount];

	static void SortIFD ( TweakedIFDInfo* thisIFD );

	XMP_Uns32 ProcessOneIFD ( XMP_Uns32 ifdOffset, XMP_Uns8 ifd );

	const TweakedIFDEntry* FindTagInIFD ( XMP_Uns8 ifd, XMP_Uns16 id ) const;

	const inline void* GetDataPtr ( const TweakedIFDEntry* tifdEntry ) const
		{ if ( GetUns32AsIs(&tifdEntry->bytes) <= 4 ) {
		  	return &tifdEntry->dataOrPos;
		  } else {
		  	return (this->tiffStream + GetUns32AsIs(&tifdEntry->dataOrPos));
		  }
		}

	static inline void NotAppropriate() { XMP_Throw ( "Not appropriate for TIFF_Reader", kXMPErr_InternalFailure ); };

};	// TIFF_MemoryReader


// =================================================================================================
// =================================================================================================


// =================================================================================================
// TIFF_FileWriter
// ===============

class TIFF_FileWriter : public TIFF_Manager {	// The derived class for file-based or read-write access.
public:

	bool HasExifIFD() const { return this->containedIFDs[kTIFF_ExifIFD].tagMap.size() != 0; };
	bool HasGPSInfoIFD() const { return this->containedIFDs[kTIFF_GPSInfoIFD].tagMap.size() != 0; };

	bool GetIFD ( XMP_Uns8 ifd, TagInfoMap* ifdMap ) const;

	bool GetTag ( XMP_Uns8 ifd, XMP_Uns16 id, TagInfo* info ) const;

	void SetTag ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16 type, XMP_Uns32 count, const void* dataPtr );

	void DeleteTag ( XMP_Uns8 ifd, XMP_Uns16 id );

	XMP_Uns32 GetValueOffset ( XMP_Uns8 ifd, XMP_Uns16 id ) const;

	bool GetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const;

	bool GetTag_Byte   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8* data ) const;
	bool GetTag_SByte  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8* data ) const;
	bool GetTag_Short  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16* data ) const;
	bool GetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16* data ) const;
	bool GetTag_Long   ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const;
	bool GetTag_SLong  ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32* data ) const;

	bool GetTag_Rational  ( XMP_Uns8 ifd, XMP_Uns16 id, Rational* data ) const;
	bool GetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, SRational* data ) const;

	bool GetTag_Float  ( XMP_Uns8 ifd, XMP_Uns16 id, float* data ) const;
	bool GetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double* data ) const;

	bool GetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr* dataPtr, XMP_StringLen* dataLen ) const;

	bool GetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, std::string* utf8Str ) const;

	void SetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, const std::string& utf8Str, XMP_Uns8 encoding );

	bool IsChanged() { return this->changed; };

	bool IsLegacyChanged();

	enum { kDoNotCopyData = false };

	void ParseMemoryStream ( const void* data, XMP_Uns32 length, bool copyData = true );
	void ParseFileStream   ( XMP_IO* fileRef );

	void IntegrateFromPShop6 ( const void * buriedPtr, size_t buriedLen );

	XMP_Uns32 UpdateMemoryStream ( void** dataPtr, bool condenseStream = false );
	void      UpdateFileStream   ( XMP_IO* fileRef, XMP_ProgressTracker* progressTracker );

	TIFF_FileWriter();

	virtual ~TIFF_FileWriter();

private:

	bool changed, legacyDeleted;
	bool memParsed, fileParsed;
	bool ownedStream;

	XMP_Uns8* memStream;
	XMP_Uns32 tiffLength;

	// Memory usage notes: TIFF_FileWriter is for file-based OR read/write usage. For memory-based
	// streams the dataPtr is initially into the stream, regardless of size. For file-based streams
	// the dataPtr is initially a separate allocation for large values (over 4 bytes), and points to
	// the smallValue field for small values. This is also the usage when a tag is changed (for both
	// memory and file cases), the dataPtr is a separate allocation for large values (over 4 bytes),
	// and points to the smallValue field for small values.

	// ! The working data values are always stream endian, no matter where stored. They are flipped
	// ! as necessary by GetTag and SetTag.

	static const bool kIsFileBased   = true;	// For use in the InternalTagInfo constructor.
	static const bool kIsMemoryBased = false;

	class InternalTagInfo {
	public:

		XMP_Uns16 id;
		XMP_Uns16 type;
		XMP_Uns32 count;
		XMP_Uns32 dataLen;
		XMP_Uns32 smallValue;		// Small value in stream endianness, but "left" justified.
		XMP_Uns8* dataPtr;			// Parsing captures all small values, only large ones that we care about.
		XMP_Uns32 origDataLen;		// The original (parse time) data length in bytes.
		XMP_Uns32 origDataOffset;	// The original data offset, regardless of length.
		bool      changed;
		bool      fileBased;

		inline void FreeData() {
			if ( this->fileBased || this->changed ) {
				if ( (this->dataLen > 4) && (this->dataPtr != 0) ) { free ( this->dataPtr ); this->dataPtr = 0; }
			}
		}

		InternalTagInfo ( XMP_Uns16 _id, XMP_Uns16 _type, XMP_Uns32 _count, bool _fileBased )
			: id(_id), type(_type), count(_count), dataLen(0), smallValue(0), dataPtr(0),
			  origDataLen(0), origDataOffset(0), changed(false), fileBased(_fileBased) {};
		~InternalTagInfo() { this->FreeData(); };

		void operator=  ( const InternalTagInfo & in )
		{
			// ! Gag! Transfer ownership of the dataPtr!
			this->FreeData();
			memcpy ( this, &in, sizeof(*this) );	// AUDIT: Use of sizeof(InternalTagInfo) is safe.
			if ( this->dataLen <= 4 ) {
				this->dataPtr = (XMP_Uns8*) &this->smallValue;	// Don't use the copied pointer.
			} else {
				*((XMP_Uns8**)&in.dataPtr) = 0;	// The pointer is now owned by "this".
			}
		};

	private:

		InternalTagInfo()	// Hidden on purpose, fileBased must be properly set.
			: id(0), type(0), count(0), dataLen(0), smallValue(0), dataPtr(0),
			  origDataLen(0), origDataOffset(0), changed(false), fileBased(false) {};

	};

	typedef std::map<XMP_Uns16,InternalTagInfo> InternalTagMap;

	struct InternalIFDInfo {
		bool changed;
		XMP_Uns16 origCount;		// Original number of IFD entries.
		XMP_Uns32 origIFDOffset;	// Original stream offset of the IFD.
		XMP_Uns32 origNextIFD;		// Original stream offset of the following IFD.
		InternalTagMap tagMap;
		InternalIFDInfo() : changed(false), origCount(0), origIFDOffset(0), origNextIFD(0) {};
		inline void clear()
		{
			this->changed = false;
			this->origCount = 0;
			this->origIFDOffset = this->origNextIFD = 0;
			this->tagMap.clear();
		};
	};

	InternalIFDInfo containedIFDs[kTIFF_KnownIFDCount];

	static XMP_Uns8 PickIFD ( XMP_Uns8 ifd, XMP_Uns16 id );
	const InternalTagInfo* FindTagInIFD ( XMP_Uns8 ifd, XMP_Uns16 id ) const;

	void DeleteExistingInfo();

	XMP_Uns32 ProcessMemoryIFD ( XMP_Uns32 ifdOffset, XMP_Uns8 ifd );
	XMP_Uns32 ProcessFileIFD   ( XMP_Uns8 ifd, XMP_Uns32 ifdOffset, XMP_IO* fileRef );

	void ProcessPShop6IFD ( const TIFF_MemoryReader& buriedExif, XMP_Uns8 ifd );

	void* CopyTagToMasterIFD ( const TagInfo& ps6Tag, InternalIFDInfo* masterIFD );

	void PreflightIFDLinkage();

	XMP_Uns32 DetermineVisibleLength();

	XMP_Uns32 DetermineAppendInfo ( XMP_Uns32 appendedOrigin,
									bool      appendedIFDs[kTIFF_KnownIFDCount],
									XMP_Uns32 newIFDOffsets[kTIFF_KnownIFDCount],
									bool      appendAll = false );

	void UpdateMemByAppend  ( XMP_Uns8** newStream_out, XMP_Uns32* newLength_out,
							  bool appendAll = false, XMP_Uns32 extraSpace = 0 );
	void UpdateMemByRewrite ( XMP_Uns8** newStream_out, XMP_Uns32* newLength_out );

	void WriteFileIFD ( XMP_IO* fileRef, InternalIFDInfo & thisIFD );

};	// TIFF_FileWriter

XMP_Bool IsOffsetValid( XMP_Uns32 offset, XMP_Uns32 lowerBound, XMP_Uns32 upperBound );

// =================================================================================================

#endif	// __TIFF_Support_hpp__
