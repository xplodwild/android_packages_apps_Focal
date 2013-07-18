// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/UnicodeConversions.hpp"
#include "source/XIO.hpp"

#include <stdio.h>
#if XMP_WinBuild
	#define snprintf _snprintf
#endif

#include "source/EndianUtils.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4146 )	// unary minus operator applied to unsigned type
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false'
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

// =================================================================================================
/// \file ReconcileTIFF.cpp
/// \brief Utilities to reconcile between XMP and legacy TIFF/Exif metadata.
///
// =================================================================================================

// =================================================================================================

#ifndef SupportOldExifProperties
	#define SupportOldExifProperties	1
	// This controls support of the old Adobe names for things that have official names as of Exif 2.3.
#endif

// =================================================================================================
// Tables of the TIFF/Exif tags that are mapped into XMP. For the most part, the tags have obvious
// mappings based on their IFD, tag number, type and count. These tables do not list tags that are
// mapped as subsidiary parts of others, e.g. TIFF SubSecTime or GPS Info GPSDateStamp. Tags that
// have special mappings are marked by having an empty string for the XMP property name.

// ! These tables have the tags listed in the order of tables 3, 4, 5, and 12 of Exif 2.2, with the
// ! exception of ImageUniqueID (which is listed at the end of the Exif mappings). This order is
// ! very important to consistent checking of the legacy status. The NativeDigest properties list
// ! all possible mapped tags in this order. The NativeDigest strings are compared as a whole, so
// ! the same tags listed in a different order would compare as different.

// ! The sentinel tag value can't be 0, that is a valid GPS Info tag, 0xFFFF is unused so far.

enum {
	kExport_Never = 0,		// Never export.
	kExport_Always = 1,		// Add, modify, or delete.
	kExport_NoDelete = 2,	// Add or modify, do not delete if no XMP.
	kExport_InjectOnly = 3	// Add tag if new, never modify or delete existing values.
};

struct TIFF_MappingToXMP {
	XMP_Uns16    id;
	XMP_Uns16    type;
	XMP_Uns32    count;	// Zero means any.
	XMP_Uns8     exportMode;
	const char * ns;	// The namespace of the mapped XMP property.
	const char * name;	// The name of the mapped XMP property.
};

enum { kAnyCount = 0 };

static const TIFF_MappingToXMP sPrimaryIFDMappings[] = {	// A blank name indicates a special mapping.
	{ /*   256 */ kTIFF_ImageWidth,                kTIFF_ShortOrLongType, 1,         kExport_Never,      kXMP_NS_TIFF,  "ImageWidth" },
	{ /*   257 */ kTIFF_ImageLength,               kTIFF_ShortOrLongType, 1,         kExport_Never,      kXMP_NS_TIFF,  "ImageLength" },
	{ /*   258 */ kTIFF_BitsPerSample,             kTIFF_ShortType,       3,         kExport_Never,      kXMP_NS_TIFF,  "BitsPerSample" },
	{ /*   259 */ kTIFF_Compression,               kTIFF_ShortType,       1,         kExport_Never,      kXMP_NS_TIFF,  "Compression" },
	{ /*   262 */ kTIFF_PhotometricInterpretation, kTIFF_ShortType,       1,         kExport_Never,      kXMP_NS_TIFF,  "PhotometricInterpretation" },
	{ /*   274 */ kTIFF_Orientation,               kTIFF_ShortType,       1,         kExport_NoDelete,   kXMP_NS_TIFF,  "Orientation" },
	{ /*   277 */ kTIFF_SamplesPerPixel,           kTIFF_ShortType,       1,         kExport_Never,      kXMP_NS_TIFF,  "SamplesPerPixel" },
	{ /*   284 */ kTIFF_PlanarConfiguration,       kTIFF_ShortType,       1,         kExport_Never,      kXMP_NS_TIFF,  "PlanarConfiguration" },
	{ /*   529 */ kTIFF_YCbCrCoefficients,         kTIFF_RationalType,    3,         kExport_Never,      kXMP_NS_TIFF,  "YCbCrCoefficients" },
	{ /*   530 */ kTIFF_YCbCrSubSampling,          kTIFF_ShortType,       2,         kExport_Never,      kXMP_NS_TIFF,  "YCbCrSubSampling" },
	{ /*   282 */ kTIFF_XResolution,               kTIFF_RationalType,    1,         kExport_NoDelete,   kXMP_NS_TIFF,  "XResolution" },
	{ /*   283 */ kTIFF_YResolution,               kTIFF_RationalType,    1,         kExport_NoDelete,   kXMP_NS_TIFF,  "YResolution" },
	{ /*   296 */ kTIFF_ResolutionUnit,            kTIFF_ShortType,       1,         kExport_NoDelete,   kXMP_NS_TIFF,  "ResolutionUnit" },
	{ /*   301 */ kTIFF_TransferFunction,          kTIFF_ShortType,       3*256,     kExport_Never,      kXMP_NS_TIFF,  "TransferFunction" },
	{ /*   318 */ kTIFF_WhitePoint,                kTIFF_RationalType,    2,         kExport_Never,      kXMP_NS_TIFF,  "WhitePoint" },
	{ /*   319 */ kTIFF_PrimaryChromaticities,     kTIFF_RationalType,    6,         kExport_Never,      kXMP_NS_TIFF,  "PrimaryChromaticities" },
	{ /*   531 */ kTIFF_YCbCrPositioning,          kTIFF_ShortType,       1,         kExport_Never,      kXMP_NS_TIFF,  "YCbCrPositioning" },
	{ /*   532 */ kTIFF_ReferenceBlackWhite,       kTIFF_RationalType,    6,         kExport_Never,      kXMP_NS_TIFF,  "ReferenceBlackWhite" },
	{ /*   306 */ kTIFF_DateTime,                  kTIFF_ASCIIType,       20,        kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /*   270 */ kTIFF_ImageDescription,          kTIFF_ASCIIType,       kAnyCount, kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /*   271 */ kTIFF_Make,                      kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_TIFF,  "Make" },
	{ /*   272 */ kTIFF_Model,                     kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_TIFF,  "Model" },
	{ /*   305 */ kTIFF_Software,                  kTIFF_ASCIIType,       kAnyCount, kExport_Always,     kXMP_NS_TIFF,  "Software" },	// Has alias to xmp:CreatorTool.
	{ /*   315 */ kTIFF_Artist,                    kTIFF_ASCIIType,       kAnyCount, kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /* 33432 */ kTIFF_Copyright,                 kTIFF_ASCIIType,       kAnyCount, kExport_Always,     "", "" },	// ! Has a special mapping.
	{ 0xFFFF, 0, 0, 0 }	// ! Must end with sentinel.
};

