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

import java.io.IOException;
import java.io.InputStream;

public class BitInputStream extends InputStream implements BinaryConstants
{
	// TODO should be byte order conscious, ie TIFF for reading 
	// samples size<8 - shuoldn't that effect their order within byte?
	private final InputStream is;

	public BitInputStream(InputStream is)
	{
		this.is = is;
		//			super(is);
	}

	public int read() throws IOException
	{
		if (cacheBitsRemaining > 0)
			throw new IOException("BitInputStream: incomplete bit read");
		return is.read();
	}

	private int cache;
	private int cacheBitsRemaining = 0;
	private long bytes_read = 0;

	public final int readBits(int count) throws IOException
	{
		if (count < 8)
		{
			if (cacheBitsRemaining == 0)
			{
				// fill cache
				cache = is.read();
				cacheBitsRemaining = 8;
				bytes_read++;
			}
			if (count > cacheBitsRemaining)
				throw new IOException(
						"BitInputStream: can't read bit fields across bytes");

			//				int bits_to_shift = cache_bits_remaining - count;
			cacheBitsRemaining -= count;
			int bits = cache >> cacheBitsRemaining;

			switch (count)
			{
				case 1 :
					return bits & 1;
				case 2 :
					return bits & 3;
				case 3 :
					return bits & 7;
				case 4 :
					return bits & 15;
				case 5 :
					return bits & 31;
				case 6 :
					return bits & 63;
				case 7 :
					return bits & 127;
			}

		}
		if (cacheBitsRemaining > 0)
			throw new IOException("BitInputStream: incomplete bit read");

		if (count == 8)
		{
			bytes_read++;
			return is.read();
		}

		if (count == 16)
		{
			bytes_read += 2;
			return (is.read() << 8) | (is.read() << 0);
		}

		if (count == 24)
		{
			bytes_read += 3;
			return (is.read() << 16) | (is.read() << 8) | (is.read() << 0);
		}

		if (count == 32)
		{
			bytes_read += 4;
			return (is.read() << 24) | (is.read() << 16) | (is.read() << 8)
					| (is.read() << 0);
		}

		throw new IOException("BitInputStream: unknown error");
	}

	public void flushCache()
	{
		cacheBitsRemaining = 0;
	}

	public long getBytesRead()
	{
		return bytes_read;
	}
}