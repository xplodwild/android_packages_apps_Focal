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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class MyLZWDecompressor
{
	private static final int MAX_TABLE_SIZE = 1 << 12;

	private final byte[][] table;
	private int codeSize;
	private final int initialCodeSize;
	private int codes = -1;

	private final int byteOrder;

	private final Listener listener;

	public static interface Listener
	{
		public void code(int code);
		
		public void init(int clearCode, int eoiCode);
	}

	public MyLZWDecompressor(int initialCodeSize, int byteOrder)
	{
		this(initialCodeSize, byteOrder, null);
	}

	public MyLZWDecompressor(int initialCodeSize, int byteOrder,
			Listener listener)
	{
		this.listener = listener;
		this.byteOrder = byteOrder;

		this.initialCodeSize = initialCodeSize;

		table = new byte[MAX_TABLE_SIZE][];
		clearCode = 1 << initialCodeSize;
		eoiCode = clearCode + 1;

		if (null != listener)
			listener.init(clearCode, eoiCode);

		InitializeTable();
	}

	private final void InitializeTable()
	{
		codeSize = initialCodeSize;

		int intial_entries_count = 1 << codeSize + 2;

		for (int i = 0; i < intial_entries_count; i++)
			table[i] = new byte[] { (byte) i, };
	}

	private final void clearTable()
	{
		codes = (1 << initialCodeSize) + 2;
		codeSize = initialCodeSize;
		incrementCodeSize();
	}

	private final int clearCode;
	private final int eoiCode;

	private final int getNextCode(MyBitInputStream is) throws IOException
	{
		int code = is.readBits(codeSize);

		if (null != listener)
			listener.code(code);
		return code;
	}

	private final byte[] stringFromCode(int code) throws IOException
	{
		if ((code >= codes) || (code < 0))
			throw new IOException("Bad Code: " + code + " codes: " + codes
					+ " code_size: " + codeSize + ", table: " + table.length);

		return table[code];
	}

	private final boolean isInTable(int Code)
	{
		return Code < codes;
	}

	private final byte firstChar(byte bytes[])
	{
		return bytes[0];
	}

	private final void addStringToTable(byte bytes[]) throws IOException
	{
		if (codes < (1 << codeSize))
		{
			table[codes] = bytes;
			codes++;
		} else
			throw new IOException("AddStringToTable: codes: " + codes
					+ " code_size: " + codeSize);

		checkCodeSize();
	}

	private final byte[] appendBytes(byte bytes[], byte b)
	{
		byte result[] = new byte[bytes.length + 1];

		System.arraycopy(bytes, 0, result, 0, bytes.length);
		result[result.length - 1] = b;
		return result;
	}

	private int written = 0;

	private final void writeToResult(OutputStream os, byte bytes[])
			throws IOException
	{
		os.write(bytes);
		written += bytes.length;
	}

	private boolean tiffLZWMode = false;

	public void setTiffLZWMode()
	{
		tiffLZWMode = true;
	}

	public byte[] decompress(InputStream is, int expectedLength)
			throws IOException
	{
		int code, oldCode = -1;
		MyBitInputStream mbis = new MyBitInputStream(is, byteOrder);
		if (tiffLZWMode)
			mbis.setTiffLZWMode();

		ByteArrayOutputStream baos = new ByteArrayOutputStream(expectedLength);

		clearTable();

		while ((code = getNextCode(mbis)) != eoiCode)
		{
			if (code == clearCode)
			{
				clearTable();

				if (written >= expectedLength)
					break;
				code = getNextCode(mbis);

				if (code == eoiCode)
				{
					break;
				}
				writeToResult(baos, stringFromCode(code));

				oldCode = code;
			} // end of ClearCode case
			else
			{
				if (isInTable(code))
				{
					writeToResult(baos, stringFromCode(code));

					addStringToTable(appendBytes(stringFromCode(oldCode),
							firstChar(stringFromCode(code))));
					oldCode = code;
				} else
				{
					byte OutString[] = appendBytes(stringFromCode(oldCode),
							firstChar(stringFromCode(oldCode)));
					writeToResult(baos, OutString);
					addStringToTable(OutString);
					oldCode = code;
				}
			} // end of not-ClearCode case

			if (written >= expectedLength)
				break;
		} // end of while loop

		byte result[] = baos.toByteArray();

		return result;
	}

	private final void checkCodeSize() // throws IOException
	{
		int limit = (1 << codeSize);
		if (tiffLZWMode)
			limit--;

		if (codes == limit)
			incrementCodeSize();
	}

	private final void incrementCodeSize() // throws IOException
	{
		if (codeSize != 12)
			codeSize++;
	}
}