static const TIFF_MappingToXMP sExifIFDMappings[] = {

	// From Exif 2.3 table 7:
	{ /* 36864 */ kTIFF_ExifVersion,               kTIFF_UndefinedType,   4,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 40960 */ kTIFF_FlashpixVersion,           kTIFF_UndefinedType,   4,         kExport_Never,      "", "" },	// ! Has a special mapping.
	{ /* 40961 */ kTIFF_ColorSpace,                kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ColorSpace" },
	{ /* 42240 */ kTIFF_Gamma,                     kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_ExifEX, "Gamma" },
	{ /* 37121 */ kTIFF_ComponentsConfiguration,   kTIFF_UndefinedType,   4,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 37122 */ kTIFF_CompressedBitsPerPixel,    kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "CompressedBitsPerPixel" },
	{ /* 40962 */ kTIFF_PixelXDimension,           kTIFF_ShortOrLongType, 1,         kExport_InjectOnly, kXMP_NS_EXIF,  "PixelXDimension" },
	{ /* 40963 */ kTIFF_PixelYDimension,           kTIFF_ShortOrLongType, 1,         kExport_InjectOnly, kXMP_NS_EXIF,  "PixelYDimension" },
	{ /* 37510 */ kTIFF_UserComment,               kTIFF_UndefinedType,   kAnyCount, kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /* 40964 */ kTIFF_RelatedSoundFile,          kTIFF_ASCIIType,       kAnyCount, kExport_Always,     kXMP_NS_EXIF,  "RelatedSoundFile" },	// ! Exif spec says count of 13.
	{ /* 36867 */ kTIFF_DateTimeOriginal,          kTIFF_ASCIIType,       20,        kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /* 36868 */ kTIFF_DateTimeDigitized,         kTIFF_ASCIIType,       20,        kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /* 42016 */ kTIFF_ImageUniqueID,             kTIFF_ASCIIType,       33,        kExport_InjectOnly, kXMP_NS_EXIF,  "ImageUniqueID" },
	{ /* 42032 */ kTIFF_CameraOwnerName,           kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_ExifEX, "CameraOwnerName" },
	{ /* 42033 */ kTIFF_BodySerialNumber,          kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_ExifEX, "BodySerialNumber" },
	{ /* 42034 */ kTIFF_LensSpecification,         kTIFF_RationalType,    4,         kExport_InjectOnly, kXMP_NS_ExifEX, "LensSpecification" },
	{ /* 42035 */ kTIFF_LensMake,                  kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_ExifEX, "LensMake" },
	{ /* 42036 */ kTIFF_LensModel,                 kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_ExifEX, "LensModel" },
	{ /* 42037 */ kTIFF_LensSerialNumber,          kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_ExifEX, "LensSerialNumber" },

	// From Exif 2.3 table 8:
	{ /* 33434 */ kTIFF_ExposureTime,              kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ExposureTime" },
	{ /* 33437 */ kTIFF_FNumber,                   kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FNumber" },
	{ /* 34850 */ kTIFF_ExposureProgram,           kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ExposureProgram" },
	{ /* 34852 */ kTIFF_SpectralSensitivity,       kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_EXIF,  "SpectralSensitivity" },
	{ /* 34855 */ kTIFF_PhotographicSensitivity,   kTIFF_ShortType,       1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 34856 */ kTIFF_OECF,                      kTIFF_UndefinedType,   kAnyCount, kExport_Never,      "", "" },	// ! Has a special mapping.
	{ /* 34864 */ kTIFF_SensitivityType,           kTIFF_ShortType,       1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 34865 */ kTIFF_StandardOutputSensitivity, kTIFF_LongType,        1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 34866 */ kTIFF_RecommendedExposureIndex,  kTIFF_LongType,        1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 34867 */ kTIFF_ISOSpeed,                  kTIFF_LongType,        1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 34868 */ kTIFF_ISOSpeedLatitudeyyy,       kTIFF_LongType,        1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 34869 */ kTIFF_ISOSpeedLatitudezzz,       kTIFF_LongType,        1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 37377 */ kTIFF_ShutterSpeedValue,         kTIFF_SRationalType,   1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ShutterSpeedValue" },
	{ /* 37378 */ kTIFF_ApertureValue,             kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ApertureValue" },
	{ /* 37379 */ kTIFF_BrightnessValue,           kTIFF_SRationalType,   1,         kExport_InjectOnly, kXMP_NS_EXIF,  "BrightnessValue" },
	{ /* 37380 */ kTIFF_ExposureBiasValue,         kTIFF_SRationalType,   1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ExposureBiasValue" },
	{ /* 37381 */ kTIFF_MaxApertureValue,          kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "MaxApertureValue" },
	{ /* 37382 */ kTIFF_SubjectDistance,           kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "SubjectDistance" },
	{ /* 37383 */ kTIFF_MeteringMode,              kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "MeteringMode" },
	{ /* 37384 */ kTIFF_LightSource,               kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "LightSource" },
	{ /* 37385 */ kTIFF_Flash,                     kTIFF_ShortType,       1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 37386 */ kTIFF_FocalLength,               kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FocalLength" },
	{ /* 37396 */ kTIFF_SubjectArea,               kTIFF_ShortType,       kAnyCount, kExport_Never,      kXMP_NS_EXIF,  "SubjectArea" },	// ! Actually 2..4.
	{ /* 41483 */ kTIFF_FlashEnergy,               kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FlashEnergy" },
	{ /* 41484 */ kTIFF_SpatialFrequencyResponse,  kTIFF_UndefinedType,   kAnyCount, kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 41486 */ kTIFF_FocalPlaneXResolution,     kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FocalPlaneXResolution" },
	{ /* 41487 */ kTIFF_FocalPlaneYResolution,     kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FocalPlaneYResolution" },
	{ /* 41488 */ kTIFF_FocalPlaneResolutionUnit,  kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FocalPlaneResolutionUnit" },
	{ /* 41492 */ kTIFF_SubjectLocation,           kTIFF_ShortType,       2,         kExport_Never,      kXMP_NS_EXIF,  "SubjectLocation" },
	{ /* 41493 */ kTIFF_ExposureIndex,             kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ExposureIndex" },
	{ /* 41495 */ kTIFF_SensingMethod,             kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "SensingMethod" },
	{ /* 41728 */ kTIFF_FileSource,                kTIFF_UndefinedType,   1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 41729 */ kTIFF_SceneType,                 kTIFF_UndefinedType,   1,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 41730 */ kTIFF_CFAPattern,                kTIFF_UndefinedType,   kAnyCount, kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 41985 */ kTIFF_CustomRendered,            kTIFF_ShortType,       1,         kExport_Never,      kXMP_NS_EXIF,  "CustomRendered" },
	{ /* 41986 */ kTIFF_ExposureMode,              kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "ExposureMode" },
	{ /* 41987 */ kTIFF_WhiteBalance,              kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "WhiteBalance" },
	{ /* 41988 */ kTIFF_DigitalZoomRatio,          kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "DigitalZoomRatio" },
	{ /* 41989 */ kTIFF_FocalLengthIn35mmFilm,     kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "FocalLengthIn35mmFilm" },
	{ /* 41990 */ kTIFF_SceneCaptureType,          kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "SceneCaptureType" },
	{ /* 41991 */ kTIFF_GainControl,               kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GainControl" },
	{ /* 41992 */ kTIFF_Contrast,                  kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "Contrast" },
	{ /* 41993 */ kTIFF_Saturation,                kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "Saturation" },
	{ /* 41994 */ kTIFF_Sharpness,                 kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "Sharpness" },
	{ /* 41995 */ kTIFF_DeviceSettingDescription,  kTIFF_UndefinedType,   kAnyCount, kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /* 41996 */ kTIFF_SubjectDistanceRange,      kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "SubjectDistanceRange" },

	{ 0xFFFF, 0, 0, 0 }	// ! Must end with sentinel.
};

static const TIFF_MappingToXMP sGPSInfoIFDMappings[] = {
	{ /*     0 */ kTIFF_GPSVersionID,              kTIFF_ByteType,        4,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /*     2 */ kTIFF_GPSLatitude,               kTIFF_RationalType,    3,         kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /*     4 */ kTIFF_GPSLongitude,              kTIFF_RationalType,    3,         kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /*     5 */ kTIFF_GPSAltitudeRef,            kTIFF_ByteType,        1,         kExport_Always,     kXMP_NS_EXIF,  "GPSAltitudeRef" },
	{ /*     6 */ kTIFF_GPSAltitude,               kTIFF_RationalType,    1,         kExport_Always,     kXMP_NS_EXIF,  "GPSAltitude" },
	{ /*     7 */ kTIFF_GPSTimeStamp,              kTIFF_RationalType,    3,         kExport_Always,     "", "" },	// ! Has a special mapping.
	{ /*     8 */ kTIFF_GPSSatellites,             kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_EXIF,  "GPSSatellites" },
	{ /*     9 */ kTIFF_GPSStatus,                 kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSStatus" },
	{ /*    10 */ kTIFF_GPSMeasureMode,            kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSMeasureMode" },
	{ /*    11 */ kTIFF_GPSDOP,                    kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSDOP" },
	{ /*    12 */ kTIFF_GPSSpeedRef,               kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSSpeedRef" },
	{ /*    13 */ kTIFF_GPSSpeed,                  kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSSpeed" },
	{ /*    14 */ kTIFF_GPSTrackRef,               kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSTrackRef" },
	{ /*    15 */ kTIFF_GPSTrack,                  kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSTrack" },
	{ /*    16 */ kTIFF_GPSImgDirectionRef,        kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSImgDirectionRef" },
	{ /*    17 */ kTIFF_GPSImgDirection,           kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSImgDirection" },
	{ /*    18 */ kTIFF_GPSMapDatum,               kTIFF_ASCIIType,       kAnyCount, kExport_InjectOnly, kXMP_NS_EXIF,  "GPSMapDatum" },
	{ /*    20 */ kTIFF_GPSDestLatitude,           kTIFF_RationalType,    3,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /*    22 */ kTIFF_GPSDestLongitude,          kTIFF_RationalType,    3,         kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /*    23 */ kTIFF_GPSDestBearingRef,         kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSDestBearingRef" },
	{ /*    24 */ kTIFF_GPSDestBearing,            kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSDestBearing" },
	{ /*    25 */ kTIFF_GPSDestDistanceRef,        kTIFF_ASCIIType,       2,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSDestDistanceRef" },
	{ /*    26 */ kTIFF_GPSDestDistance,           kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSDestDistance" },
	{ /*    27 */ kTIFF_GPSProcessingMethod,       kTIFF_UndefinedType,   kAnyCount, kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /*    28 */ kTIFF_GPSAreaInformation,        kTIFF_UndefinedType,   kAnyCount, kExport_InjectOnly, "", "" },	// ! Has a special mapping.
	{ /*    30 */ kTIFF_GPSDifferential,           kTIFF_ShortType,       1,         kExport_InjectOnly, kXMP_NS_EXIF,  "GPSDifferential" },
	{ /*    31 */ kTIFF_GPSHPositioningError,      kTIFF_RationalType,    1,         kExport_InjectOnly, kXMP_NS_ExifEX, "GPSHPositioningError" },
	{ 0xFFFF, 0, 0, 0 }	// ! Must end with sentinel.
};

// =================================================================================================

static void	// ! Needed by Import2WayExif
ExportTIFF_Date ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp, TIFF_Manager * tiff, XMP_Uns16 mainID );

// =================================================================================================

static XMP_Uns32 GatherInt ( const char * strPtr, size_t count )
{
	XMP_Uns32 value = 0;
	const char * strEnd = strPtr + count;

	while ( strPtr < strEnd ) {
		char ch = *strPtr;
		if ( (ch < '0') || (ch > '9') ) break;
		value = value*10 + (ch - '0');
		++strPtr;
	}

	return value;

}	// GatherInt

// =================================================================================================

static size_t TrimTrailingSpaces ( char * firstChar, size_t origLen )
{
	if ( origLen == 0 ) return 0;

	char * lastChar  = firstChar + origLen - 1;
	if ( (*lastChar != ' ') && (*lastChar != 0) ) return origLen;	// Nothing to do.
	
	while ( (firstChar <= lastChar) && ((*lastChar == ' ') || (*lastChar == 0)) ) --lastChar;
	
	XMP_Assert ( (lastChar == firstChar-1) ||
				 ((lastChar >= firstChar) && (*lastChar != ' ') && (*lastChar != 0)) );
	
	size_t newLen = (size_t)((lastChar+1) - firstChar);
	XMP_Assert ( newLen <= origLen );

	if ( newLen < origLen ) {
		++lastChar;
		*lastChar = 0;
	}

	return newLen;

}	// TrimTrailingSpaces

static void TrimTrailingSpaces ( TIFF_Manager::TagInfo * info )
{
	info->dataLen = (XMP_Uns32) TrimTrailingSpaces ( (char*)info->dataPtr, (size_t)info->dataLen );
}

static void TrimTrailingSpaces ( std::string * stdstr )
{
	size_t origLen = stdstr->size();
	size_t newLen = TrimTrailingSpaces ( (char*)stdstr->c_str(), origLen );
	if ( newLen != origLen ) stdstr->erase ( newLen );
}

// =================================================================================================

bool PhotoDataUtils::GetNativeInfo ( const TIFF_Manager & exif, XMP_Uns8 ifd, XMP_Uns16 id, TIFF_Manager::TagInfo * info )
{
	bool haveExif = exif.GetTag ( ifd, id, info );

	if ( haveExif ) {

		XMP_Uns32 i;
		char * chPtr;
		
		XMP_Assert ( (info->dataPtr != 0) || (info->dataLen == 0) );	// Null pointer requires zero length.

		bool isDate = ((id == kTIFF_DateTime) || (id == kTIFF_DateTimeOriginal) || (id == kTIFF_DateTimeOriginal));

		for ( i = 0, chPtr = (char*)info->dataPtr; i < info->dataLen; ++i, ++chPtr ) {
			if ( isDate && (*chPtr == ':') ) continue;	// Ignore colons, empty dates have spaces and colons.
			if ( (*chPtr != ' ') && (*chPtr != 0) ) break;	// Break if the Exif value is non-empty.
		}

		if ( i == info->dataLen ) {
			haveExif = false;	// Ignore empty Exif.
		} else {
			TrimTrailingSpaces ( info );
			if ( info->dataLen == 0 ) haveExif = false;
		}

	}

	return haveExif;

}	// PhotoDataUtils::GetNativeInfo

// =================================================================================================

size_t PhotoDataUtils::GetNativeInfo ( const IPTC_Manager & iptc, XMP_Uns8 id, int digestState, bool haveXMP, IPTC_Manager::DataSetInfo * info )
{
	size_t iptcCount = 0;

	if ( (digestState == kDigestDiffers) || ((digestState == kDigestMissing) && (! haveXMP)) ) {
		iptcCount = iptc.GetDataSet ( id, info );
	}
	
	if ( ignoreLocalText && (iptcCount > 0) && (! iptc.UsingUTF8()) ) {
		// Check to see if the new value(s) should be ignored.
		size_t i;
		IPTC_Manager::DataSetInfo tmpInfo;
		for ( i = 0; i < iptcCount; ++i ) {
			(void) iptc.GetDataSet ( id, &tmpInfo, i );
			if ( ReconcileUtils::IsASCII ( tmpInfo.dataPtr, tmpInfo.dataLen ) ) break;
		}
		if ( i == iptcCount ) iptcCount = 0;	// Return 0 if value(s) should be ignored.
	}

	return iptcCount;

}	// PhotoDataUtils::GetNativeInfo

// =================================================================================================

bool PhotoDataUtils::IsValueDifferent ( const TIFF_Manager::TagInfo & exifInfo, const std::string & xmpValue, std::string * exifValue )
{
	if ( exifInfo.dataLen == 0 ) return false;	// Ignore empty Exif values.

	if ( ReconcileUtils::IsUTF8 ( exifInfo.dataPtr, exifInfo.dataLen ) ) {	// ! Note that ASCII is UTF-8.
		exifValue->assign ( (char*)exifInfo.dataPtr, exifInfo.dataLen );
	} else {
		if ( ignoreLocalText ) return false;
		ReconcileUtils::LocalToUTF8 ( exifInfo.dataPtr, exifInfo.dataLen, exifValue );
	}

	return (*exifValue != xmpValue);

}	// PhotoDataUtils::IsValueDifferent

// =================================================================================================

bool PhotoDataUtils::IsValueDifferent ( const IPTC_Manager & newIPTC, const IPTC_Manager & oldIPTC, XMP_Uns8 id )
{
	IPTC_Manager::DataSetInfo newInfo;
	size_t newCount = newIPTC.GetDataSet ( id, &newInfo );
	if ( newCount == 0 ) return false;	// Ignore missing new IPTC values.

	IPTC_Manager::DataSetInfo oldInfo;
	size_t oldCount = oldIPTC.GetDataSet ( id, &oldInfo );
	if ( oldCount == 0 ) return true;	// Missing old IPTC values differ.
	
	if ( newCount != oldCount ) return true;

	std::string oldStr, newStr;

	for ( newCount = 0; newCount < oldCount; ++newCount ) {

		if ( ignoreLocalText & (! newIPTC.UsingUTF8()) ) {	// Check to see if the new value should be ignored.
			(void) newIPTC.GetDataSet ( id, &newInfo, newCount );
			if ( ! ReconcileUtils::IsASCII ( newInfo.dataPtr, newInfo.dataLen ) ) continue;
		}

		(void) newIPTC.GetDataSet_UTF8 ( id, &newStr, newCount );
		(void) oldIPTC.GetDataSet_UTF8 ( id, &oldStr, newCount );
		if ( newStr.size() == 0 ) continue;	// Ignore empty new IPTC.
		if ( newStr != oldStr ) break;

	}

	return ( newCount != oldCount );	// Not different if all values matched.
	
}	// PhotoDataUtils::IsValueDifferent

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportSingleTIFF_Short
// ======================

static void
ImportSingleTIFF_Short ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns16 binValue = GetUns16AsIs ( tagInfo.dataPtr );
		if ( ! nativeEndian ) binValue = Flip2 ( binValue );

		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hu", binValue );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Short

// =================================================================================================
// ImportSingleTIFF_Long
// =====================

static void
ImportSingleTIFF_Long ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 binValue = GetUns32AsIs ( tagInfo.dataPtr );
		if ( ! nativeEndian ) binValue = Flip4 ( binValue );

		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%lu", (unsigned long)binValue );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Long

// =================================================================================================
// ImportSingleTIFF_Rational
// =========================

static void
ImportSingleTIFF_Rational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
							SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
		XMP_Uns32 binNum   = GetUns32AsIs ( &binPtr[0] );
		XMP_Uns32 binDenom = GetUns32AsIs ( &binPtr[1] );
		if ( ! nativeEndian ) {
			binNum = Flip4 ( binNum );
			binDenom = Flip4 ( binDenom );
		}

		char strValue[40];
		snprintf ( strValue, sizeof(strValue), "%lu/%lu", (unsigned long)binNum, (unsigned long)binDenom );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Rational

// =================================================================================================
// ImportSingleTIFF_SRational
// ==========================

static void
ImportSingleTIFF_SRational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
							 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

#if SUNOS_SPARC
        XMP_Uns32  binPtr[2];
        memcpy(&binPtr, tagInfo.dataPtr, sizeof(XMP_Uns32)*2);
#else
	XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
#endif //#if SUNOS_SPARC
		XMP_Int32 binNum   = GetUns32AsIs ( &binPtr[0] );
		XMP_Int32 binDenom = GetUns32AsIs ( &binPtr[1] );
		if ( ! nativeEndian ) {
			Flip4 ( &binNum );
			Flip4 ( &binDenom );
		}

		char strValue[40];
		snprintf ( strValue, sizeof(strValue), "%ld/%ld", (unsigned long)binNum, (unsigned long)binDenom );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SRational

// =================================================================================================
// ImportSingleTIFF_ASCII
// ======================

static void
ImportSingleTIFF_ASCII ( const TIFF_Manager::TagInfo & tagInfo,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.
	
		TrimTrailingSpaces ( (TIFF_Manager::TagInfo*) &tagInfo );
		if ( tagInfo.dataLen == 0 ) return;	// Ignore empty tags.

		const char * chPtr  = (const char *)tagInfo.dataPtr;
		const bool   hasNul = (chPtr[tagInfo.dataLen-1] == 0);
		const bool   isUTF8 = ReconcileUtils::IsUTF8 ( chPtr, tagInfo.dataLen );

		if ( isUTF8 && hasNul ) {
			xmp->SetProperty ( xmpNS, xmpProp, chPtr );
		} else {
			std::string strValue;
			if ( isUTF8 ) {
				strValue.assign ( chPtr, tagInfo.dataLen );
			} else {
				if ( ignoreLocalText ) return;
				ReconcileUtils::LocalToUTF8 ( chPtr, tagInfo.dataLen, &strValue );
			}
			xmp->SetProperty ( xmpNS, xmpProp, strValue.c_str() );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_ASCII

// =================================================================================================
// ImportSingleTIFF_Byte
// =====================

static void
ImportSingleTIFF_Byte ( const TIFF_Manager::TagInfo & tagInfo,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns8 binValue = *((XMP_Uns8*)tagInfo.dataPtr);

		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hu", (XMP_Uns16)binValue );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Byte

// =================================================================================================
// ImportSingleTIFF_SByte
// ======================

static void
ImportSingleTIFF_SByte ( const TIFF_Manager::TagInfo & tagInfo,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int8 binValue = *((XMP_Int8*)tagInfo.dataPtr);

		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hd", (short)binValue );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SByte

// =================================================================================================
// ImportSingleTIFF_SShort
// =======================

static void
ImportSingleTIFF_SShort ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int16 binValue = *((XMP_Int16*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip2 ( &binValue );

		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hd", binValue );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SShort

// =================================================================================================
// ImportSingleTIFF_SLong
// ======================

static void
ImportSingleTIFF_SLong ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 binValue = *((XMP_Int32*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip4 ( &binValue );

		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%ld", (long)binValue );	// AUDIT: Using sizeof(strValue) is safe.

		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SLong

// =================================================================================================
// ImportSingleTIFF_Float
// ======================

static void
ImportSingleTIFF_Float ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		float binValue = *((float*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip4 ( &binValue );

		xmp->SetProperty_Float ( xmpNS, xmpProp, binValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Float

// =================================================================================================
// ImportSingleTIFF_Double
// =======================

static void
ImportSingleTIFF_Double ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		double binValue = *((double*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip8 ( &binValue );

		xmp->SetProperty_Float ( xmpNS, xmpProp, binValue );	// ! Yes, SetProperty_Float.

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Double

// =================================================================================================
// ImportSingleTIFF
// ================

static void
ImportSingleTIFF ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
				   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{

	// We've got a tag to map to XMP, decide how based on actual type and the expected count. Using
	// the actual type eliminates a ShortOrLong case. Using the expected count is needed to know
	// whether to create an XMP array. The actual count for an array could be 1. Put the most
	// common cases first for better iCache utilization.

	switch ( tagInfo.type ) {

		case kTIFF_ShortType :
			ImportSingleTIFF_Short ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_LongType :
			ImportSingleTIFF_Long ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_RationalType :
			ImportSingleTIFF_Rational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SRationalType :
			ImportSingleTIFF_SRational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ASCIIType :
			ImportSingleTIFF_ASCII ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ByteType :
			ImportSingleTIFF_Byte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SByteType :
			ImportSingleTIFF_SByte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SShortType :
			ImportSingleTIFF_SShort ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SLongType :
			ImportSingleTIFF_SLong ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_FloatType :
			ImportSingleTIFF_Float ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_DoubleType :
			ImportSingleTIFF_Double ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

	}

}	// ImportSingleTIFF

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportArrayTIFF_Short
// =====================

static void
ImportArrayTIFF_Short ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns16 * binPtr = (XMP_Uns16*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			XMP_Uns16 binValue = *binPtr;
			if ( ! nativeEndian ) binValue = Flip2 ( binValue );

			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hu", binValue );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Short

// =================================================================================================
// ImportArrayTIFF_Long
// ====================

static void
ImportArrayTIFF_Long ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
					   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			XMP_Uns32 binValue = *binPtr;
			if ( ! nativeEndian ) binValue = Flip4 ( binValue );

			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%lu", (unsigned long)binValue );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Long

// =================================================================================================
// ImportArrayTIFF_Rational
// ========================

static void
ImportArrayTIFF_Rational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, binPtr += 2 ) {

			XMP_Uns32 binNum  = GetUns32AsIs ( &binPtr[0] );
			XMP_Uns32 binDenom = GetUns32AsIs ( &binPtr[1] );
			if ( ! nativeEndian ) {
				binNum = Flip4 ( binNum );
				binDenom = Flip4 ( binDenom );
			}

			char strValue[40];
			snprintf ( strValue, sizeof(strValue), "%lu/%lu", (unsigned long)binNum, (unsigned long)binDenom );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Rational

// =================================================================================================
// ImportArrayTIFF_SRational
// =========================

static void
ImportArrayTIFF_SRational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
							SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 * binPtr = (XMP_Int32*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, binPtr += 2 ) {

			XMP_Int32 binNum   = binPtr[0];
			XMP_Int32 binDenom = binPtr[1];
			if ( ! nativeEndian ) {
				Flip4 ( &binNum );
				Flip4 ( &binDenom );
			}

			char strValue[40];
			snprintf ( strValue, sizeof(strValue), "%ld/%ld", (long)binNum, (long)binDenom );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SRational

// =================================================================================================
// ImportArrayTIFF_ASCII
// =====================

static void
ImportArrayTIFF_ASCII ( const TIFF_Manager::TagInfo & tagInfo,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.
	
		TrimTrailingSpaces ( (TIFF_Manager::TagInfo*) &tagInfo );
		if ( tagInfo.dataLen == 0 ) return;	// Ignore empty tags.

		const char * chPtr  = (const char *)tagInfo.dataPtr;
		const char * chEnd  = chPtr + tagInfo.dataLen;
		const bool   hasNul = (chPtr[tagInfo.dataLen-1] == 0);
		const bool   isUTF8 = ReconcileUtils::IsUTF8 ( chPtr, tagInfo.dataLen );

		std::string  strValue;

		if ( (! isUTF8) || (! hasNul) ) {
			if ( isUTF8 ) {
				strValue.assign ( chPtr, tagInfo.dataLen );
			} else {
				if ( ignoreLocalText ) return;
				ReconcileUtils::LocalToUTF8 ( chPtr, tagInfo.dataLen, &strValue );
			}
			chPtr = strValue.c_str();
			chEnd = chPtr + strValue.size();
		}
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( ; chPtr < chEnd; chPtr += (strlen(chPtr) + 1) ) {
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, chPtr );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_ASCII

// =================================================================================================
// ImportArrayTIFF_Byte
// ====================

static void
ImportArrayTIFF_Byte ( const TIFF_Manager::TagInfo & tagInfo,
					   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns8 * binPtr = (XMP_Uns8*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			XMP_Uns8 binValue = *binPtr;

			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hu", (XMP_Uns16)binValue );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Byte

// =================================================================================================
// ImportArrayTIFF_SByte
// =====================

static void
ImportArrayTIFF_SByte ( const TIFF_Manager::TagInfo & tagInfo,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int8 * binPtr = (XMP_Int8*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			XMP_Int8 binValue = *binPtr;

			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hd", (short)binValue );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SByte

// =================================================================================================
// ImportArrayTIFF_SShort
// ======================

static void
ImportArrayTIFF_SShort ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int16 * binPtr = (XMP_Int16*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			XMP_Int16 binValue = *binPtr;
			if ( ! nativeEndian ) Flip2 ( &binValue );

			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hd", binValue );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SShort

// =================================================================================================
// ImportArrayTIFF_SLong
// =====================

static void
ImportArrayTIFF_SLong ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 * binPtr = (XMP_Int32*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			XMP_Int32 binValue = *binPtr;
			if ( ! nativeEndian ) Flip4 ( &binValue );

			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%ld", (long)binValue );	// AUDIT: Using sizeof(strValue) is safe.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SLong

// =================================================================================================
// ImportArrayTIFF_Float
// =====================

static void
ImportArrayTIFF_Float ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		float * binPtr = (float*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			float binValue = *binPtr;
			if ( ! nativeEndian ) Flip4 ( &binValue );

			std::string strValue;
			SXMPUtils::ConvertFromFloat ( binValue, "", &strValue );

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue.c_str() );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Float

// =================================================================================================
// ImportArrayTIFF_Double
// ======================

static void
ImportArrayTIFF_Double ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		double * binPtr = (double*)tagInfo.dataPtr;
		
		xmp->DeleteProperty ( xmpNS, xmpProp );	// ! Don't keep appending, create a new array.

		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {

			double binValue = *binPtr;
			if ( ! nativeEndian ) Flip8 ( &binValue );

			std::string strValue;
			SXMPUtils::ConvertFromFloat ( binValue, "", &strValue );	// ! Yes, ConvertFromFloat.

			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue.c_str() );

		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Double

// =================================================================================================
// ImportArrayTIFF
// ===============

static void
ImportArrayTIFF ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
				  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{

	// We've got a tag to map to XMP, decide how based on actual type and the expected count. Using
	// the actual type eliminates a ShortOrLong case. Using the expected count is needed to know
	// whether to create an XMP array. The actual count for an array could be 1. Put the most
	// common cases first for better iCache utilization.

	switch ( tagInfo.type ) {

		case kTIFF_ShortType :
			ImportArrayTIFF_Short ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_LongType :
			ImportArrayTIFF_Long ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_RationalType :
			ImportArrayTIFF_Rational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SRationalType :
			ImportArrayTIFF_SRational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ASCIIType :
			ImportArrayTIFF_ASCII ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ByteType :
			ImportArrayTIFF_Byte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SByteType :
			ImportArrayTIFF_SByte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SShortType :
			ImportArrayTIFF_SShort ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SLongType :
			ImportArrayTIFF_SLong ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_FloatType :
			ImportArrayTIFF_Float ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_DoubleType :
			ImportArrayTIFF_Double ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

	}

}	// ImportArrayTIFF

// =================================================================================================
// ImportTIFF_CheckStandardMapping
// ===============================

static bool
ImportTIFF_CheckStandardMapping ( const TIFF_Manager::TagInfo & tagInfo, const TIFF_MappingToXMP & mapInfo )
{
	XMP_Assert ( (kTIFF_ByteType <= tagInfo.type) && (tagInfo.type <= kTIFF_LastType) );
	XMP_Assert ( mapInfo.type <= kTIFF_LastType );

	if ( (tagInfo.type < kTIFF_ByteType) || (tagInfo.type > kTIFF_LastType) ) return false;

	if ( tagInfo.type != mapInfo.type ) {
		// Be tolerant of reasonable mismatches among numeric types.
		if ( kTIFF_IsIntegerType[mapInfo.type] ) {
			if ( ! kTIFF_IsIntegerType[tagInfo.type] ) return false;
		} else if ( kTIFF_IsRationalType[mapInfo.type] ) {
			if ( ! kTIFF_IsRationalType[tagInfo.type] ) return false;
		} else if ( kTIFF_IsFloatType[mapInfo.type] ) {
			if ( ! kTIFF_IsFloatType[tagInfo.type] ) return false;
		} else {
			return false;
		}
	}

	if ( (tagInfo.count != mapInfo.count) &&	// Maybe there is a problem because the counts don't match.
		 // (mapInfo.count != kAnyCount) &&		... don't need this because of the new check below ...
		 (mapInfo.count == 1) ) return false;	// Be tolerant of mismatch in expected array size.

	return true;

}	// ImportTIFF_CheckStandardMapping

// =================================================================================================
// ImportTIFF_StandardMappings
// ===========================

static void
ImportTIFF_StandardMappings ( XMP_Uns8 ifd, const TIFF_Manager & tiff, SXMPMeta * xmp )
{
	const bool nativeEndian = tiff.IsNativeEndian();
	TIFF_Manager::TagInfo tagInfo;

	const TIFF_MappingToXMP * mappings = 0;

	if ( ifd == kTIFF_PrimaryIFD ) {
		mappings = sPrimaryIFDMappings;
	} else if ( ifd == kTIFF_ExifIFD ) {
		mappings = sExifIFDMappings;
	} else if ( ifd == kTIFF_GPSInfoIFD ) {
		mappings = sGPSInfoIFDMappings;
	} else {
		XMP_Throw ( "Invalid IFD for standard mappings", kXMPErr_InternalFailure );
	}

	for ( size_t i = 0; mappings[i].id != 0xFFFF; ++i ) {

		try {	// Don't let errors with one stop the others.

			const TIFF_MappingToXMP & mapInfo =  mappings[i];
			const bool mapSingle = ((mapInfo.count == 1) || (mapInfo.type == kTIFF_ASCIIType));

			if ( mapInfo.name[0] == 0 ) continue;	// Skip special mappings, handled higher up.
			
			bool found = tiff.GetTag ( ifd, mapInfo.id, &tagInfo );
			if ( ! found ) continue;

			XMP_Assert ( tagInfo.type != kTIFF_UndefinedType );	// These must have a special mapping.
			if ( tagInfo.type == kTIFF_UndefinedType ) continue;
			if ( ! ImportTIFF_CheckStandardMapping ( tagInfo, mapInfo ) ) continue;

			if ( mapSingle ) {
				ImportSingleTIFF ( tagInfo, nativeEndian, xmp, mapInfo.ns, mapInfo.name );
			} else {
				ImportArrayTIFF ( tagInfo, nativeEndian, xmp, mapInfo.ns, mapInfo.name );
			}

		} catch ( ... ) {

			// Do nothing, let other imports proceed.
			// ? Notify client?

		}

	}

}	// ImportTIFF_StandardMappings

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportTIFF_Date
// ===============
//
// Convert an Exif 2.2 master date/time tag plus associated fractional seconds to an XMP date/time.
// The Exif date/time part is a 20 byte ASCII value formatted as "YYYY:MM:DD HH:MM:SS" with a
// terminating nul. Any of the numeric portions can be blanks if unknown. The fractional seconds
// are a nul terminated ASCII string with possible space padding. They are literally the fractional
// part, the digits that would be to the right of the decimal point.

static void
ImportTIFF_Date ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & dateInfo,
				  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	XMP_Uns16 secID;
	switch ( dateInfo.id ) {
		case kTIFF_DateTime          : secID = kTIFF_SubSecTime;			break;
		case kTIFF_DateTimeOriginal  : secID = kTIFF_SubSecTimeOriginal;	break;
		case kTIFF_DateTimeDigitized : secID = kTIFF_SubSecTimeDigitized;	break;
	}
	
	try {	// Don't let errors with one stop the others.
	
		if ( (dateInfo.type != kTIFF_ASCIIType) || (dateInfo.count != 20) ) return;

		const char * dateStr = (const char *) dateInfo.dataPtr;
		if ( (dateStr[4] != ':')  || (dateStr[7] != ':')  ||
			 (dateStr[10] != ' ') || (dateStr[13] != ':') || (dateStr[16] != ':') ) return;

		XMP_DateTime binValue;

		binValue.year   = GatherInt ( &dateStr[0], 4 );
		binValue.month  = GatherInt ( &dateStr[5], 2 );
		binValue.day    = GatherInt ( &dateStr[8], 2 );
		if ( (binValue.year != 0) | (binValue.month != 0) | (binValue.day != 0) ) binValue.hasDate = true;

		binValue.hour   = GatherInt ( &dateStr[11], 2 );
		binValue.minute = GatherInt ( &dateStr[14], 2 );
		binValue.second = GatherInt ( &dateStr[17], 2 );
		binValue.nanoSecond = 0;	// Get the fractional seconds later.
		if ( (binValue.hour != 0) | (binValue.minute != 0) | (binValue.second != 0) ) binValue.hasTime = true;

		binValue.tzSign = 0;	// ! Separate assignment, avoid VS integer truncation warning.
		binValue.tzHour = binValue.tzMinute = 0;
		binValue.hasTimeZone = false;	// Exif times have no zone.

		// *** Consider looking at the TIFF/EP TimeZoneOffset tag?

		TIFF_Manager::TagInfo secInfo;
		bool found = tiff.GetTag ( kTIFF_ExifIFD, secID, &secInfo );	// ! Subseconds are all in the Exif IFD.

		if ( found && (secInfo.type == kTIFF_ASCIIType) ) {
			const char * fracPtr = (const char *) secInfo.dataPtr;
			binValue.nanoSecond = GatherInt ( fracPtr, secInfo.dataLen );
			size_t digits = 0;
			for ( ; (('0' <= *fracPtr) && (*fracPtr <= '9')); ++fracPtr ) ++digits;
			for ( ; digits < 9; ++digits ) binValue.nanoSecond *= 10;
			if ( binValue.nanoSecond != 0 ) binValue.hasTime = true;
		}

		xmp->SetProperty_Date ( xmpNS, xmpProp, binValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_Date

// =================================================================================================
// ImportTIFF_LocTextASCII
// =======================

static void
ImportTIFF_LocTextASCII ( const TIFF_Manager & tiff, XMP_Uns8 ifd, XMP_Uns16 tagID,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		TIFF_Manager::TagInfo tagInfo;

		bool found = tiff.GetTag ( ifd, tagID, &tagInfo );
		if ( (! found) || (tagInfo.type != kTIFF_ASCIIType) ) return;

		TrimTrailingSpaces ( (TIFF_Manager::TagInfo*) &tagInfo );
		if ( tagInfo.dataLen == 0 ) return;	// Ignore empty tags.

		const char * chPtr  = (const char *)tagInfo.dataPtr;
		const bool   hasNul = (chPtr[tagInfo.dataLen-1] == 0);
		const bool   isUTF8 = ReconcileUtils::IsUTF8 ( chPtr, tagInfo.dataLen );

		if ( isUTF8 && hasNul ) {
			xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", chPtr );
		} else {
			std::string strValue;
			if ( isUTF8 ) {
				strValue.assign ( chPtr, tagInfo.dataLen );
			} else {
				if ( ignoreLocalText ) return;
				ReconcileUtils::LocalToUTF8 ( chPtr, tagInfo.dataLen, &strValue );
			}
			xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", strValue.c_str() );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_LocTextASCII

// =================================================================================================
// ImportTIFF_EncodedString
// ========================

static void
ImportTIFF_EncodedString ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & tagInfo,
						   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp, bool isLangAlt = false )
{
	try {	// Don't let errors with one stop the others.

		std::string strValue;

		bool ok = tiff.DecodeString ( tagInfo.dataPtr, tagInfo.dataLen, &strValue );
		if ( ! ok ) return;

		TrimTrailingSpaces ( &strValue );
		if ( strValue.empty() ) return;

		if ( ! isLangAlt ) {
			xmp->SetProperty ( xmpNS, xmpProp, strValue.c_str() );
		} else {
			xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", strValue.c_str() );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_EncodedString

// =================================================================================================
// ImportTIFF_Flash
// ================

static void
ImportTIFF_Flash ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
				   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns16 binValue = GetUns16AsIs ( tagInfo.dataPtr );
		if ( ! nativeEndian ) binValue = Flip2 ( binValue );

		bool fired    = (bool)(binValue & 1);	// Avoid implicit 0/1 conversion.
		int  rtrn     = (binValue >> 1) & 3;
		int  mode     = (binValue >> 3) & 3;
		bool function = (bool)((binValue >> 5) & 1);
		bool redEye   = (bool)((binValue >> 6) & 1);

		static const char * sTwoBits[] = { "0", "1", "2", "3" };

		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Fired", (fired ? kXMP_TrueStr : kXMP_FalseStr) );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Return", sTwoBits[rtrn] );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Mode", sTwoBits[mode] );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Function", (function ? kXMP_TrueStr : kXMP_FalseStr) );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "RedEyeMode", (redEye ? kXMP_TrueStr : kXMP_FalseStr) );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_Flash

// =================================================================================================
// ImportConversionTable
// =====================
//
// Although the XMP for the OECF and SFR tables is the same, the Exif is not. The OECF table has
// signed rational values and the SFR table has unsigned. But they are otherwise the same.

static void
ImportConversionTable ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
					    SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	const bool isSigned = (tagInfo.id == kTIFF_OECF);
	XMP_Assert ( (tagInfo.id == kTIFF_OECF) || (tagInfo.id == kTIFF_SpatialFrequencyResponse) );
	
	xmp->DeleteProperty ( xmpNS, xmpProp );
	
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;

		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! nativeEndian ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}

		char buffer[40];

		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );

		std::string arrayPath;

		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Names", &arrayPath );

		bytePtr += 4;	// Move to the list of names. Don't convert from local text, should really be ASCII.
		for ( size_t i = columns; i > 0; --i ) {
			size_t nameLen = strlen((XMP_StringPtr)bytePtr) + 1;	// ! Include the terminating nul.
			if ( (bytePtr + nameLen) > byteEnd ) XMP_Throw ( "OECF-SFR name overflow", kXMPErr_BadValue );
			if ( ! ReconcileUtils::IsUTF8 ( bytePtr, nameLen ) ) XMP_Throw ( "OECF-SFR name error", kXMPErr_BadValue );
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, (XMP_StringPtr)bytePtr );
			bytePtr += nameLen;
		}

		if ( (byteEnd - bytePtr) != (8 * columns * rows) ) XMP_Throw ( "OECF-SFR data overflow", kXMPErr_BadValue );
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Values", &arrayPath );

		XMP_Uns32 * binPtr = (XMP_Uns32*)bytePtr;
		for ( size_t i = (columns * rows); i > 0; --i, binPtr += 2 ) {

			XMP_Uns32 binNum   = binPtr[0];
			XMP_Uns32 binDenom = binPtr[1];
			if ( ! nativeEndian ) {
				Flip4 ( &binNum );
				Flip4 ( &binDenom );
			}

			if ( (binDenom == 0) && (binNum != 0) ) XMP_Throw ( "OECF-SFR data overflow", kXMPErr_BadValue );
			if ( isSigned ) {
				snprintf ( buffer, sizeof(buffer), "%ld/%ld", (long)binNum, (long)binDenom );	// AUDIT: Use of sizeof(buffer) is safe.
			} else {
				snprintf ( buffer, sizeof(buffer), "%lu/%lu", (unsigned long)binNum, (unsigned long)binDenom );	// AUDIT: Use of sizeof(buffer) is safe.
			}

			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, buffer );

		}

		return;

	} catch ( ... ) {
		xmp->DeleteProperty ( xmpNS, xmpProp );
		// ? Notify client?
	}

}	// ImportConversionTable

// =================================================================================================
// ImportTIFF_CFATable
// ===================

static void
ImportTIFF_CFATable ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
					  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;

		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! nativeEndian ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}

		char buffer[20];
		std::string arrayPath;

		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );

		bytePtr += 4;	// Move to the matrix of values.
		if ( (byteEnd - bytePtr) != (columns * rows) ) goto BadExif;	// Make sure the values are present.

		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Values", &arrayPath );

		for ( size_t i = (columns * rows); i > 0; --i, ++bytePtr ) {
			snprintf ( buffer, sizeof(buffer), "%hu", (XMP_Uns16)(*bytePtr) );	// AUDIT: Use of sizeof(buffer) is safe.
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, buffer );
		}

		return;

	BadExif:	// Ignore the tag if the table is ill-formed.
		xmp->DeleteProperty ( xmpNS, xmpProp );
		return;

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_CFATable

// =================================================================================================
// ImportTIFF_DSDTable
// ===================

static void
ImportTIFF_DSDTable ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & tagInfo,
					  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;

		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! tiff.IsNativeEndian() ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}

		char buffer[20];

		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );

		std::string arrayPath;
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Settings", &arrayPath );

		bytePtr += 4;	// Move to the list of settings.
		UTF16Unit * utf16Ptr = (UTF16Unit*)bytePtr;
		UTF16Unit * utf16End = (UTF16Unit*)byteEnd;

		std::string utf8;

		// Figure 17 in the Exif 2.2 spec is unclear. It has counts for rows and columns, but the
		// settings are listed as 1..n, not as a rectangular matrix. So, ignore the counts and copy
		// strings until the end of the Exif value.

		while ( utf16Ptr < utf16End ) {

			size_t nameLen = 0;
			while ( utf16Ptr[nameLen] != 0 ) ++nameLen;
			++nameLen;	// ! Include the terminating nul.
			if ( (utf16Ptr + nameLen) > utf16End ) goto BadExif;

			try {
				FromUTF16 ( utf16Ptr, nameLen, &utf8, tiff.IsBigEndian() );
			} catch ( ... ) {
				goto BadExif; // Ignore the tag if there are conversion errors.
			}

			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, utf8.c_str() );

			utf16Ptr += nameLen;

		}

		return;

	BadExif:	// Ignore the tag if the table is ill-formed.
		xmp->DeleteProperty ( xmpNS, xmpProp );
		return;

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_DSDTable

// =================================================================================================
// ImportTIFF_GPSCoordinate
// ========================

static void
ImportTIFF_GPSCoordinate ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & posInfo,
						   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others. Tolerate ill-formed values where reasonable.

		const bool nativeEndian = tiff.IsNativeEndian();
		
		if ( (posInfo.type != kTIFF_RationalType) || (posInfo.count == 0) ) return;

		XMP_Uns16 refID = posInfo.id - 1;	// ! The GPS refs and coordinates are all tag n and n+1.
		TIFF_Manager::TagInfo refInfo;
		bool found = tiff.GetTag ( kTIFF_GPSInfoIFD, refID, &refInfo );
		if ( (! found) || (refInfo.count == 0) ) return;
		char ref = *((char*)refInfo.dataPtr);
		if ( (ref != 'N') && (ref != 'S') && (ref != 'E') && (ref != 'W') ) return;

		XMP_Uns32 * binPtr = (XMP_Uns32*)posInfo.dataPtr;
		XMP_Uns32 degNum = 0, degDenom = 1;	// Defaults for missing parts.
		XMP_Uns32 minNum = 0, minDenom = 1;
		XMP_Uns32 secNum = 0, secDenom = 1;
		if ( ! nativeEndian ) {
			degDenom = Flip4 ( degDenom );	// So they can be flipped again below.
			minDenom = Flip4 ( minDenom );
			secDenom = Flip4 ( secDenom );
		}
		
		degNum   = GetUns32AsIs ( &binPtr[0] );
		degDenom = GetUns32AsIs ( &binPtr[1] );

		if ( posInfo.count >= 2 ) {
			minNum   = GetUns32AsIs ( &binPtr[2] );
			minDenom = GetUns32AsIs ( &binPtr[3] );
			if ( posInfo.count >= 3 ) {
				secNum   = GetUns32AsIs ( &binPtr[4] );
				secDenom = GetUns32AsIs ( &binPtr[5] );
			}
		}
		
		if ( ! nativeEndian ) {
			degNum = Flip4 ( degNum );
			degDenom = Flip4 ( degDenom );
			minNum = Flip4 ( minNum );
			minDenom = Flip4 ( minDenom );
			secNum = Flip4 ( secNum );
			secDenom = Flip4 ( secDenom );
		}

		// *** No check is made, whether the numerator exceed the maximum values,
		//	which is 90 for Latitude and 180 for Longitude
		// ! The Exif spec says denominator must not be zero, but in reality they can be

		char buffer[40];

		// Simplest case: all denominators are 1
		if ( (degDenom == 1) && (minDenom == 1) && (secDenom == 1) ) {

			snprintf ( buffer, sizeof(buffer), "%lu,%lu,%lu%c", (unsigned long)degNum, (unsigned long)minNum, (unsigned long)secNum, ref );	// AUDIT: Using sizeof(buffer) is safe.

		} else if ( (degDenom == 0 && degNum != 0) || (minDenom == 0 && minNum != 0) || (secDenom == 0 && secNum != 0) ) { 

			// Error case: a denominator is zero and the numerator is non-zero
			return; // Do not continue with import

		} else {

			// Determine precision
			// The code rounds up to the next power of 10 to get the number of fractional digits
			XMP_Uns32 maxDenom = degDenom;
			if ( minDenom > maxDenom ) maxDenom = minDenom;
			if ( secDenom > maxDenom ) maxDenom = secDenom;

			int fracDigits = 1;
			while ( maxDenom > 10 ) { ++fracDigits; maxDenom = maxDenom/10; }

			// Calculate the values
			// At this point we know that the fractions are either 0/0, 0/y or x/y

			double degrees, minutes;

			// Degrees
			if ( degDenom == 0 && degNum == 0 ) {
				degrees = 0;
			} else {
				degrees = (double)( (XMP_Uns32)((double)degNum / (double)degDenom) );	// Just the integral number of degrees.
			}

			// Minutes
			if ( minDenom == 0 && minNum == 0 ) {
				minutes = 0;
			} else {
				double temp = 0;
				if( degrees != 0 ) temp = ((double)degNum / (double)degDenom) - degrees;

				minutes = (temp * 60.0) + ((double)minNum / (double)minDenom);
			}

			// Seconds, are added to minutes
			if ( secDenom != 0 && secNum != 0 ) {
				minutes += ((double)secNum / (double)secDenom) / 60.0;
			}

			snprintf ( buffer, sizeof(buffer), "%.0f,%.*f%c", degrees, fracDigits, minutes, ref );	// AUDIT: Using sizeof(buffer) is safe.

		}

		xmp->SetProperty ( xmpNS, xmpProp, buffer );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_GPSCoordinate

// =================================================================================================
// ImportTIFF_GPSTimeStamp
// =======================

static void
ImportTIFF_GPSTimeStamp ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & timeInfo,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const bool nativeEndian = tiff.IsNativeEndian();

		bool haveDate;
		TIFF_Manager::TagInfo dateInfo;
		haveDate = tiff.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDateStamp, &dateInfo );
		if ( ! haveDate ) haveDate = tiff.GetTag ( kTIFF_ExifIFD, kTIFF_DateTimeOriginal, &dateInfo );
		if ( ! haveDate ) haveDate = tiff.GetTag ( kTIFF_ExifIFD, kTIFF_DateTimeDigitized, &dateInfo );
		if ( ! haveDate ) return;

		const char * dateStr = (const char *) dateInfo.dataPtr;
		if ( ((dateStr[4] != ':') && (dateStr[4] != '-')) || ((dateStr[7] != ':') && (dateStr[7] != '-')) ) return;
		if ( (dateStr[10] != 0)  && (dateStr[10] != ' ') ) return;

		XMP_Uns32 * binPtr = (XMP_Uns32*)timeInfo.dataPtr;
		XMP_Uns32 hourNum   = GetUns32AsIs ( &binPtr[0] );
		XMP_Uns32 hourDenom = GetUns32AsIs ( &binPtr[1] );
		XMP_Uns32 minNum    = GetUns32AsIs ( &binPtr[2] );
		XMP_Uns32 minDenom  = GetUns32AsIs ( &binPtr[3] );
		XMP_Uns32 secNum    = GetUns32AsIs ( &binPtr[4] );
		XMP_Uns32 secDenom  = GetUns32AsIs ( &binPtr[5] );
		if ( ! nativeEndian ) {
			hourNum = Flip4 ( hourNum );
			hourDenom = Flip4 ( hourDenom );
			minNum = Flip4 ( minNum );
			minDenom = Flip4 ( minDenom );
			secNum = Flip4 ( secNum );
			secDenom = Flip4 ( secDenom );
		}

		double fHour, fMin, fSec, fNano, temp;
		fSec  =  (double)secNum / (double)secDenom;
		temp  =  (double)minNum / (double)minDenom;
		fMin  =  (double)((XMP_Uns32)temp);
		fSec  += (temp - fMin) * 60.0;
		temp  =  (double)hourNum / (double)hourDenom;
		fHour =  (double)((XMP_Uns32)temp);
		fSec  += (temp - fHour) * 3600.0;
		temp  =  (double)((XMP_Uns32)fSec);
		fNano =  ((fSec - temp) * (1000.0*1000.0*1000.0)) + 0.5;	// Try to avoid n999... problems.
		fSec  =  temp;

		XMP_DateTime binStamp;
		binStamp.year   = GatherInt ( dateStr, 4 );
		binStamp.month  = GatherInt ( dateStr+5, 2 );
		binStamp.day    = GatherInt ( dateStr+8, 2 );
		binStamp.hour   = (XMP_Int32)fHour;
		binStamp.minute = (XMP_Int32)fMin;
		binStamp.second = (XMP_Int32)fSec;
		binStamp.nanoSecond  = (XMP_Int32)fNano;
		binStamp.hasTimeZone = true;	// Exif GPS TimeStamp is implicitly UTC.
		binStamp.tzSign = kXMP_TimeIsUTC;
		binStamp.tzHour = binStamp.tzMinute = 0;

		xmp->SetProperty_Date ( xmpNS, xmpProp, binStamp );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_GPSTimeStamp

// =================================================================================================

static void ImportTIFF_PhotographicSensitivity ( const TIFF_Manager & exif, SXMPMeta * xmp ) {

	// PhotographicSensitivity has special cases for values over 65534 because the tag is SHORT. It
	// has a count of "any", but all known cameras used a count of 1. Exif 2.3 still allows a count
	// of "any" but says only 1 value should ever be recorded. We only process the first value if
	// the count is greater than 1.
	//
	// Prior to Exif 2.3 the tag was called ISOSpeedRatings. Some cameras omit the tag and some
	// write 65535. Most put the real ISO in MakerNote, which might be in the XMP from ACR.
	//
	// With Exif 2.3 five speed-related tags were added of type LONG: StandardOutputSensitivity,
	// RecommendedExposureIndex, ISOSpeed, ISOSpeedLatitudeyyy, and ISOSpeedLatitudezzz. The tag
	// SensitivityType was added to say which of these five is copied to PhotographicSensitivity.
	//
	// Import logic:
	//	Save exif:ISOSpeedRatings when cleaning namespaces (done in ImportPhotoData)
	//	If ExifVersion is less than "0230" (or missing)
	//		If PhotographicSensitivity[1] < 65535 or no exif:ISOSpeedRatings: set exif:ISOSpeedRatings from tag
	//	Else (ExifVersion is at least "0230")
	//		Import SensitivityType and related long tags
	//		If PhotographicSensitivity[1] < 65535: set exifEX:PhotographicSensitivity and exif:ISOSpeedRatings from tag
	//		Else (no PhotographicSensitivity tag or value is 65535)
	//			Set exifEX:PhotographicSensitivity from tag (missing or 65535)
	//			If have SensitivityType and indicated tag: set exif:ISOSpeedRatings from indicated tag
	
	try {
	
		bool found;
		TIFF_Manager::TagInfo tagInfo;
	
		bool haveOldExif = true;	// Default to old Exif if no version tag.
		bool haveTag34855 = false;
		bool haveLowISO = false;	// Set for real if haveTag34855 is true.
		
		XMP_Uns32 valueTag34855;	// ! Only care about the first value if more than 1.
	
		found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, &tagInfo );
		if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
			haveOldExif = (strncmp ( (char*)tagInfo.dataPtr, "0230", 4 ) < 0);
		}
	
		haveTag34855 = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, &valueTag34855 );
		if ( haveTag34855 ) haveLowISO = (valueTag34855 < 65535);

		if ( haveOldExif ) {	// Exif version is before 2.3, use just the old tag and property.

			if ( haveTag34855 ) {
				if ( haveLowISO || (! xmp->DoesPropertyExist ( kXMP_NS_EXIF, "ISOSpeedRatings" )) ) {
					xmp->DeleteProperty ( kXMP_NS_EXIF, "ISOSpeedRatings" );
					xmp->AppendArrayItem ( kXMP_NS_EXIF, "ISOSpeedRatings", kXMP_PropArrayIsOrdered, "" );
					xmp->SetProperty_Int ( kXMP_NS_EXIF, "ISOSpeedRatings[1]", valueTag34855 );
				}
			}
		
		} else {	// Exif version is 2.3 or newer, use the Exif 2.3 tags and properties.
		
			// Import the SensitivityType and related long tags.
	
			XMP_Uns16 whichLongTag = 0;
			XMP_Uns32 sensitivityType, tagValue;
			
			bool haveSensitivityType = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_SensitivityType, &sensitivityType );
			if ( haveSensitivityType ) {
				xmp->SetProperty_Int ( kXMP_NS_ExifEX, "SensitivityType", sensitivityType );
				switch ( sensitivityType ) {
					case 1  : // Use StandardOutputSensitivity for both 1 and 4.
					case 4  : whichLongTag = kTIFF_StandardOutputSensitivity; break;
					case 2  : whichLongTag = kTIFF_RecommendedExposureIndex; break;
					case 3  : // Use ISOSpeed for all of 3, 5, 6, and 7.
					case 5  :
					case 6  :
					case 7  : whichLongTag = kTIFF_ISOSpeed; break;
				}
			}
			
			found = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_StandardOutputSensitivity, &tagValue );
			if ( found ) {
				xmp->SetProperty_Int64 ( kXMP_NS_ExifEX, "StandardOutputSensitivity", tagValue );
			}
			
			found = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_RecommendedExposureIndex, &tagValue );
			if ( found ) {
				xmp->SetProperty_Int64 ( kXMP_NS_ExifEX, "RecommendedExposureIndex", tagValue );
			}
			
			found = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_ISOSpeed, &tagValue );
			if ( found ) {
				xmp->SetProperty_Int64 ( kXMP_NS_ExifEX, "ISOSpeed", tagValue );
			}
			
			found = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_ISOSpeedLatitudeyyy, &tagValue );
			if ( found ) {
				xmp->SetProperty_Int64 ( kXMP_NS_ExifEX, "ISOSpeedLatitudeyyy", tagValue );
			}
			
			found = exif.GetTag_Integer ( kTIFF_ExifIFD, kTIFF_ISOSpeedLatitudezzz, &tagValue );
			if ( found ) {
				xmp->SetProperty_Int64 ( kXMP_NS_ExifEX, "ISOSpeedLatitudezzz", tagValue );
			}
		
			// Deal with the special cases for exifEX:PhotographicSensitivity and exif:ISOSpeedRatings.
	
			if ( haveTag34855 & haveLowISO ) {	// The easier low ISO case.
	
				xmp->DeleteProperty ( kXMP_NS_EXIF, "ISOSpeedRatings" );
				xmp->AppendArrayItem ( kXMP_NS_EXIF, "ISOSpeedRatings", kXMP_PropArrayIsOrdered, "" );
				xmp->SetProperty_Int ( kXMP_NS_EXIF, "ISOSpeedRatings[1]", valueTag34855 );
				xmp->SetProperty_Int ( kXMP_NS_ExifEX, "PhotographicSensitivity", valueTag34855 );
	
			} else {	// The more complex high ISO case, or no PhotographicSensitivity tag.
				
				if ( haveTag34855 ) {
					XMP_Assert ( valueTag34855 == 65535 );
					xmp->SetProperty_Int ( kXMP_NS_ExifEX, "PhotographicSensitivity", valueTag34855 );
				}
				
				if ( whichLongTag != 0 ) {
					found = exif.GetTag ( kTIFF_ExifIFD, whichLongTag, &tagInfo );
					if ( found && (tagInfo.type == kTIFF_LongType) && (tagInfo.count == 1) ){
						xmp->DeleteProperty ( kXMP_NS_EXIF, "ISOSpeedRatings" );
						xmp->AppendArrayItem ( kXMP_NS_EXIF, "ISOSpeedRatings", kXMP_PropArrayIsOrdered, "" );
						xmp->SetProperty_Int ( kXMP_NS_EXIF, "ISOSpeedRatings[1]", exif.GetUns32 ( tagInfo.dataPtr ) );
					}
				}
	
			}
		
		}
	
	} catch ( ... ) {
	
		// Do nothing, don't let failures here stop other exports.
		
	}

}	// ImportTIFF_PhotographicSensitivity

// =================================================================================================
// =================================================================================================

// =================================================================================================
// PhotoDataUtils::Import2WayExif
// ==============================
//
// Import the TIFF/Exif tags that have 2 way mappings to XMP, i.e. no correspondence to IPTC.
// These are always imported for the tiff: and exif: namespaces, but not for others.

void
PhotoDataUtils::Import2WayExif ( const TIFF_Manager & exif, SXMPMeta * xmp, int iptcDigestState )
{
	const bool nativeEndian = exif.IsNativeEndian();

	bool found, foundFromXMP;
	TIFF_Manager::TagInfo tagInfo;
	XMP_OptionBits flags;

	ImportTIFF_StandardMappings ( kTIFF_PrimaryIFD, exif, xmp );
	ImportTIFF_StandardMappings ( kTIFF_ExifIFD, exif, xmp );
	ImportTIFF_StandardMappings ( kTIFF_GPSInfoIFD, exif, xmp );
	
	// --------------------------------------------------------
	// Add the old Adobe names for new Exif 2.3 tags if wanted.
	
	#if SupportOldExifProperties
		
		// CameraOwnerName -> aux:OwnerName
		found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_CameraOwnerName, &tagInfo );
		if ( found && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count > 0) ) {
			ImportSingleTIFF ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF_Aux, "OwnerName" );
		}
		
		// BodySerialNumber -> aux:SerialNumber
		found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_BodySerialNumber, &tagInfo );
		if ( found && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count > 0) ) {
			ImportSingleTIFF ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF_Aux, "SerialNumber" );
		}
		
		// LensModel -> aux:Lens
		found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_LensModel, &tagInfo );
		if ( found && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count > 0) ) {
			ImportSingleTIFF ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF_Aux, "Lens" );
		}
		
		// LensSpecification -> aux:LensInfo as a single string - use the XMP form for simplicity
		found = xmp->GetProperty ( kXMP_NS_ExifEX, "LensSpecification", 0, &flags );
		if ( found && XMP_PropIsArray(flags) ) {
			std::string fullStr, oneItem;
			size_t count = (size_t) xmp->CountArrayItems ( kXMP_NS_ExifEX, "LensSpecification" );
			if ( count > 0 ) {
				(void) xmp->GetArrayItem ( kXMP_NS_ExifEX, "LensSpecification", 1, &fullStr, 0 );
				for ( size_t i = 2; i <= count; ++i ) {
					fullStr += ' ';
					(void) xmp->GetArrayItem ( kXMP_NS_ExifEX, "LensSpecification", i, &oneItem, 0 );
					fullStr += oneItem;
				}
			}
			xmp->SetProperty ( kXMP_NS_EXIF_Aux, "LensInfo", fullStr.c_str(), kXMP_DeleteExisting );
		}
	
	#endif
	
	// --------------------------------------------------------------------------------------------
	// Fixup erroneous cases that appear to have a negative value for GPSAltitude in the Exif. This
	// treats any value with the high bit set as a negative, which is more likely in practice than
	// an actual value over 2 billion.

	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSAltitude, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_RationalType) &&  (tagInfo.count == 1) ) {

		XMP_Uns32 num = exif.GetUns32 ( tagInfo.dataPtr );
		XMP_Uns32 denom = exif.GetUns32 ( (XMP_Uns8*)tagInfo.dataPtr + 4 );
		
		bool fixXMP = false;
		bool numNeg = num >> 31;
		bool denomNeg = denom >> 31;
		
		if ( denomNeg ) {	// The denominator looks negative, shift the sign to the numerator.
			denom = -denom;
			num = -num;
			numNeg = num >> 31;
			fixXMP = true;
		}

		if ( numNeg ) {	// The numerator looks negative, fix the XMP.
			xmp->SetProperty ( kXMP_NS_EXIF, "GPSAltitudeRef", "1" );
			num = -num;
			fixXMP = true;
		}

		if ( fixXMP ) {
			char buffer [32];
			snprintf ( buffer, sizeof(buffer), "%lu/%lu", (unsigned long) num, (unsigned long) denom );	// AUDIT: Using sizeof(buffer) is safe.
			xmp->SetProperty ( kXMP_NS_EXIF, "GPSAltitude", buffer );
		}

	}
	
	// ---------------------------------------------------------------
	// Import DateTimeOriginal and DateTime if the XMP doss not exist.

	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_DateTimeOriginal, &tagInfo );
	foundFromXMP = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "DateTimeOriginal" );
	
	if ( found && (! foundFromXMP) && (tagInfo.type == kTIFF_ASCIIType) ) {
		ImportTIFF_Date ( exif, tagInfo, xmp, kXMP_NS_EXIF, "DateTimeOriginal" );
	}

	found = exif.GetTag ( kTIFF_PrimaryIFD, kTIFF_DateTime, &tagInfo );
	foundFromXMP = xmp->DoesPropertyExist ( kXMP_NS_XMP, "ModifyDate" );
	
	if ( found && (! foundFromXMP) && (tagInfo.type == kTIFF_ASCIIType) ) {
		ImportTIFF_Date ( exif, tagInfo, xmp, kXMP_NS_XMP, "ModifyDate" );
	}

	// ----------------------------------------------------
	// Import the Exif IFD tags that have special mappings.

	// There are moderately complex import special cases for PhotographicSensitivity.
	ImportTIFF_PhotographicSensitivity ( exif, xmp );
	
	// 42032 CameraOwnerName has a standard mapping. As a special case also set dc:creator if there
	// is no Exif Artist tag and no dc:creator in the XMP.
	found = exif.GetTag ( kTIFF_PrimaryIFD, kTIFF_Artist, &tagInfo );
	foundFromXMP = xmp->DoesPropertyExist ( kXMP_NS_DC, "creator" );
	if ( (! found) && (! foundFromXMP) ) {
		found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_CameraOwnerName, &tagInfo );
		if ( found ) {
			std::string xmpValue ( (char*)tagInfo.dataPtr, tagInfo.dataLen );
			xmp->AppendArrayItem ( kXMP_NS_DC, "creator", kXMP_PropArrayIsOrdered, xmpValue.c_str() );
		}
	}

	// 36864 ExifVersion is 4 "undefined" ASCII characters.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		char str[5];

		*((XMP_Uns32*)str) = GetUns32AsIs ( tagInfo.dataPtr );
		str[4] = 0;
		xmp->SetProperty ( kXMP_NS_EXIF, "ExifVersion", str );
	}

	// 40960 FlashpixVersion is 4 "undefined" ASCII characters.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_FlashpixVersion, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		char str[5];

		*((XMP_Uns32*)str) = GetUns32AsIs ( tagInfo.dataPtr );
		str[4] = 0;
		xmp->SetProperty ( kXMP_NS_EXIF, "FlashpixVersion", str );
	}

	// 37121 ComponentsConfiguration is an array of 4 "undefined" UInt8 bytes.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_ComponentsConfiguration, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		ImportArrayTIFF_Byte ( tagInfo, xmp, kXMP_NS_EXIF, "ComponentsConfiguration" );
	}

	// 37510 UserComment is a string with explicit encoding.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_UserComment, &tagInfo );
	if ( found ) {
		ImportTIFF_EncodedString ( exif, tagInfo, xmp, kXMP_NS_EXIF, "UserComment", true /* isLangAlt */ );
	}

	// 34856 OECF is an OECF/SFR table.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_OECF, &tagInfo );
	if ( found ) {
		ImportConversionTable ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "OECF" );
	}

	// 37385 Flash is a UInt16 collection of bit fields and is mapped to a struct in XMP.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_Flash, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_ShortType) && (tagInfo.count == 1) ) {
		ImportTIFF_Flash ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "Flash" );
	}

	// 41484 SpatialFrequencyResponse is an OECF/SFR table.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_SpatialFrequencyResponse, &tagInfo );
	if ( found ) {
		ImportConversionTable ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "SpatialFrequencyResponse" );
	}

	// 41728 FileSource is an "undefined" UInt8.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_FileSource, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 1) ) {
		ImportSingleTIFF_Byte ( tagInfo, xmp, kXMP_NS_EXIF, "FileSource" );
	}

	// 41729 SceneType is an "undefined" UInt8.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_SceneType, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 1) ) {
		ImportSingleTIFF_Byte ( tagInfo, xmp, kXMP_NS_EXIF, "SceneType" );
	}

	// 41730 CFAPattern is a custom table.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_CFAPattern, &tagInfo );
	if ( found ) {
		ImportTIFF_CFATable ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "CFAPattern" );
	}

	// 41995 DeviceSettingDescription is a custom table.
	found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_DeviceSettingDescription, &tagInfo );
	if ( found ) {
		ImportTIFF_DSDTable ( exif, tagInfo, xmp, kXMP_NS_EXIF, "DeviceSettingDescription" );
	}

	// --------------------------------------------------------
	// Import the GPS Info IFD tags that have special mappings.

	// 0 GPSVersionID is 4 UInt8 bytes and mapped as "n.n.n.n".
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSVersionID, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_ByteType) && (tagInfo.count == 4) ) {
		const XMP_Uns8 * binValue = (const XMP_Uns8 *) tagInfo.dataPtr;
		char strOut[20];// ! Value could be up to 16 bytes: "255.255.255.255" plus nul.
		snprintf ( strOut, sizeof(strOut), "%u.%u.%u.%u",	// AUDIT: Use of sizeof(strOut) is safe.
				   binValue[0], binValue[1], binValue[2], binValue[3] );
		xmp->SetProperty ( kXMP_NS_EXIF, "GPSVersionID", strOut );
	}

	// 2 GPSLatitude is a GPS coordinate master.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSLatitude, &tagInfo );
	if ( found ) {
		ImportTIFF_GPSCoordinate ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSLatitude" );
	}

	// 4 GPSLongitude is a GPS coordinate master.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSLongitude, &tagInfo );
	if ( found ) {
		ImportTIFF_GPSCoordinate ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSLongitude" );
	}

	// 7 GPSTimeStamp is a UTC time as 3 rationals, mated with the optional GPSDateStamp.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSTimeStamp, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_RationalType) && (tagInfo.count == 3) ) {
		ImportTIFF_GPSTimeStamp ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSTimeStamp" );
	}

	// 20 GPSDestLatitude is a GPS coordinate master.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDestLatitude, &tagInfo );
	if ( found ) {
		ImportTIFF_GPSCoordinate ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSDestLatitude" );
	}

	// 22 GPSDestLongitude is a GPS coordinate master.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDestLongitude, &tagInfo );
	if ( found ) {
		ImportTIFF_GPSCoordinate ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSDestLongitude" );
	}

	// 27 GPSProcessingMethod is a string with explicit encoding.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSProcessingMethod, &tagInfo );
	if ( found ) {
		ImportTIFF_EncodedString ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSProcessingMethod" );
	}

	// 28 GPSAreaInformation is a string with explicit encoding.
	found = exif.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSAreaInformation, &tagInfo );
	if ( found ) {
		ImportTIFF_EncodedString ( exif, tagInfo, xmp, kXMP_NS_EXIF, "GPSAreaInformation" );
	}

}	// PhotoDataUtils::Import2WayExif

// =================================================================================================
// Import3WayDateTime
// ==================

static void Import3WayDateTime ( XMP_Uns16 exifTag, const TIFF_Manager & exif, const IPTC_Manager & iptc,
								 SXMPMeta * xmp, int iptcDigestState, const IPTC_Manager & oldIPTC )
{
	XMP_Uns8  iptcDS;
	XMP_StringPtr xmpNS, xmpProp;
	
	if ( exifTag == kTIFF_DateTimeOriginal ) {
		iptcDS  = kIPTC_DateCreated;
		xmpNS   = kXMP_NS_Photoshop;
		xmpProp = "DateCreated";
	} else if ( exifTag == kTIFF_DateTimeDigitized ) {
		iptcDS  = kIPTC_DigitalCreateDate;
		xmpNS   = kXMP_NS_XMP;
		xmpProp = "CreateDate";
	} else {
		XMP_Throw ( "Unrecognized dateID", kXMPErr_BadParam );
	}

	size_t iptcCount;
	bool haveXMP, haveExif, haveIPTC;	// ! These are manipulated to simplify MWG-compliant logic.
	std::string xmpValue, exifValue, iptcValue;
	TIFF_Manager::TagInfo exifInfo;
	IPTC_Manager::DataSetInfo iptcInfo;

	// Get the basic info about available values.
	haveXMP   = xmp->GetProperty ( xmpNS, xmpProp, &xmpValue, 0 );
	iptcCount = PhotoDataUtils::GetNativeInfo ( iptc, iptcDS, iptcDigestState, haveXMP, &iptcInfo );
	haveIPTC = (iptcCount > 0);
	XMP_Assert ( (iptcDigestState == kDigestMatches) ? (! haveIPTC) : true );
	haveExif  = (! haveXMP) && (! haveIPTC) && PhotoDataUtils::GetNativeInfo ( exif, kTIFF_ExifIFD, exifTag, &exifInfo );
	XMP_Assert ( (! (haveExif & haveXMP)) & (! (haveExif & haveIPTC)) );

	if ( haveIPTC ) {

		PhotoDataUtils::ImportIPTC_Date ( iptcDS, iptc, xmp );

	} else if ( haveExif && (exifInfo.type == kTIFF_ASCIIType) ) {

		// Only import the Exif form if the non-TZ information differs from the XMP.
	
		TIFF_FileWriter exifFromXMP;
		TIFF_Manager::TagInfo infoFromXMP;

		ExportTIFF_Date ( *xmp, xmpNS, xmpProp, &exifFromXMP, exifTag );
		bool foundFromXMP = exifFromXMP.GetTag ( kTIFF_ExifIFD, exifTag, &infoFromXMP );

		if ( (! foundFromXMP) || (exifInfo.dataLen != infoFromXMP.dataLen) ||
			 (! XMP_LitNMatch ( (char*)exifInfo.dataPtr, (char*)infoFromXMP.dataPtr, exifInfo.dataLen )) ) {
			ImportTIFF_Date ( exif, exifInfo, xmp, xmpNS, xmpProp );
		}

	}

}	// Import3WayDateTime

// =================================================================================================
// PhotoDataUtils::Import3WayItems
// ===============================
//
// Handle the imports that involve all 3 of Exif, IPTC, and XMP. There are only 4 properties with
// 3-way mappings, copyright, description, creator, and date/time. Following the MWG guidelines,
// this general policy is applied separately to each:
//
//   If the new IPTC digest differs from the stored digest (favor IPTC over Exif and XMP)
//      If the IPTC value differs from the predicted old IPTC value
//         Import the IPTC value, including deleting the XMP
//      Else if the Exif is non-empty and differs from the XMP
//         Import the Exif value (does not delete existing XMP)
//   Else if the stored IPTC digest is missing (favor Exif over IPTC, or IPTC over missing XMP)
//      If the Exif is non-empty and differs from the XMP
//         Import the Exif value (does not delete existing XMP)
//      Else if the XMP is missing and the Exif is missing or empty
//         Import the IPTC value
//   Else (the new IPTC digest matches the stored digest - ignore the IPTC)
//      If the Exif is non-empty and differs from the XMP
//         Import the Exif value (does not delete existing XMP)
//
// Note that missing or empty Exif will never cause existing XMP to be deleted. This is a pragmatic
// choice to improve compatibility with pre-MWG software. There are few Exif-only editors for these
// 3-way properties, there are important existing IPTC-only editors.

// -------------------------------------------------------------------------------------------------

void PhotoDataUtils::Import3WayItems ( const TIFF_Manager & exif, const IPTC_Manager & iptc, SXMPMeta * xmp, int iptcDigestState )
{
	size_t iptcCount;

	bool haveXMP, haveExif, haveIPTC;	// ! These are manipulated to simplify MWG-compliant logic.
	std::string xmpValue, exifValue, iptcValue;

	TIFF_Manager::TagInfo exifInfo;
	IPTC_Manager::DataSetInfo iptcInfo;

	IPTC_Writer oldIPTC;
	if ( iptcDigestState == kDigestDiffers ) {
		PhotoDataUtils::ExportIPTC ( *xmp, &oldIPTC );	// Predict old IPTC DataSets based on the existing XMP.
	}
	
	// ---------------------------------------------------------------------------------
	// Process the copyright. Replace internal nuls in the Exif to "merge" the portions.
	
	// Get the basic info about available values.
	haveXMP   = xmp->GetLocalizedText ( kXMP_NS_DC, "rights", "", "x-default", 0, &xmpValue, 0 );
	iptcCount = PhotoDataUtils::GetNativeInfo ( iptc, kIPTC_CopyrightNotice, iptcDigestState, haveXMP, &iptcInfo );
	haveIPTC  = (iptcCount > 0);
	XMP_Assert ( (iptcDigestState == kDigestMatches) ? (! haveIPTC) : true );
	haveExif  = (! haveXMP) && (! haveIPTC) && PhotoDataUtils::GetNativeInfo ( exif, kTIFF_PrimaryIFD, kTIFF_Copyright, &exifInfo );
	XMP_Assert ( (! (haveExif & haveXMP)) & (! (haveExif & haveIPTC)) );

	if ( haveExif && (exifInfo.dataLen > 1) ) {	// Replace internal nul characters with linefeed.
		for ( XMP_Uns32 i = 0; i < exifInfo.dataLen-1; ++i ) {
			if ( ((char*)exifInfo.dataPtr)[i] == 0 ) ((char*)exifInfo.dataPtr)[i] = 0x0A;
		}
	}
	
	if ( haveIPTC ) {
		PhotoDataUtils::ImportIPTC_LangAlt ( iptc, xmp, kIPTC_CopyrightNotice, kXMP_NS_DC, "rights" );
	} else if ( haveExif && PhotoDataUtils::IsValueDifferent ( exifInfo, xmpValue, &exifValue ) ) {
		xmp->SetLocalizedText ( kXMP_NS_DC, "rights", "", "x-default", exifValue.c_str() );
	}
	
	// ------------------------
	// Process the description.
	
	// Get the basic info about available values.
	haveXMP   = xmp->GetLocalizedText ( kXMP_NS_DC, "description", "", "x-default", 0, &xmpValue, 0 );
	iptcCount = PhotoDataUtils::GetNativeInfo ( iptc, kIPTC_Description, iptcDigestState, haveXMP, &iptcInfo );
	haveIPTC  = (iptcCount > 0);
	XMP_Assert ( (iptcDigestState == kDigestMatches) ? (! haveIPTC) : true );
	haveExif  = (! haveXMP) && (! haveIPTC) && PhotoDataUtils::GetNativeInfo ( exif, kTIFF_PrimaryIFD, kTIFF_ImageDescription, &exifInfo );
	XMP_Assert ( (! (haveExif & haveXMP)) & (! (haveExif & haveIPTC)) );
	
	if ( haveIPTC ) {
		PhotoDataUtils::ImportIPTC_LangAlt ( iptc, xmp, kIPTC_Description, kXMP_NS_DC, "description" );
	} else if ( haveExif && PhotoDataUtils::IsValueDifferent ( exifInfo, xmpValue, &exifValue ) ) {
		xmp->SetLocalizedText ( kXMP_NS_DC, "description", "", "x-default", exifValue.c_str() );
	}

	// -------------------------------------------------------------------------------------------
	// Process the creator. The XMP and IPTC are arrays, the Exif is a semicolon separated string.
	
	// Get the basic info about available values.
	haveXMP   = xmp->DoesPropertyExist ( kXMP_NS_DC, "creator" );
	haveExif  = PhotoDataUtils::GetNativeInfo ( exif, kTIFF_PrimaryIFD, kTIFF_Artist, &exifInfo );
	iptcCount = PhotoDataUtils::GetNativeInfo ( iptc, kIPTC_Creator, iptcDigestState, haveXMP, &iptcInfo );
	haveIPTC  = (iptcCount > 0);
	XMP_Assert ( (iptcDigestState == kDigestMatches) ? (! haveIPTC) : true );
	haveExif  = (! haveXMP) && (! haveIPTC) && PhotoDataUtils::GetNativeInfo ( exif, kTIFF_PrimaryIFD, kTIFF_Artist, &exifInfo );
	XMP_Assert ( (! (haveExif & haveXMP)) & (! (haveExif & haveIPTC)) );

	if ( haveIPTC ) {
		PhotoDataUtils::ImportIPTC_Array ( iptc, xmp, kIPTC_Creator, kXMP_NS_DC, "creator" );
	} else if ( haveExif && PhotoDataUtils::IsValueDifferent ( exifInfo, xmpValue, &exifValue ) ) {
		SXMPUtils::SeparateArrayItems ( xmp, kXMP_NS_DC, "creator",
										(kXMP_PropArrayIsOrdered | kXMPUtil_AllowCommas), exifValue );
	}
	
	// ------------------------------------------------------------------------------
	// Process DateTimeDigitized; DateTimeOriginal and DateTime are 2-way.
	// ***   Exif DateTimeOriginal <-> XMP exif:DateTimeOriginal
	// ***   IPTC DateCreated <-> XMP photoshop:DateCreated
	// ***   Exif DateTimeDigitized <-> IPTC DigitalCreateDate <-> XMP xmp:CreateDate
	// ***   TIFF DateTime <-> XMP xmp:ModifyDate

	Import3WayDateTime ( kTIFF_DateTimeDigitized, exif, iptc, xmp, iptcDigestState, oldIPTC );

}	// PhotoDataUtils::Import3WayItems

// =================================================================================================
// =================================================================================================

// =================================================================================================

static bool DecodeRational ( const char * ratio, XMP_Uns32 * num, XMP_Uns32 * denom ) {

	unsigned long locNum, locDenom;
	char nextChar;	// Used to make sure sscanf consumes all of the string.

	int items = sscanf ( ratio, "%lu/%lu%c", &locNum, &locDenom, &nextChar );	// AUDIT: This is safe, check the calls.

	if ( items != 2 ) {
		if ( items != 1 ) return false;
		locDenom = 1;	// The XMP was just an integer, assume a denominator of 1.
	}
	
	*num = (XMP_Uns32)locNum;
	*denom = (XMP_Uns32)locDenom;
	return true;

}	// DecodeRational

// =================================================================================================
// ExportSingleTIFF
// ================
//
// This is only called when the XMP exists and will be exported. And only for standard mappings.

// ! Only implemented for the types known to be needed.

static void
ExportSingleTIFF ( TIFF_Manager * tiff, XMP_Uns8 ifd, const TIFF_MappingToXMP & mapInfo,
				   bool nativeEndian, const std::string & xmpValue )
{
	XMP_Assert ( (mapInfo.count == 1) || (mapInfo.type == kTIFF_ASCIIType) );
	XMP_Assert ( mapInfo.name[0] != 0 );	// Must be a standard mapping.
	
	bool ok;
	char nextChar;	// Used to make sure sscanf consumes all of the string.
	
	switch ( mapInfo.type ) {

		case kTIFF_ByteType : {
			unsigned short binValue;
			int items = sscanf ( xmpValue.c_str(), "%hu%c", &binValue, &nextChar );	// AUDIT: Using xmpValue.c_str() is safe.
			if ( items != 1 ) return;	// ? complain? notify client?
			tiff->SetTag_Byte ( ifd, mapInfo.id, (XMP_Uns8)binValue );
			break;
		}

		case kTIFF_ShortType : {
			unsigned long binValue;
			int items = sscanf ( xmpValue.c_str(), "%lu%c", &binValue, &nextChar );	// AUDIT: Using xmpValue.c_str() is safe.
			if ( items != 1 ) return;	// ? complain? notify client?
			tiff->SetTag_Short ( ifd, mapInfo.id, (XMP_Uns16)binValue );
			break;
		}

		case kTIFF_LongType : {
			unsigned long binValue;
			int items = sscanf ( xmpValue.c_str(), "%lu%c", &binValue, &nextChar );	// AUDIT: Using xmpValue.c_str() is safe.
			if ( items != 1 ) return;	// ? complain? notify client?
			tiff->SetTag_Long ( ifd, mapInfo.id, (XMP_Uns32)binValue );
			break;
		}

		case kTIFF_ShortOrLongType : {
			unsigned long binValue;
			int items = sscanf ( xmpValue.c_str(), "%lu%c", &binValue, &nextChar );	// AUDIT: Using xmpValue.c_str() is safe.
			if ( items != 1 ) return;	// ? complain? notify client?
			if ( binValue <= 0xFFFF ) {
				tiff->SetTag_Short ( ifd, mapInfo.id, (XMP_Uns16)binValue );
			} else {
				tiff->SetTag_Long ( ifd, mapInfo.id, (XMP_Uns32)binValue );
			}
			break;
		}

		case kTIFF_RationalType : {	// The XMP is formatted as "num/denom".
			XMP_Uns32 num, denom;
			ok = DecodeRational ( xmpValue.c_str(), &num, &denom );
			if ( ! ok ) return;	// ? complain? notify client?
			tiff->SetTag_Rational ( ifd, mapInfo.id, num, denom );
			break;
		}

		case kTIFF_SRationalType : {	// The XMP is formatted as "num/denom".
			signed long num, denom;
			int items = sscanf ( xmpValue.c_str(), "%ld/%ld%c", &num, &denom, &nextChar );	// AUDIT: Using xmpValue.c_str() is safe.
			if ( items != 2 ) {
				if ( items != 1 ) return;	// ? complain? notify client?
				denom = 1;	// The XMP was just an integer, assume a denominator of 1.
			}
			tiff->SetTag_SRational ( ifd, mapInfo.id, (XMP_Int32)num, (XMP_Int32)denom );
			break;
		}

		case kTIFF_ASCIIType :
			tiff->SetTag ( ifd, mapInfo.id, kTIFF_ASCIIType, (XMP_Uns32)(xmpValue.size()+1), xmpValue.c_str() );
			break;
			
		default:
			XMP_Assert ( false );	// Force a debug assert for unexpected types.

	}

}	// ExportSingleTIFF

// =================================================================================================
// ExportArrayTIFF
// ================
//
// This is only called when the XMP exists and will be exported. And only for standard mappings.

// ! Only implemented for the types known to be needed.

static void
ExportArrayTIFF ( TIFF_Manager * tiff, XMP_Uns8 ifd, const TIFF_MappingToXMP & mapInfo, bool nativeEndian,
				  const SXMPMeta & xmp, const char * xmpNS, const char * xmpArray )
{
	XMP_Assert ( (mapInfo.count != 1) && (mapInfo.type != kTIFF_ASCIIType) );
	XMP_Assert ( mapInfo.name[0] != 0 );	// Must be a standard mapping.
	XMP_Assert ( (mapInfo.type == kTIFF_ShortType) || (mapInfo.type == kTIFF_RationalType) );
	XMP_Assert ( xmp.DoesPropertyExist ( xmpNS, xmpArray ) );
	
	size_t arraySize = xmp.CountArrayItems ( xmpNS, xmpArray );
	if ( arraySize == 0 ) {
		tiff->DeleteTag ( ifd, mapInfo.id );
		return;
	}
	
	if ( mapInfo.type == kTIFF_ShortType ) {
		
		std::vector<XMP_Uns16> shortVector;
		shortVector.assign ( arraySize, 0 );
		XMP_Uns16 * shortPtr = (XMP_Uns16*) &shortVector[0];
		
		std::string itemPath;
		XMP_Int32 int32;
		XMP_Uns16 uns16;
		for ( size_t i = 1; i <= arraySize; ++i, ++shortPtr ) {
			SXMPUtils::ComposeArrayItemPath ( xmpNS, xmpArray, (XMP_Index)i, &itemPath );
			xmp.GetProperty_Int ( xmpNS, itemPath.c_str(), &int32, 0 );
			uns16 = (XMP_Uns16)int32;
			if ( ! nativeEndian ) uns16 = Flip2 ( uns16 );
			*shortPtr = uns16;
		}
	
		tiff->SetTag ( ifd, mapInfo.id, kTIFF_ShortType, (XMP_Uns32)arraySize, &shortVector[0] );
	
	} else if ( mapInfo.type == kTIFF_RationalType ) {
		
		std::vector<XMP_Uns32> rationalVector;
		rationalVector.assign ( 2*arraySize, 0 );
		XMP_Uns32 * rationalPtr = (XMP_Uns32*) &rationalVector[0];
		
		std::string itemPath, xmpValue;
		XMP_Uns32 num, denom;
		for ( size_t i = 1; i <= arraySize; ++i, rationalPtr += 2 ) {
			SXMPUtils::ComposeArrayItemPath ( xmpNS, xmpArray, (XMP_Index)i, &itemPath );
			xmp.GetProperty ( xmpNS, itemPath.c_str(), &xmpValue, 0 );
			bool ok = DecodeRational ( xmpValue.c_str(), &num, &denom );
			if ( ! ok ) return;
			if ( ! nativeEndian ) { num = Flip4 ( num ); denom = Flip4 ( denom ); }
			rationalPtr[0] = num;
			rationalPtr[1] = denom;
		}
	
		tiff->SetTag ( ifd, mapInfo.id, kTIFF_RationalType, (XMP_Uns32)arraySize, &rationalVector[0] );

	}

}	// ExportArrayTIFF

// =================================================================================================
// ExportTIFF_StandardMappings
// ===========================

static void
ExportTIFF_StandardMappings ( XMP_Uns8 ifd, TIFF_Manager * tiff, const SXMPMeta & xmp )
{
	const bool nativeEndian = tiff->IsNativeEndian();
	TIFF_Manager::TagInfo tagInfo;
	std::string xmpValue;
	XMP_OptionBits xmpForm;

	const TIFF_MappingToXMP * mappings = 0;

	if ( ifd == kTIFF_PrimaryIFD ) {
		mappings = sPrimaryIFDMappings;
	} else if ( ifd == kTIFF_ExifIFD ) {
		mappings = sExifIFDMappings;
	} else if ( ifd == kTIFF_GPSInfoIFD ) {
		mappings = sGPSInfoIFDMappings;
	} else {
		XMP_Throw ( "Invalid IFD for standard mappings", kXMPErr_InternalFailure );
	}

	for ( size_t i = 0; mappings[i].id != 0xFFFF; ++i ) {

		try {	// Don't let errors with one stop the others.

			const TIFF_MappingToXMP & mapInfo =  mappings[i];

			if ( mapInfo.exportMode == kExport_Never ) continue;
			if ( mapInfo.name[0] == 0 ) continue;	// Skip special mappings, handled higher up.

			bool haveTIFF = tiff->GetTag ( ifd, mapInfo.id, &tagInfo );
			if ( haveTIFF && (mapInfo.exportMode == kExport_InjectOnly) ) continue;
			
			bool haveXMP  = xmp.GetProperty ( mapInfo.ns, mapInfo.name, &xmpValue, &xmpForm );
			if ( ! haveXMP ) {
			
				if ( haveTIFF && (mapInfo.exportMode == kExport_Always) ) tiff->DeleteTag ( ifd, mapInfo.id );

			} else {
			
				XMP_Assert ( tagInfo.type != kTIFF_UndefinedType );	// These must have a special mapping.
				if ( tagInfo.type == kTIFF_UndefinedType ) continue;
	
				const bool mapSingle = ((mapInfo.count == 1) || (mapInfo.type == kTIFF_ASCIIType));
				if ( mapSingle ) {
					if ( ! XMP_PropIsSimple ( xmpForm ) ) continue;	// ? Notify client?
					ExportSingleTIFF ( tiff, ifd, mapInfo, nativeEndian, xmpValue );
				} else {
					if ( ! XMP_PropIsArray ( xmpForm ) ) continue;	// ? Notify client?
					ExportArrayTIFF ( tiff, ifd, mapInfo, nativeEndian, xmp, mapInfo.ns, mapInfo.name );
				}
				
			}

		} catch ( ... ) {

			// Do nothing, let other imports proceed.
			// ? Notify client?

		}

	}

}	// ExportTIFF_StandardMappings

// =================================================================================================
// ExportTIFF_Date
// ===============
//
// Convert  an XMP date/time to an Exif 2.2 master date/time tag plus associated fractional seconds.
// The Exif date/time part is a 20 byte ASCII value formatted as "YYYY:MM:DD HH:MM:SS" with a
// terminating nul. The fractional seconds are a nul terminated ASCII string with possible space
// padding. They are literally the fractional part, the digits that would be to the right of the
// decimal point.

static void
ExportTIFF_Date ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp, TIFF_Manager * tiff, XMP_Uns16 mainID )
{
	XMP_Uns8 mainIFD = kTIFF_ExifIFD;
	XMP_Uns16 fracID;
	switch ( mainID ) {
		case kTIFF_DateTime : mainIFD = kTIFF_PrimaryIFD; fracID = kTIFF_SubSecTime;	break;
		case kTIFF_DateTimeOriginal  : fracID = kTIFF_SubSecTimeOriginal;	break;
		case kTIFF_DateTimeDigitized : fracID = kTIFF_SubSecTimeDigitized;	break;
	}

	try {	// Don't let errors with one stop the others.

		std::string  xmpStr;
		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, &xmpStr, 0 );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( mainIFD, mainID );
			tiff->DeleteTag ( kTIFF_ExifIFD, fracID );	// ! The subseconds are always in the Exif IFD.
			return;
		}

		// Format using all of the numbers. Then overwrite blanks for missing fields. The fields
		// missing from the XMP are detected with length checks: YYYY-MM-DDThh:mm:ss
		//   < 18 - no seconds
		//   < 15 - no minutes
		//   < 12 - no hours
		//   <  9 - no day
		//   <  6 - no month
		//   <  1 - no year

		XMP_DateTime xmpBin;
		SXMPUtils::ConvertToDate ( xmpStr.c_str(), &xmpBin );
		
		char buffer[24];
		snprintf ( buffer, sizeof(buffer), "%04d:%02d:%02d %02d:%02d:%02d",	// AUDIT: Use of sizeof(buffer) is safe.
				   xmpBin.year, xmpBin.month, xmpBin.day, xmpBin.hour, xmpBin.minute, xmpBin.second );

		size_t xmpLen = xmpStr.size();
		if ( xmpLen < 18 ) {
			buffer[17] = buffer[18] = ' ';
			if ( xmpLen < 15 ) {
				buffer[14] = buffer[15] = ' ';
				if ( xmpLen < 12 ) {
					buffer[11] = buffer[12] = ' ';
					if ( xmpLen < 9 ) {
						buffer[8] = buffer[9] = ' ';
						if ( xmpLen < 6 ) {
							buffer[5] = buffer[6] = ' ';
							if ( xmpLen < 1 ) {
								buffer[0] = buffer[1] = buffer[2] = buffer[3] = ' ';
							}
						}
					}
				}
			}
		}
		
		tiff->SetTag_ASCII ( mainIFD, mainID, buffer );

		if ( xmpBin.nanoSecond == 0 ) {
		
			tiff->DeleteTag ( kTIFF_ExifIFD, fracID );
		
		} else {

			snprintf ( buffer, sizeof(buffer), "%09d", xmpBin.nanoSecond );	// AUDIT: Use of sizeof(buffer) is safe.
			for ( size_t i = strlen(buffer)-1; i > 0; --i ) {
				if ( buffer[i] != '0' ) break;
				buffer[i] = 0;	// Strip trailing zero digits.
			}

			tiff->SetTag_ASCII ( kTIFF_ExifIFD, fracID, buffer );	// ! The subseconds are always in the Exif IFD.

		}

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// ExportTIFF_Date

