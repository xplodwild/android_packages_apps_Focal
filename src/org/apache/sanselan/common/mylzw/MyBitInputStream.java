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
package org.apache.sanselan.common.mylzw;

import java.io.IOException;
import java.io.InputStream;

import org.apache.sanselan.common.BinaryConstants;

public class MyBitInputStream extends InputStream implements BinaryConstants
{
	private final InputStream is;
	private final int byteOrder;
	private boolean tiffLZWMode = false;

	public MyBitInputStream(InputStream is, int byteOrder)
	{
		this.byteOrder = byteOrder;
		this.is = is;
	}

	public int read() throws IOException
	{
		return readBits(8);
	}

	private long bytesRead = 0;
	private int bitsInCache = 0;
	private int bitCache = 0;

	public void setTiffLZWMode()
	{
		tiffLZWMode = true;
	}

	public int readBits(int SampleBits) throws IOException
	{
		while (bitsInCache < SampleBits)
		{
			int next = is.read();

			if (next < 0)
			{
				if (tiffLZWMode)
				{
					// pernicious special case!
					return 257;
				}
				return -1;
			}

			int newByte = (0xff & next);

			if (byteOrder == BYTE_ORDER_NETWORK) // MSB, so add to right
				bitCache = (bitCache << 8) | newByte;
			else if (byteOrder == BYTE_ORDER_INTEL) // LSB, so add to left
				bitCache = (newByte << bitsInCache) | bitCache;
			else
				throw new IOException("Unknown byte order: " + byteOrder);

			bytesRead++;
			bitsInCache += 8;
		}
		int sampleMask = (1 << SampleBits) - 1;

		int sample;

		if (byteOrder == BYTE_ORDER_NETWORK) // MSB, so read from left
		{
			sample = sampleMask & (bitCache >> (bitsInCache - SampleBits));
		}
		else if (byteOrder == BYTE_ORDER_INTEL) // LSB, so read from right
		{
			sample = sampleMask & bitCache;
			bitCache >>= SampleBits;
		}
		else
			throw new IOException("Unknown byte order: " + byteOrder);

		int result = sample;

		bitsInCache -= SampleBits;
		int remainderMask = (1 << bitsInCache) - 1;
		bitCache &= remainderMask;

		return result;
	}

	public void flushCache()
	{
		bitsInCache = 0;
		bitCache = 0;
	}

	public long getBytesRead()
	{
		return bytesRead;
	}

}