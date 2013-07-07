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
package org.apache.sanselan.common.byteSources;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

public class ByteSourceArray extends ByteSource
{
	private final byte bytes[];

	public ByteSourceArray(String filename, byte bytes[])
	{
		super(filename);
		this.bytes = bytes;
	}

	public ByteSourceArray(byte bytes[])
	{
		super(null);
		this.bytes = bytes;
	}

	public InputStream getInputStream() //throws IOException
	{
		return new ByteArrayInputStream(bytes);
	}

	public byte[] getBlock(int start, int length) throws IOException
	{
		if (start + length > bytes.length)
			throw new IOException("Could not read block (block start: " + start
					+ ", block length: " + length + ", data length: "
					+ bytes.length + ").");

		byte result[] = new byte[length];
		System.arraycopy(bytes, start, result, 0, length);
		return result;
	}

	public long getLength()
	{
		return bytes.length;
	}

	public byte[] getAll() throws IOException
	{
		return bytes;
	}

	public String getDescription()
	{
		return bytes.length + " byte array";
	}

}