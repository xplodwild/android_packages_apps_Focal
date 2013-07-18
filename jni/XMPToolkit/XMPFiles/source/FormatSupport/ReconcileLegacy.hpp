#ifndef __ReconcileLegacy_hpp__
#define __ReconcileLegacy_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"

// =================================================================================================
/// \file ReconcileLegacy.hpp
/// \brief Utilities to reconcile between XMP and photo metadata forms such as TIFF/Exif and IPTC.
///
// =================================================================================================

// ImportPhotoData imports TIFF/Exif and IPTC metadata from JPEG, TIFF, and Photoshop files into
// XMP. The caller must have already done the file specific processing to select the appropriate
// sources of the TIFF stream, the Photoshop image resources, and the IPTC.
//
// The reconciliation logic used here is based on the Metadata Working Group guidelines. This is a
// simpler approach than used previously - which was modeled after historical Photoshop behavior.

enum {	// Bits for the options to ImportJTPtoXMP.
	k2XMP_FileHadXMP  = 0x0001,	// Set if the file had an XMP packet.
	k2XMP_FileHadIPTC = 0x0002,	// Set if the file had legacy IPTC.
	k2XMP_FileHadExif = 0x0004	// Set if the file had legacy Exif.
};

extern void ImportPhotoData ( const TIFF_Manager & exif,
						 	  const IPTC_Manager & iptc,
							  const PSIR_Manager & psir,
							  int                  iptcDigestState,
							  SXMPMeta *		   xmp,
							  XMP_OptionBits	   options = 0 );

// ExportPhotoData exports XMP into TIFF/Exif and IPTC metadata for JPEG, TIFF, and Photoshop files.

extern void ExportPhotoData ( XMP_FileFormat destFormat,
							  SXMPMeta *     xmp,
							  TIFF_Manager * exif, // Pass 0 if not wanted.
							  IPTC_Manager * iptc, // Pass 0 if not wanted.
							  PSIR_Manager * psir, // Pass 0 if not wanted.
							  XMP_OptionBits options = 0 );

// *** Mapping notes need revision for MWG related changes.

// =================================================================================================
// Summary of TIFF/Exif mappings to XMP
// ====================================
//
// The mapping for each tag is driven mainly by the tag ID, and secondarily by the type. E.g. there
// is no blanket rule that all ASCII tags are mapped to simple strings in XMP. Some, such as
// SubSecTime or GPSLatitudeRef, are combined with other tags; others, like Flash, are reformated.
// However, most tags are in fact mapped in an obvious manner based on their type and count.
//
// Photoshop practice has been to truncate ASCII tags at the first NUL, not supporting the TIFF
// specification's notion of multi-part ASCII values.
//
// Rational values are mapped to XMP as "num/denom".
//
// The tags of UNDEFINED type that are mapped to XMP text are either special cases like ExifVersion
// or the strings with an explicit encoding like UserComment.
//
// Latitude and logitude are mapped to XMP as "DDD,MM,SSk" or "DDD,MM.mmk"; k is N, S, E, or W.
//
// Flash struct in XMP separates the Fired, Return, Mode, Function, and RedEyeMode portions of the
// Exif value. Fired, Function, and RedEyeMode are Boolean; Return and Mode are integers.
//
// The OECF/SFR, CFA, and DeviceSettings tables are described in the XMP spec.
//
// Instead of iterating through all tags in the various IFDs, it is probably more efficient to have
// explicit processing for the tags that get special treatment, and a static table listing those
// that get mapped by type and count. The type and count processing will verify that the actual
// type and count are as expected, if not the tag is ignored.
//
// Here are the primary (0th) IFD tags that get special treatment:
//
// 270, 33432 - ASCII mapped to alt-text['x-default']
// 306 - DateTime master
// 315 - ASCII mapped to text seq[1]
//
// Here are the primary (0th) IFD tags that get mapped by type and count:
//
// 256, 257, 258, 259, 262, 271, 272, 274, 277, 282, 283, 284, 296, 301, 305, 318, 319,
// 529, 530, 531, 532
//
// Here are the Exif IFD tags that get special treatment:
//
// 34856, 41484 - OECF/SFR table
// 36864, 40960 - 4 ASCII chars to text
// 36867, 36868 - DateTime master
// 37121 - 4 UInt8 to integer seq
// 37385 - Flash struct
// 37510 - explicitly encoded text to alt-text['x-default']
// 41728, 41729 - UInt8 to integer
// 41730 - CFA table
// 41995 - DeviceSettings table
//
// Here are the Exif IFD tags that get mapped by type and count:
//
// 33434, 33437, 34850, 34852, 34855, 37122, 37377, 37378, 37379, 37380, 37381, 37382, 37383, 37384,
// 37386, 37396, 40961, 40962, 40963, 40964, 41483, 41486, 41487, 41488, 41492, 41493, 41495, 41985,
// 41986, 41987, 41988, 41989, 41990, 41991, 41992, 41993, 41994, 41996, 42016
//
// Here are the GPS IFD tags that get special treatment:
//
// 0 - 4 UInt8 to text "n.n.n.n"
// 2, 4, 20, 22 - Latitude or longitude master
// 7 - special DateTime master, the time part
// 27, 28 - explicitly encoded text
//
// Here are the GPS IFD tags that get mapped by type and count:
//
// 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 23, 24, 25, 26, 30
// =================================================================================================