// =================================================================================================
// ExportTIFF_ArrayASCII
// =====================
//
// Catenate all of the XMP array values into a string. Use a "; " separator for Artist, nul for others.

static void
ExportTIFF_ArrayASCII ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string    itemValue, fullValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		if ( ! XMP_PropIsArray ( xmpFlags ) ) return;	// ? Complain? Delete the tag?

		if ( id == kTIFF_Artist ) {
			SXMPUtils::CatenateArrayItems ( xmp, xmpNS, xmpProp, 0, 0,
											(kXMP_PropArrayIsOrdered | kXMPUtil_AllowCommas), &fullValue );
			fullValue += '\x0';	// ! Need explicit final nul for SetTag below.
		} else {
			size_t count = xmp.CountArrayItems ( xmpNS, xmpProp );
			for ( size_t i = 1; i <= count; ++i ) {	// ! XMP arrays are indexed from 1.
				(void) xmp.GetArrayItem ( xmpNS, xmpProp, (XMP_Index)i, &itemValue, &xmpFlags );
				if ( ! XMP_PropIsSimple ( xmpFlags ) ) continue;	// ? Complain?
				fullValue.append ( itemValue );
				fullValue.append ( 1, '\x0' );
			}
		}

		tiff->SetTag ( ifd, id, kTIFF_ASCIIType, (XMP_Uns32)fullValue.size(), fullValue.c_str() );	// ! Already have trailing nul.

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// ExportTIFF_ArrayASCII

