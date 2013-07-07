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

public class App2Segment extends APPNSegment implements Comparable
{
	public final byte icc_bytes[];
	public final int cur_marker, num_markers;

	public App2Segment(int marker, byte segmentData[])
			throws ImageReadException, IOException
	{
		this(marker, segmentData.length, new ByteArrayInputStream(segmentData));
	}

	public App2Segment(int marker, int marker_length, InputStream is2)
			throws ImageReadException, IOException
	{
		super(marker, marker_length, is2);

		if (startsWith(bytes, JpegImageParser.icc_profile_label))
		{
			InputStream is = new ByteArrayInputStream(bytes);

			readAndVerifyBytes(is, JpegImageParser.icc_profile_label,
					"Not a Valid App2 Segment: missing ICC Profile label");

			cur_marker = readByte("cur_marker", is, "Not a valid App2 Marker");
			num_markers = readByte("num_markers", is, "Not a valid App2 Marker");

			marker_length -= JpegImageParser.icc_profile_label.length;
			marker_length -= (1 + 1);

			icc_bytes = readByteArray("App2 Data", marker_length, is,
					"Invalid App2 Segment: insufficient data");
		}
		else
		{
			//			debugByteArray("Unknown APP2 Segment Type", bytes);
			cur_marker = -1;
			num_markers = -1;
			icc_bytes = null;
		}
	}

	public int compareTo(Object o)
	{
		App2Segment other = (App2Segment) o;
		return cur_marker - other.cur_marker;
	}

	//	public String getDescription()
	//	{
	//		return "APPN (APP"
	//				+ (marker - JpegImageParser.JPEG_APP0)
	//				+ ") (" + getDescription() + ")";
	//	}
}