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
package org.apache.sanselan.formats.tiff;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.sanselan.FormatCompliance;
import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.common.BinaryFileParser;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.formats.tiff.TiffDirectory.ImageDataElement;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;
import org.apache.sanselan.util.Debug;

public class TiffReader extends BinaryFileParser implements TiffConstants
{

	private final boolean strict;

	public TiffReader(boolean strict)
	{
		this.strict = strict;
	}

	private TiffHeader readTiffHeader(ByteSource byteSource,
			FormatCompliance formatCompliance) throws ImageReadException,
			IOException
	{
		InputStream is = null;
		try
		{
			is = byteSource.getInputStream();
			return readTiffHeader(is, formatCompliance);
		} finally
		{
			try
			{
				if (is != null)
					is.close();
			} catch (Exception e)
			{
				Debug.debug(e);
			}
		}
	}

	private TiffHeader readTiffHeader(InputStream is,
			FormatCompliance formatCompliance) throws ImageReadException,
			IOException
	{
		int BYTE_ORDER_1 = readByte("BYTE_ORDER_1", is, "Not a Valid TIFF File");
		int BYTE_ORDER_2 = readByte("BYTE_ORDER_2", is, "Not a Valid TIFF File");
		setByteOrder(BYTE_ORDER_1, BYTE_ORDER_2);

		int tiffVersion = read2Bytes("tiffVersion", is, "Not a Valid TIFF File");
		if (tiffVersion != 42)
			throw new ImageReadException("Unknown Tiff Version: " + tiffVersion);

		int offsetToFirstIFD = read4Bytes("offsetToFirstIFD", is,
				"Not a Valid TIFF File");

		skipBytes(is, offsetToFirstIFD - 8,
				"Not a Valid TIFF File: couldn't find IFDs");

		if (debug)
			System.out.println("");

		return new TiffHeader(BYTE_ORDER_1, tiffVersion, offsetToFirstIFD);
	}

	private void readDirectories(ByteSource byteSource,
			FormatCompliance formatCompliance, Listener listener)
			throws ImageReadException, IOException
	{
		TiffHeader tiffHeader = readTiffHeader(byteSource, formatCompliance);
		if (!listener.setTiffHeader(tiffHeader))
			return;

		int offset = tiffHeader.offsetToFirstIFD;
		int dirType = TiffDirectory.DIRECTORY_TYPE_ROOT;

		List visited = new ArrayList();
		readDirectory(byteSource, offset, dirType, formatCompliance, listener,
				visited);
	}

	private boolean readDirectory(ByteSource byteSource, int offset,
			int dirType, FormatCompliance formatCompliance, Listener listener,
			List visited) throws ImageReadException, IOException
	{
		boolean ignoreNextDirectory = false;
		return readDirectory(byteSource, offset, dirType, formatCompliance,
				listener, ignoreNextDirectory, visited);
	}

