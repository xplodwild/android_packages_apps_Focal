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

package org.apache.sanselan.util;

import java.io.UnsupportedEncodingException;

import org.apache.sanselan.common.BinaryConstants;

public abstract class UnicodeUtils implements BinaryConstants
{
	/**
	 * This class should never be instantiated.
	 */
	private UnicodeUtils()
	{
	}
	
	public static class UnicodeException extends Exception
	{
		public UnicodeException(String message)
		{
			super(message);
		}
	}

	// A default single-byte charset.
	public static final int CHAR_ENCODING_CODE_ISO_8859_1 = 0;
	public static final int CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_WITH_BOM = 1;
	public static final int CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_WITH_BOM = 2;
	public static final int CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_NO_BOM = 3;
	public static final int CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_NO_BOM = 4;
	public static final int CHAR_ENCODING_CODE_UTF_8 = 5;
	public static final int CHAR_ENCODING_CODE_AMBIGUOUS = -1;

	// /*
	// * Guess the character encoding of arbitrary character data in a data
	// * buffer.
	// *
	// * The data may not run to the end of the buffer; it may be terminated.
	// This
	// * makes the problem much harder, since the character data may be followed
	// * by arbitrary data.
	// */
	// public static int guessCharacterEncoding(byte bytes[], int index)
	// {
	// int length = bytes.length - index;
	//
	// if (length < 1)
	// return CHAR_ENCODING_CODE_AMBIGUOUS;
	//
	// if (length >= 2)
	// {
	// // look for BOM.
	//
	// int c1 = 0xff & bytes[index];
	// int c2 = 0xff & bytes[index + 1];
	// if (c1 == 0xFF && c2 == 0xFE)
	// return CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_WITH_BOM;
	// else if (c1 == 0xFE && c2 == 0xFF)
	// return CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_WITH_BOM;
	// }
	//
	// }
	//
	// /*
	// * Guess the character encoding of arbitrary character data in a data
	// * buffer.
	// *
	// * The data fills the entire buffer. If it is terminated, the terminator
	// * byte(s) will be the last bytes in the buffer.
	// *
	// * This makes the problem a bit easier.
	// */
	// public static int guessCharacterEncodingSimple(byte bytes[], int index)
	// throws UnicodeException
	// {
	// int length = bytes.length - index;
	//
	// if (length < 1)
	// return CHAR_ENCODING_CODE_AMBIGUOUS;
	//
	// if (length >= 2)
	// {
	// // identify or eliminate UTF-16 with a BOM.
	//
	// int c1 = 0xff & bytes[index];
	// int c2 = 0xff & bytes[index + 1];
	// if (c1 == 0xFF && c2 == 0xFE)
	// return CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_WITH_BOM;
	// else if (c1 == 0xFE && c2 == 0xFF)
	// return CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_WITH_BOM;
	// }
	//
	// if (length >= 2)
	// {
	// // look for optional double-byte terminator.
	//
	// int c1 = 0xff & bytes[bytes.length - 2];
	// int c2 = 0xff & bytes[bytes.length - 1];
	// if (c1 == 0 && c2 == 0)
	// {
	// // definitely a flavor of UTF-16.
	// if (length % 2 != 0)
	// throw new UnicodeException(
	// "Character data with double-byte terminator has an odd length.");
	//
	// boolean mayHaveTerminator = true;
	// boolean mustHaveTerminator = false;
	// boolean possibleBigEndian = new UnicodeMetricsUTF16NoBOM(
	// BYTE_ORDER_BIG_ENDIAN).isValid(bytes, index,
	// mayHaveTerminator, mustHaveTerminator);
	// boolean possibleLittleEndian = new UnicodeMetricsUTF16NoBOM(
	// BYTE_ORDER_LITTLE_ENDIAN).isValid(bytes, index,
	// mayHaveTerminator, mustHaveTerminator);
	// if ((!possibleBigEndian) && (!possibleLittleEndian))
	// throw new UnicodeException(
	// "Invalid character data, possibly UTF-16.");
	// if (possibleBigEndian && possibleLittleEndian)
	// return CHAR_ENCODING_CODE_AMBIGUOUS;
	// if (possibleBigEndian)
	// return CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_NO_BOM;
	// if (possibleLittleEndian)
	// return CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_NO_BOM;
	// }
	// }
	//
	// List possibleEncodings = new ArrayList();
	// if (length % 2 == 0)
	// {
	// boolean mayHaveTerminator = true;
	// boolean mustHaveTerminator = false;
	// boolean possibleBigEndian = new UnicodeMetricsUTF16NoBOM(
	// BYTE_ORDER_BIG_ENDIAN).isValid(bytes, index,
	// mayHaveTerminator, mustHaveTerminator);
	// boolean possibleLittleEndian = new UnicodeMetricsUTF16NoBOM(
	// BYTE_ORDER_LITTLE_ENDIAN).isValid(bytes, index,
	// mayHaveTerminator, mustHaveTerminator);
	//
	// if (possibleBigEndian)
	// return CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_NO_BOM;
	// if (possibleLittleEndian)
	// return CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_NO_BOM;
	// }
	//
	// }

