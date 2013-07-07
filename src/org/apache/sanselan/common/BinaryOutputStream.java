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

import org.apache.sanselan.ImageWriteException;

public class BinaryOutputStream extends OutputStream implements BinaryConstants
{
	protected boolean debug = false;
	private int count = 0;

	public final void setDebug(boolean b)
	{
		debug = b;
	}

	public final boolean getDebug()
	{
		return debug;
	}

	private final OutputStream os;

	public BinaryOutputStream(OutputStream os, int byteOrder)
	{
		this.byteOrder = byteOrder;
		this.os = os;
	}

	public BinaryOutputStream(OutputStream os)
	{
		this.os = os;
	}

	// default byte order for Java, many file formats.
	private int byteOrder = BYTE_ORDER_NETWORK;

	protected void setByteOrder(int a, int b) throws ImageWriteException,
			IOException
	{
		if (a != b)
			throw new ImageWriteException("Byte Order bytes don't match (" + a
					+ ", " + b + ").");

		if (a == BYTE_ORDER_MOTOROLA)
			byteOrder = a;
		else if (a == BYTE_ORDER_INTEL)
			byteOrder = a;
		else
			throw new ImageWriteException("Unknown Byte Order hint: " + a);
	}

	protected void setByteOrder(int byteOrder)
	{
		this.byteOrder = byteOrder;
	}

	public int getByteOrder()
	{
		return byteOrder;
	}

	public void write(int i) throws IOException
	{
		os.write(i);
		count++;
	}

	public int getByteCount()
	{
		return count;
	}

	public final void write4Bytes(int value) throws ImageWriteException,
			IOException
	{
		writeNBytes(value, 4);
	}

	public final void write3Bytes(int value) throws ImageWriteException,
			IOException
	{
		writeNBytes(value, 3);
	}

	public final void write2Bytes(int value) throws ImageWriteException,
			IOException
	{
		writeNBytes(value, 2);
	}

	public final void write4ByteInteger(int value) throws ImageWriteException,
			IOException
	{
		if (byteOrder == BYTE_ORDER_MOTOROLA)
		{
			write(0xff & (value >> 24));
			write(0xff & (value >> 16));
			write(0xff & (value >> 8));
			write(0xff & value);
		} else
		{
			write(0xff & value);
			write(0xff & (value >> 8));
			write(0xff & (value >> 16));
			write(0xff & (value >> 24));
		}
	}

	public final void write2ByteInteger(int value) throws ImageWriteException,
			IOException
	{
		if (byteOrder == BYTE_ORDER_MOTOROLA)
		{
			write(0xff & (value >> 8));
			write(0xff & value);
		} else
		{
			write(0xff & value);
			write(0xff & (value >> 8));
		}
	}

	public final void writeByteArray(byte bytes[]) throws IOException
	{
		os.write(bytes, 0, bytes.length);
		count += bytes.length;
	}

	private byte[] convertValueToByteArray(int value, int n)
	{
		byte result[] = new byte[n];

		if (byteOrder == BYTE_ORDER_MOTOROLA)
		{
			for (int i = 0; i < n; i++)
			{
				int b = 0xff & (value >> (8 * (n - i - 1)));
				result[i] = (byte) b;
			}
		} else
		{
			for (int i = 0; i < n; i++)
			{
				int b = 0xff & (value >> (8 * i));
				result[i] = (byte) b;
			}
		}

		return result;
	}

	private final void writeNBytes(int value, int n)
			throws ImageWriteException, IOException
	{
		write(convertValueToByteArray(value, n));
	}

}