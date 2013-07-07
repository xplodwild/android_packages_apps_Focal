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
package org.apache.sanselan.common;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import org.apache.sanselan.ImageReadException;

public class PackBits
{

	public byte[] decompress(byte bytes[], int expected)
			throws ImageReadException, IOException
	{
		int total = 0;

		ByteArrayOutputStream baos = new ByteArrayOutputStream();

		//	Loop until you get the number of unpacked bytes you are expecting:
		int i = 0;
		while (total < expected)

		{
			//		Read the next source byte into n.
			if (i >= bytes.length)
				throw new ImageReadException(
						"Tiff: Unpack bits source exhausted: " + i
								+ ", done + " + total + ", expected + "
								+ expected);

			int n = bytes[i++];
			//				If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
			if ((n >= 0) && (n <= 127))
			{

				int count = n + 1;

				total += count;
				for (int j = 0; j < count; j++)
					baos.write(bytes[i++]);
			}
			//				Else if n is between -127 and -1 inclusive, copy the next byte -n+1
			//				times.
			else if ((n >= -127) && (n <= -1))
			{
				int b = bytes[i++];
				int count = -n + 1;

				total += count;
				for (int j = 0; j < count; j++)
					baos.write(b);
			}
			else if (n == -128)
				throw new ImageReadException("Packbits: " + n);
			//				Else if n is between -127 and -1 inclusive, copy the next byte -n+1
			//				times.
			//		else 
			//				Else if n is -128, noop.
		}
		byte result[] = baos.toByteArray();

		return result;

	}

	private int findNextDuplicate(byte bytes[], int start)
	{
		//		int last = -1;
		if (start >= bytes.length)
			return -1;

		byte prev = bytes[start];

		for (int i = start + 1; i < bytes.length; i++)
		{
			byte b = bytes[i];

			if (b == prev)
				return i - 1;

			prev = b;
		}

		return -1;
	}

	private int findRunLength(byte bytes[], int start)
	{
		byte b = bytes[start];

		int i;

		for (i = start + 1; (i < bytes.length) && (bytes[i] == b); i++)
			;

		return i - start;
	}

	public byte[] compress(byte bytes[]) throws IOException
	{
		MyByteArrayOutputStream baos = new MyByteArrayOutputStream(
				bytes.length * 2); // max length 1 extra byte for every 128

		int ptr = 0;
		int count = 0;
		while (ptr < bytes.length)
		{
			count++;
			int dup = findNextDuplicate(bytes, ptr);

			if (dup == ptr) // write run length
			{
				int len = findRunLength(bytes, dup);
				int actual_len = Math.min(len, 128);
				baos.write(-(actual_len - 1));
				baos.write(bytes[ptr]);
				ptr += actual_len;
			}
			else
			{ // write literals
				int len = dup - ptr;

				if (dup > 0)
				{
					int runlen = findRunLength(bytes, dup);
					if (runlen < 3) // may want to discard next run.
					{
						int nextptr = ptr + len + runlen;
						int nextdup = findNextDuplicate(bytes, nextptr);
						if (nextdup != nextptr) // discard 2-byte run
						{
							dup = nextdup;
							len = dup - ptr;
						}
					}
				}

				if (dup < 0)
					len = bytes.length - ptr;
				int actual_len = Math.min(len, 128);

				baos.write(actual_len - 1);
				for (int i = 0; i < actual_len; i++)
				{
					baos.write(bytes[ptr]);
					ptr++;
				}
			}
		}
		byte result[] = baos.toByteArray();

		return result;

	}
}