	public static final boolean isValidISO_8859_1(String s)
	{
		try
		{
			String roundtrip = new String(s.getBytes("ISO-8859-1"),
					"ISO-8859-1");
			return s.equals(roundtrip);
		} catch (UnsupportedEncodingException e)
		{
			// should never be thrown.
			throw new RuntimeException("Error parsing string.", e);
		}
	}

	/*
	 * Return the index of the first utf-16 terminator (ie. two even-aligned
	 * nulls). If not found, return -1.
	 */
	private static int findFirstDoubleByteTerminator(byte bytes[], int index)
	{
		for (int i = index; i < bytes.length - 1; i += 2)
		{
			int c1 = 0xff & bytes[index];
			int c2 = 0xff & bytes[index + 1];
			if (c1 == 0 && c2 == 0)
				return i;
		}
		return -1;
	}

	public final int findEndWithTerminator(byte bytes[], int index)
			throws UnicodeException
	{
		return findEnd(bytes, index, true);
	}

	public final int findEndWithoutTerminator(byte bytes[], int index)
			throws UnicodeException
	{
		return findEnd(bytes, index, false);
	}

	protected abstract int findEnd(byte bytes[], int index,
			boolean includeTerminator) throws UnicodeException;

	public static UnicodeUtils getInstance(int charEncodingCode)
			throws UnicodeException
	{
		switch (charEncodingCode)
		{
		case CHAR_ENCODING_CODE_ISO_8859_1:
			return new UnicodeMetricsASCII();
		case CHAR_ENCODING_CODE_UTF_8:
			// Debug.debug("CHAR_ENCODING_CODE_UTF_8");
			return new UnicodeMetricsUTF8();
		case CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_WITH_BOM:
		case CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_WITH_BOM:
			// Debug.debug("CHAR_ENCODING_CODE_UTF_16_WITH_BOM");
			return new UnicodeMetricsUTF16WithBOM();
		case CHAR_ENCODING_CODE_UTF_16_BIG_ENDIAN_NO_BOM:
			return new UnicodeMetricsUTF16NoBOM(BYTE_ORDER_BIG_ENDIAN);
		case CHAR_ENCODING_CODE_UTF_16_LITTLE_ENDIAN_NO_BOM:
			return new UnicodeMetricsUTF16NoBOM(BYTE_ORDER_LITTLE_ENDIAN);
		default:
			throw new UnicodeException("Unknown char encoding code: "
					+ charEncodingCode);
		}
	}

	private static class UnicodeMetricsASCII extends UnicodeUtils
	{
		public int findEnd(byte bytes[], int index, boolean includeTerminator)
				throws UnicodeException
		{
			for (int i = index; i < bytes.length; i++)
			{
				if (bytes[i] == 0)
					return includeTerminator ? i + 1 : i;
			}
			return bytes.length;
			// throw new UnicodeException("Terminator not found.");
		}
	}

	// private static class UnicodeMetricsISO_8859_1 extends UnicodeUtils
	// {
	// public int findEnd(byte bytes[], int index, boolean includeTerminator)
	// throws UnicodeException
	// {
	// for (int i = index; i < bytes.length; i++)
	// {
	// if (bytes[i] == 0)
	// return includeTerminator ? i + 1 : i;
	// }
	// return bytes.length;
	// // throw new UnicodeException("Terminator not found.");
	// }
	// }

	private static class UnicodeMetricsUTF8 extends UnicodeUtils
	{

		public int findEnd(byte bytes[], int index, boolean includeTerminator)
				throws UnicodeException
		{
			// http://en.wikipedia.org/wiki/UTF-8

			while (true)
			{
				if (index == bytes.length)
					return bytes.length;
				if (index > bytes.length)
					throw new UnicodeException("Terminator not found.");

				int c1 = 0xff & bytes[index++];
				if (c1 == 0)
					return includeTerminator ? index : index - 1;
				else if (c1 <= 0x7f)
					continue;
				else if (c1 <= 0xDF)
				{
					if (index >= bytes.length)
						throw new UnicodeException("Invalid unicode.");

					int c2 = 0xff & bytes[index++];
					if (c2 < 0x80 || c2 > 0xBF)
						throw new UnicodeException("Invalid code point.");
				} else if (c1 <= 0xEF)
				{
					if (index >= bytes.length - 1)
						throw new UnicodeException("Invalid unicode.");

					int c2 = 0xff & bytes[index++];
					if (c2 < 0x80 || c2 > 0xBF)
						throw new UnicodeException("Invalid code point.");
					int c3 = 0xff & bytes[index++];
					if (c3 < 0x80 || c3 > 0xBF)
						throw new UnicodeException("Invalid code point.");
				} else if (c1 <= 0xF4)
				{
					if (index >= bytes.length - 2)
						throw new UnicodeException("Invalid unicode.");

					int c2 = 0xff & bytes[index++];
					if (c2 < 0x80 || c2 > 0xBF)
						throw new UnicodeException("Invalid code point.");
					int c3 = 0xff & bytes[index++];
					if (c3 < 0x80 || c3 > 0xBF)
						throw new UnicodeException("Invalid code point.");
					int c4 = 0xff & bytes[index++];
					if (c4 < 0x80 || c4 > 0xBF)
						throw new UnicodeException("Invalid code point.");
				} else
					throw new UnicodeException("Invalid code point.");
			}
		}
	}

