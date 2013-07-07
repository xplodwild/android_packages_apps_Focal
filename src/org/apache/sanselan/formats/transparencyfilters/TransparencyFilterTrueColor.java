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
package org.apache.sanselan.formats.transparencyfilters;

import java.io.ByteArrayInputStream;
import java.io.IOException;

import org.apache.sanselan.ImageReadException;

public class TransparencyFilterTrueColor extends TransparencyFilter
{
	private final int transparent_red;
	private final int transparent_green;
	private final int transparent_blue;
	private final int transparent_color;

	public TransparencyFilterTrueColor(byte bytes[]) throws ImageReadException,
			IOException
	{
		super(bytes);

		ByteArrayInputStream is = new ByteArrayInputStream(bytes);
		transparent_red = read2Bytes("transparent_red", is,
				"tRNS: Missing transparent_color");
		transparent_green = read2Bytes("transparent_green", is,
				"tRNS: Missing transparent_color");
		transparent_blue = read2Bytes("transparent_blue", is,
				"tRNS: Missing transparent_color");

		transparent_color = ((0xff & transparent_red) << 16)
				| ((0xff & transparent_green) << 8)
				| ((0xff & transparent_blue) << 0);

	}

	public int filter(int rgb, int sample) throws ImageReadException,
			IOException
	{
		if ((0x00ffffff & rgb) == transparent_color)
			return 0x00;

		return rgb;
	}
}