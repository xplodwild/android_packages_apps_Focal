/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.sanselan.formats.tiff.constants;

public interface TiffTagConstants
		extends
			TiffDirectoryConstants,
			TiffFieldTypeConstants
{

	public static final TagInfo TIFF_TAG_NEW_SUBFILE_TYPE = new TagInfo(
			"New Subfile Type", 0xFE, FIELD_TYPE_DESCRIPTION_LONG, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_SUBFILE_TYPE = new TagInfo(
			"Subfile Type", 0xFF, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_IMAGE_WIDTH = new TagInfo(
			"Image Width", 0x100, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_IMAGE_LENGTH = new TagInfo(
			"Image Length", 0x101, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_BITS_PER_SAMPLE = new TagInfo(
			"Bits Per Sample", 0x102, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_COMPRESSION = new TagInfo(
			"Compression", 0x103, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_PHOTOMETRIC_INTERPRETATION = new TagInfo(
			"Photometric Interpretation", 0x106, FIELD_TYPE_DESCRIPTION_SHORT,
			1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_THRESHHOLDING = new TagInfo(
			"Threshholding", 0x107, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_CELL_WIDTH = new TagInfo("Cell Width",
			0x108, FIELD_TYPE_DESCRIPTION_SHORT, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_CELL_LENGTH = new TagInfo(
			"Cell Length", 0x109, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_FILL_ORDER = new TagInfo("Fill Order",
			0x10A, FIELD_TYPE_DESCRIPTION_SHORT, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_DOCUMENT_NAME = new TagInfo(
			"Document Name", 0x10D, FIELD_TYPE_DESCRIPTION_ASCII, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_IMAGE_DESCRIPTION = new TagInfo(
			"Image Description", 0x10E, FIELD_TYPE_DESCRIPTION_ASCII, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_MAKE = new TagInfo("Make", 0x10F,
			FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_MODEL = new TagInfo("Model", 0x110,
			FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_STRIP_OFFSETS = new TagInfo.Offset(
			"Strip Offsets", 0x111, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_ORIENTATION = new TagInfo(
			"Orientation", 0x112, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_SAMPLES_PER_PIXEL = new TagInfo(
			"Samples Per Pixel", 0x115, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_ROWS_PER_STRIP = new TagInfo(
			"Rows Per Strip", 0x116, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_STRIP_BYTE_COUNTS = new TagInfo(
			"Strip Byte Counts", 0x117, FIELD_TYPE_DESCRIPTION_LONG_OR_SHORT,
			-1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_MIN_SAMPLE_VALUE = new TagInfo(
			"Min Sample Value", 0x118, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_MAX_SAMPLE_VALUE = new TagInfo(
			"Max Sample Value", 0x119, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_XRESOLUTION = new TagInfo(
			"XResolution", 0x11A, FIELD_TYPE_DESCRIPTION_RATIONAL, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_YRESOLUTION = new TagInfo(
			"YResolution", 0x11B, FIELD_TYPE_DESCRIPTION_RATIONAL, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_PLANAR_CONFIGURATION = new TagInfo(
			"Planar Configuration", 0x11C, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_PAGE_NAME = new TagInfo("Page Name",
			0x11D, FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_XPOSITION = new TagInfo("XPosition",
			0x11E, FIELD_TYPE_DESCRIPTION_RATIONAL, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_YPOSITION = new TagInfo("YPosition",
			0x11F, FIELD_TYPE_DESCRIPTION_RATIONAL, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_FREE_OFFSETS = new TagInfo(
			"Free Offsets", 0x120, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_FREE_BYTE_COUNTS = new TagInfo(
			"Free Byte Counts", 0x121, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_GRAY_RESPONSE_UNIT = new TagInfo(
			"Gray Response Unit", 0x122, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_GRAY_RESPONSE_CURVE = new TagInfo(
			"Gray Response Curve", 0x123, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_T4_OPTIONS = new TagInfo("T4 Options",
			0x124, FIELD_TYPE_DESCRIPTION_LONG, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_T6_OPTIONS = new TagInfo("T6 Options",
			0x125, FIELD_TYPE_DESCRIPTION_LONG, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_RESOLUTION_UNIT = new TagInfo(
			"Resolution Unit", 0x128, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_PAGE_NUMBER = new TagInfo(
			"Page Number", 0x129, FIELD_TYPE_DESCRIPTION_SHORT, 2,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TRANSFER_FUNCTION = new TagInfo(
			"Transfer Function", 0x12D, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_SOFTWARE = new TagInfo("Software",
			0x131, FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_DATE_TIME = new TagInfo("Date Time",
			0x132, FIELD_TYPE_DESCRIPTION_ASCII, 20, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_ARTIST = new TagInfo("Artist", 0x13B,
			FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_HOST_COMPUTER = new TagInfo(
			"Host Computer", 0x13C, FIELD_TYPE_DESCRIPTION_ASCII, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_PREDICTOR = new TagInfo("Predictor",
			0x13D, FIELD_TYPE_DESCRIPTION_SHORT, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_WHITE_POINT = new TagInfo(
			"White Point", 0x13E, FIELD_TYPE_DESCRIPTION_RATIONAL, 2,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_PRIMARY_CHROMATICITIES = new TagInfo(
			"Primary Chromaticities", 0x13F, FIELD_TYPE_DESCRIPTION_RATIONAL,
			6, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_COLOR_MAP = new TagInfo("Color Map",
			0x140, FIELD_TYPE_DESCRIPTION_SHORT, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_HALFTONE_HINTS = new TagInfo(
			"Halftone Hints", 0x141, FIELD_TYPE_DESCRIPTION_SHORT, 2,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TILE_WIDTH = new TagInfo("Tile Width",
			0x142, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TILE_LENGTH = new TagInfo(
			"Tile Length", 0x143, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TILE_OFFSETS = new TagInfo.Offset(
			"Tile Offsets", 0x144, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TILE_BYTE_COUNTS = new TagInfo(
			"Tile Byte Counts", 0x145, FIELD_TYPE_DESCRIPTION_SHORT_OR_LONG,
			-1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_INK_SET = new TagInfo("Ink Set",
			0x14C, FIELD_TYPE_DESCRIPTION_SHORT, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_INK_NAMES = new TagInfo("Ink Names",
			0x14D, FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_NUMBER_OF_INKS = new TagInfo(
			"Number Of Inks", 0x14E, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_DOT_RANGE = new TagInfo("Dot Range",
			0x150, FIELD_TYPE_DESCRIPTION_BYTE_OR_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TARGET_PRINTER = new TagInfo(
			"Target Printer", 0x151, FIELD_TYPE_DESCRIPTION_ASCII, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_EXTRA_SAMPLES = new TagInfo(
			"Extra Samples", 0x152, FIELD_TYPE_DESCRIPTION_BYTE, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_SAMPLE_FORMAT = new TagInfo(
			"Sample Format", 0x153, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_SMIN_SAMPLE_VALUE = new TagInfo(
			"SMin Sample Value", 0x154, FIELD_TYPE_DESCRIPTION_ANY, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_SMAX_SAMPLE_VALUE = new TagInfo(
			"SMax Sample Value", 0x155, FIELD_TYPE_DESCRIPTION_ANY, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_TRANSFER_RANGE = new TagInfo(
			"Transfer Range", 0x156, FIELD_TYPE_DESCRIPTION_SHORT, 6,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_PROC = new TagInfo("JPEGProc",
			0x200, FIELD_TYPE_DESCRIPTION_SHORT, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_INTERCHANGE_FORMAT = new TagInfo.Offset(
			"JPEGInterchange Format", 0x201, FIELD_TYPE_DESCRIPTION_LONG, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH = new TagInfo(
			"JPEGInterchange Format Length", 0x202,
			FIELD_TYPE_DESCRIPTION_LONG, 1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_RESTART_INTERVAL = new TagInfo(
			"JPEGRestart Interval", 0x203, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_LOSSLESS_PREDICTORS = new TagInfo(
			"JPEGLossless Predictors", 0x205, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_POINT_TRANSFORMS = new TagInfo(
			"JPEGPoint Transforms", 0x206, FIELD_TYPE_DESCRIPTION_SHORT, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_QTABLES = new TagInfo(
			"JPEGQTables", 0x207, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_DCTABLES = new TagInfo(
			"JPEGDCTables", 0x208, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_JPEG_ACTABLES = new TagInfo(
			"JPEGACTables", 0x209, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_YCBCR_COEFFICIENTS = new TagInfo(
			"YCbCr Coefficients", 0x211, FIELD_TYPE_DESCRIPTION_RATIONAL, 3,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_YCBCR_SUB_SAMPLING = new TagInfo(
			"YCbCr Sub Sampling", 0x212, FIELD_TYPE_DESCRIPTION_SHORT, 2,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_YCBCR_POSITIONING = new TagInfo(
			"YCbCr Positioning", 0x213, FIELD_TYPE_DESCRIPTION_SHORT, 1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_REFERENCE_BLACK_WHITE = new TagInfo(
			"Reference Black White", 0x214, FIELD_TYPE_DESCRIPTION_LONG, -1,
			TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_COPYRIGHT = new TagInfo("Copyright",
			0x8298, FIELD_TYPE_DESCRIPTION_ASCII, -1, TIFF_DIRECTORY_ROOT);

	public static final TagInfo TIFF_TAG_XMP = new TagInfo("XMP",
			0x2BC, FIELD_TYPE_DESCRIPTION_BYTE, -1, TIFF_DIRECTORY_ROOT);

	// TODO:
	//	public static final TagInfo2 TIFF_TAG_UNKNOWN = null;
	public static final TagInfo TIFF_TAG_UNKNOWN = new TagInfo.Unknown(
			"Unknown Tag", -1, FIELD_TYPE_DESCRIPTION_UNKNOWN,
			TagInfo.LENGTH_UNKNOWN, EXIF_DIRECTORY_UNKNOWN);

	public static final TagInfo ALL_TIFF_TAGS[] = {
			TIFF_TAG_NEW_SUBFILE_TYPE, TIFF_TAG_SUBFILE_TYPE,
			TIFF_TAG_IMAGE_WIDTH, TIFF_TAG_IMAGE_LENGTH,
			TIFF_TAG_BITS_PER_SAMPLE, TIFF_TAG_COMPRESSION,
			TIFF_TAG_PHOTOMETRIC_INTERPRETATION, TIFF_TAG_THRESHHOLDING,
			TIFF_TAG_CELL_WIDTH, TIFF_TAG_CELL_LENGTH, TIFF_TAG_FILL_ORDER,
			TIFF_TAG_DOCUMENT_NAME, TIFF_TAG_IMAGE_DESCRIPTION, TIFF_TAG_MAKE,
			TIFF_TAG_MODEL, TIFF_TAG_STRIP_OFFSETS, TIFF_TAG_ORIENTATION,
			TIFF_TAG_SAMPLES_PER_PIXEL, TIFF_TAG_ROWS_PER_STRIP,
			TIFF_TAG_STRIP_BYTE_COUNTS, TIFF_TAG_MIN_SAMPLE_VALUE,
			TIFF_TAG_MAX_SAMPLE_VALUE, TIFF_TAG_XRESOLUTION,
			TIFF_TAG_YRESOLUTION, TIFF_TAG_PLANAR_CONFIGURATION,
			TIFF_TAG_PAGE_NAME, TIFF_TAG_XPOSITION, TIFF_TAG_YPOSITION,
			TIFF_TAG_FREE_OFFSETS, TIFF_TAG_FREE_BYTE_COUNTS,
			TIFF_TAG_GRAY_RESPONSE_UNIT, TIFF_TAG_GRAY_RESPONSE_CURVE,
			TIFF_TAG_T4_OPTIONS, TIFF_TAG_T6_OPTIONS, TIFF_TAG_RESOLUTION_UNIT,
			TIFF_TAG_PAGE_NUMBER, TIFF_TAG_TRANSFER_FUNCTION,
			TIFF_TAG_SOFTWARE, TIFF_TAG_DATE_TIME, TIFF_TAG_ARTIST,
			TIFF_TAG_HOST_COMPUTER, TIFF_TAG_PREDICTOR, TIFF_TAG_WHITE_POINT,
			TIFF_TAG_PRIMARY_CHROMATICITIES, TIFF_TAG_COLOR_MAP,
			TIFF_TAG_HALFTONE_HINTS, TIFF_TAG_TILE_WIDTH, TIFF_TAG_TILE_LENGTH,
			TIFF_TAG_TILE_OFFSETS, TIFF_TAG_TILE_BYTE_COUNTS, TIFF_TAG_INK_SET,
			TIFF_TAG_INK_NAMES, TIFF_TAG_NUMBER_OF_INKS, TIFF_TAG_DOT_RANGE,
			TIFF_TAG_TARGET_PRINTER, TIFF_TAG_EXTRA_SAMPLES,
			TIFF_TAG_SAMPLE_FORMAT, TIFF_TAG_SMIN_SAMPLE_VALUE,
			TIFF_TAG_SMAX_SAMPLE_VALUE, TIFF_TAG_TRANSFER_RANGE,
			TIFF_TAG_JPEG_PROC, TIFF_TAG_JPEG_INTERCHANGE_FORMAT,
			TIFF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH,
			TIFF_TAG_JPEG_RESTART_INTERVAL, TIFF_TAG_JPEG_LOSSLESS_PREDICTORS,
			TIFF_TAG_JPEG_POINT_TRANSFORMS, TIFF_TAG_JPEG_QTABLES,
			TIFF_TAG_JPEG_DCTABLES, TIFF_TAG_JPEG_ACTABLES,
			TIFF_TAG_YCBCR_COEFFICIENTS, TIFF_TAG_YCBCR_SUB_SAMPLING,
			TIFF_TAG_YCBCR_POSITIONING, TIFF_TAG_REFERENCE_BLACK_WHITE,
			TIFF_TAG_COPYRIGHT,
			//
			TIFF_TAG_XMP,
	};

}