	private boolean readDirectory(ByteSource byteSource, int offset,
			int dirType, FormatCompliance formatCompliance, Listener listener,
			boolean ignoreNextDirectory, List visited)
			throws ImageReadException, IOException
	{
		Number key = new Integer(offset);

		// Debug.debug();
		// Debug.debug("dir offset", offset + " (0x" +
		// Integer.toHexString(offset)
		// + ")");
		// Debug.debug("dir key", key);
		// Debug.debug("dir visited", visited);
		// Debug.debug("dirType", dirType);
		// Debug.debug();

		if (visited.contains(key))
			return false;
		visited.add(key);

		InputStream is = null;
		try
		{
			is = byteSource.getInputStream();
			if (offset > 0)
				is.skip(offset);

			ArrayList fields = new ArrayList();

			if (offset >= byteSource.getLength())
			{
				// Debug.debug("skipping invalid directory!");
				return true;
			}

			int entryCount;
			try
			{
				entryCount = read2Bytes("DirectoryEntryCount", is,
						"Not a Valid TIFF File");
			} catch (IOException e)
			{
				if (strict)
					throw e;
				else
					return true;
			}

			// Debug.debug("entryCount", entryCount);

			for (int i = 0; i < entryCount; i++)
			{
				int tag = read2Bytes("Tag", is, "Not a Valid TIFF File");
				int type = read2Bytes("Type", is, "Not a Valid TIFF File");
				int length = read4Bytes("Length", is, "Not a Valid TIFF File");

				// Debug.debug("tag*", tag + " (0x" + Integer.toHexString(tag)
				// + ")");

				byte valueOffsetBytes[] = readByteArray("ValueOffset", 4, is,
						"Not a Valid TIFF File");
				int valueOffset = convertByteArrayToInt("ValueOffset",
						valueOffsetBytes);

				if (tag == 0)
				{
					// skip invalid fields.
					// These are seen very rarely, but can have invalid value
					// lengths,
					// which can cause OOM problems.
					continue;
				}

				// if (keepField(tag, tags))
				// {
				TiffField field = new TiffField(tag, dirType, type, length,
						valueOffset, valueOffsetBytes, getByteOrder());
				field.setSortHint(i);

				// Debug.debug("tagInfo", field.tagInfo);

				field.fillInValue(byteSource);

				// Debug.debug("\t" + "value", field.getValueDescription());

				fields.add(field);

				if (!listener.addField(field))
					return true;
			}

			int nextDirectoryOffset = read4Bytes("nextDirectoryOffset", is,
					"Not a Valid TIFF File");
			// Debug.debug("nextDirectoryOffset", nextDirectoryOffset);

			TiffDirectory directory = new TiffDirectory(dirType, fields,
					offset, nextDirectoryOffset);

			if (listener.readImageData())
			{
//				if (directory.hasTiffImageData())
//				{
//					TiffImageData rawImageData = getTiffRawImageData(
//							byteSource, directory);
//					directory.setTiffImageData(rawImageData);
//				}
				if (directory.hasJpegImageData())
				{
					JpegImageData rawJpegImageData = getJpegRawImageData(
							byteSource, directory);
					directory.setJpegImageData(rawJpegImageData);
				}
			}

			if (!listener.addDirectory(directory))
				return true;

			if (listener.readOffsetDirectories())
			{
				List fieldsToRemove = new ArrayList();
				for (int j = 0; j < fields.size(); j++)
				{
					TiffField entry = (TiffField) fields.get(j);

					if (entry.tag == TiffConstants.EXIF_TAG_EXIF_OFFSET.tag
							|| entry.tag == TiffConstants.EXIF_TAG_GPSINFO.tag
							|| entry.tag == TiffConstants.EXIF_TAG_INTEROP_OFFSET.tag)
						;
					else
						continue;

					int subDirectoryOffset = ((Number) entry.getValue())
							.intValue();
					int subDirectoryType;
					if (entry.tag == TiffConstants.EXIF_TAG_EXIF_OFFSET.tag)
						subDirectoryType = TiffDirectory.DIRECTORY_TYPE_EXIF;
					else if (entry.tag == TiffConstants.EXIF_TAG_GPSINFO.tag)
						subDirectoryType = TiffDirectory.DIRECTORY_TYPE_GPS;
					else if (entry.tag == TiffConstants.EXIF_TAG_INTEROP_OFFSET.tag)
						subDirectoryType = TiffDirectory.DIRECTORY_TYPE_INTEROPERABILITY;
					else
						throw new ImageReadException(
								"Unknown subdirectory type.");

					// Debug.debug("sub dir", subDirectoryOffset);
					boolean subDirectoryRead = readDirectory(byteSource,
							subDirectoryOffset, subDirectoryType,
							formatCompliance, listener, true, visited);

					if (!subDirectoryRead)
					{
						// Offset field pointed to invalid location.
						// This is a bug in certain cameras. Ignore offset
						// field.
						fieldsToRemove.add(entry);
					}

				}
				fields.removeAll(fieldsToRemove);
			}

			if (!ignoreNextDirectory && directory.nextDirectoryOffset > 0)
			{
				// Debug.debug("next dir", directory.nextDirectoryOffset );
				readDirectory(byteSource, directory.nextDirectoryOffset,
						dirType + 1, formatCompliance, listener, visited);
			}

			return true;
		} finally
		{
			try
			{
				if (is != null)
					is.close();
			} catch (Exception e)
			{
				Debug.debug(e);
			}
		}
	}

	public static interface Listener
	{
		public boolean setTiffHeader(TiffHeader tiffHeader);

		public boolean addDirectory(TiffDirectory directory);

		public boolean addField(TiffField field);

		public boolean readImageData();

		public boolean readOffsetDirectories();
	}

	private static class Collector implements Listener
	{
		private TiffHeader tiffHeader = null;
		private ArrayList directories = new ArrayList();
		private ArrayList fields = new ArrayList();
		private final boolean readThumbnails;

		public Collector()
		{
			this(null);
		}

		public Collector(Map params)
		{
			boolean readThumbnails = true;
			if (params != null && params.containsKey(PARAM_KEY_READ_THUMBNAILS))
				readThumbnails = Boolean.TRUE.equals(params
						.get(PARAM_KEY_READ_THUMBNAILS));
			this.readThumbnails = readThumbnails;
		}

		public boolean setTiffHeader(TiffHeader tiffHeader)
		{
			this.tiffHeader = tiffHeader;
			return true;
		}

		public boolean addDirectory(TiffDirectory directory)
		{
			directories.add(directory);
			return true;
		}