// =================================================================================================
// ExportTIFF_LocTextASCII
// ======================

static void
ExportTIFF_LocTextASCII ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						  TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string xmpValue;

		bool foundXMP = xmp.GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, &xmpValue, 0 );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		tiff->SetTag ( ifd, id, kTIFF_ASCIIType, (XMP_Uns32)( xmpValue.size()+1 ), xmpValue.c_str() );

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// ExportTIFF_LocTextASCII

// =================================================================================================
// ExportTIFF_EncodedString
// ========================

static void
ExportTIFF_EncodedString ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						   TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id, bool isLangAlt = false )
{
	try {	// Don't let errors with one stop the others.

		std::string    xmpValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, &xmpValue, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		if ( ! isLangAlt ) {
			if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the tag?
		} else {
			if ( ! XMP_ArrayIsAltText ( xmpFlags ) ) return;	// ? Complain? Delete the tag?
			bool ok = xmp.GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, &xmpValue, 0 );
			if ( ! ok ) return;	// ? Complain? Delete the tag?
		}

		XMP_Uns8 encoding = kTIFF_EncodeASCII;
		for ( size_t i = 0; i < xmpValue.size(); ++i ) {
			if ( (XMP_Uns8)xmpValue[i] >= 0x80 ) {
				encoding = kTIFF_EncodeUnicode;
				break;
			}
		}

		tiff->SetTag_EncodedString ( ifd, id, xmpValue.c_str(), encoding );

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// ExportTIFF_EncodedString

// =================================================================================================
// ExportTIFF_GPSCoordinate
// ========================
//
// The XMP format is either "deg,min,secR" or "deg,min.fracR", where 'R' is the reference direction,
// 'N', 'S', 'E', or 'W'. The location gets output as ( deg/1, min/1, sec/1 ) for the first form,
// and ( deg/1, minFrac/denom, 0/1 ) for the second form.

// ! We arbitrarily limit the number of fractional minute digits to 6 to avoid overflow in the
// ! combined numerator. But we don't otherwise check for overflow or range errors.

static void
ExportTIFF_GPSCoordinate ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						   TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 _id )
{
	XMP_Uns16 refID = _id-1;	// ! The GPS refs and locations are all tag N-1 and N pairs.
	XMP_Uns16 locID = _id;

	XMP_Assert ( (locID & 1) == 0 );

	try {	// Don't let errors with one stop the others. Tolerate ill-formed values where reasonable.

		std::string xmpValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, &xmpValue, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, refID );
			tiff->DeleteTag ( ifd, locID );
			return;
		}

		if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;

		const char * chPtr = xmpValue.c_str();	// Guaranteed to have a nul terminator.

		XMP_Uns32 deg=0, minNum=0, minDenom=1, sec=0;

		// Extract the degree part, it must be present.

		while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;
		if ( (*chPtr < '0') || (*chPtr > '9') ) return;	// Bad XMP string.
		for ( ; ('0' <= *chPtr) && (*chPtr <= '9'); ++chPtr ) deg = deg*10 + (*chPtr - '0');
		while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;
		if ( (*chPtr == ',') || (*chPtr == ';') ) ++chPtr;
		while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;

		// Extract the whole minutes if present.

		if ( ('0' <= *chPtr) && (*chPtr <= '9') ) {

			for ( ; ('0' <= *chPtr) && (*chPtr <= '9'); ++chPtr ) minNum = minNum*10 + (*chPtr - '0');
			
			// Extract the fractional minutes or seconds if present.
			
			if ( *chPtr == '.' ) {

				++chPtr;	// Skip the period.
				for ( ; ('0' <= *chPtr) && (*chPtr <= '9'); ++chPtr ) {
					if ( minDenom > 100*1000 ) continue;	// Don't accumulate any more digits.
					minDenom *= 10;
					minNum = minNum*10 + (*chPtr - '0');
				}

			} else {

				while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;
				if ( (*chPtr == ',') || (*chPtr == ';') ) ++chPtr;
				while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;
				for ( ; ('0' <= *chPtr) && (*chPtr <= '9'); ++chPtr ) sec = sec*10 + (*chPtr - '0');

			}

		}
		
		// The compass direction part is required. But it isn't worth the bother to make sure it is
		// appropriate for the tag. Little chance of ever seeing an error, it causes no great harm.

		while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;
		if ( (*chPtr == ',') || (*chPtr == ';') ) ++chPtr;	// Tolerate ',' or ';' here also.
		while ( (*chPtr == ' ') || (*chPtr == '\t') ) ++chPtr;

		char ref[2];
		ref[0] = *chPtr;
		ref[1] = 0;

		if ( ('a' <= ref[0]) && (ref[0] <= 'z') ) ref[0] -= 0x20;
		if ( (ref[0] != 'N') && (ref[0] != 'S') && (ref[0] != 'E') && (ref[0] != 'W') ) return;

		tiff->SetTag ( ifd, refID, kTIFF_ASCIIType, 2, &ref[0] );

		XMP_Uns32 loc[6];
		tiff->PutUns32 ( deg, &loc[0] );
		tiff->PutUns32 ( 1,   &loc[1] );
		tiff->PutUns32 ( minNum,   &loc[2] );
		tiff->PutUns32 ( minDenom, &loc[3] );
		tiff->PutUns32 ( sec, &loc[4] );
		tiff->PutUns32 ( 1,   &loc[5] );

		tiff->SetTag ( ifd, locID, kTIFF_RationalType, 3, &loc[0] );

	} catch ( ... ) {

		// Do nothing, let other exports proceed.
		// ? Notify client?

	}

}	// ExportTIFF_GPSCoordinate

