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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.RandomAccessFile;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.ImageWriteException;

public class BinaryFileFunctions implements BinaryConstants
{
	protected boolean debug = false;

	public final void setDebug(boolean b)
	{
		debug = b;
	}

	public final boolean getDebug()
	{
		return debug;
	}

	protected final void readRandomBytes(InputStream is)
			throws ImageReadException, IOException
	{

		for (int counter = 0; counter < 100; counter++)
		{
			readByte("" + counter, is, "Random Data");
		}
	}

	public final void debugNumber(String msg, int data)
	{
		debugNumber(msg, data, 1);
	}

	public final void debugNumber(String msg, int data, int bytes)
	{
		PrintWriter pw = new PrintWriter(System.out);
		debugNumber(pw, msg,
				data, bytes);
		pw.flush();
	}
	

	public final void debugNumber(PrintWriter pw, String msg, int data)
	{
		debugNumber(pw, msg, data, 1);
	}

	public final void debugNumber(PrintWriter pw, String msg, int data,
			int bytes)
	{
		pw.print(msg + ": " + data + " (");
		int byteData = data;
		for (int i = 0; i < bytes; i++)
		{
			if (i > 0)
				pw.print(",");
			int singleByte = 0xff & byteData;
			pw.print((char) singleByte + " [" + singleByte + "]");
			byteData >>= 8;
		}
		pw.println(") [0x" + Integer.toHexString(data) + ", "
				+ Integer.toBinaryString(data) + "]");
		pw.flush();
	}

	public final boolean startsWith(byte haystack[], byte needle[])
	{
		if (needle == null)
			return false;
		if (haystack == null)
			return false;
		if (needle.length > haystack.length)
			return false;

		for (int i = 0; i < needle.length; i++)
		{
			if (needle[i] != haystack[i])
				return false;
		}

		return true;
	}

	public final byte[] readBytes(InputStream is, int count)
			throws ImageReadException, IOException
	{
		byte result[] = new byte[count];
		for (int i = 0; i < count; i++)
		{
			int data = is.read();
			result[i] = (byte) data;
		}
		return result;
	}

	public final void readAndVerifyBytes(InputStream is, byte expected[],
			String exception) throws ImageReadException, IOException
	{
		for (int i = 0; i < expected.length; i++)
		{
			int data = is.read();
			byte b = (byte) (0xff & data);

			if (data < 0)
				throw new ImageReadException("Unexpected EOF.");

			if (b != expected[i])
			{
				// System.out.println("i" + ": " + i);

				// this.debugByteArray("expected", expected);
				// debugNumber("data[" + i + "]", b);
				// debugNumber("expected[" + i + "]", expected[i]);

				throw new ImageReadException(exception);
			}
		}
	}

	protected final void readAndVerifyBytes(String name, InputStream is,
			byte expected[], String exception) throws ImageReadException,
			IOException
	{
		byte bytes[] = readByteArray(name, expected.length, is, exception);

		for (int i = 0; i < expected.length; i++)
		{
			if (bytes[i] != expected[i])
			{
				// System.out.println("i" + ": " + i);
				// debugNumber("bytes[" + i + "]", bytes[i]);
				// debugNumber("expected[" + i + "]", expected[i]);

				throw new ImageReadException(exception);
			}
		}
	}

	public final void skipBytes(InputStream is, int length, String exception)
			throws IOException
	{
		long total = 0;
		while (length != total)
		{
			long skipped = is.skip(length - total);
			if (skipped < 1)
				throw new IOException(exception + " (" + skipped + ")");
			total += skipped;
		}
	}

	protected final void scanForByte(InputStream is, byte value)
			throws IOException
	{
		int count = 0;
		for (int i = 0; count < 3; i++)
		// while(count<3)
		{
			int b = is.read();
			if (b < 0)
				return;
			if ((0xff & b) == value)
			{
				System.out.println("\t" + i + ": match.");
				count++;
			}
		}
	}

	public final byte readByte(String name, InputStream is, String exception)
			throws ImageReadException, IOException
	{
		int result = is.read();

		if ((result < 0))
		{
			System.out.println(name + ": " + result);
			throw new IOException(exception);
		}

		if (debug)
			debugNumber(name, result);

		return (byte) (0xff & result);
	}

