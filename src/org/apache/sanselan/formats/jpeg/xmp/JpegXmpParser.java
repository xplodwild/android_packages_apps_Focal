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

import java.io.UnsupportedEncodingException;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.common.BinaryFileParser;
import org.apache.sanselan.formats.jpeg.JpegConstants;

public class JpegXmpParser extends BinaryFileParser implements JpegConstants
{

	public JpegXmpParser()
	{
		setByteOrder(BYTE_ORDER_NETWORK);
	}


	public boolean isXmpJpegSegment(byte segmentData[])
	{
		int index = 0;

		if (segmentData.length < XMP_IDENTIFIER.length)
			return false;
		for (; index < XMP_IDENTIFIER.length; index++)
			if (segmentData[index] < XMP_IDENTIFIER[index])
				return false;

		return true;
	}

	public String parseXmpJpegSegment(byte segmentData[])
			throws ImageReadException
	{
		int index = 0;

		if (segmentData.length < XMP_IDENTIFIER.length)
			throw new ImageReadException("Invalid JPEG XMP Segment.");
		for (; index < XMP_IDENTIFIER.length; index++)
			if (segmentData[index] < XMP_IDENTIFIER[index])
				throw new ImageReadException("Invalid JPEG XMP Segment.");

		try
		{
			// segment data is UTF-8 encoded xml.
			String xml = new String(segmentData, index, segmentData.length
					- index, "utf-8");
			return xml;
		} catch (UnsupportedEncodingException e)
		{
			throw new ImageReadException("Invalid JPEG XMP Segment.");
		}
	}

}