// =================================================================================================
// ExportTIFF_GPSTimeStamp
// =======================
//
// The Exif is in 2 tags, GPSTimeStamp and GPSDateStamp. The time is 3 rationals for the hour, minute,
// and second in UTC. The date is a nul terminated string "YYYY:MM:DD".

static const double kBillion = 1000.0*1000.0*1000.0;
static const double mMaxSec  = 4.0*kBillion - 1.0;

static void
ExportTIFF_GPSTimeStamp ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp, TIFF_Manager * tiff )
{
	
	try {	// Don't let errors with one stop the others.

		XMP_DateTime binXMP;
		bool foundXMP = xmp.GetProperty_Date ( xmpNS, xmpProp, &binXMP, 0 );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( kTIFF_GPSInfoIFD, kTIFF_GPSTimeStamp );
			tiff->DeleteTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDateStamp );
			return;
		}
		
		SXMPUtils::ConvertToUTCTime ( &binXMP );

		XMP_Uns32 exifTime[6];
		tiff->PutUns32 ( binXMP.hour, &exifTime[0] );
		tiff->PutUns32 ( 1, &exifTime[1] );
		tiff->PutUns32 ( binXMP.minute, &exifTime[2] );
		tiff->PutUns32 ( 1, &exifTime[3] );
		if ( binXMP.nanoSecond == 0 ) {
			tiff->PutUns32 ( binXMP.second, &exifTime[4] );
			tiff->PutUns32 ( 1,   &exifTime[5] );
		} else {
			double fSec = (double)binXMP.second + ((double)binXMP.nanoSecond / kBillion );
			XMP_Uns32 denom = 1000*1000;	// Choose microsecond resolution by default.
			TIFF_Manager::TagInfo oldInfo;
			bool hadExif = tiff->GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSTimeStamp, &oldInfo );
			if ( hadExif && (oldInfo.type == kTIFF_RationalType) && (oldInfo.count == 3) ) {
				XMP_Uns32 oldDenom = tiff->GetUns32 ( &(((XMP_Uns32*)oldInfo.dataPtr)[5]) );
				if ( oldDenom != 1 ) denom = oldDenom;
			}
			fSec *= denom;
			while ( fSec > mMaxSec ) { fSec /= 10; denom /= 10; }
			tiff->PutUns32 ( (XMP_Uns32)fSec, &exifTime[4] );
			tiff->PutUns32 ( denom, &exifTime[5] );
		}
		tiff->SetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSTimeStamp, kTIFF_RationalType, 3, &exifTime[0] );

		char exifDate[16];	// AUDIT: Long enough, only need 11.
		snprintf ( exifDate, 12, "%04d:%02d:%02d", binXMP.year, binXMP.month, binXMP.day );
		if ( exifDate[10] == 0 ) {	// Make sure there is no value overflow.
			tiff->SetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDateStamp, kTIFF_ASCIIType, 11, exifDate );
		}

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// ExportTIFF_GPSTimeStamp

