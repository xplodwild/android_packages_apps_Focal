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

import org.apache.sanselan.ImageReadException;

public class BinaryFileParser extends BinaryFileFunctions
{
	public BinaryFileParser(int byteOrder)
	{
		this.byteOrder = byteOrder;
	}

	public BinaryFileParser()
	{

	}

	// default byte order for Java, many file formats.
	private int byteOrder = BYTE_ORDER_NETWORK;

	// protected boolean BYTE_ORDER_reversed = true;

	protected void setByteOrder(int a, int b) throws ImageReadException,
			IOException
	{
		if (a != b)
			throw new ImageReadException("Byte Order bytes don't match (" + a
					+ ", " + b + ").");

		if (a == BYTE_ORDER_MOTOROLA)
			byteOrder = a;
		else if (a == BYTE_ORDER_INTEL)
			byteOrder = a;
		else
			throw new ImageReadException("Unknown Byte Order hint: " + a);
	}

	protected void setByteOrder(int byteOrder)
	{
		this.byteOrder = byteOrder;
	}

	protected int getByteOrder()
	{
		return byteOrder;
	}

	protected final int convertByteArrayToInt(String name, int start,
			byte bytes[])
	{
		return convertByteArrayToInt(name, bytes, start, byteOrder);
	}

	protected final int convertByteArrayToInt(String name, byte bytes[])
	{
		return convertByteArrayToInt(name, bytes, byteOrder);
	}

	public final int convertByteArrayToShort(String name, byte bytes[])
			throws ImageReadException
	{
		return convertByteArrayToShort(name, bytes, byteOrder);
	}

	public final int convertByteArrayToShort(String name, int start,
			byte bytes[]) throws ImageReadException
	{
		return convertByteArrayToShort(name, start, bytes, byteOrder);
	}

	public final int read4Bytes(String name, InputStream is, String exception)
			throws ImageReadException, IOException
	{
		return read4Bytes(name, is, exception, byteOrder);
	}

	public final int read3Bytes(String name, InputStream is, String exception)
			throws ImageReadException, IOException
	{
		return read3Bytes(name, is, exception, byteOrder);
	}

	public final int read2Bytes(String name, InputStream is, String exception)
			throws ImageReadException, IOException
	{
		return read2Bytes(name, is, exception, byteOrder);
	}

	public static boolean byteArrayHasPrefix(byte bytes[], byte prefix[])
	{
		if ((bytes == null) || (bytes.length < prefix.length))
			return false;

		for (int i = 0; i < prefix.length; i++)
			if (bytes[i] != prefix[i])
				return false;

		return true;
	}

	protected final byte[] int2ToByteArray(int value)
	{
		return int2ToByteArray(value, byteOrder);
	}

}