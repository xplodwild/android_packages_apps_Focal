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

package org.apache.sanselan.formats.tiff.constants;

import java.io.UnsupportedEncodingException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryFileFunctions;
import org.apache.sanselan.formats.tiff.TiffField;
import org.apache.sanselan.formats.tiff.fieldtypes.FieldType;
import org.apache.sanselan.util.Debug;

public class TagInfo implements TiffDirectoryConstants, TiffFieldTypeConstants
{
	protected static final int LENGTH_UNKNOWN = -1;

	public TagInfo(String name, int tag, FieldType dataType, int length,
			ExifDirectoryType exifDirectory)
	{
		this(name, tag, new FieldType[]{
			dataType
		}, length, exifDirectory);
	}

	public TagInfo(String name, int tag, FieldType dataType, int length)
	{
		this(name, tag, new FieldType[]{
			dataType
		}, length, EXIF_DIRECTORY_UNKNOWN);
	}

	public TagInfo(String name, int tag, FieldType dataType,
			String lengthDescription)
	{
		this(name, tag, new FieldType[]{
			dataType
		}, LENGTH_UNKNOWN, EXIF_DIRECTORY_UNKNOWN);
	}

	public TagInfo(String name, int tag, FieldType dataTypes[],
			String lengthDescription)
	{
		this(name, tag, dataTypes, LENGTH_UNKNOWN, EXIF_DIRECTORY_UNKNOWN);
	}

	public TagInfo(String name, int tag, FieldType dataType)
	{
		this(name, tag, dataType, LENGTH_UNKNOWN, EXIF_DIRECTORY_UNKNOWN);
	}

	public TagInfo(String name, int tag, FieldType dataTypes[], int length,
			String lengthDescription)
	{
		this(name, tag, dataTypes, length, EXIF_DIRECTORY_UNKNOWN);
	}

	public final String name;
	public final int tag;
	public final FieldType dataTypes[];
	public final int length;
	public final ExifDirectoryType directoryType;

	//	public final String lengthDescription;

	public TagInfo(String name, int tag, FieldType dataTypes[], int length,
			ExifDirectoryType exifDirectory
	//			, String lengthDescription
	)
	{
		this.name = name;
		this.tag = tag;
		this.dataTypes = dataTypes;
		this.length = length;
		//		this.lengthDescription = lengthDescription;
		this.directoryType = exifDirectory;
	}

	public Object getValue(TiffField entry) throws ImageReadException
	{
		Object o = entry.fieldType.getSimpleValue(entry);
		return o;
	}

	public byte[] encodeValue(FieldType fieldType, Object value, int byteOrder)
			throws ImageWriteException
	{
		return fieldType.writeData(value, byteOrder);
	}

	public String getDescription()
	{
		return tag + " (0x" + Integer.toHexString(tag) + ": " + name + "): ";
	}

	public String toString()
	{
		return "[TagInfo. tag: " + tag + " (0x" + Integer.toHexString(tag)
				+ ", name: " + name + "]";
	}

	public boolean isDate()
	{
		return false;
	}

	public boolean isOffset()
	{
		return false;
	}

	public boolean isText()
	{
		return false;
	}

	public boolean isUnknown()
	{
		return false;
	}

	public static class Offset extends TagInfo
	{
		public Offset(String name, int tag, FieldType dataTypes[], int length,
				ExifDirectoryType exifDirectory)
		{
			super(name, tag, dataTypes, length, exifDirectory);
		}

		public Offset(String name, int tag, FieldType dataType, int length,
				ExifDirectoryType exifDirectory)
		{
			super(name, tag, dataType, length, exifDirectory);
		}

		public Offset(String name, int tag, FieldType dataType, int length)
		{
			super(name, tag, dataType, length);
		}

		//		"Exif Offset", 0x8769, FIELD_TYPE_DESCRIPTION_UNKNOWN, 1,
		//		EXIF_DIRECTORY_UNKNOWN);
		public boolean isOffset()
		{
			return true;
		}
	}

	public static class Date extends TagInfo
	{
		public Date(String name, int tag, FieldType dataType, int length)
		{
			super(name, tag, dataType, length);
		}

		private static final DateFormat DATE_FORMAT_1 = new SimpleDateFormat(
				"yyyy:MM:dd HH:mm:ss");
		private static final DateFormat DATE_FORMAT_2 = new SimpleDateFormat(
				"yyyy:MM:dd:HH:mm:ss");

