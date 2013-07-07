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

public class BitInputStreamFlexible extends InputStream
		implements
			BinaryConstants
{
	// TODO should be byte order conscious, ie TIFF for reading 
	// samples size<8 - shuoldn't that effect their order within byte?
	private final InputStream is;

	public BitInputStreamFlexible(InputStream is)
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
	private long bytesRead = 0;

	public final int readBits(int count) throws IOException
	{

		if (count <= 32) // catch-all
		{
			int result = 0;
			//			int done = 0;

			if (cacheBitsRemaining > 0)
			{
				if (count >= cacheBitsRemaining)
				{
					result = ((1 << cacheBitsRemaining) - 1) & cache;
					count -= cacheBitsRemaining;
					cacheBitsRemaining = 0;
				}
				else
				{
					//					cache >>= count;
					cacheBitsRemaining -= count;
					result = ((1 << count) - 1) & (cache >> cacheBitsRemaining);
					count = 0;
				}
			}
			while (count >= 8)
			{
				cache = is.read();
				if (cache < 0)
					throw new IOException("couldn't read bits");
				System.out.println("cache 1: " + cache + " ("
						+ Integer.toHexString(cache) + ", "
						+ Integer.toBinaryString(cache) + ")");
				bytesRead++;
				result = (result << 8) | (0xff & cache);
				count -= 8;
			}
			if (count > 0)
			{
				cache = is.read();
				if (cache < 0)
					throw new IOException("couldn't read bits");
				System.out.println("cache 2: " + cache + " ("
						+ Integer.toHexString(cache) + ", "
						+ Integer.toBinaryString(cache) + ")");
				bytesRead++;
				cacheBitsRemaining = 8 - count;
				result = (result << count)
						| (((1 << count) - 1) & (cache >> cacheBitsRemaining));
				count = 0;
			}

			return result;
		}

		throw new IOException("BitInputStream: unknown error");

	}

	public void flushCache()
	{
		cacheBitsRemaining = 0;
	}

	public long getBytesRead()
	{
		return bytesRead;
	}
}