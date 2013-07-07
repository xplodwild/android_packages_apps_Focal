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

import java.io.IOException;
import java.io.InputStream;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryFileParser;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.util.Debug;

public class JpegUtils extends BinaryFileParser implements JpegConstants
{
	public JpegUtils()
	{
		setByteOrder(BYTE_ORDER_NETWORK);
	}

	public static interface Visitor
	{
		// return false to exit before reading image data.
		public boolean beginSOS();

		public void    visitSOS(int marker, byte markerBytes[], byte imageData[]);
		public boolean visitSOS(int marker, byte markerBytes[], InputStream is);
		
		// return false to exit traversal.
		public boolean visitSegment(int marker, byte markerBytes[],
				int segmentLength, byte segmentLengthBytes[],
				byte segmentData[]) throws ImageReadException,
		// ImageWriteException,
				IOException;
	}

	public void traverseJFIF(ByteSource byteSource, Visitor visitor)
			throws ImageReadException,
			// ImageWriteException,
			IOException
	{
		InputStream is = null;
		boolean doClose = true;
		try
		{
			is = byteSource.getInputStream();

			readAndVerifyBytes(is, SOI,
					"Not a Valid JPEG File: doesn't begin with 0xffd8");

			int byteOrder = getByteOrder();

			for (int markerCount = 0; true; markerCount++)
			{
				byte markerBytes[] = readByteArray("markerBytes", 2, is,
						"markerBytes");
				int marker = convertByteArrayToShort("marker", markerBytes,
						byteOrder);

//				Debug.debug("marker", marker + " (0x" + Integer.toHexString(marker) + ")");
//				Debug.debug("markerBytes", markerBytes);
				
				if (marker == 0xffd9 || marker == SOS_Marker)
				{
					if (!visitor.beginSOS())
						return;

					//commented out the code below. imageData could become waaay 
					//too large for mobile device.
					//byte imageData[] = getStreamBytes(is);
					//
					doClose = !visitor.visitSOS(marker, markerBytes, is);
					//
					break;
				}

				byte segmentLengthBytes[] = readByteArray("segmentLengthBytes",
						2, is, "segmentLengthBytes");
				int segmentLength = convertByteArrayToShort("segmentLength",
						segmentLengthBytes, byteOrder);

//				Debug.debug("segmentLength", segmentLength + " (0x" + Integer.toHexString(segmentLength) + ")");
//				Debug.debug("segmentLengthBytes", segmentLengthBytes);

				byte segmentData[] = readByteArray("Segment Data",
						segmentLength - 2, is,
						"Invalid Segment: insufficient data");

				// Debug.debug("segmentLength", segmentLength);

				if (!visitor.visitSegment(marker, markerBytes, segmentLength,
						segmentLengthBytes, segmentData))
					return;
			}

		} finally
		{
			try
			{
				if (is != null && doClose) 
					is.close();
			} catch (Exception e)
			{
				Debug.debug(e);
			}
		}
	}

	public static String getMarkerName(int marker)
	{
		switch (marker)
		{
		case SOS_Marker:
			return "SOS_Marker";
			// case JPEG_APP0 :
			// return "JPEG_APP0";
			// case JPEG_APP0_Marker :
			// return "JPEG_APP0_Marker";
		case JPEG_APP1_Marker:
			return "JPEG_APP1_Marker";
		case JPEG_APP2_Marker:
			return "JPEG_APP2_Marker";
		case JPEG_APP13_Marker:
			return "JPEG_APP13_Marker";
		case JPEG_APP14_Marker:
			return "JPEG_APP14_Marker";
		case JPEG_APP15_Marker:
			return "JPEG_APP15_Marker";
		case JFIFMarker:
			return "JFIFMarker";
		case SOF0Marker:
			return "SOF0Marker";
		case SOF1Marker:
			return "SOF1Marker";
		case SOF2Marker:
			return "SOF2Marker";
		case SOF3Marker:
			return "SOF3Marker";
		case SOF4Marker:
			return "SOF4Marker";
		case SOF5Marker:
			return "SOF5Marker";
		case SOF6Marker:
			return "SOF6Marker";
		case SOF7Marker:
			return "SOF7Marker";
		case SOF8Marker:
			return "SOF8Marker";
		case SOF9Marker:
			return "SOF9Marker";
		case SOF10Marker:
			return "SOF10Marker";
		case SOF11Marker:
			return "SOF11Marker";
		case SOF12Marker:
			return "SOF12Marker";
		case SOF13Marker:
			return "SOF13Marker";
		case SOF14Marker:
			return "SOF14Marker";
		case SOF15Marker:
			return "SOF15Marker";
		default:
			return "Unknown";
		}
	}

	public void dumpJFIF(ByteSource byteSource) throws ImageReadException,
			IOException, ImageWriteException
	{
		Visitor visitor = new Visitor() {
			// return false to exit before reading image data.
			public boolean beginSOS()
			{
				return true;
			}

			public void visitSOS(int marker, byte markerBytes[],
					byte imageData[])
			{
				Debug.debug("SOS marker.  " + imageData.length
						+ " bytes of image data.");
				Debug.debug("");
			}

			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
				return false;
			}
			
			// return false to exit traversal.
			public boolean visitSegment(int marker, byte markerBytes[],
					int segmentLength, byte segmentLengthBytes[],
					byte segmentData[])
			{
				Debug.debug("Segment marker: " + Integer.toHexString(marker)
						+ " (" + getMarkerName(marker) + "), "
						+ segmentData.length + " bytes of segment data.");
				return true;
			}
		};

		traverseJFIF(byteSource, visitor);
	}
}