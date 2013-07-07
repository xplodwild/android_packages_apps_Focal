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

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.common.byteSources.ByteSourceArray;
import org.apache.sanselan.common.byteSources.ByteSourceFile;
import org.apache.sanselan.common.byteSources.ByteSourceInputStream;

/**
 * Interface for Exif write/update/remove functionality for Jpeg/JFIF images.
 * <p>
 * <p>
 * See the source of the XmpXmlUpdateExample class for example usage.
 *
 */
public class JpegXmpRewriter extends JpegRewriter
{

	/**
	 * Reads a Jpeg image, removes all XMP XML (by removing the APP1 segment),
	 * and writes the result to a stream.
	 * <p>
	 * 
	 * @param src
	 *            Image file.
	 * @param os
	 *            OutputStream to write the image to.
	 * 
	 * @see java.io.File
	 * @see java.io.OutputStream
	 */
	public void removeXmpXml(File src, OutputStream os)
			throws ImageReadException, IOException, ImageWriteException
	{
		ByteSource byteSource = new ByteSourceFile(src);
		removeXmpXml(byteSource, os);
	}

	/**
	 * Reads a Jpeg image, removes all XMP XML (by removing the APP1 segment),
	 * and writes the result to a stream.
	 * <p>
	 * 
	 * @param src
	 *            Byte array containing Jpeg image data.
	 * @param os
	 *            OutputStream to write the image to.
	 */
	public void removeXmpXml(byte src[], OutputStream os)
			throws ImageReadException, IOException, ImageWriteException
	{
		ByteSource byteSource = new ByteSourceArray(src);
		removeXmpXml(byteSource, os);
	}

	/**
	 * Reads a Jpeg image, removes all XMP XML (by removing the APP1 segment),
	 * and writes the result to a stream.
	 * <p>
	 * 
	 * @param src
	 *            InputStream containing Jpeg image data.
	 * @param os
	 *            OutputStream to write the image to.
	 */
	public void removeXmpXml(InputStream src, OutputStream os)
			throws ImageReadException, IOException, ImageWriteException
	{
		ByteSource byteSource = new ByteSourceInputStream(src, null);
		removeXmpXml(byteSource, os);
	}

	/**
	 * Reads a Jpeg image, removes all XMP XML (by removing the APP1 segment),
	 * and writes the result to a stream.
	 * <p>
	 * 
	 * @param byteSource
	 *            ByteSource containing Jpeg image data.
	 * @param os
	 *            OutputStream to write the image to.
	 */
	public void removeXmpXml(ByteSource byteSource, OutputStream os)
			throws ImageReadException, IOException, ImageWriteException
	{
		JFIFPieces jfifPieces = analyzeJFIF(byteSource);
		List pieces = jfifPieces.pieces;
		pieces = removeXmpSegments(pieces);
		writeSegments(os, pieces);
	}

	/**
	 * Reads a Jpeg image, replaces the XMP XML and writes the result to a
	 * stream.
	 * 
	 * @param src
	 *            Byte array containing Jpeg image data.
	 * @param os
	 *            OutputStream to write the image to.
	 * @param xmpXml
	 *            String containing XMP XML.
	 */
	public void updateXmpXml(byte src[], OutputStream os, String xmpXml)
			throws ImageReadException, IOException, ImageWriteException
	{
		ByteSource byteSource = new ByteSourceArray(src);
		updateXmpXml(byteSource, os, xmpXml);
	}

	/**
	 * Reads a Jpeg image, replaces the XMP XML and writes the result to a
	 * stream.
	 * 
	 * @param src
	 *            InputStream containing Jpeg image data.
	 * @param os
	 *            OutputStream to write the image to.
	 * @param xmpXml
	 *            String containing XMP XML.
	 */
	public void updateXmpXml(InputStream src, OutputStream os, String xmpXml)
			throws ImageReadException, IOException, ImageWriteException
	{
		ByteSource byteSource = new ByteSourceInputStream(src, null);
		updateXmpXml(byteSource, os, xmpXml);
	}

	/**
	 * Reads a Jpeg image, replaces the XMP XML and writes the result to a
	 * stream.
	 * 
	 * @param src
	 *            Image file.
	 * @param os
	 *            OutputStream to write the image to.
	 * @param xmpXml
	 *            String containing XMP XML.
	 */
	public void updateXmpXml(File src, OutputStream os, String xmpXml)
			throws ImageReadException, IOException, ImageWriteException
	{
		ByteSource byteSource = new ByteSourceFile(src);
		updateXmpXml(byteSource, os, xmpXml);
	}

	/**
	 * Reads a Jpeg image, replaces the XMP XML and writes the result to a
	 * stream.
	 * 
	 * @param byteSource
	 *            ByteSource containing Jpeg image data.
	 * @param os
	 *            OutputStream to write the image to.
	 * @param xmpXml
	 *            String containing XMP XML.
	 */
	public void updateXmpXml(ByteSource byteSource, OutputStream os,
			String xmpXml) throws ImageReadException, IOException,
			ImageWriteException
	{
		JFIFPieces jfifPieces = analyzeJFIF(byteSource);
		List pieces = jfifPieces.pieces;
		pieces = removeXmpSegments(pieces);

		List newPieces = new ArrayList();
		byte xmpXmlBytes[] = xmpXml.getBytes("utf-8");
		int index = 0;
		while (index < xmpXmlBytes.length)
		{
			int segmentSize = Math.min(xmpXmlBytes.length, MAX_SEGMENT_SIZE);
			byte segmentData[] = writeXmpSegment(xmpXmlBytes, index,
					segmentSize);
			newPieces.add(new JFIFPieceSegment(JPEG_APP1_Marker, segmentData));
			index += segmentSize;
		}

		pieces = insertAfterLastAppSegments(pieces, newPieces);

		writeSegments(os, pieces);
	}

	private byte[] writeXmpSegment(byte xmpXmlData[], int start, int length)
			throws IOException
	{
		ByteArrayOutputStream os = new ByteArrayOutputStream();

		os.write(XMP_IDENTIFIER);
		os.write(xmpXmlData, start, length);

		return os.toByteArray();
	}

}