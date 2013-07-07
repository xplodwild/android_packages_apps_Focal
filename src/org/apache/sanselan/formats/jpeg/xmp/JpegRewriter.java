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
package org.apache.sanselan.formats.jpeg.xmp;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryFileParser;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.formats.jpeg.JpegConstants;
import org.apache.sanselan.formats.jpeg.JpegUtils;
//import org.apache.sanselan.formats.jpeg.iptc.IPTCParser;

/**
 * Interface for Exif write/update/remove functionality for Jpeg/JFIF images.
 * <p>
 * <p>
 * See the source of the XmpXmlUpdateExample class for example usage.
 * 
 * @see org.apache.sanselan.sampleUsage.WriteXmpXmlExample
 */
public class JpegRewriter extends BinaryFileParser implements JpegConstants
{
	private static final int JPEG_BYTE_ORDER = BYTE_ORDER_NETWORK;

	/**
	 * Constructor. to guess whether a file contains an image based on its file
	 * extension.
	 */
	public JpegRewriter()
	{
		setByteOrder(JPEG_BYTE_ORDER);
	}

	protected static class JFIFPieces
	{
		public final List pieces;
		public final List segmentPieces;

		public JFIFPieces(final List pieces, final List segmentPieces)
		{
			this.pieces = pieces;
			this.segmentPieces = segmentPieces;
		}

	}

	protected abstract static class JFIFPiece
	{
		protected abstract void write(OutputStream os) throws IOException;
		
		public String toString()
		{
			return "[" + this.getClass().getName() + "]";
		}
	}

	protected static class JFIFPieceSegment extends JFIFPiece
	{
		public final int marker;
		public final byte markerBytes[];
		public final byte segmentLengthBytes[];
		public final byte segmentData[];

		public JFIFPieceSegment(final int marker, final byte[] segmentData)
		{
			this(marker, int2ToByteArray(marker, JPEG_BYTE_ORDER),
					int2ToByteArray(segmentData.length + 2, JPEG_BYTE_ORDER),
					segmentData);
		}

		public JFIFPieceSegment(final int marker, final byte[] markerBytes,
				final byte[] segmentLengthBytes, final byte[] segmentData)
		{
			this.marker = marker;
			this.markerBytes = markerBytes;
			this.segmentLengthBytes = segmentLengthBytes;
			this.segmentData = segmentData;
		}

		public String toString()
		{
			return "[" + this.getClass().getName() + " (0x" + Integer.toHexString(marker) + ")]";
		}

		protected void write(OutputStream os) throws IOException
		{
			os.write(markerBytes);
			os.write(segmentLengthBytes);
			os.write(segmentData);
		}

		public boolean isApp1Segment()
		{
			return marker == JPEG_APP1_Marker;
		}

		public boolean isAppSegment()
		{
			return marker >= JPEG_APP0_Marker && marker <= JPEG_APP15_Marker;
		}

		public boolean isExifSegment()
		{
			if (marker != JPEG_APP1_Marker)
				return false;
			if (!byteArrayHasPrefix(segmentData, EXIF_IDENTIFIER_CODE))
				return false;
			return true;
		}

		public boolean isPhotoshopApp13Segment()
		{
			return false;
//			if (marker != JPEG_APP13_Marker)
//				return false;
//			if (!new IPTCParser().isPhotoshopJpegSegment(segmentData))
//				return false;
//			return true;
		}

		public boolean isXmpSegment()
		{
			if (marker != JPEG_APP1_Marker)
				return false;
			if (!byteArrayHasPrefix(segmentData, XMP_IDENTIFIER))
				return false;
			return true;
		}

	}

	protected static class JFIFPieceImageData extends JFIFPiece
	{
		public final byte markerBytes[];
		public final byte imageData[];
		//
		public final InputStream isImageData;
		//

		public JFIFPieceImageData(final byte[] markerBytes,
				final byte[] imageData)
		{
			super();
			this.markerBytes = markerBytes;
			this.imageData = imageData;
			this.isImageData = null;
		}

		public JFIFPieceImageData(final byte[] markerBytes,
				final InputStream isImageData)
		{
			super();
			this.markerBytes = markerBytes;
			this.imageData = null;
			this.isImageData = isImageData;
		}

		protected void write(OutputStream os) throws IOException
		{
			os.write(markerBytes);
			if (imageData != null) {
				os.write(imageData);
			}
			else {
				byte buffer[] = new byte[1024];
				int read;
				while ((read = isImageData.read(buffer)) > 0)
				{
					os.write(buffer, 0, read);
				}
				try {isImageData.close();} catch(Exception e) {}
			}
		}
	}

	protected JFIFPieces analyzeJFIF(ByteSource byteSource)
			throws ImageReadException, IOException
	// , ImageWriteException
	{
		final ArrayList pieces = new ArrayList();
		final List segmentPieces = new ArrayList();

		JpegUtils.Visitor visitor = new JpegUtils.Visitor() {
			// return false to exit before reading image data.
			public boolean beginSOS()
			{
				return true;
			}

			public void visitSOS(int marker, byte markerBytes[],
					byte imageData[])
			{
				pieces.add(new JFIFPieceImageData(markerBytes, imageData));
			}

			public boolean visitSOS(int marker, byte markerBytes[], InputStream is) {
				pieces.add(new JFIFPieceImageData(markerBytes, is));
				return true;
			}
			
			// return false to exit traversal.
			public boolean visitSegment(int marker, byte markerBytes[],
					int segmentLength, byte segmentLengthBytes[],
					byte segmentData[]) throws ImageReadException, IOException
			{
				JFIFPiece piece = new JFIFPieceSegment(marker, markerBytes,
						segmentLengthBytes, segmentData);
				pieces.add(piece);
				segmentPieces.add(piece);

				return true;
			}
		};

		new JpegUtils().traverseJFIF(byteSource, visitor);

		return new JFIFPieces(pieces, segmentPieces);
	}

