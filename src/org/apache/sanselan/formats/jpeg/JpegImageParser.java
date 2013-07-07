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

//import java.awt.Dimension;
//import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.sanselan.ImageFormat;
import org.apache.sanselan.ImageInfo;
import org.apache.sanselan.ImageParser;
import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.common.IImageMetadata;
import org.apache.sanselan.common.byteSources.ByteSource;
//import org.apache.sanselan.formats.jpeg.iptc.IPTCParser;
//import org.apache.sanselan.formats.jpeg.iptc.PhotoshopApp13Data;
//import org.apache.sanselan.formats.jpeg.segments.App13Segment;
import org.apache.sanselan.formats.jpeg.segments.App2Segment;
import org.apache.sanselan.formats.jpeg.segments.GenericSegment;
import org.apache.sanselan.formats.jpeg.segments.JFIFSegment;
import org.apache.sanselan.formats.jpeg.segments.SOFNSegment;
import org.apache.sanselan.formats.jpeg.segments.Segment;
import org.apache.sanselan.formats.jpeg.segments.UnknownSegment;
//import org.apache.sanselan.formats.jpeg.xmp.JpegXmpParser;
import org.apache.sanselan.formats.tiff.TiffField;
import org.apache.sanselan.formats.tiff.TiffImageMetadata;
import org.apache.sanselan.formats.tiff.TiffImageParser;
import org.apache.sanselan.formats.tiff.constants.TiffTagConstants;
import org.apache.sanselan.util.Debug;