		public Object getValue(TiffField entry) throws ImageReadException
		{
			Object o = entry.fieldType.getSimpleValue(entry);

			String s = (String) o;
			try
			{
				java.util.Date date = DATE_FORMAT_1.parse(s);
				return date;
			}
			catch (Exception e)
			{
				//		Debug.debug(e);
			}
			try
			{
				java.util.Date date = DATE_FORMAT_2.parse(s);
				return date;
			}
			catch (Exception e)
			{
				Debug.debug(e);
			}

			return o;
		}

		public byte[] encodeValue(FieldType fieldType, Object value,
				int byteOrder) throws ImageWriteException
		{
			throw new ImageWriteException("date encode value: " + value + " ("
					+ Debug.getType(value) + ")");
			//			return fieldType.writeData(value, byteOrder);
			//			Object o = entry.fieldType.getSimpleValue(entry);
			//			byte bytes2[];
			//			if (tagInfo.isDate())
			//				bytes2 = fieldType.getRawBytes(srcField);
			//			else
			//				bytes2 = fieldType.writeData(value, byteOrder);
			//			return o;
		}

		public String toString()
		{
			return "[TagInfo. tag: " + tag + ", name: " + name + " (data)"
					+ "]";
		}

		// TODO: use polymorphism
		public boolean isDate()
		{
			return true;
		}

	}

	public static final class Text extends TagInfo
	{
		public Text(String name, int tag, FieldType dataType, int length,
				ExifDirectoryType exifDirectory)
		{
			super(name, tag, dataType, length, exifDirectory);
		}

		public Text(String name, int tag, FieldType dataTypes[], int length,
				ExifDirectoryType exifDirectory)
		{
			super(name, tag, dataTypes, length, exifDirectory);
		}

		public boolean isText()
		{
			return true;
		}

		private static final class TextEncoding
		{
			public final byte prefix[];
			public final String encodingName;

			public TextEncoding(final byte[] prefix, final String encodingName)
			{
				this.prefix = prefix;
				this.encodingName = encodingName;
			}
		}

		private static final TextEncoding TEXT_ENCODING_ASCII = new TextEncoding(
				new byte[]{
						0x41, 0x53, 0x43, 0x49, 0x49, 0x00, 0x00, 0x00,
				}, "US-ASCII"); // ITU-T T.50 IA5
		private static final TextEncoding TEXT_ENCODING_JIS = new TextEncoding(
				new byte[]{
						0x4A, 0x49, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00,
				}, "JIS"); // JIS X208-1990
		private static final TextEncoding TEXT_ENCODING_UNICODE = new TextEncoding(
				new byte[]{
						0x55, 0x4E, 0x49, 0x43, 0x4F, 0x44, 0x45, 0x00,
				// Which Unicode encoding to use, UTF-8?  
				}, "UTF-8"); // Unicode Standard
		private static final TextEncoding TEXT_ENCODING_UNDEFINED = new TextEncoding(
				new byte[]{
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				// Try to interpret an undefined text as ISO-8859-1 (Latin)
				}, "ISO-8859-1"); // Undefined
		private static final TextEncoding TEXT_ENCODINGS[] = {
				TEXT_ENCODING_ASCII, //
				TEXT_ENCODING_JIS, //
				TEXT_ENCODING_UNICODE, //
				TEXT_ENCODING_UNDEFINED, //
		};

