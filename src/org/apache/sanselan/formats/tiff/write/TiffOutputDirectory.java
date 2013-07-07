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
package org.apache.sanselan.formats.tiff.write;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryOutputStream;
import org.apache.sanselan.formats.tiff.JpegImageData;
import org.apache.sanselan.formats.tiff.TiffDirectory;
import org.apache.sanselan.formats.tiff.TiffElement;
//import org.apache.sanselan.formats.tiff.TiffImageData;
import org.apache.sanselan.formats.tiff.constants.TagConstantsUtils;
import org.apache.sanselan.formats.tiff.constants.TagInfo;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;
import org.apache.sanselan.formats.tiff.fieldtypes.FieldType;

public final class TiffOutputDirectory extends TiffOutputItem implements
		TiffConstants
{
	public final int type;
	private final ArrayList fields = new ArrayList();

	private TiffOutputDirectory nextDirectory = null;

	public void setNextDirectory(TiffOutputDirectory nextDirectory)
	{
		this.nextDirectory = nextDirectory;
	}

	public TiffOutputDirectory(final int type)
	{
		this.type = type;
	}

	public void add(TiffOutputField field)
	{
		fields.add(field);
	}

	public ArrayList getFields()
	{
		return new ArrayList(fields);
	}

	public void removeField(TagInfo tagInfo)
	{
		removeField(tagInfo.tag);
	}

	public void removeField(int tag)
	{
		ArrayList matches = new ArrayList();
		for (int i = 0; i < fields.size(); i++)
		{
			TiffOutputField field = (TiffOutputField) fields.get(i);
			if (field.tag == tag)
				matches.add(field);
		}
		fields.removeAll(matches);
	}

	public TiffOutputField findField(TagInfo tagInfo)
	{
		return findField(tagInfo.tag);
	}

	public TiffOutputField findField(int tag)
	{
		for (int i = 0; i < fields.size(); i++)
		{
			TiffOutputField field = (TiffOutputField) fields.get(i);
			if (field.tag == tag)
				return field;
		}
		return null;
	}

	public void sortFields()
	{
		Comparator comparator = new Comparator() {
			public int compare(Object o1, Object o2)
			{
				TiffOutputField e1 = (TiffOutputField) o1;
				TiffOutputField e2 = (TiffOutputField) o2;

				if (e1.tag != e2.tag)
					return e1.tag - e2.tag;
				return e1.getSortHint() - e2.getSortHint();
			}
		};
		Collections.sort(fields, comparator);
	}

	public String description()
	{
		return TiffDirectory.description(type);
	}

	public void writeItem(BinaryOutputStream bos) throws IOException,
			ImageWriteException
	{
		// Write Directory Field Count
		bos.write2Bytes(fields.size()); // DirectoryFieldCount

		// Write Fields
		for (int i = 0; i < fields.size(); i++)
		{
			TiffOutputField field = (TiffOutputField) fields.get(i);
			field.writeField(bos);

//			 Debug.debug("\t" + "writing field (" + field.tag + ", 0x" +
//			 Integer.toHexString(field.tag) + ")", field.tagInfo);
//			 if(field.tagInfo.isOffset())
//			 Debug.debug("\t\tOFFSET!", field.bytes);
		}

		int nextDirectoryOffset = 0;
		if (nextDirectory != null)
			nextDirectoryOffset = nextDirectory.getOffset();

		// Write nextDirectoryOffset
		if (nextDirectoryOffset == UNDEFINED_VALUE)
			bos.write4Bytes(0);
		else
			bos.write4Bytes(nextDirectoryOffset);
	}

	private JpegImageData jpegImageData = null;

	public void setJpegImageData(JpegImageData rawJpegImageData)
	{
		this.jpegImageData = rawJpegImageData;
	}

	public JpegImageData getRawJpegImageData()
	{
		return jpegImageData;
	}

//	private TiffImageData tiffImageData = null;
//
//	public void setTiffImageData(TiffImageData rawTiffImageData)
//	{
//		this.tiffImageData = rawTiffImageData;
//	}
//
//	public TiffImageData getRawTiffImageData()
//	{
//		return tiffImageData;
//	}

	public int getItemLength()
	{
		return TIFF_ENTRY_LENGTH * fields.size() + TIFF_DIRECTORY_HEADER_LENGTH
				+ TIFF_DIRECTORY_FOOTER_LENGTH;
	}

	public String getItemDescription()
	{
		ExifDirectoryType dirType = TagConstantsUtils
				.getExifDirectoryType(type);
		return "Directory: " + dirType.name + " (" + type + ")";
	}

	private void removeFieldIfPresent(TagInfo tagInfo)
	{
		TiffOutputField field = findField(tagInfo);
		if (null != field)
			fields.remove(field);
	}

	protected List getOutputItems(TiffOutputSummary outputSummary)
			throws ImageWriteException
	{
		// first validate directory fields.

		removeFieldIfPresent(TIFF_TAG_JPEG_INTERCHANGE_FORMAT);
		removeFieldIfPresent(TIFF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH);

		TiffOutputField jpegOffsetField = null;
		if (null != jpegImageData)
		{
			jpegOffsetField = new TiffOutputField(
					TIFF_TAG_JPEG_INTERCHANGE_FORMAT, FIELD_TYPE_LONG, 1,
					FieldType.getStubLocalValue());
			add(jpegOffsetField);

			byte lengthValue[] = FIELD_TYPE_LONG.writeData(
					new int[] { jpegImageData.length, },
					outputSummary.byteOrder);

			TiffOutputField jpegLengthField = new TiffOutputField(
					TIFF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH, FIELD_TYPE_LONG,
					1, lengthValue);
			add(jpegLengthField);

		}

		// --------------------------------------------------------------

		removeFieldIfPresent(TIFF_TAG_STRIP_OFFSETS);
		removeFieldIfPresent(TIFF_TAG_STRIP_BYTE_COUNTS);
		removeFieldIfPresent(TIFF_TAG_TILE_OFFSETS);
		removeFieldIfPresent(TIFF_TAG_TILE_BYTE_COUNTS);

		TiffOutputField imageDataOffsetField;
		ImageDataOffsets imageDataInfo = null;
//		if (null != tiffImageData)
//		{
//			boolean stripsNotTiles = tiffImageData.stripsNotTiles();
//
//			TagInfo offsetTag;
//			TagInfo byteCountsTag;
//			if (stripsNotTiles)
//			{
//				offsetTag = TIFF_TAG_STRIP_OFFSETS;
//				byteCountsTag = TIFF_TAG_STRIP_BYTE_COUNTS;
//			} else
//			{
//				offsetTag = TIFF_TAG_TILE_OFFSETS;
//				byteCountsTag = TIFF_TAG_TILE_BYTE_COUNTS;
//			}
//
//			// --------
//
//			TiffElement.DataElement imageData[] = tiffImageData.getImageData();
//
//			int imageDataOffsets[] = null;
//			int imageDataByteCounts[] = null;
//			// TiffOutputField imageDataOffsetsField = null;
//
//			imageDataOffsets = new int[imageData.length];
//			imageDataByteCounts = new int[imageData.length];
//			for (int i = 0; i < imageData.length; i++)
//			{
//				imageDataByteCounts[i] = imageData[i].length;
//			}
//
//			// --------
//
//			// Append imageData-related fields to first directory
//			imageDataOffsetField = new TiffOutputField(offsetTag,
//					FIELD_TYPE_LONG, imageDataOffsets.length, FIELD_TYPE_LONG
//							.writeData(imageDataOffsets,
//									outputSummary.byteOrder));
//			add(imageDataOffsetField);
//
//			// --------
//
//			byte data[] = FIELD_TYPE_LONG.writeData(imageDataByteCounts,
//					outputSummary.byteOrder);
//			TiffOutputField byteCountsField = new TiffOutputField(
//					byteCountsTag, FIELD_TYPE_LONG, imageDataByteCounts.length,
//					data);
//			add(byteCountsField);
//
//			// --------
//
//			imageDataInfo = new ImageDataOffsets(imageData, imageDataOffsets,
//					imageDataOffsetField);
//		}

		// --------------------------------------------------------------

		List result = new ArrayList();
		result.add(this);
		sortFields();

		for (int i = 0; i < fields.size(); i++)
		{
			TiffOutputField field = (TiffOutputField) fields.get(i);
			if (field.isLocalValue())
				continue;

			TiffOutputItem item = field.getSeperateValue();
			result.add(item);
			// outputSummary.add(item, field);
		}

		if (null != imageDataInfo)
		{
			for (int i = 0; i < imageDataInfo.outputItems.length; i++)
				result.add(imageDataInfo.outputItems[i]);

			outputSummary.addTiffImageData(imageDataInfo);
		}

		if (null != jpegImageData)
		{
			TiffOutputItem item = new TiffOutputItem.Value("JPEG image data",
					jpegImageData.data);
			result.add(item);
			outputSummary.add(item, jpegOffsetField);
		}

		return result;
	}
}