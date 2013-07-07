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
import java.io.OutputStream;

public class MyByteArrayOutputStream extends OutputStream
// some performace benefit, because not thread safe.
{
	private final byte bytes[];

	public MyByteArrayOutputStream(int length)
	{
		bytes = new byte[length];
	}

	private int count = 0;

	public void write(int value) throws IOException
	{
		if (count >= bytes.length)
			throw new IOException("Write exceeded expected length (" + count
					+ ", " + bytes.length + ")");

		bytes[count] = (byte) value;
		count++;
	}

	public byte[] toByteArray()
	{
		if (count < bytes.length)
		{
			byte result[] = new byte[count];
			System.arraycopy(bytes, 0, result, 0, count);
			return result;
		}
		return bytes;
	}

	public int getBytesWritten()
	{
		return count;
	}
}