	protected final RationalNumber[] convertByteArrayToRationalArray(
			String name, byte bytes[], int start, int length, int byteOrder)
	{
		int expectedLength = start + length * 8;

		if (bytes.length < expectedLength)
		{
			System.out.println(name + ": expected length: " + expectedLength
					+ ", actual length: " + bytes.length);
			return null;
		}

		RationalNumber result[] = new RationalNumber[length];

		for (int i = 0; i < length; i++)
		{
			result[i] = convertByteArrayToRational(name, bytes, start + i * 8,
					byteOrder);
		}

		return result;
	}

	protected final RationalNumber convertByteArrayToRational(String name,
			byte bytes[], int byteOrder)
	{
		return convertByteArrayToRational(name, bytes, 0, byteOrder);
	}

	protected final RationalNumber convertByteArrayToRational(String name,
			byte bytes[], int start, int byteOrder)
	{
		int numerator = convertByteArrayToInt(name, bytes, start + 0, byteOrder);
		int divisor = convertByteArrayToInt(name, bytes, start + 4, byteOrder);

		return new RationalNumber(numerator, divisor);
	}

	protected final int convertByteArrayToInt(String name, byte bytes[],
			int byteOrder)
	{
		return convertByteArrayToInt(name, bytes, 0, byteOrder);
	}

	protected final int convertByteArrayToInt(String name, byte bytes[],
			int start, int byteOrder)
	{
		byte byte0 = bytes[start + 0];
		byte byte1 = bytes[start + 1];
		byte byte2 = bytes[start + 2];
		byte byte3 = bytes[start + 3];

		int result;

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			result = ((0xff & byte0) << 24) | ((0xff & byte1) << 16)
					| ((0xff & byte2) << 8) | ((0xff & byte3) << 0);
		} else
		{
			// intel, little endian
			result = ((0xff & byte3) << 24) | ((0xff & byte2) << 16)
					| ((0xff & byte1) << 8) | ((0xff & byte0) << 0);
		}

		if (debug)
			debugNumber(name, result, 4);