		public boolean addField(TiffField field)
		{
			fields.add(field);
			return true;
		}

		public boolean readImageData()
		{
			return readThumbnails;
		}

		public boolean readOffsetDirectories()
		{
			return true;
		}

		public TiffContents getContents()
		{
			return new TiffContents(tiffHeader, directories);
		}
	}

	private static class FirstDirectoryCollector extends Collector
	{
		private final boolean readImageData;

		public FirstDirectoryCollector(final boolean readImageData)
		{
			this.readImageData = readImageData;
		}

		public boolean addDirectory(TiffDirectory directory)
		{
			super.addDirectory(directory);
			return false;
		}

		public boolean readImageData()
		{
			return readImageData;
		}
	}

	private static class DirectoryCollector extends Collector
	{
		private final boolean readImageData;

		public DirectoryCollector(final boolean readImageData)
		{
			this.readImageData = readImageData;
		}

		public boolean addDirectory(TiffDirectory directory)
		{
			super.addDirectory(directory);
			return false;
		}

		public boolean readImageData()
		{
			return readImageData;
		}
	}

	public TiffContents readFirstDirectory(ByteSource byteSource, Map params,
			boolean readImageData, FormatCompliance formatCompliance)
			throws ImageReadException, IOException
	{
		Collector collector = new FirstDirectoryCollector(readImageData);
		read(byteSource, params, formatCompliance, collector);
		TiffContents contents = collector.getContents();
		if (contents.directories.size() < 1)
			throw new ImageReadException(
					"Image did not contain any directories.");
		return contents;
	}

	public TiffContents readDirectories(ByteSource byteSource,
			boolean readImageData, FormatCompliance formatCompliance)
			throws ImageReadException, IOException
	{
		Collector collector = new FirstDirectoryCollector(readImageData);
		readDirectories(byteSource, formatCompliance, collector);
		TiffContents contents = collector.getContents();
		if (contents.directories.size() < 1)
			throw new ImageReadException(
					"Image did not contain any directories.");
		return contents;
	}

	public TiffContents readContents(ByteSource byteSource, Map params,
			FormatCompliance formatCompliance) throws ImageReadException,
			IOException
	{

		Collector collector = new Collector(params);
		read(byteSource, params, formatCompliance, collector);
		TiffContents contents = collector.getContents();
		return contents;
	}

	public void read(ByteSource byteSource, Map params,
			FormatCompliance formatCompliance, Listener listener)
			throws ImageReadException, IOException
	{
		// TiffContents contents =
		readDirectories(byteSource, formatCompliance, listener);
	}

//	private TiffImageData getTiffRawImageData(ByteSource byteSource,
//			TiffDirectory directory) throws ImageReadException, IOException
//	{
//
//		ArrayList elements = directory.getTiffRawImageDataElements();
//		TiffImageData.Data data[] = new TiffImageData.Data[elements.size()];
//		for (int i = 0; i < elements.size(); i++)
//		{
//			TiffDirectory.ImageDataElement element = (TiffDirectory.ImageDataElement) elements
//					.get(i);
//			byte bytes[] = byteSource.getBlock(element.offset, element.length);
//			data[i] = new TiffImageData.Data(element.offset, element.length,
//					bytes);
//		}
//
//		if (directory.imageDataInStrips())
//		{
//			TiffField rowsPerStripField = directory
//					.findField(TIFF_TAG_ROWS_PER_STRIP);
//			if (null == rowsPerStripField)
//				throw new ImageReadException("Can't find rows per strip field.");
//			int rowsPerStrip = rowsPerStripField.getIntValue();
//
//			return new TiffImageData.Strips(data, rowsPerStrip);
//		} else
//		{
//			TiffField tileWidthField = directory.findField(TIFF_TAG_TILE_WIDTH);
//			if (null == tileWidthField)
//				throw new ImageReadException("Can't find tile width field.");
//			int tileWidth = tileWidthField.getIntValue();
//
//			TiffField tileLengthField = directory
//					.findField(TIFF_TAG_TILE_LENGTH);
//			if (null == tileLengthField)
//				throw new ImageReadException("Can't find tile length field.");
//			int tileLength = tileLengthField.getIntValue();
//
//			return new TiffImageData.Tiles(data, tileWidth, tileLength);
//		}
//	}

	private JpegImageData getJpegRawImageData(ByteSource byteSource,
			TiffDirectory directory) throws ImageReadException, IOException
	{
		ImageDataElement element = directory.getJpegRawImageDataElement();
		int offset = element.offset;
		int length = element.length;
		// Sony DCR-PC110 has an off-by-one error.
		if (offset + length == byteSource.getLength() + 1)
			length--;
		byte data[] = byteSource.getBlock(offset, length);
		return new JpegImageData(offset, length, data);
	}

}