	private static interface SegmentFilter
	{
		public boolean filter(JFIFPieceSegment segment);
	}

	private static final SegmentFilter EXIF_SEGMENT_FILTER = new SegmentFilter() {
		public boolean filter(JFIFPieceSegment segment)
		{
			return segment.isExifSegment();
		}
	};

	private static final SegmentFilter XMP_SEGMENT_FILTER = new SegmentFilter() {
		public boolean filter(JFIFPieceSegment segment)
		{
			return segment.isXmpSegment();
		}
	};

	private static final SegmentFilter PHOTOSHOP_APP13_SEGMENT_FILTER = new SegmentFilter() {
		public boolean filter(JFIFPieceSegment segment)
		{
			return segment.isPhotoshopApp13Segment();
		}
	};

	protected List removeXmpSegments(List segments)
	{
		return filterSegments(segments, XMP_SEGMENT_FILTER);
	}

	protected List removePhotoshopApp13Segments(List segments)
	{
		return filterSegments(segments, PHOTOSHOP_APP13_SEGMENT_FILTER);
	}

	protected List findPhotoshopApp13Segments(List segments)
	{
		return filterSegments(segments, PHOTOSHOP_APP13_SEGMENT_FILTER, true);
	}

	protected List removeExifSegments(List segments)
	{
		return filterSegments(segments, EXIF_SEGMENT_FILTER);
	}

	protected List filterSegments(List segments, SegmentFilter filter)
	{
		return filterSegments(segments, filter, false);
	}

	protected List filterSegments(List segments, SegmentFilter filter,
			boolean reverse)
	{
		List result = new ArrayList();

		for (int i = 0; i < segments.size(); i++)
		{
			JFIFPiece piece = (JFIFPiece) segments.get(i);
			if (piece instanceof JFIFPieceSegment)
			{
				if (filter.filter((JFIFPieceSegment) piece) ^ !reverse)
					result.add(piece);
			} else if(!reverse)
				result.add(piece);
		}

		return result;
	}

	protected List insertBeforeFirstAppSegments(List segments, List newSegments)
			throws ImageWriteException
	{
		int firstAppIndex = -1;
		for (int i = 0; i < segments.size(); i++)
		{
			JFIFPiece piece = (JFIFPiece) segments.get(i);
			if (!(piece instanceof JFIFPieceSegment))
				continue;

			JFIFPieceSegment segment = (JFIFPieceSegment) piece;
			if (segment.isAppSegment())
			{
				if (firstAppIndex == -1)
					firstAppIndex = i;
			}
		}

		List result = new ArrayList(segments);
		if (firstAppIndex == -1)
			throw new ImageWriteException("JPEG file has no APP segments.");
		result.addAll(firstAppIndex, newSegments);
		return result;
	}

	protected List insertAfterLastAppSegments(List segments, List newSegments)
			throws ImageWriteException
	{
		int lastAppIndex = -1;
		for (int i = 0; i < segments.size(); i++)
		{
			JFIFPiece piece = (JFIFPiece) segments.get(i);
			if (!(piece instanceof JFIFPieceSegment))
				continue;

			JFIFPieceSegment segment = (JFIFPieceSegment) piece;
			if (segment.isAppSegment())
				lastAppIndex = i;
		}

		List result = new ArrayList(segments);
		if (lastAppIndex == -1)
		{
			if(segments.size()<1)
				throw new ImageWriteException("JPEG file has no APP segments.");
			result.addAll(1, newSegments);
		}
		else
		result.addAll(lastAppIndex + 1, newSegments);
		
		return result;
	}

	protected void writeSegments(OutputStream os, List segments)
			throws ImageWriteException, IOException
	{
		try
		{
			os.write(SOI);

			for (int i = 0; i < segments.size(); i++)
			{
				JFIFPiece piece = (JFIFPiece) segments.get(i);
				piece.write(os);
			}
			os.close();
			os = null;
		} finally
		{
			try
			{
				if (os != null)
					os.close();
			} catch (Exception e)
			{
				// swallow exception; already in the context of an exception.
			}
		}
	}

	// private void writeSegment(OutputStream os, JFIFPieceSegment piece)
	// throws ImageWriteException, IOException
	// {
	// byte markerBytes[] = convertShortToByteArray(JPEG_APP1_Marker,
	// JPEG_BYTE_ORDER);
	// if (piece.segmentData.length > 0xffff)
	// throw new JpegSegmentOverflowException("Jpeg segment is too long: "
	// + piece.segmentData.length);
	// int segmentLength = piece.segmentData.length + 2;
	// byte segmentLengthBytes[] = convertShortToByteArray(segmentLength,
	// JPEG_BYTE_ORDER);
	//
	// os.write(markerBytes);
	// os.write(segmentLengthBytes);
	// os.write(piece.segmentData);
	// }

	public static class JpegSegmentOverflowException extends
			ImageWriteException
	{
		public JpegSegmentOverflowException(String s)
		{
			super(s);
		}
	}

}