		return result;
	}

	protected final int[] convertByteArrayToIntArray(String name, byte bytes[],
			int start, int length, int byteOrder)
	{
		int expectedLength = start + length * 4;

		if (bytes.length < expectedLength)
		{
			System.out.println(name + ": expected length: " + expectedLength
					+ ", actual length: " + bytes.length);
			return null;
		}

		int result[] = new int[length];

		for (int i = 0; i < length; i++)
		{
			result[i] = convertByteArrayToInt(name, bytes, start + i * 4,
					byteOrder);
		}

		return result;
	}

	protected final void writeIntInToByteArray(int value, byte bytes[],
			int start, int byteOrder)
	{
		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			bytes[start + 0] = (byte) (value >> 24);
			bytes[start + 1] = (byte) (value >> 16);
			bytes[start + 2] = (byte) (value >> 8);
			bytes[start + 3] = (byte) (value >> 0);
		} else
		{
			bytes[start + 3] = (byte) (value >> 24);
			bytes[start + 2] = (byte) (value >> 16);
			bytes[start + 1] = (byte) (value >> 8);
			bytes[start + 0] = (byte) (value >> 0);
		}
	}

	protected static final byte[] int2ToByteArray(int value, int byteOrder)
	{
		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
			return new byte[] { (byte) (value >> 8), (byte) (value >> 0), };
		else
			return new byte[] { (byte) (value >> 0), (byte) (value >> 8), };
	}

	protected final byte[] convertIntArrayToByteArray(int values[],
			int byteOrder)
	{
		byte result[] = new byte[values.length * 4];

		for (int i = 0; i < values.length; i++)
		{
			writeIntInToByteArray(values[i], result, i * 4, byteOrder);
		}

		return result;
	}

	protected final byte[] convertShortArrayToByteArray(int values[],
			int byteOrder)
	{
		byte result[] = new byte[values.length * 2];

		for (int i = 0; i < values.length; i++)
		{
			int value = values[i];

			if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
			{
				result[i * 2 + 0] = (byte) (value >> 8);
				result[i * 2 + 1] = (byte) (value >> 0);
			} else
			{
				result[i * 2 + 1] = (byte) (value >> 8);
				result[i * 2 + 0] = (byte) (value >> 0);
			}
		}

		return result;
	}

	protected final byte[] convertShortToByteArray(int value, int byteOrder)
	{
		byte result[] = new byte[2];

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			result[0] = (byte) (value >> 8);
			result[1] = (byte) (value >> 0);
		} else
		{
			result[1] = (byte) (value >> 8);
			result[0] = (byte) (value >> 0);
		}

		return result;
	}

	protected final byte[] convertIntArrayToRationalArray(int numerators[],
			int denominators[], int byteOrder) throws ImageWriteException
	{
		if (numerators.length != denominators.length)
			throw new ImageWriteException("numerators.length ("
					+ numerators.length + " != denominators.length ("
					+ denominators.length + ")");

		byte result[] = new byte[numerators.length * 8];

		for (int i = 0; i < numerators.length; i++)
		{
			writeIntInToByteArray(numerators[i], result, i * 8, byteOrder);
			writeIntInToByteArray(denominators[i], result, i * 8 + 4, byteOrder);
		}

		return result;
	}

	protected final byte[] convertRationalArrayToByteArray(
			RationalNumber numbers[], int byteOrder) throws ImageWriteException
	{
		// Debug.debug("convertRationalArrayToByteArray 2");
		byte result[] = new byte[numbers.length * 8];

		for (int i = 0; i < numbers.length; i++)
		{
			writeIntInToByteArray(numbers[i].numerator, result, i * 8,
					byteOrder);
			writeIntInToByteArray(numbers[i].divisor, result, i * 8 + 4,
					byteOrder);
		}

		return result;
	}

	protected final byte[] convertRationalToByteArray(RationalNumber number,
			int byteOrder) throws ImageWriteException
	{
		byte result[] = new byte[8];

		writeIntInToByteArray(number.numerator, result, 0, byteOrder);
		writeIntInToByteArray(number.divisor, result, 4, byteOrder);

		return result;
	}

	protected final int convertByteArrayToShort(String name, byte bytes[],
			int byteOrder) throws ImageReadException
	{
		return convertByteArrayToShort(name, 0, bytes, byteOrder);
	}

	protected final int convertByteArrayToShort(String name, int index,
			byte bytes[], int byteOrder) throws ImageReadException
	{
		if (index + 1 >= bytes.length)
			throw new ImageReadException("Index out of bounds. Array size: "
					+ bytes.length + ", index: " + index);

		int byte0 = 0xff & bytes[index + 0];
		int byte1 = 0xff & bytes[index + 1];

		int result;

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
			result = (byte0 << 8) | byte1;
		else
			// intel, little endian
			result = (byte1 << 8) | byte0;

		if (debug)
			debugNumber(name, result, 2);

		return result;
	}

	protected final int[] convertByteArrayToShortArray(String name,
			byte bytes[], int start, int length, int byteOrder)
			throws ImageReadException

	{
		int expectedLength = start + length * 2;

		if (bytes.length < expectedLength)
		{
			System.out.println(name + ": expected length: " + expectedLength
					+ ", actual length: " + bytes.length);
			return null;
		}

		int result[] = new int[length];

		for (int i = 0; i < length; i++)
		{
			result[i] = convertByteArrayToShort(name, start + i * 2, bytes,
					byteOrder);
		}

		return result;
	}

	public final byte[] readByteArray(String name, int length, InputStream is)
			throws IOException
	{
		String exception = name + " could not be read.";
		return readByteArray(name, length, is, exception);
	}

	public final byte[] readByteArray(String name, int length, InputStream is,
			String exception) throws IOException
	{
		byte result[] = new byte[length];

		int read = 0;
		while (read < length)
		{
			int count = is.read(result, read, length - read);
			// Debug.debug("count", count);
			if (count < 1)
				throw new IOException(exception);

			read += count;
		}

		if (debug)
		{
			for (int i = 0; ((i < length) && (i < 50)); i++)
			{
				debugNumber(name + " (" + i + ")", 0xff & result[i]);
			}
		}
		return result;
	}

	public final void debugByteArray(String name, byte bytes[])
	{
		System.out.println(name + ": " + bytes.length);

		for (int i = 0; ((i < bytes.length) && (i < 50)); i++)
		{
			debugNumber("\t" + " (" + i + ")", 0xff & bytes[i]);
		}
	}

	protected final void debugNumberArray(String name, int numbers[], int length)
	{
		System.out.println(name + ": " + numbers.length);

		for (int i = 0; ((i < numbers.length) && (i < 50)); i++)
		{
			debugNumber(name + " (" + i + ")", numbers[i], length);
		}
	}

	public final byte[] readBytearray(String name, byte bytes[], int start,
			int count) throws ImageReadException
	{
		if (bytes.length < (start + count))
		{
			throw new ImageReadException("Invalid read. bytes.length: " + bytes.length+ ", start: " + start + ", count: " + count);
			// return null;
		}
		
		byte result[] = new byte[count];
		System.arraycopy(bytes, start, result, 0, count);

		if (debug)
			debugByteArray(name, result);

		return result;
	}

	protected final byte[] getByteArrayTail(String name, byte bytes[], int count) throws ImageReadException
	{
		return readBytearray(name, bytes, count, bytes.length - count);
	}

	protected final byte[] getBytearrayHead(String name, byte bytes[], int count) throws ImageReadException
	{
		return readBytearray(name, bytes, 0, bytes.length - count);
	}

	public static final byte[] slice(byte bytes[], int start, int count)
	{
		if (bytes.length < (start + count))
			return null;

		byte result[] = new byte[count];
		System.arraycopy(bytes, start, result, 0, count);

		return result;
	}

	public static final byte[] tail(byte bytes[], int count)
	{
		if (count > bytes.length)
			count = bytes.length;
		return slice(bytes, bytes.length - count, count);
	}

	public static final byte[] head(byte bytes[], int count)
	{
		if (count > bytes.length)
			count = bytes.length;
		return slice(bytes, 0, count);
	}

	public final boolean compareByteArrays(byte a[], byte b[])
	{
		if (a.length != b.length)
			return false;

		return compareByteArrays(a, 0, b, 0, a.length);
	}

	public final boolean compareByteArrays(byte a[], int aStart, byte b[],
			int bStart, int length)
	{
		if (a.length < (aStart + length))
		{
			return false;
		}
		if (b.length < (bStart + length))
			return false;

		for (int i = 0; i < length; i++)
		{
			if (a[aStart + i] != b[bStart + i])
			{
				// debugNumber("\t" + "a[" + (aStart + i) + "]", a[aStart + i]);
				// debugNumber("\t" + "b[" + (bStart + i) + "]", b[bStart + i]);

				return false;
			}
		}

		return true;
	}

	public static final boolean compareBytes(byte a[], byte b[])
	{
		if (a.length != b.length)
			return false;

		return compareBytes(a, 0, b, 0, a.length);
	}

	public static final boolean compareBytes(byte a[], int aStart, byte b[],
			int bStart, int length)
	{
		if (a.length < (aStart + length))
			return false;
		if (b.length < (bStart + length))
			return false;

		for (int i = 0; i < length; i++)
		{
			if (a[aStart + i] != b[bStart + i])
				return false;
		}

		return true;
	}

	protected final int read4Bytes(String name, InputStream is,
			String exception, int byteOrder) throws ImageReadException,
			IOException
	{
		int size = 4;
		byte bytes[] = new byte[size];

		int read = 0;
		while (read < size)
		{
			int count = is.read(bytes, read, size - read);
			if (count < 1)
				throw new IOException(exception);

			read += count;
		}

		return convertByteArrayToInt(name, bytes, byteOrder);
	}

	protected final int read3Bytes(String name, InputStream is,
			String exception, int byteOrder) throws ImageReadException,
			IOException
	{
		byte byte0 = (byte) is.read();
		byte byte1 = (byte) is.read();
		byte byte2 = (byte) is.read();

		int result;

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
			result = ((0xff & byte0) << 16) | ((0xff & byte1) << 8)
					| ((0xff & byte2) << 0);
		else
			// intel, little endian
			result = ((0xff & byte2) << 16) | ((0xff & byte1) << 8)
					| ((0xff & byte0) << 0);

		if (debug)
			debugNumber(name, result, 3);

		return result;
		//		
		//		
		// int size = 3;
		// byte bytes[] = new byte[size];
		//
		// int read = 0;
		// while (read < size)
		// {
		// int count = is.read(bytes, read, size - read);
		// if (count < 1)
		// throw new IOException(exception);
		//
		// read += count;
		// }
		//
		// return convertByteArrayToInt(name, bytes, 0, 3, byteOrder);
	}

	protected final int read2Bytes(String name, InputStream is,
			String exception, int byteOrder) throws ImageReadException,
			IOException
	{
		int size = 2;
		byte bytes[] = new byte[size];

		int read = 0;
		while (read < size)
		{
			int count = is.read(bytes, read, size - read);
			if (count < 1)
				throw new IOException(exception);

			read += count;
		}

		return convertByteArrayToShort(name, bytes, byteOrder);
	}

	protected final void printCharQuad(String msg, int i)
	{
		System.out.println(msg + ": '" + (char) (0xff & (i >> 24))
				+ (char) (0xff & (i >> 16)) + (char) (0xff & (i >> 8))
				+ (char) (0xff & (i >> 0)) + "'");

	}

	protected final void printCharQuad(PrintWriter pw, String msg, int i)
	{
		pw.println(msg + ": '" + (char) (0xff & (i >> 24))
				+ (char) (0xff & (i >> 16)) + (char) (0xff & (i >> 8))
				+ (char) (0xff & (i >> 0)) + "'");

	}

	protected final void printByteBits(String msg, byte i)
	{
		System.out.println(msg + ": '" + Integer.toBinaryString(0xff & i));
	}

	public final static int CharsToQuad(char c1, char c2, char c3, char c4)
	{
		return (((0xff & c1) << 24) | ((0xff & c2) << 16) | ((0xff & c3) << 8) | ((0xff & c4) << 0));
	}

	public final int findNull(byte src[])
	{
		return findNull(src, 0);
	}

	public final int findNull(byte src[], int start)
	{
		for (int i = start; i < src.length; i++)
		{
			if (src[i] == 0)
				return i;

		}
		return -1;
	}

	protected final byte[] getRAFBytes(RandomAccessFile raf, long pos,
			int length, String exception) throws IOException
	{
		if (debug)
		{
			System.out.println("getRAFBytes pos" + ": " + pos);
			System.out.println("getRAFBytes length" + ": " + length);
		}

		byte result[] = new byte[length];

		raf.seek(pos);

		int read = 0;
		while (read < length)
		{
			int count = raf.read(result, read, length - read);
			if (count < 1)
				throw new IOException(exception);

			read += count;
		}

		return result;

	}

	protected final float convertByteArrayToFloat(String name, byte bytes[],
			int byteOrder)
	{
		return convertByteArrayToFloat(name, bytes, 0, byteOrder);
	}

	protected final float convertByteArrayToFloat(String name, byte bytes[],
			int start, int byteOrder)
	{
		// TODO: not tested; probably wrong.

		byte byte0 = bytes[start + 0];
		byte byte1 = bytes[start + 1];
		byte byte2 = bytes[start + 2];
		byte byte3 = bytes[start + 3];

		int bits;

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			bits = ((0xff & byte0) << 24) | ((0xff & byte1) << 16)
					| ((0xff & byte2) << 8) | ((0xff & byte3) << 0);
		} else
		{
			// intel, little endian
			bits = ((0xff & byte3) << 24) | ((0xff & byte2) << 16)
					| ((0xff & byte1) << 8) | ((0xff & byte0) << 0);
		}

		float result = Float.intBitsToFloat(bits);

		// if (debug)
		// debugNumber(name, result, 4);

		return result;
	}

	protected final float[] convertByteArrayToFloatArray(String name,
			byte bytes[], int start, int length, int byteOrder)
	{
		int expectedLength = start + length * 4;

		if (bytes.length < expectedLength)
		{
			System.out.println(name + ": expected length: " + expectedLength
					+ ", actual length: " + bytes.length);
			return null;
		}

		float result[] = new float[length];

		for (int i = 0; i < length; i++)
		{
			result[i] = convertByteArrayToFloat(name, bytes, start + i * 4,
					byteOrder);
		}

		return result;
	}

	protected final byte[] convertFloatToByteArray(float value, int byteOrder)
	{
		// TODO: not tested; probably wrong.
		byte result[] = new byte[4];

		int bits = Float.floatToRawIntBits(value);

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			result[0] = (byte) (0xff & (bits >> 0));
			result[1] = (byte) (0xff & (bits >> 8));
			result[2] = (byte) (0xff & (bits >> 16));
			result[3] = (byte) (0xff & (bits >> 24));
		} else
		{
			result[3] = (byte) (0xff & (bits >> 0));
			result[2] = (byte) (0xff & (bits >> 8));
			result[1] = (byte) (0xff & (bits >> 16));
			result[0] = (byte) (0xff & (bits >> 24));
		}

		return result;
	}

	protected final byte[] convertFloatArrayToByteArray(float values[],
			int byteOrder)
	{
		// TODO: not tested; probably wrong.
		byte result[] = new byte[values.length * 4];
		for (int i = 0; i < values.length; i++)
		{
			float value = values[i];
			int bits = Float.floatToRawIntBits(value);

			int start = i * 4;
			if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
			{
				result[start + 0] = (byte) (0xff & (bits >> 0));
				result[start + 1] = (byte) (0xff & (bits >> 8));
				result[start + 2] = (byte) (0xff & (bits >> 16));
				result[start + 3] = (byte) (0xff & (bits >> 24));
			} else
			{
				result[start + 3] = (byte) (0xff & (bits >> 0));
				result[start + 2] = (byte) (0xff & (bits >> 8));
				result[start + 1] = (byte) (0xff & (bits >> 16));
				result[start + 0] = (byte) (0xff & (bits >> 24));
			}
		}
		return result;
	}

	protected final byte[] convertDoubleToByteArray(double value, int byteOrder)
	{
		// TODO: not tested; probably wrong.
		byte result[] = new byte[8];

		long bits = Double.doubleToRawLongBits(value);

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			result[0] = (byte) (0xff & (bits >> 0));
			result[1] = (byte) (0xff & (bits >> 8));
			result[2] = (byte) (0xff & (bits >> 16));
			result[3] = (byte) (0xff & (bits >> 24));
			result[4] = (byte) (0xff & (bits >> 32));
			result[5] = (byte) (0xff & (bits >> 40));
			result[6] = (byte) (0xff & (bits >> 48));
			result[7] = (byte) (0xff & (bits >> 56));
		} else
		{
			result[7] = (byte) (0xff & (bits >> 0));
			result[6] = (byte) (0xff & (bits >> 8));
			result[5] = (byte) (0xff & (bits >> 16));
			result[4] = (byte) (0xff & (bits >> 24));
			result[3] = (byte) (0xff & (bits >> 32));
			result[2] = (byte) (0xff & (bits >> 40));
			result[1] = (byte) (0xff & (bits >> 48));
			result[0] = (byte) (0xff & (bits >> 56));
		}

		return result;
	}

	protected final byte[] convertDoubleArrayToByteArray(double values[],
			int byteOrder)
	{
		// TODO: not tested; probably wrong.
		byte result[] = new byte[values.length * 8];
		for (int i = 0; i < values.length; i++)
		{
			double value = values[i];
			long bits = Double.doubleToRawLongBits(value);

			int start = i * 8;
			if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
			{
				result[start + 0] = (byte) (0xff & (bits >> 0));
				result[start + 1] = (byte) (0xff & (bits >> 8));
				result[start + 2] = (byte) (0xff & (bits >> 16));
				result[start + 3] = (byte) (0xff & (bits >> 24));
				result[start + 4] = (byte) (0xff & (bits >> 32));
				result[start + 5] = (byte) (0xff & (bits >> 40));
				result[start + 6] = (byte) (0xff & (bits >> 48));
				result[start + 7] = (byte) (0xff & (bits >> 56));
			} else
			{
				result[start + 7] = (byte) (0xff & (bits >> 0));
				result[start + 6] = (byte) (0xff & (bits >> 8));
				result[start + 5] = (byte) (0xff & (bits >> 16));
				result[start + 4] = (byte) (0xff & (bits >> 24));
				result[start + 3] = (byte) (0xff & (bits >> 32));
				result[start + 2] = (byte) (0xff & (bits >> 40));
				result[start + 1] = (byte) (0xff & (bits >> 48));
				result[start + 0] = (byte) (0xff & (bits >> 56));
			}
		}
		return result;
	}

	protected final double convertByteArrayToDouble(String name, byte bytes[],
			int byteOrder)
	{
		return convertByteArrayToDouble(name, bytes, 0, byteOrder);
	}

	protected final double convertByteArrayToDouble(String name, byte bytes[],
			int start, int byteOrder)
	{
		// TODO: not tested; probably wrong.

		byte byte0 = bytes[start + 0];
		byte byte1 = bytes[start + 1];
		byte byte2 = bytes[start + 2];
		byte byte3 = bytes[start + 3];
		byte byte4 = bytes[start + 4];
		byte byte5 = bytes[start + 5];
		byte byte6 = bytes[start + 6];
		byte byte7 = bytes[start + 7];

		long bits;

		if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		{
			bits = ((0xff & byte0) << 56) | ((0xff & byte1) << 48)
					| ((0xff & byte2) << 40) | ((0xff & byte3) << 32)
					| ((0xff & byte4) << 24) | ((0xff & byte5) << 16)
					| ((0xff & byte6) << 8) | ((0xff & byte7) << 0);

		} else
		{
			// intel, little endian
			bits = ((0xff & byte7) << 56) | ((0xff & byte6) << 48)
					| ((0xff & byte5) << 40) | ((0xff & byte4) << 32)
					| ((0xff & byte3) << 24) | ((0xff & byte2) << 16)
					| ((0xff & byte1) << 8) | ((0xff & byte0) << 0);
		}

		double result = Double.longBitsToDouble(bits);

		// if (debug)
		// debugNumber(name, result, 4);

		return result;

		// byte array[];
		// if (byteOrder == BYTE_ORDER_MOTOROLA) // motorola, big endian
		// // ?? dunno byte order very likely wrong here.
		// array = new byte[]{
		// bytes[start + 0], bytes[start + 1], bytes[start + 2],
		// bytes[start + 3], bytes[start + 4], bytes[start + 5],
		// bytes[start + 6], bytes[start + 7],
		//
		// };
		// else
		// // ?? dunno byte order very likely wrong here.
		// array = new byte[]{
		// bytes[start + 3], bytes[start + 2], bytes[start + 1],
		// bytes[start + 0], bytes[start + 7], bytes[start + 6],
		// bytes[start + 5], bytes[start + 4],
		// };
		//
		// double result = Double.NaN;
		//
		// try
		// {
		// ByteArrayInputStream bais = new ByteArrayInputStream(array);
		// if (start > 0)
		// {
		// skipBytes(bais, start);
		// // bais.skip(start);
		// }
		// DataInputStream dis = new DataInputStream(bais);
		// result = dis.readDouble();
		//
		// dis.close();
		// }
		// catch (Exception e)
		// {
		// Debug.debug(e);
		// }
		//
		// return result;
	}

	protected final double[] convertByteArrayToDoubleArray(String name,
			byte bytes[], int start, int length, int byteOrder)
	{
		int expectedLength = start + length * 8;

		if (bytes.length < expectedLength)
		{
			System.out.println(name + ": expected length: " + expectedLength
					+ ", actual length: " + bytes.length);
			return null;
		}

		double result[] = new double[length];

		for (int i = 0; i < length; i++)
		{
			result[i] = convertByteArrayToDouble(name, bytes, start + i * 8,
					byteOrder);
		}

		return result;
	}

	protected void skipBytes(InputStream is, int length) throws IOException
	{
		this.skipBytes(is, length, "Couldn't skip bytes");
	}

	public final void copyStreamToStream(InputStream is, OutputStream os)
			throws IOException
	{
		byte buffer[] = new byte[1024];
		int read;
		while ((read = is.read(buffer)) > 0)
		{
			os.write(buffer, 0, read);
		}
	}

	public final byte[] getStreamBytes(InputStream is) throws IOException
	{
		ByteArrayOutputStream os = new ByteArrayOutputStream();
		copyStreamToStream(is, os);
		return os.toByteArray();
	}
}