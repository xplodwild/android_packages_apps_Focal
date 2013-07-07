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

import java.io.IOException;
import java.io.InputStream;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.util.Debug;

public class SOSSegment extends Segment
{
	//	public final int width, height;
	//	public final int Number_of_components;
	//	public final int Precision;

	//		public final byte bytes[];
	//		public final int cur_marker, num_markers;

	public SOSSegment(int marker, int marker_length, InputStream is)
			throws ImageReadException, IOException
	{
		super(marker, marker_length);

		if (getDebug())
			System.out.println("SOSSegment marker_length: " + marker_length);

		Debug.debug("SOS", marker_length);
		//		{
		int number_of_components_in_scan = readByte(
				"number_of_components_in_scan", is, "Not a Valid JPEG File");
		Debug.debug("number_of_components_in_scan",
				number_of_components_in_scan);

		for (int i = 0; i < number_of_components_in_scan; i++)
		{
			int scan_component_selector = readByte("scan_component_selector",
					is, "Not a Valid JPEG File");
			Debug.debug("scan_component_selector", scan_component_selector);

			int ac_dc_entrooy_coding_table_selector = readByte(
					"ac_dc_entrooy_coding_table_selector", is,
					"Not a Valid JPEG File");
			Debug.debug("ac_dc_entrooy_coding_table_selector",
					ac_dc_entrooy_coding_table_selector);
		}

		int start_of_spectral_selection = readByte(
				"start_of_spectral_selection", is, "Not a Valid JPEG File");
		Debug.debug("start_of_spectral_selection", start_of_spectral_selection);
		int end_of_spectral_selection = readByte("end_of_spectral_selection",
				is, "Not a Valid JPEG File");
		Debug.debug("end_of_spectral_selection", end_of_spectral_selection);
		int successive_approximation_bit_position = readByte(
				"successive_approximation_bit_position", is,
				"Not a Valid JPEG File");
		Debug.debug("successive_approximation_bit_position",
				successive_approximation_bit_position);

		//			height = read2Bytes("Image_height", is, "Not a Valid JPEG File");
		//			width = read2Bytes("Image_Width", is, "Not a Valid JPEG File");
		//			Number_of_components = read_byte("Number_of_components", is,
		//					"Not a Valid JPEG File");
		//
		//			// ignore the rest of the segment for now...
		//			skipBytes(is, marker_length - 6,
		//					"Not a Valid JPEG File: SOF0 Segment");
		//
		//			//				int Each_component1 = read_byte("Each_component1", is,
		//			//						"Not a Valid JPEG File");
		//			//				int Each_component2 = read_byte("Each_component2", is,
		//			//						"Not a Valid JPEG File");
		//			//				int Each_component3 = read_byte("Each_component3", is,
		//			//						"Not a Valid JPEG File");
		//		}

		if (getDebug())
			System.out.println("");
	}

	public String getDescription()
	{
		return "SOS (" + getSegmentType() + ")";
	}

}