// =================================================================================================

static void ExportTIFF_PhotographicSensitivity ( SXMPMeta * xmp, TIFF_Manager * exif ) {

	// PhotographicSensitivity has special cases for values over 65534 because the tag is SHORT. It
	// has a count of "any", but all known cameras used a count of 1. Exif 2.3 still allows a count
	// of "any" but says only 1 value should ever be recorded. We only process the first value if
	// the count is greater than 1.
	//
	// Prior to Exif 2.3 the tag was called ISOSpeedRatings. Some cameras omit the tag and some
	// write 65535. Most put the real ISO in MakerNote, which be in the XMP from ACR.
	//
	// With Exif 2.3 five speed-related tags were added of type LONG: StandardOutputSensitivity,
	// RecommendedExposureIndex, ISOSpeed, ISOSpeedLatitudeyyy, and ISOSpeedLatitudezzz. The tag
	// SensitivityType was added to say which of these five is copied to PhotographicSensitivity.
	//
	// Export logic:
	//	If ExifVersion is less than "0230" (or missing)
	//		If exif:ISOSpeedRatings[1] <= 65535: inject tag (if missing), remove exif:ISOSpeedRatings
	//	Else (ExifVersion is at least "0230")
	//		If no exifEX:PhotographicSensitivity: set it from exif:ISOSpeedRatings[1]
	//		Remove exif:ISOSpeedRatings (to not be saved in namespace cleanup)
	//		If exifEX:PhotographicSensitivity <= 65535: inject tag (if missing)
	//		Else if exifEX:PhotographicSensitivity over 65535
	//			If no PhotographicSensitivity tag and no SensitivityType tag and no ISOSpeed tag:
	//				Inject PhotographicSensitivity tag as 65535
	//				If no exifEX:SensitivityType and no exifEX:ISOSpeed
	//					Set exifEX:SensitivityType to 3
	//					Set exifEX:ISOSpeed to exifEX:PhotographicSensitivity
	//		Inject SensitivityType and long tags (if missing)
	//	Save exif:ISOSpeedRatings when cleaning namespaces (done in ExportPhotoData)
	
	try {
	
		bool foundXMP, foundExif;
		TIFF_Manager::TagInfo tagInfo;
		std::string xmpValue;
		XMP_OptionBits flags;
		XMP_Int32 binValue;
	
		bool haveOldExif = true;	// Default to old Exif if no version tag.
	
		foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, &tagInfo );
		if ( foundExif && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
			haveOldExif = (strncmp ( (char*)tagInfo.dataPtr, "0230", 4 ) < 0);
		}
	
		if ( haveOldExif ) {	// Exif version is before 2.3, use just the old tag and property.
		
			foundXMP = xmp->GetProperty ( kXMP_NS_EXIF, "ISOSpeedRatings", 0, &flags );
			if ( foundXMP && XMP_PropIsArray(flags) && (xmp->CountArrayItems ( kXMP_NS_EXIF, "ISOSpeedRatings" ) > 0) ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_EXIF, "ISOSpeedRatings[1]", &binValue, 0 );
			}
			
			if ( ! foundXMP ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "PhotographicSensitivity", &binValue, 0 );
			}

			if ( foundXMP && (0 <= binValue) && (binValue <= 65535) ) {
				xmp->DeleteProperty ( kXMP_NS_EXIF, "ISOSpeedRatings" );	// So namespace cleanup won't keep it.
				foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, &tagInfo );
				if ( ! foundExif ) {
					exif->SetTag_Short ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, (XMP_Uns16)binValue );
				}
			}
		
		} else {	// Exif version is 2.3 or newer, use the Exif 2.3 tags and properties.
		
			// Deal with the special cases for exifEX:PhotographicSensitivity and exif:ISOSpeedRatings.

			if ( ! xmp->DoesPropertyExist ( kXMP_NS_ExifEX,  "PhotographicSensitivity" ) ) {
				foundXMP = xmp->GetProperty ( kXMP_NS_EXIF, "ISOSpeedRatings", 0, &flags );
				if ( foundXMP && XMP_PropIsArray(flags) &&
					 (xmp->CountArrayItems ( kXMP_NS_EXIF, "ISOSpeedRatings" ) > 0) ) {
					xmp->GetArrayItem ( kXMP_NS_EXIF, "ISOSpeedRatings", 1, &xmpValue, 0 );
					xmp->SetProperty ( kXMP_NS_ExifEX,  "PhotographicSensitivity", xmpValue.c_str() );
				}
			}
			
			xmp->DeleteProperty ( kXMP_NS_EXIF, "ISOSpeedRatings" );	// Don't want it kept after namespace cleanup.
			
			foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "PhotographicSensitivity", &binValue, 0 );

			if ( foundXMP && (0 <= binValue) && (binValue <= 65535) ) {	// The simpler low ISO case.
			
				foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, &tagInfo );
				if ( ! foundExif ) {
					exif->SetTag_Short ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, (XMP_Uns16)binValue );
				}
			
			} else if ( foundXMP ) {	// The more commplex high ISO case.
			
				bool havePhotographicSensitivityTag = exif->GetTag ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, 0 );
				bool haveSensitivityTypeTag = exif->GetTag ( kTIFF_ExifIFD, kTIFF_SensitivityType, 0 );
				bool haveISOSpeedTag = exif->GetTag ( kTIFF_ExifIFD, kTIFF_ISOSpeed, 0 );
			
				bool haveSensitivityTypeXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "SensitivityType" );
				bool haveISOSpeedXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "ISOSpeed" );
			
				if ( (! havePhotographicSensitivityTag) && (! haveSensitivityTypeTag) && (! haveISOSpeedTag) ) {

					exif->SetTag_Short ( kTIFF_ExifIFD, kTIFF_PhotographicSensitivity, 65535 );
					
					if ( (! haveSensitivityTypeXMP) && (! haveISOSpeedXMP) ) {
						xmp->SetProperty ( kXMP_NS_ExifEX, "SensitivityType", "3" );
						xmp->SetProperty_Int ( kXMP_NS_ExifEX, "ISOSpeed", binValue );
					}

				}
			
			}
	
			// Export SensitivityType and the related long tags. Must be done after the special
			// cases because they might set exifEX:SensitivityType and exifEX:ISOSpeed.
	
			foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_SensitivityType, &tagInfo );
			if ( ! foundExif ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "SensitivityType", &binValue, 0 );
				if ( foundXMP && (0 <= binValue) && (binValue <= 65535) ) {
					exif->SetTag_Short ( kTIFF_ExifIFD, kTIFF_SensitivityType, (XMP_Uns16)binValue );
				}
			}
	
			foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_StandardOutputSensitivity, &tagInfo );
			if ( ! foundExif ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "StandardOutputSensitivity", &binValue, 0 );
				if ( foundXMP && (binValue >= 0) ) {
					exif->SetTag_Long ( kTIFF_ExifIFD, kTIFF_StandardOutputSensitivity, (XMP_Uns32)binValue );
				}
			}
	
			foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_RecommendedExposureIndex, &tagInfo );
			if ( ! foundExif ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "RecommendedExposureIndex", &binValue, 0 );
				if ( foundXMP && (binValue >= 0) ) {
					exif->SetTag_Long ( kTIFF_ExifIFD, kTIFF_RecommendedExposureIndex, (XMP_Uns32)binValue );
				}
			}
	
			foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_ISOSpeed, &tagInfo );
			if ( ! foundExif ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "ISOSpeed", &binValue, 0 );
				if ( foundXMP && (binValue >= 0) ) {
					exif->SetTag_Long ( kTIFF_ExifIFD, kTIFF_ISOSpeed, (XMP_Uns32)binValue );
				}
			}
	
			foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_ISOSpeedLatitudeyyy, &tagInfo );
			if ( ! foundExif ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "ISOSpeedLatitudeyyy", &binValue, 0 );
				if ( foundXMP && (binValue >= 0) ) {
					exif->SetTag_Long ( kTIFF_ExifIFD, kTIFF_ISOSpeedLatitudeyyy, (XMP_Uns32)binValue );
				}
			}
	
			foundExif = exif->GetTag ( kTIFF_ExifIFD, kTIFF_ISOSpeedLatitudezzz, &tagInfo );
			if ( ! foundExif ) {
				foundXMP = xmp->GetProperty_Int ( kXMP_NS_ExifEX, "ISOSpeedLatitudezzz", &binValue, 0 );
				if ( foundXMP && (binValue >= 0) ) {
					exif->SetTag_Long ( kTIFF_ExifIFD, kTIFF_ISOSpeedLatitudezzz, (XMP_Uns32)binValue );
				}
			}
		
		}

	} catch ( ... ) {

		// Do nothing, don't let this failure stop other exports.

	}

}	// ExportTIFF_PhotographicSensitivity