// *** What about the Camera Raw tags that MDKit maps:
// ***	0xFDE8, 0xFDE9, 0xFDEA, 0xFE4C, 0xFE4D, 0xFE4E, 0xFE4F, 0xFE50, 0xFE51, 0xFE52, 0xFE53,
// ***	0xFE54, 0xFE55, 0xFE56, 0xFE57, 0xFE58

// =================================================================================================
// Summary of TIFF/Exif mappings from XMP
// ======================================
//
// Only a small number of properties are written back from XMP to TIFF/Exif. Most of the TIFF/Exif
// tags mapped into XMP are information about the image or capture process, not things that users
// should be editing. The tags that can be edited and written back to TIFF/Exif are:
//
// 270, 274, 282, 283, 296, 305, 306, 315, 33432; 36867, 36868, 37510, 40964
// =================================================================================================

// =================================================================================================
// Details of TIFF/Exif mappings
// =============================
//
// General (primary and thumbnail, 0th and 1st) IFD tags
//   tag  TIFF type    count  Name                       XMP mapping
//
//   256  SHORTorLONG      1  ImageWidth                 integer
//   257  SHORTorLONG      1  ImageLength                integer
//   258  SHORT            3  BitsPerSample              integer seq
//   259  SHORT            1  Compression                integer
//   262  SHORT            1  PhotometricInterpretation  integer
//   270  ASCII          Any  ImageDescription           text, dc:description['x-default']
//   271  ASCII          Any  Make                       text
//   272  ASCII          Any  Model                      text
//   274  SHORT            1  Orientation                integer
//   277  SHORT            1  SamplesPerPixel            integer
//   282  RATIONAL         1  XResolution                rational
//   283  RATIONAL         1  YResolution                rational
//   284  SHORT            1  PlanarConfiguration        integer
//   296  SHORT            1  ResolutionUnit             integer
//   301  SHORT        3*256  TransferFunction           integer seq
//   305  ASCII          Any  Software                   text, xmp:CreatorTool
//   306  ASCII           20  DateTime                   date, master of 37520, xmp:DateTime
//   315  ASCII          Any  Artist                     text, dc:creator[1]
//   318  RATIONAL         2  WhitePoint                 rational seq
//   319  RATIONAL         6  PrimaryChromaticities      rational seq
//   529  RATIONAL         3  YCbCrCoefficients          rational seq
//   530  SHORT            2  YCbCrSubSampling           integer seq
//   531  SHORT            1  YCbCrPositioning           integer
//   532  RATIONAL         6  ReferenceBlackWhite        rational seq
// 33432  ASCII          Any  Copyright                  text, dc:rights['x-default']
//
// Exif IFD tags
//   tag  TIFF type    count  Name                       XMP mapping
//
// 33434  RATIONAL         1  ExposureTime               rational
// 33437  RATIONAL         1  FNumber                    rational
// 34850  SHORT            1  ExposureProgram            integer
// 34852  ASCII          Any  SpectralSensitivity        text
// 34855  SHORT          Any  ISOSpeedRatings            integer seq
// 34856  UNDEFINED      Any  OECF                       OECF/SFR table
// 36864  UNDEFINED        4  ExifVersion                text, Exif has 4 ASCII chars
// 36867  ASCII           20  DateTimeOriginal           date, master of 37521
// 36868  ASCII           20  DateTimeDigitized          date, master of 37522
// 37121  UNDEFINED        4  ComponentsConfiguration    integer seq, Exif has 4 UInt8
// 37122  RATIONAL         1  CompressedBitsPerPixel     rational
// 37377  SRATIONAL        1  ShutterSpeedValue          rational
// 37378  RATIONAL         1  ApertureValue              rational
// 37379  SRATIONAL        1  BrightnessValue            rational
// 37380  SRATIONAL        1  ExposureBiasValue          rational
// 37381  RATIONAL         1  MaxApertureValue           rational
// 37382  RATIONAL         1  SubjectDistance            rational
// 37383  SHORT            1  MeteringMode               integer
// 37384  SHORT            1  LightSource                integer
// 37385  SHORT            1  Flash                      Flash struct
// 37386  RATIONAL         1  FocalLength                rational
// 37396  SHORT         2..4  SubjectArea                integer seq
// 37510  UNDEFINED      Any  UserComment                text, explicit encoding, exif:UserComment['x-default]
// 37520  ASCII          Any  SubSecTime                 date, with 306
// 37521  ASCII          Any  SubSecTimeOriginal         date, with 36867
// 37522  ASCII          Any  SubSecTimeDigitized        date, with 36868
// 40960  UNDEFINED        4  FlashpixVersion            text, Exif has 4 ASCII chars
// 40961  SHORT            1  ColorSpace                 integer
// 40962  SHORTorLONG      1  PixelXDimension            integer
// 40963  SHORTorLONG      1  PixelYDimension            integer
// 40964  ASCII           13  RelatedSoundFile           text
// 41483  RATIONAL         1  FlashEnergy                rational
// 41484  UNDEFINED      Any  SpatialFrequencyResponse   OECF/SFR table
// 41486  RATIONAL         1  FocalPlaneXResolution      rational
// 41487  RATIONAL         1  FocalPlaneYResolution      rational
// 41488  SHORT            1  FocalPlaneResolutionUnit   integer
// 41492  SHORT            2  SubjectLocation            integer seq
// 41493  RATIONAL         1  ExposureIndex              rational
// 41495  SHORT            1  SensingMethod              integer
// 41728  UNDEFINED        1  FileSource                 integer, Exif has UInt8
// 41729  UNDEFINED        1  SceneType                  integer, Exif has UInt8
// 41730  UNDEFINED      Any  CFAPattern                 CFA table
// 41985  SHORT            1  CustomRendered             integer
// 41986  SHORT            1  ExposureMode               integer
// 41987  SHORT            1  WhiteBalance               integer
// 41988  RATIONAL         1  DigitalZoomRatio           rational
// 41989  SHORT            1  FocalLengthIn35mmFilm      integer
// 41990  SHORT            1  SceneCaptureType           integer
// 41991  SHORT            1  GainControl                integer
// 41992  SHORT            1  Contrast                   integer
// 41993  SHORT            1  Saturation                 integer
// 41994  SHORT            1  Sharpness                  integer
// 41995  UNDEFINED      Any  DeviceSettingDescription   DeviceSettings table
// 41996  SHORT            1  SubjectDistanceRange       integer
// 42016  ASCII           33  ImageUniqueID              text
//
// GPS IFD tags
//   tag  TIFF type    count  Name                       XMP mapping
//
//     0  BYTE             4  GPSVersionID               text, "n.n.n.n", Exif has 4 UInt8
//     1  ASCII            2  GPSLatitudeRef             latitude, with 2
//     2  RATIONAL         3  GPSLatitude                latitude, master of 2
//     3  ASCII            2  GPSLongitudeRef            longitude, with 4
//     4  RATIONAL         3  GPSLongitude               longitude, master of 3
//     5  BYTE             1  GPSAltitudeRef             integer
//     6  RATIONAL         1  GPSAltitude                rational
//     7  RATIONAL         3  GPSTimeStamp               date, master of 29
//     8  ASCII          Any  GPSSatellites              text
//     9  ASCII            2  GPSStatus                  text
//    10  ASCII            2  GPSMeasureMode             text
//    11  RATIONAL         1  GPSDOP                     rational
//    12  ASCII            2  GPSSpeedRef                text
//    13  RATIONAL         1  GPSSpeed                   rational
//    14  ASCII            2  GPSTrackRef                text
//    15  RATIONAL         1  GPSTrack                   rational
//    16  ASCII            2  GPSImgDirectionRef         text
//    17  RATIONAL         1  GPSImgDirection            rational
//    18  ASCII          Any  GPSMapDatum                text
//    19  ASCII            2  GPSDestLatitudeRef         latitude, with 20
//    20  RATIONAL         3  GPSDestLatitude            latitude, master of 19
//    21  ASCII            2  GPSDestLongitudeRef        longitude, with 22
//    22  RATIONAL         3  GPSDestLongitude           logitude, master of 21
//    23  ASCII            2  GPSDestBearingRef          text
//    24  RATIONAL         1  GPSDestBearing             rational
//    25  ASCII            2  GPSDestDistanceRef         text
//    26  RATIONAL         1  GPSDestDistance            rational
//    27  UNDEFINED      Any  GPSProcessingMethod        text, explicit encoding
//    28  UNDEFINED      Any  GPSAreaInformation         text, explicit encoding
//    29  ASCII           11  GPSDateStamp               date, with 29
//    30  SHORT            1  GPSDifferential            integer
//
// =================================================================================================

// =================================================================================================

#endif	// #ifndef __ReconcileLegacy_hpp__
