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
package org.apache.sanselan.formats.jpeg;

public interface JpegConstants
{
	public static final int MAX_SEGMENT_SIZE = 0xffff;

	public static final byte JFIF0_SIGNATURE[] = new byte[] { //
	0x4a, // J
			0x46, // F
			0x49, // I
			0x46, // F
			0x0, //  
	};
	public static final byte JFIF0_SIGNATURE_ALTERNATIVE[] = new byte[] { //
	0x4a, // J
			0x46, // F
			0x49, // I
			0x46, // F
			0x20, //  
	};

	public static final byte EXIF_IDENTIFIER_CODE[] = { //
	0x45, // E
			0x78, // x
			0x69, // i
			0x66, // f
	};

	public static final byte XMP_IDENTIFIER[] = { //
	0x68, // h
			0x74, // t
			0x74, // t
			0x70, // p
			0x3A, // :
			0x2F, // /
			0x2F, // /
			0x6E, // n
			0x73, // s
			0x2E, // .
			0x61, // a
			0x64, // d
			0x6F, // o
			0x62, // b
			0x65, // e
			0x2E, // .
			0x63, // c
			0x6F, // o
			0x6D, // m
			0x2F, // /
			0x78, // x
			0x61, // a
			0x70, // p
			0x2F, // /
			0x31, // 1
			0x2E, // .
			0x30, // 0
			0x2F, // /
			0, // 0-terminated us-ascii string.
	};

	public static final byte SOI[] = new byte[] { (byte) 0xff, (byte) 0xd8 };
	public static final byte EOI[] = new byte[] { (byte) 0xff, (byte) 0xd9 };

	public static final int SOS_Marker = (0xff00) | (0xda);

	public static final int JPEG_APP0 = 0xE0;
	// public static final int JPEG_APP1 = JPEG_APP0 + 1;
	// public static final int JPEG_APP1_Marker = (0xff00) | JPEG_APP1;
	public static final int JPEG_APP0_Marker = (0xff00) | (JPEG_APP0);
	public static final int JPEG_APP1_Marker = (0xff00) | (JPEG_APP0 + 1);
	// public static final int JPEG_APP2 = ;
	public static final int JPEG_APP2_Marker = (0xff00) | (JPEG_APP0 + 2);
	public static final int JPEG_APP13_Marker = (0xff00) | (JPEG_APP0 + 13);
	public static final int JPEG_APP14_Marker = (0xff00) | (JPEG_APP0 + 14);
	public static final int JPEG_APP15_Marker = (0xff00) | (JPEG_APP0 + 15);

	public static final int JFIFMarker = 0xFFE0;
	public static final int SOF0Marker = 0xFFc0;
	public static final int SOF1Marker = 0xFFc0 + 0x1;
	public static final int SOF2Marker = 0xFFc0 + 0x2;
	public static final int SOF3Marker = 0xFFc0 + 0x3;
	public static final int SOF4Marker = 0xFFc0 + 0x4;
	public static final int SOF5Marker = 0xFFc0 + 0x5;
	public static final int SOF6Marker = 0xFFc0 + 0x6;
	public static final int SOF7Marker = 0xFFc0 + 0x7;
	public static final int SOF8Marker = 0xFFc0 + 0x8;
	public static final int SOF9Marker = 0xFFc0 + 0x9;
	public static final int SOF10Marker = 0xFFc0 + 0xa;
	public static final int SOF11Marker = 0xFFc0 + 0xb;
	public static final int SOF12Marker = 0xFFc0 + 0xc;
	public static final int SOF13Marker = 0xFFc0 + 0xd;
	public static final int SOF14Marker = 0xFFc0 + 0xe;
	public static final int SOF15Marker = 0xFFc0 + 0xf;

	public static final int MARKERS[] = { SOS_Marker, JPEG_APP0,
			JPEG_APP0_Marker, JPEG_APP1_Marker, JPEG_APP2_Marker,
			JPEG_APP13_Marker, JPEG_APP14_Marker, JPEG_APP15_Marker,
			JFIFMarker, SOF0Marker, SOF1Marker, SOF2Marker, SOF3Marker,
			SOF4Marker, SOF5Marker, SOF6Marker, SOF7Marker, SOF8Marker,
			SOF9Marker, SOF10Marker, SOF11Marker, SOF12Marker, SOF13Marker,
			SOF14Marker, SOF15Marker, };

	public static final byte icc_profile_label[] = { 0x49, 0x43, 0x43, 0x5F,
			0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x0 };

	public static final byte PHOTOSHOP_IDENTIFICATION_STRING[] = { //
	0x50, // P
			0x68, // h
			0x6F, // o
			0x74, // t
			0x6F, // o
			0x73, // s
			0x68, // h
			0x6F, // o
			0x70, // p
			0x20, //
			0x33, // 3
			0x2E, // .
			0x30, // 0
			0,
	};
	public static final byte CONST_8BIM[] = { //
	0x38, // 8
			0x42, // B
			0x49, // I
			0x4D, // M
	};

}