// =================================================================================================
// =================================================================================================

// =================================================================================================
// PhotoDataUtils::ExportExif
// ==========================

void
PhotoDataUtils::ExportExif ( SXMPMeta * xmp, TIFF_Manager * exif )
{
	bool haveXMP;
	std::string xmpValue;

	XMP_Int32 int32;
	XMP_Uns8 uns8;
	
	// ---------------------------------------------------------
	// Read the old Adobe names for new Exif 2.3 tags if wanted.
	
	#if SupportOldExifProperties
	
		XMP_OptionBits flags;
	
		haveXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "PhotographicSensitivity" );
		if ( ! haveXMP ) {
			haveXMP = xmp->GetProperty ( kXMP_NS_EXIF, "ISOSpeedRatings", 0, &flags );
			if ( haveXMP && XMP_PropIsArray(flags) ) {
				haveXMP = xmp->GetArrayItem ( kXMP_NS_EXIF, "ISOSpeedRatings", 1, &xmpValue, 0 );
				if ( haveXMP ) xmp->SetProperty ( kXMP_NS_ExifEX, "PhotographicSensitivity", xmpValue.c_str() );
			}
		}
	
		haveXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "CameraOwnerName" );
		if ( ! haveXMP ) {
			haveXMP = xmp->GetProperty ( kXMP_NS_EXIF_Aux, "OwnerName", &xmpValue, 0 );
			if ( haveXMP ) xmp->SetProperty ( kXMP_NS_ExifEX, "CameraOwnerName", xmpValue.c_str() );
		}
	
		haveXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "BodySerialNumber" );
		if ( ! haveXMP ) {
			haveXMP = xmp->GetProperty ( kXMP_NS_EXIF_Aux, "SerialNumber", &xmpValue, 0 );
			if ( haveXMP ) xmp->SetProperty ( kXMP_NS_ExifEX, "BodySerialNumber", xmpValue.c_str() );
		}
	
		haveXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "LensModel" );
		if ( ! haveXMP ) {
			haveXMP = xmp->GetProperty ( kXMP_NS_EXIF_Aux, "Lens", &xmpValue, 0 );
			if ( haveXMP ) xmp->SetProperty ( kXMP_NS_ExifEX, "LensModel", xmpValue.c_str() );
		}
	
		haveXMP = xmp->DoesPropertyExist ( kXMP_NS_ExifEX, "LensSpecification" );
		if ( ! haveXMP ) {
			haveXMP = xmp->GetProperty ( kXMP_NS_EXIF_Aux, "LensInfo", &xmpValue, 0 );
			if ( haveXMP ) {
				size_t start, end;
				std::string nextItem;
				for ( start = 0; start < xmpValue.size(); start = end + 1 ) {
					end = xmpValue.find ( ' ', start );
					if ( end == start ) continue;
					if ( end == std::string::npos ) end = xmpValue.size();
					nextItem = xmpValue.substr ( start, (end-start) );
					xmp->AppendArrayItem ( kXMP_NS_ExifEX, "LensSpecification", kXMP_PropArrayIsOrdered, nextItem.c_str() );
				}
			}
		}
	
	#endif
	
	// Do all of the table driven standard exports.
	
	ExportTIFF_StandardMappings ( kTIFF_PrimaryIFD, exif, *xmp );
	ExportTIFF_StandardMappings ( kTIFF_ExifIFD, exif, *xmp );
	ExportTIFF_StandardMappings ( kTIFF_GPSInfoIFD, exif, *xmp );
	
	// --------------------------------------------------------------------------------------------
	// Fixup erroneous cases that appear to have a negative value for GPSAltitude in the Exif. This
	// treats any value with the high bit set as a negative, which is more likely in practice than
	// an actual value over 2 billion. The XMP was exported by the tables and is left alone since it
	// won't be kept in the file.
	
	TIFF_Manager::Rational altValue;
	bool haveExif = exif->GetTag_Rational ( kTIFF_GPSInfoIFD, kTIFF_GPSAltitude, &altValue );
	if ( haveExif ) {

		bool fixExif = false;

		if ( altValue.denom >> 31 ) {	// Shift the sign to the numerator.
			altValue.denom = -altValue.denom;
			altValue.num = -altValue.num;
			fixExif = true;
		}

		if ( altValue.num >> 31 ) {	// Fix the numerator and set GPSAltitudeRef.
			exif->SetTag_Byte ( kTIFF_GPSInfoIFD, kTIFF_GPSAltitudeRef, 1 );
			altValue.num = -altValue.num;
			fixExif = true;
		}

		if ( fixExif ) exif->SetTag_Rational ( kTIFF_GPSInfoIFD, kTIFF_GPSAltitude, altValue.num, altValue.denom );

	}

	// Export dc:description to TIFF ImageDescription, and exif:UserComment to EXIF UserComment.
	
	// *** This is not following the MWG guidelines. The policy here tries to be more backward compatible.

	ExportTIFF_LocTextASCII ( *xmp, kXMP_NS_DC, "description",
							  exif, kTIFF_PrimaryIFD, kTIFF_ImageDescription );

	ExportTIFF_EncodedString ( *xmp, kXMP_NS_EXIF, "UserComment",
							   exif, kTIFF_ExifIFD, kTIFF_UserComment, true /* isLangAlt */ );
	
	// Export all of the date/time tags.
	// ! Special case: Don't create Exif DateTimeDigitized. This can avoid PSD full rewrite due to
	// ! new mapping from xmp:CreateDate.

	if ( exif->GetTag ( kTIFF_ExifIFD, kTIFF_DateTimeDigitized, 0 ) ) {
		ExportTIFF_Date ( *xmp, kXMP_NS_XMP, "CreateDate", exif, kTIFF_DateTimeDigitized );
	}

	ExportTIFF_Date ( *xmp, kXMP_NS_EXIF, "DateTimeOriginal", exif, kTIFF_DateTimeOriginal );
	ExportTIFF_Date ( *xmp, kXMP_NS_XMP, "ModifyDate", exif, kTIFF_DateTime );
	
	// Export the remaining TIFF, Exif, and GPS IFD tags.

	ExportTIFF_ArrayASCII ( *xmp, kXMP_NS_DC, "creator", exif, kTIFF_PrimaryIFD, kTIFF_Artist );

	ExportTIFF_LocTextASCII ( *xmp, kXMP_NS_DC, "rights", exif, kTIFF_PrimaryIFD, kTIFF_Copyright );
		
	haveXMP = xmp->GetProperty ( kXMP_NS_EXIF, "ExifVersion", &xmpValue, 0 );
	if ( haveXMP && (xmpValue.size() == 4) && (! exif->GetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, 0 )) ) {
		// 36864 ExifVersion is 4 "undefined" ASCII characters.
		exif->SetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, kTIFF_UndefinedType, 4, xmpValue.data() );
	}

	// There are moderately complex export special cases for PhotographicSensitivity.
	ExportTIFF_PhotographicSensitivity ( xmp, exif );
	
	haveXMP = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "ComponentsConfiguration" );
	if ( haveXMP && (xmp->CountArrayItems ( kXMP_NS_EXIF, "ComponentsConfiguration" ) == 4) &&
		 (! exif->GetTag ( kTIFF_ExifIFD, kTIFF_ComponentsConfiguration, 0 )) ) {
		// 37121 ComponentsConfiguration is an array of 4 "undefined" UInt8 bytes.
		XMP_Uns8 compConfig[4];
		xmp->GetProperty_Int ( kXMP_NS_EXIF, "ComponentsConfiguration[1]", &int32, 0 );
		compConfig[0] = (XMP_Uns8)int32;
		xmp->GetProperty_Int ( kXMP_NS_EXIF, "ComponentsConfiguration[2]", &int32, 0 );
		compConfig[1] = (XMP_Uns8)int32;
		xmp->GetProperty_Int ( kXMP_NS_EXIF, "ComponentsConfiguration[3]", &int32, 0 );
		compConfig[2] = (XMP_Uns8)int32;
		xmp->GetProperty_Int ( kXMP_NS_EXIF, "ComponentsConfiguration[4]", &int32, 0 );
		compConfig[3] = (XMP_Uns8)int32;
		exif->SetTag ( kTIFF_ExifIFD, kTIFF_ComponentsConfiguration, kTIFF_UndefinedType, 4, &compConfig[0] );
	}
	
	haveXMP = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "Flash" );
	if ( haveXMP && (! exif->GetTag ( kTIFF_ExifIFD, kTIFF_Flash, 0 )) ) {
		// 37385 Flash is a UInt16 collection of bit fields and is mapped to a struct in XMP.
		XMP_Uns16 binFlash = 0;
		bool field;
		haveXMP = xmp->GetProperty_Bool ( kXMP_NS_EXIF, "Flash/exif:Fired", &field, 0 );
		if ( haveXMP & field ) binFlash |= 0x0001;
		haveXMP = xmp->GetProperty_Int ( kXMP_NS_EXIF, "Flash/exif:Return", &int32, 0 );
		if ( haveXMP ) binFlash |= (int32 & 3) << 1;
		haveXMP = xmp->GetProperty_Int ( kXMP_NS_EXIF, "Flash/exif:Mode", &int32, 0 );
		if ( haveXMP ) binFlash |= (int32 & 3) << 3;
		haveXMP = xmp->GetProperty_Bool ( kXMP_NS_EXIF, "Flash/exif:Function", &field, 0 );
		if ( haveXMP & field ) binFlash |= 0x0020;
		haveXMP = xmp->GetProperty_Bool ( kXMP_NS_EXIF, "Flash/exif:RedEyeMode", &field, 0 );
		if ( haveXMP & field ) binFlash |= 0x0040;
		exif->SetTag_Short ( kTIFF_ExifIFD, kTIFF_Flash, binFlash );
	}
	
	haveXMP = xmp->GetProperty_Int ( kXMP_NS_EXIF, "FileSource", &int32, 0 );
	if ( haveXMP && (! exif->GetTag ( kTIFF_ExifIFD, kTIFF_FileSource, 0 )) ) {
		// 41728 FileSource is an "undefined" UInt8.
		uns8 = (XMP_Uns8)int32;
		exif->SetTag ( kTIFF_ExifIFD, kTIFF_FileSource, kTIFF_UndefinedType, 1, &uns8 );
	}
	
	haveXMP = xmp->GetProperty_Int ( kXMP_NS_EXIF, "SceneType", &int32, 0 );
	if ( haveXMP && (! exif->GetTag ( kTIFF_ExifIFD, kTIFF_SceneType, 0 )) ) {
		// 41729 SceneType is an "undefined" UInt8.
		uns8 = (XMP_Uns8)int32;
		exif->SetTag ( kTIFF_ExifIFD, kTIFF_SceneType, kTIFF_UndefinedType, 1, &uns8 );
	}
	
	// *** Deferred inject-only tags: SpatialFrequencyResponse, DeviceSettingDescription, CFAPattern

	haveXMP = xmp->GetProperty ( kXMP_NS_EXIF, "GPSVersionID", &xmpValue, 0 );	// This is inject-only.
	if ( haveXMP && (! exif->GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSVersionID, 0 )) ) {
		XMP_Uns8 gpsID[4];	// 0 GPSVersionID is 4 UInt8 bytes and mapped in XMP as "n.n.n.n".
		unsigned int fields[4];	// ! Need ints for output from sscanf.
		int count = sscanf ( xmpValue.c_str(), "%u.%u.%u.%u", &fields[0], &fields[1], &fields[2], &fields[3] );
		if ( (count == 4) && (fields[0] <= 255) && (fields[1] <= 255) && (fields[2] <= 255) && (fields[3] <= 255) ) {
			gpsID[0] = fields[0]; gpsID[1] = fields[1]; gpsID[2] = fields[2]; gpsID[3] = fields[3];
			exif->SetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSVersionID, kTIFF_ByteType, 4, &gpsID[0] );
		}
	}
	
	ExportTIFF_GPSCoordinate ( *xmp, kXMP_NS_EXIF, "GPSLatitude", exif, kTIFF_GPSInfoIFD, kTIFF_GPSLatitude );

	ExportTIFF_GPSCoordinate ( *xmp, kXMP_NS_EXIF, "GPSLongitude", exif, kTIFF_GPSInfoIFD, kTIFF_GPSLongitude );
	
	ExportTIFF_GPSTimeStamp ( *xmp, kXMP_NS_EXIF, "GPSTimeStamp", exif );

	// The following GPS tags are inject-only.
	
	haveXMP = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "GPSDestLatitude" );
	if ( haveXMP && (! exif->GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDestLatitude, 0 )) ) {
		ExportTIFF_GPSCoordinate ( *xmp, kXMP_NS_EXIF, "GPSDestLatitude", exif, kTIFF_GPSInfoIFD, kTIFF_GPSDestLatitude );
	}

	haveXMP = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "GPSDestLongitude" );
	if ( haveXMP && (! exif->GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDestLongitude, 0 )) ) {
		ExportTIFF_GPSCoordinate ( *xmp, kXMP_NS_EXIF, "GPSDestLongitude", exif, kTIFF_GPSInfoIFD, kTIFF_GPSDestLongitude );
	}

	haveXMP = xmp->GetProperty ( kXMP_NS_EXIF, "GPSProcessingMethod", &xmpValue, 0 );
	if ( haveXMP && (! xmpValue.empty()) && (! exif->GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSProcessingMethod, 0 )) ) {
		// 27 GPSProcessingMethod is a string with explicit encoding.
		ExportTIFF_EncodedString ( *xmp, kXMP_NS_EXIF, "GPSProcessingMethod", exif, kTIFF_GPSInfoIFD, kTIFF_GPSProcessingMethod );
	}

	haveXMP = xmp->GetProperty ( kXMP_NS_EXIF, "GPSAreaInformation", &xmpValue, 0 );
	if ( haveXMP && (! xmpValue.empty()) && (! exif->GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSAreaInformation, 0 )) ) {
		// 28 GPSAreaInformation is a string with explicit encoding.
		ExportTIFF_EncodedString ( *xmp, kXMP_NS_EXIF, "GPSAreaInformation", exif, kTIFF_GPSInfoIFD, kTIFF_GPSAreaInformation );
	}
	
}	// PhotoDataUtils::ExportExif
