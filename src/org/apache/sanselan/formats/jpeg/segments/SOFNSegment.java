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
package org.apache.sanselan.formats.jpeg.segments;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.formats.jpeg.JpegImageParser;

public class SOFNSegment extends Segment
{
	public final int width, height;
	public final int numberOfComponents;
	public final int precision;

	public SOFNSegment(int marker, byte segmentData[])
			throws ImageReadException, IOException
	{
		this(marker, segmentData.length, new ByteArrayInputStream(segmentData));
	}

	public SOFNSegment(int marker, int marker_length, InputStream is)
			throws ImageReadException, IOException
	{
		super(marker, marker_length);

		if (getDebug())
			System.out.println("SOF0Segment marker_length: " + marker_length);

		{
			precision = readByte("Data_precision", is, "Not a Valid JPEG File");
			height = read2Bytes("Image_height", is, "Not a Valid JPEG File");
			width = read2Bytes("Image_Width", is, "Not a Valid JPEG File");
			numberOfComponents = readByte("Number_of_components", is,
					"Not a Valid JPEG File");

			// ignore the rest of the segment for now...
			skipBytes(is, marker_length - 6,
					"Not a Valid JPEG File: SOF0 Segment");

			//				int Each_component1 = read_byte("Each_component1", is,
			//						"Not a Valid JPEG File");
			//				int Each_component2 = read_byte("Each_component2", is,
			//						"Not a Valid JPEG File");
			//				int Each_component3 = read_byte("Each_component3", is,
			//						"Not a Valid JPEG File");
		}

		if (getDebug())
			System.out.println("");
	}

	public String getDescription()
	{
		return "SOFN (SOF" + (marker - JpegImageParser.SOF0Marker) + ") ("
				+ getSegmentType() + ")";
	}

}