		public byte[] encodeValue(FieldType fieldType, Object value,
				int byteOrder) throws ImageWriteException
		{
			if (!(value instanceof String))
				throw new ImageWriteException("Text value not String: " + value
						+ " (" + Debug.getType(value) + ")");
			String s = (String) value;

			try
			{
				// try ASCII, with NO prefix.
				byte asciiBytes[] = s
						.getBytes(TEXT_ENCODING_ASCII.encodingName);
				String decodedAscii = new String(asciiBytes,
						TEXT_ENCODING_ASCII.encodingName);
				if (decodedAscii.equals(s))
				{
					// no unicode/non-ascii values.
					byte result[] = new byte[asciiBytes.length
							+ TEXT_ENCODING_ASCII.prefix.length];
					System.arraycopy(TEXT_ENCODING_ASCII.prefix, 0, result, 0,
							TEXT_ENCODING_ASCII.prefix.length);
					System.arraycopy(asciiBytes, 0, result,
							TEXT_ENCODING_ASCII.prefix.length,
							asciiBytes.length);
					return result;
				}
				else
				{
					// use unicode
					byte unicodeBytes[] = s
							.getBytes(TEXT_ENCODING_UNICODE.encodingName);
					byte result[] = new byte[unicodeBytes.length
							+ TEXT_ENCODING_UNICODE.prefix.length];
					System.arraycopy(TEXT_ENCODING_UNICODE.prefix, 0, result,
							0, TEXT_ENCODING_UNICODE.prefix.length);
					System.arraycopy(unicodeBytes, 0, result,
							TEXT_ENCODING_UNICODE.prefix.length,
							unicodeBytes.length);
					return result;
				}
			}
			catch (UnsupportedEncodingException e)
			{
				throw new ImageWriteException(e.getMessage(), e);
			}
		}

		public Object getValue(TiffField entry) throws ImageReadException
		{
			//			Debug.debug("entry.type", entry.type);
			//			Debug.debug("entry.type", entry.getDescriptionWithoutValue());
			//			Debug.debug("entry.type", entry.fieldType);

			if (entry.type == FIELD_TYPE_ASCII.type)
				return FIELD_TYPE_ASCII.getSimpleValue(entry);
			else if (entry.type == FIELD_TYPE_UNDEFINED.type)
				;
			else if (entry.type == FIELD_TYPE_BYTE.type)
				;
			else
			{
				Debug.debug("entry.type", entry.type);
				Debug.debug("entry.directoryType", entry.directoryType);
				Debug.debug("entry.type", entry.getDescriptionWithoutValue());
				Debug.debug("entry.type", entry.fieldType);
				throw new ImageReadException("Text field not encoded as bytes.");
			}

			byte bytes[] = entry.fieldType.getRawBytes(entry);
			if (bytes.length < 8)
			{
				try
				{
					// try ASCII, with NO prefix.
					return new String(bytes, "US-ASCII");
				}
				catch (UnsupportedEncodingException e)
				{
					throw new ImageReadException(
							"Text field missing encoding prefix.");
				}
			}

			for (int i = 0; i < TEXT_ENCODINGS.length; i++)
			{
				TextEncoding encoding = TEXT_ENCODINGS[i];
				if (BinaryFileFunctions.compareBytes(bytes, 0, encoding.prefix,
						0, encoding.prefix.length))
				{
					try
					{
						//						Debug.debug("encodingName", encoding.encodingName);
						return new String(bytes, encoding.prefix.length,
								bytes.length - encoding.prefix.length,
								encoding.encodingName);
					}
					catch (UnsupportedEncodingException e)
					{
						throw new ImageReadException(e.getMessage(), e);
					}
				}
			}

			//						Debug.debug("entry.tag", entry.tag + " (0x" + Integer.toHexString(entry.tag ) +")");
			//						Debug.debug("entry.type", entry.type);
			//						Debug.debug("bytes", bytes, 10);
			//			throw new ImageReadException(
			//					"Unknown Text encoding prefix.");

			try
			{
				// try ASCII, with NO prefix.
				return new String(bytes, "US-ASCII");
			}
			catch (UnsupportedEncodingException e)
			{
				throw new ImageReadException("Unknown text encoding prefix.");
			}

		}
	}

	public static final class Unknown extends TagInfo
	{

		public Unknown(String name, int tag, FieldType dataTypes[], int length,
				ExifDirectoryType exifDirectory)
		{
			super(name, tag, dataTypes, length, exifDirectory);
		}

		public boolean isUnknown()
		{
			return true;
		}

		public byte[] encodeValue(FieldType fieldType, Object value,
				int byteOrder) throws ImageWriteException
		{
			//			Debug.debug();
			//			Debug.debug("unknown tag(0x" + Integer.toHexString(tag) + ") ",
			//					this);
			//			Debug.debug("unknown tag fieldType", fieldType);
			//			Debug.debug("unknown tag value", value);
			//			Debug.debug("unknown tag value", Debug.getType(value));
			byte result[] = super.encodeValue(fieldType, value, byteOrder);
			//			Debug.debug("unknown tag result", result);
			return result;
		}

		public Object getValue(TiffField entry) throws ImageReadException
		{
			return super.getValue(entry);
		}
	}

}