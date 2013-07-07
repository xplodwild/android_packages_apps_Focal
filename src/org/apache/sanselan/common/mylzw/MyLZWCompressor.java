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
import java.util.HashMap;
import java.util.Map;

public class MyLZWCompressor
{

	// private static final int MAX_TABLE_SIZE = 1 << 12;

	private int codeSize;
	private final int initialCodeSize;
	private int codes = -1;

	private final int byteOrder;
	private final boolean earlyLimit;
	private final int clearCode;
	private final int eoiCode;
	private final Listener listener;

	public MyLZWCompressor(int initialCodeSize, int byteOrder,
			boolean earlyLimit)
	{
		this(initialCodeSize, byteOrder, earlyLimit, null);
	}

	public MyLZWCompressor(int initialCodeSize, int byteOrder,
			boolean earlyLimit, Listener listener)
	{
		this.listener = listener;
		this.byteOrder = byteOrder;
		this.earlyLimit = earlyLimit;

		this.initialCodeSize = initialCodeSize;

		clearCode = 1 << initialCodeSize;
		eoiCode = clearCode + 1;

		if (null != listener)
			listener.init(clearCode, eoiCode);

		InitializeStringTable();
	}

	private final Map map = new HashMap();

	private final void InitializeStringTable()
	{
		codeSize = initialCodeSize;

		int intial_entries_count = (1 << codeSize) + 2;

		map.clear();
		for (codes = 0; codes < intial_entries_count; codes++)
		{
			if ((codes != clearCode) && (codes != eoiCode))
			{
				Object key = arrayToKey((byte) codes);

				map.put(key, new Integer(codes));
			}
		}
	}

	private final void clearTable()
	{
		InitializeStringTable();
		incrementCodeSize();
	}

	private final void incrementCodeSize()
	{
		if (codeSize != 12)
			codeSize++;
	}

	private final Object arrayToKey(byte b)
	{
		return arrayToKey(new byte[] { b, }, 0, 1);
	}

	private final static class ByteArray
	{
		private final byte bytes[];
		private final int start;
		private final int length;
		private final int hash;

		public ByteArray(byte bytes[])
		{
			this(bytes, 0, bytes.length);
		}

		public ByteArray(byte bytes[], int start, int length)
		{
			this.bytes = bytes;
			this.start = start;
			this.length = length;

			int tempHash = length;

			for (int i = 0; i < length; i++)
			{
				int b = 0xff & bytes[i + start];
				tempHash = tempHash + (tempHash << 8) ^ b ^ i;
			}

			hash = tempHash;
		}

		public final int hashCode()
		{
			return hash;
		}

		public final boolean equals(Object o)
		{
			ByteArray other = (ByteArray) o;
			if (other.hash != hash)
				return false;
			if (other.length != length)
				return false;

			for (int i = 0; i < length; i++)
			{
				if (other.bytes[i + other.start] != bytes[i + start])
					return false;
			}

			return true;
		}
	}

	private final Object arrayToKey(byte bytes[], int start, int length)
	{
		return new ByteArray(bytes, start, length);
	}

	private final void writeDataCode(MyBitOutputStream bos, int code)
			throws IOException
	{
		if (null != listener)
			listener.dataCode(code);
		writeCode(bos, code);
	}


	private final void writeClearCode(MyBitOutputStream bos) throws IOException
	{
		if (null != listener)
			listener.dataCode(clearCode);
		writeCode(bos, clearCode);
	}

	private final void writeEoiCode(MyBitOutputStream bos) throws IOException
	{
		if (null != listener)
			listener.eoiCode(eoiCode);
		writeCode(bos, eoiCode);
	}

	private final void writeCode(MyBitOutputStream bos, int code)
			throws IOException
	{
		bos.writeBits(code, codeSize);
	}

	private final boolean isInTable(byte bytes[], int start, int length)
	{
		Object key = arrayToKey(bytes, start, length);

		return map.containsKey(key);
	}

	private final int codeFromString(byte bytes[], int start, int length)
			throws IOException
	{
		Object key = arrayToKey(bytes, start, length);
		Object o = map.get(key);
		if (o == null)
			throw new IOException("CodeFromString");
		return ((Integer) o).intValue();
	}

	private final boolean addTableEntry(MyBitOutputStream bos, byte bytes[],
			int start, int length) throws IOException
	{
		Object key = arrayToKey(bytes, start, length);
		return addTableEntry(bos, key);
	}

	private final boolean addTableEntry(MyBitOutputStream bos, Object key)
			throws IOException
	{
		boolean cleared = false;

		{
			int limit = (1 << codeSize);
			if (earlyLimit)
				limit--;

			if (codes == limit)
			{
				if (codeSize < 12)
					incrementCodeSize();
				else
				{
					writeClearCode(bos);
					clearTable();
					cleared = true;
				}
			}
		}

		if (!cleared)
		{
			map.put(key, new Integer(codes));
			codes++;
		}

		return cleared;
	}

	public static interface Listener
	{
		public void dataCode(int code);

		public void eoiCode(int code);

		public void clearCode(int code);
		
		public void init(int clearCode, int eoiCode);
	};

	public byte[] compress(byte bytes[]) throws IOException
	{
		ByteArrayOutputStream baos = new ByteArrayOutputStream(bytes.length);
		MyBitOutputStream bos = new MyBitOutputStream(baos, byteOrder);

		InitializeStringTable();
		clearTable();
		writeClearCode(bos);
		boolean cleared = false;

		int w_start = 0;
		int w_length = 0;

		for (int i = 0; i < bytes.length; i++)
		{
			if (isInTable(bytes, w_start, w_length + 1))
			{
				w_length++;

				cleared = false;
			} else
			{
				int code = codeFromString(bytes, w_start, w_length);
				writeDataCode(bos, code);
				cleared = addTableEntry(bos, bytes, w_start, w_length + 1);

				w_start = i;
				w_length = 1;
			}
		} /* end of for loop */

		int code = codeFromString(bytes, w_start, w_length);
		writeDataCode(bos, code);

		writeEoiCode(bos);

		bos.flushCache();

		return baos.toByteArray();
	}
}