public class JpegImageParser extends ImageParser implements JpegConstants,
		TiffTagConstants
{
	public JpegImageParser()
	{
		setByteOrder(BYTE_ORDER_NETWORK);
		// setDebug(true);
	}

	protected ImageFormat[] getAcceptedTypes()
	{
		return new ImageFormat[] { ImageFormat.IMAGE_FORMAT_JPEG, //
		};
	}

	public String getName()
	{
		return "Jpeg-Custom";
	}

	public String getDefaultExtension()
	{
		return DEFAULT_EXTENSION;
	}

	private static final String DEFAULT_EXTENSION = ".jpg";

	public static final String AcceptedExtensions[] = { ".jpg", ".jpeg", };

	protected String[] getAcceptedExtensions()
	{
		return AcceptedExtensions;
	}

//	public final Object getBufferedImage(ByteSource byteSource,
//			Map params) throws ImageReadException, IOException
//	{
//		throw new ImageReadException(
//				"Sanselan cannot read or write JPEG images.");
//	}

	private boolean keepMarker(int marker, int markers[])
	{
		if (markers == null)
			return true;

		for (int i = 0; i < markers.length; i++)
		{
			if (markers[i] == marker)
				return true;
		}

		return false;
	}

	public ArrayList readSegments(ByteSource byteSource, final int markers[],
			final boolean returnAfterFirst, boolean readEverything)
			throws ImageReadException, IOException
	{
		final ArrayList result = new ArrayList();
		final JpegImageParser parser = this;

		JpegUtils.Visitor visitor = new JpegUtils.Visitor() {
			// return false to exit before reading image data.
			public boolean beginSOS()
			{
				return false;
			}

			public void visitSOS(int marker, byte markerBytes[],
					byte imageData[])
			{
			}

			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
				return false;
			}
			
			// return false to exit traversal.
			public boolean visitSegment(int marker, byte markerBytes[],
					int markerLength, byte markerLengthBytes[],
					byte segmentData[]) throws ImageReadException, IOException
			{
				if (marker == 0xffd9)
					return false;

				// Debug.debug("visitSegment marker", marker);
				// // Debug.debug("visitSegment keepMarker(marker, markers)",
				// keepMarker(marker, markers));
				// Debug.debug("visitSegment keepMarker(marker, markers)",
				// keepMarker(marker, markers));

				if (!keepMarker(marker, markers))
					return true;

				if (marker == JPEG_APP13_Marker)
				{
					// Debug.debug("app 13 segment data", segmentData.length);
//					result.add(new App13Segment(parser, marker, segmentData));
				} else if (marker == JPEG_APP2_Marker)
				{
					result.add(new App2Segment(marker, segmentData));
				} else if (marker == JFIFMarker)
				{
					result.add(new JFIFSegment(marker, segmentData));
				} else if ((marker >= SOF0Marker) && (marker <= SOF15Marker))
				{
					result.add(new SOFNSegment(marker, segmentData));
				} else if ((marker >= JPEG_APP1_Marker)
						&& (marker <= JPEG_APP15_Marker))
				{
					result.add(new UnknownSegment(marker, segmentData));
				}

				if (returnAfterFirst)
					return false;

				return true;
			}
		};

		new JpegUtils().traverseJFIF(byteSource, visitor);

		return result;
	}

	public static final boolean permissive = true;

	private byte[] assembleSegments(ArrayList v) throws ImageReadException,
			IOException
	{
		try
		{
			return assembleSegments(v, false);
		} catch (ImageReadException e)
		{
			return assembleSegments(v, true);
		}
	}

	private byte[] assembleSegments(ArrayList v, boolean start_with_zero)
			throws ImageReadException, IOException
	{
		if (v.size() < 1)
			throw new ImageReadException("No App2 Segments Found.");

		int markerCount = ((App2Segment) v.get(0)).num_markers;

		// if (permissive && (markerCount == 0))
		// markerCount = v.size();

		if (v.size() != markerCount)
			throw new ImageReadException("App2 Segments Missing.  Found: "
					+ v.size() + ", Expected: " + markerCount + ".");

		Collections.sort(v);

		int offset = start_with_zero ? 0 : 1;

		int total = 0;
		for (int i = 0; i < v.size(); i++)
		{
			App2Segment segment = (App2Segment) v.get(i);

			if ((i + offset) != segment.cur_marker)
			{
				dumpSegments(v);
				throw new ImageReadException(
						"Incoherent App2 Segment Ordering.  i: " + i
								+ ", segment[" + i + "].cur_marker: "
								+ segment.cur_marker + ".");
			}

			if (markerCount != segment.num_markers)
			{
				dumpSegments(v);
				throw new ImageReadException(
						"Inconsistent App2 Segment Count info.  markerCount: "
								+ markerCount + ", segment[" + i
								+ "].num_markers: " + segment.num_markers + ".");
			}

			total += segment.icc_bytes.length;
		}

		byte result[] = new byte[total];
		int progress = 0;

		for (int i = 0; i < v.size(); i++)
		{
			App2Segment segment = (App2Segment) v.get(i);

			System.arraycopy(segment.icc_bytes, 0, result, progress,
					segment.icc_bytes.length);
			progress += segment.icc_bytes.length;
		}

		return result;
	}

	private void dumpSegments(ArrayList v)
	{
		Debug.debug();
		Debug.debug("dumpSegments", v.size());

		for (int i = 0; i < v.size(); i++)
		{
			App2Segment segment = (App2Segment) v.get(i);

			Debug.debug((i) + ": " + segment.cur_marker + " / "
					+ segment.num_markers);
		}
		Debug.debug();
	}

	public ArrayList readSegments(ByteSource byteSource, int markers[],
			boolean returnAfterFirst) throws ImageReadException, IOException
	{
		return readSegments(byteSource, markers, returnAfterFirst, false);
	}

	public byte[] getICCProfileBytes(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		ArrayList segments = readSegments(byteSource,
				new int[] { JPEG_APP2_Marker, }, false);

		if (segments != null)
		{
			// throw away non-icc profile app2 segments.
			ArrayList filtered = new ArrayList();
			for (int i = 0; i < segments.size(); i++)
			{
				App2Segment segment = (App2Segment) segments.get(i);
				if (segment.icc_bytes != null)
					filtered.add(segment);
			}
			segments = filtered;
		}

		if ((segments == null) || (segments.size() < 1))
			return null;

		byte bytes[] = assembleSegments(segments);

		if (debug)
			System.out.println("bytes" + ": "
					+ ((bytes == null) ? null : "" + bytes.length));

		if (debug)
			System.out.println("");

		return (bytes);
	}

	public IImageMetadata getMetadata(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		TiffImageMetadata exif = getExifMetadata(byteSource, params);

		/*JpegPhotoshopMetadata*/Object photoshop = null; /* getPhotoshopMetadata(byteSource,
				params);*/

		if (null == exif && null == photoshop)
			return null;

		JpegImageMetadata result = new JpegImageMetadata(photoshop, exif);

		return result;
	}

	public static boolean isExifAPP1Segment(GenericSegment segment)
	{
		return byteArrayHasPrefix(segment.bytes, EXIF_IDENTIFIER_CODE);
	}

	private ArrayList filterAPP1Segments(ArrayList v)
	{
		ArrayList result = new ArrayList();

		for (int i = 0; i < v.size(); i++)
		{
			GenericSegment segment = (GenericSegment) v.get(i);
			if (isExifAPP1Segment(segment))
				result.add(segment);
		}

		return result;
	}

	private ArrayList filterSegments(ArrayList v, List markers)
	{
		ArrayList result = new ArrayList();

		for (int i = 0; i < v.size(); i++)
		{
			Segment segment = (Segment) v.get(i);
			Integer marker = new Integer(segment.marker);
			if (markers.contains(marker))
				result.add(segment);
		}

		return result;
	}

	public TiffImageMetadata getExifMetadata(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		byte bytes[] = getExifRawData(byteSource);
		if (null == bytes)
			return null;

		if (params == null)
			params = new HashMap();
		if (!params.containsKey(PARAM_KEY_READ_THUMBNAILS))
			params.put(PARAM_KEY_READ_THUMBNAILS, Boolean.TRUE);

		return (TiffImageMetadata) new TiffImageParser().getMetadata(bytes,
				params);
	}

	public byte[] getExifRawData(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		ArrayList segments = readSegments(byteSource,
				new int[] { JPEG_APP1_Marker, }, false);

		if ((segments == null) || (segments.size() < 1))
			return null;

		ArrayList exifSegments = filterAPP1Segments(segments);
		if (debug)
			System.out.println("exif_segments.size" + ": "
					+ exifSegments.size());

		// Debug.debug("segments", segments);
		// Debug.debug("exifSegments", exifSegments);

		// TODO: concatenate if multiple segments, need example.
		if (exifSegments.size() < 1)
			return null;
		if (exifSegments.size() > 1)
			throw new ImageReadException(
					"Sanselan currently can't parse EXIF metadata split across multiple APP1 segments.  "
							+ "Please send this image to the Sanselan project.");

		GenericSegment segment = (GenericSegment) exifSegments.get(0);
		byte bytes[] = segment.bytes;

		// byte head[] = readBytearray("exif head", bytes, 0, 6);
		//
		// Debug.debug("head", head);

		return getByteArrayTail("trimmed exif bytes", bytes, 6);
	}

	public boolean hasExifSegment(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		final boolean result[] = { false, };

		JpegUtils.Visitor visitor = new JpegUtils.Visitor() {
			// return false to exit before reading image data.
			public boolean beginSOS()
			{
				return false;
			}

			public void visitSOS(int marker, byte markerBytes[],
					byte imageData[])
			{
			}

			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
				return false;
			}
			
			// return false to exit traversal.
			public boolean visitSegment(int marker, byte markerBytes[],
					int markerLength, byte markerLengthBytes[],
					byte segmentData[]) throws ImageReadException, IOException
			{
				if (marker == 0xffd9)
					return false;

				if (marker == JPEG_APP1_Marker)
				{
					if (byteArrayHasPrefix(segmentData, EXIF_IDENTIFIER_CODE))
					{
						result[0] = true;
						return false;
					}
				}

				return true;
			}
		};

		new JpegUtils().traverseJFIF(byteSource, visitor);

		return result[0];
	}

	public boolean hasIptcSegment(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		final boolean result[] = { false, };

//		JpegUtils.Visitor visitor = new JpegUtils.Visitor() {
//			// return false to exit before reading image data.
//			public boolean beginSOS()
//			{
//				return false;
//			}
//
//			public void visitSOS(int marker, byte markerBytes[],
//					byte imageData[])
//			{
//			}
//
//			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
//				return false;
//			}
//			
//			// return false to exit traversal.
//			public boolean visitSegment(int marker, byte markerBytes[],
//					int markerLength, byte markerLengthBytes[],
//					byte segmentData[]) throws ImageReadException, IOException
//			{
//				if (marker == 0xffd9)
//					return false;
//
//				if (marker == JPEG_APP13_Marker)
//				{
//					if (new IPTCParser().isPhotoshopJpegSegment(segmentData))
//					{
//						result[0] = true;
//						return false;
//					}
//				}
//
//				return true;
//			}
//		};
//
//		new JpegUtils().traverseJFIF(byteSource, visitor);

		return result[0];
	}

	public boolean hasXmpSegment(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		final boolean result[] = { false, };

//		JpegUtils.Visitor visitor = new JpegUtils.Visitor() {
//			// return false to exit before reading image data.
//			public boolean beginSOS()
//			{
//				return false;
//			}
//
//			public void visitSOS(int marker, byte markerBytes[],
//					byte imageData[])
//			{
//			}
//
//			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
//				return false;
//			}
//			
//			// return false to exit traversal.
//			public boolean visitSegment(int marker, byte markerBytes[],
//					int markerLength, byte markerLengthBytes[],
//					byte segmentData[]) throws ImageReadException, IOException
//			{
//				if (marker == 0xffd9)
//					return false;
//
//				if (marker == JPEG_APP1_Marker)
//				{
//					if (new JpegXmpParser().isXmpJpegSegment(segmentData))
//					{
//						result[0] = true;
//						return false;
//					}
//				}
//
//				return true;
//			}
//		};
//		new JpegUtils().traverseJFIF(byteSource, visitor);

		return result[0];
	}

	/**
	 * Extracts embedded XML metadata as XML string.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return Xmp Xml as String, if present. Otherwise, returns null..
	 */
	public String getXmpXml(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		return null;
//		final List result = new ArrayList();
//
//		JpegUtils.Visitor visitor = new JpegUtils.Visitor() {
//			// return false to exit before reading image data.
//			public boolean beginSOS()
//			{
//				return false;
//			}
//
//			public void visitSOS(int marker, byte markerBytes[],
//					byte imageData[])
//			{
//			}
//
//			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
//				return false;
//			}
//			
//			// return false to exit traversal.
//			public boolean visitSegment(int marker, byte markerBytes[],
//					int markerLength, byte markerLengthBytes[],
//					byte segmentData[]) throws ImageReadException, IOException
//			{
//				if (marker == 0xffd9)
//					return false;
//
//				if (marker == JPEG_APP1_Marker)
//				{
//					if (new JpegXmpParser().isXmpJpegSegment(segmentData))
//					{
//						result.add(new JpegXmpParser()
//								.parseXmpJpegSegment(segmentData));
//						return false;
//					}
//				}
//
//				return true;
//			}
//		};
//		new JpegUtils().traverseJFIF(byteSource, visitor);
//
//		if (result.size() < 1)
//			return null;
//		if (result.size() > 1)
//			throw new ImageReadException(
//					"Jpeg file contains more than one XMP segment.");
//		return (String) result.get(0);
	}

	public /*JpegPhotoshopMetadata*/Object getPhotoshopMetadata(ByteSource byteSource,
			Map params) throws ImageReadException, IOException
	{
		return null;
//		ArrayList segments = readSegments(byteSource,
//				new int[] { JPEG_APP13_Marker, }, false);
//
//		if ((segments == null) || (segments.size() < 1))
//			return null;
//
//		PhotoshopApp13Data photoshopApp13Data = null;
//
//		for (int i = 0; i < segments.size(); i++)
//		{
//			App13Segment segment = (App13Segment) segments.get(i);
//
//			PhotoshopApp13Data data = segment.parsePhotoshopSegment(params);
//			if (data != null && photoshopApp13Data != null)
//				throw new ImageReadException(
//						"Jpeg contains more than one Photoshop App13 segment.");
//
//			photoshopApp13Data = data;
//		}
//
//		if(null==photoshopApp13Data)
//			return null;
//		return new JpegPhotoshopMetadata(photoshopApp13Data);
	}

	public int[] getImageSize(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		ArrayList segments = readSegments(byteSource, new int[] {
				// kJFIFMarker,
				SOF0Marker,

				SOF1Marker, SOF2Marker, SOF3Marker, SOF5Marker, SOF6Marker,
				SOF7Marker, SOF9Marker, SOF10Marker, SOF11Marker, SOF13Marker,
				SOF14Marker, SOF15Marker,

		}, true);

		if ((segments == null) || (segments.size() < 1))
			throw new ImageReadException("No JFIF Data Found.");

		if (segments.size() > 1)
			throw new ImageReadException("Redundant JFIF Data Found.");

		SOFNSegment fSOFNSegment = (SOFNSegment) segments.get(0);

		return new int[]{fSOFNSegment.width, fSOFNSegment.height};
	}

	public byte[] embedICCProfile(byte image[], byte profile[])
	{
		return null;
	}

	public boolean embedICCProfile(File src, File dst, byte profile[])
	{
		return false;
	}

	public ImageInfo getImageInfo(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		// ArrayList allSegments = readSegments(byteSource, null, false);

		ArrayList SOF_segments = readSegments(byteSource, new int[] {
				// kJFIFMarker,

				SOF0Marker, SOF1Marker, SOF2Marker, SOF3Marker, SOF5Marker,
				SOF6Marker, SOF7Marker, SOF9Marker, SOF10Marker, SOF11Marker,
				SOF13Marker, SOF14Marker, SOF15Marker,

		}, false);

		if (SOF_segments == null)
			throw new ImageReadException("No SOFN Data Found.");

		// if (SOF_segments.size() != 1)
		// System.out.println("Incoherent SOFN Data Found: "
		// + SOF_segments.size());

		ArrayList jfifSegments = readSegments(byteSource,
				new int[] { JFIFMarker, }, true);

		SOFNSegment fSOFNSegment = (SOFNSegment) SOF_segments.get(0);
		// SOFNSegment fSOFNSegment = (SOFNSegment) findSegment(segments,
		// SOFNmarkers);

		if (fSOFNSegment == null)
			throw new ImageReadException("No SOFN Data Found.");

		int Width = fSOFNSegment.width;
		int Height = fSOFNSegment.height;

		JFIFSegment jfifSegment = null;

		if ((jfifSegments != null) && (jfifSegments.size() > 0))
			jfifSegment = (JFIFSegment) jfifSegments.get(0);

		// JFIFSegment fTheJFIFSegment = (JFIFSegment) findSegment(segments,
		// kJFIFMarker);

		double x_density = -1.0;
		double y_density = -1.0;
		double units_per_inch = -1.0;
		// int JFIF_major_version;
		// int JFIF_minor_version;
		String FormatDetails;

		if (jfifSegment != null)
		{
			x_density = jfifSegment.xDensity;
			y_density = jfifSegment.yDensity;
			int density_units = jfifSegment.densityUnits;
			// JFIF_major_version = fTheJFIFSegment.JFIF_major_version;
			// JFIF_minor_version = fTheJFIFSegment.JFIF_minor_version;

			FormatDetails = "Jpeg/JFIF v." + jfifSegment.jfifMajorVersion + "."
					+ jfifSegment.jfifMinorVersion;

			switch (density_units)
			{
			case 0:
				break;
			case 1: // inches
				units_per_inch = 1.0;
				break;
			case 2: // cms
				units_per_inch = 2.54;
				break;
			default:
				break;
			}
		} else
		{
			JpegImageMetadata metadata = (JpegImageMetadata) getMetadata(
					byteSource, params);

			if (metadata != null)
			{
				{
					TiffField field = metadata
							.findEXIFValue(TIFF_TAG_XRESOLUTION);
					if (field != null)
						x_density = ((Number) field.getValue()).doubleValue();
				}
				{
					TiffField field = metadata
							.findEXIFValue(TIFF_TAG_YRESOLUTION);
					if (field != null)
						y_density = ((Number) field.getValue()).doubleValue();
				}
				{
					TiffField field = metadata
							.findEXIFValue(TIFF_TAG_RESOLUTION_UNIT);
					if (field != null)
					{
						int density_units = ((Number) field.getValue())
								.intValue();

						switch (density_units)
						{
						case 1:
							break;
						case 2: // inches
							units_per_inch = 1.0;
							break;
						case 3: // cms
							units_per_inch = 2.54;
							break;
						default:
							break;
						}
					}

				}
			}

			FormatDetails = "Jpeg/DCM";

		}

		int PhysicalHeightDpi = -1;
		float PhysicalHeightInch = -1;
		int PhysicalWidthDpi = -1;
		float PhysicalWidthInch = -1;

		if (units_per_inch > 0)
		{
			PhysicalWidthDpi = (int) Math.round((double) x_density
					/ units_per_inch);
			PhysicalWidthInch = (float) ((double) Width / (x_density * units_per_inch));
			PhysicalHeightDpi = (int) Math.round((double) y_density
					* units_per_inch);
			PhysicalHeightInch = (float) ((double) Height / (y_density * units_per_inch));
		}

		ArrayList Comments = new ArrayList();
		// TODO: comments...

		int Number_of_components = fSOFNSegment.numberOfComponents;
		int Precision = fSOFNSegment.precision;

		int BitsPerPixel = Number_of_components * Precision;
		ImageFormat Format = ImageFormat.IMAGE_FORMAT_JPEG;
		String FormatName = "JPEG (Joint Photographic Experts Group) Format";
		String MimeType = "image/jpeg";
		// we ought to count images, but don't yet.
		int NumberOfImages = 1;
		// not accurate ... only reflects first
		boolean isProgressive = fSOFNSegment.marker == SOF2Marker;

		boolean isTransparent = false; // TODO: inaccurate.
		boolean usesPalette = false; // TODO: inaccurate.
		int ColorType;
		if (Number_of_components == 1)
			ColorType = ImageInfo.COLOR_TYPE_BW;
		else if (Number_of_components == 3)
			ColorType = ImageInfo.COLOR_TYPE_RGB;
		else if (Number_of_components == 4)
			ColorType = ImageInfo.COLOR_TYPE_CMYK;
		else
			ColorType = ImageInfo.COLOR_TYPE_UNKNOWN;

		String compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_JPEG;

		ImageInfo result = new ImageInfo(FormatDetails, BitsPerPixel, Comments,
				Format, FormatName, Height, MimeType, NumberOfImages,
				PhysicalHeightDpi, PhysicalHeightInch, PhysicalWidthDpi,
				PhysicalWidthInch, Width, isProgressive, isTransparent,
				usesPalette, ColorType, compressionAlgorithm);

		return result;
	}

	// public ImageInfo getImageInfo(ByteSource byteSource, Map params)
	// throws ImageReadException, IOException
	// {
	//
	// ArrayList allSegments = readSegments(byteSource, null, false);
	//
	// final int SOF_MARKERS[] = new int[]{
	// SOF0Marker, SOF1Marker, SOF2Marker, SOF3Marker, SOF5Marker,
	// SOF6Marker, SOF7Marker, SOF9Marker, SOF10Marker, SOF11Marker,
	// SOF13Marker, SOF14Marker, SOF15Marker,
	// };
	//
	// ArrayList sofMarkers = new ArrayList();
	// for(int i=0;i<SOF_MARKERS.length;i++)
	// sofMarkers.add(new Integer(SOF_MARKERS[i]));
	// ArrayList SOFSegments = filterSegments(allSegments, sofMarkers);
	// if (SOFSegments == null || SOFSegments.size()<1)
	// throw new ImageReadException("No SOFN Data Found.");
	//
	// List jfifMarkers = new ArrayList();
	// jfifMarkers.add(new Integer(JFIFMarker));
	// ArrayList jfifSegments = filterSegments(allSegments, jfifMarkers);
	//
	// SOFNSegment firstSOFNSegment = (SOFNSegment) SOFSegments.get(0);
	//
	// int Width = firstSOFNSegment.width;
	// int Height = firstSOFNSegment.height;
	//
	// JFIFSegment jfifSegment = null;
	//
	// if (jfifSegments != null && jfifSegments.size() > 0)
	// jfifSegment = (JFIFSegment) jfifSegments.get(0);
	//
	// double x_density = -1.0;
	// double y_density = -1.0;
	// double units_per_inch = -1.0;
	// // int JFIF_major_version;
	// // int JFIF_minor_version;
	// String FormatDetails;
	//
	// if (jfifSegment != null)
	// {
	// x_density = jfifSegment.xDensity;
	// y_density = jfifSegment.yDensity;
	// int density_units = jfifSegment.densityUnits;
	// // JFIF_major_version = fTheJFIFSegment.JFIF_major_version;
	// // JFIF_minor_version = fTheJFIFSegment.JFIF_minor_version;
	//
	// FormatDetails = "Jpeg/JFIF v." + jfifSegment.jfifMajorVersion
	// + "." + jfifSegment.jfifMinorVersion;
	//
	// switch (density_units)
	// {
	// case 0 :
	// break;
	// case 1 : // inches
	// units_per_inch = 1.0;
	// break;
	// case 2 : // cms
	// units_per_inch = 2.54;
	// break;
	// default :
	// break;
	// }
	// }
	// else
	// {
	// JpegImageMetadata metadata = (JpegImageMetadata) getMetadata(byteSource,
	// params);
	//
	// {
	// TiffField field = metadata
	// .findEXIFValue(TiffField.TIFF_TAG_XRESOLUTION);
	// if (field == null)
	// throw new ImageReadException("No XResolution");
	//
	// x_density = ((Number) field.getValue()).doubleValue();
	// }
	// {
	// TiffField field = metadata
	// .findEXIFValue(TiffField.TIFF_TAG_YRESOLUTION);
	// if (field == null)
	// throw new ImageReadException("No YResolution");
	//
	// y_density = ((Number) field.getValue()).doubleValue();
	// }
	// {
	// TiffField field = metadata
	// .findEXIFValue(TiffField.TIFF_TAG_RESOLUTION_UNIT);
	// if (field == null)
	// throw new ImageReadException("No ResolutionUnits");
	//
	// int density_units = ((Number) field.getValue()).intValue();
	//
	// switch (density_units)
	// {
	// case 1 :
	// break;
	// case 2 : // inches
	// units_per_inch = 1.0;
	// break;
	// case 3 : // cms
	// units_per_inch = 2.54;
	// break;
	// default :
	// break;
	// }
	//
	// }
	//
	// FormatDetails = "Jpeg/DCM";
	//
	// }
	//
	// int PhysicalHeightDpi = -1;
	// float PhysicalHeightInch = -1;
	// int PhysicalWidthDpi = -1;
	// float PhysicalWidthInch = -1;
	//
	// if (units_per_inch > 0)
	// {
	// PhysicalWidthDpi = (int) Math.round((double) x_density
	// / units_per_inch);
	// PhysicalWidthInch = (float) ((double) Width / (x_density *
	// units_per_inch));
	// PhysicalHeightDpi = (int) Math.round((double) y_density
	// * units_per_inch);
	// PhysicalHeightInch = (float) ((double) Height / (y_density *
	// units_per_inch));
	// }
	//
	// ArrayList Comments = new ArrayList();
	// // TODO: comments...
	//
	// int Number_of_components = firstSOFNSegment.numberOfComponents;
	// int Precision = firstSOFNSegment.precision;
	//
	// int BitsPerPixel = Number_of_components * Precision;
	// ImageFormat Format = ImageFormat.IMAGE_FORMAT_JPEG;
	// String FormatName = "JPEG (Joint Photographic Experts Group) Format";
	// String MimeType = "image/jpeg";
	// // we ought to count images, but don't yet.
	// int NumberOfImages = -1;
	// // not accurate ... only reflects first
	// boolean isProgressive = firstSOFNSegment.marker == SOF2Marker;
	//
	// boolean isTransparent = false; // TODO: inaccurate.
	// boolean usesPalette = false; // TODO: inaccurate.
	// int ColorType;
	// if (Number_of_components == 1)
	// ColorType = ImageInfo.COLOR_TYPE_BW;
	// else if (Number_of_components == 3)
	// ColorType = ImageInfo.COLOR_TYPE_RGB;
	// else if (Number_of_components == 4)
	// ColorType = ImageInfo.COLOR_TYPE_CMYK;
	// else
	// ColorType = ImageInfo.COLOR_TYPE_UNKNOWN;
	//
	// String compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_JPEG;
	//
	// ImageInfo result = new ImageInfo(FormatDetails, BitsPerPixel, Comments,
	// Format, FormatName, Height, MimeType, NumberOfImages,
	// PhysicalHeightDpi, PhysicalHeightInch, PhysicalWidthDpi,
	// PhysicalWidthInch, Width, isProgressive, isTransparent,
	// usesPalette, ColorType, compressionAlgorithm);
	//
	// return result;
	// }

	public boolean dumpImageFile(PrintWriter pw, ByteSource byteSource)
			throws ImageReadException, IOException
	{
		pw.println("tiff.dumpImageFile");

		{
			ImageInfo imageInfo = getImageInfo(byteSource);
			if (imageInfo == null)
				return false;

			imageInfo.toString(pw, "");
		}

		pw.println("");

		{
			ArrayList segments = readSegments(byteSource, null, false);

			if (segments == null)
				throw new ImageReadException("No Segments Found.");

			for (int d = 0; d < segments.size(); d++)
			{

				Segment segment = (Segment) segments.get(d);

				NumberFormat nf = NumberFormat.getIntegerInstance();
				// this.debugNumber("found, marker: ", marker, 4);
				pw.println(d + ": marker: "
						+ Integer.toHexString(segment.marker) + ", "
						+ segment.getDescription() + " (length: "
						+ nf.format(segment.length) + ")");
				segment.dump(pw);
			}

			pw.println("");
		}

		return true;
	}

}