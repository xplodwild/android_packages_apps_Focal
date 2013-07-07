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

import java.io.IOException;

import org.apache.sanselan.ImageReadException;

public class TransparencyFilterIndexedColor extends TransparencyFilter
{

	public TransparencyFilterIndexedColor(byte bytes[])
	{
		super(bytes);
	}

	int count = 0;

	public int filter(int rgb, int index) throws ImageReadException,
			IOException
	{
		if (index >= bytes.length)
			return rgb;

		if ((index < 0) || (index > bytes.length))
			throw new ImageReadException(
					"TransparencyFilterIndexedColor index: " + index
							+ ", bytes.length: " + bytes.length);

		int alpha = bytes[index];
		int result = ((0xff & alpha) << 24) | (0x00ffffff & rgb);

		if ((count < 100) && (index > 0))
		{
			count++;
		}
		return result;
	}
}