	private abstract static class UnicodeMetricsUTF16 extends UnicodeUtils
	{
		protected static final int BYTE_ORDER_BIG_ENDIAN = 0;
		protected static final int BYTE_ORDER_LITTLE_ENDIAN = 1;
		protected int byteOrder = BYTE_ORDER_BIG_ENDIAN;

		public UnicodeMetricsUTF16(int byteOrder)
		{
			this.byteOrder = byteOrder;
		}

		public boolean isValid(byte bytes[], int index,
				boolean mayHaveTerminator, boolean mustHaveTerminator)
				throws UnicodeException
		{
			// http://en.wikipedia.org/wiki/UTF-16/UCS-2

			while (true)
			{
				if (index == bytes.length)
				{
					// end of buffer, no terminator found.
					return !mustHaveTerminator;
				}

				if (index >= bytes.length - 1)
				{
					// end of odd-length buffer, no terminator found.
					return false;
				}

				int c1 = 0xff & bytes[index++];
				int c2 = 0xff & bytes[index++];
				int msb1 = byteOrder == BYTE_ORDER_BIG_ENDIAN ? c1 : c2;

				if (c1 == 0 && c2 == 0)
				{
					// terminator found.
					return mayHaveTerminator;
				}

				if (msb1 >= 0xD8)
				{
					// Surrogate pair found.

					if (msb1 >= 0xDC)
					{
						// invalid first surrogate.
						return false;
					}

					if (index >= bytes.length - 1)
					{
						// missing second surrogate.
						return false;
					}

					// second word.
					int c3 = 0xff & bytes[index++];
					int c4 = 0xff & bytes[index++];
					int msb2 = byteOrder == BYTE_ORDER_BIG_ENDIAN ? c3 : c4;
					if (msb2 < 0xDC)
					{
						// invalid second surrogate.
						return false;
					}
				}
			}
		}

		public int findEnd(byte bytes[], int index, boolean includeTerminator)
				throws UnicodeException
		{
			// http://en.wikipedia.org/wiki/UTF-16/UCS-2

			while (true)
			{
				if (index == bytes.length)
					return bytes.length;
				if (index > bytes.length - 1)
					throw new UnicodeException("Terminator not found.");

				int c1 = 0xff & bytes[index++];
				int c2 = 0xff & bytes[index++];
				int msb1 = byteOrder == BYTE_ORDER_BIG_ENDIAN ? c1 : c2;

				if (c1 == 0 && c2 == 0)
				{
					return includeTerminator ? index : index - 2;
				} else if (msb1 >= 0xD8)
				{
					if (index > bytes.length - 1)
						throw new UnicodeException("Terminator not found.");

					// second word.
					int c3 = 0xff & bytes[index++];
					int c4 = 0xff & bytes[index++];
					int msb2 = byteOrder == BYTE_ORDER_BIG_ENDIAN ? c3 : c4;
					if (msb2 < 0xDC)
						throw new UnicodeException("Invalid code point.");
				}
			}
		}
	}

	private static class UnicodeMetricsUTF16NoBOM extends UnicodeMetricsUTF16
	{

		public UnicodeMetricsUTF16NoBOM(final int byteOrder)
		{
			super(byteOrder);
		}

	}

	private static class UnicodeMetricsUTF16WithBOM extends UnicodeMetricsUTF16
	{

		public UnicodeMetricsUTF16WithBOM()
		{
			super(BYTE_ORDER_BIG_ENDIAN);
		}

		public int findEnd(byte bytes[], int index, boolean includeTerminator)
				throws UnicodeException
		{
			// http://en.wikipedia.org/wiki/UTF-16/UCS-2

			if (index >= bytes.length - 1)
				throw new UnicodeException("Missing BOM.");

			int c1 = 0xff & bytes[index++];
			int c2 = 0xff & bytes[index++];
			if (c1 == 0xFF && c2 == 0xFE)
				byteOrder = BYTE_ORDER_LITTLE_ENDIAN;
			else if (c1 == 0xFE && c2 == 0xFF)
				byteOrder = BYTE_ORDER_BIG_ENDIAN;
			else
				throw new UnicodeException("Invalid byte order mark.");

			return super.findEnd(bytes, index, includeTerminator);
		}
	}

}
