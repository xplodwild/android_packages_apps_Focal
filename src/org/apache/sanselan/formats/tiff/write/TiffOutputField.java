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

import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryOutputStream;
import org.apache.sanselan.formats.tiff.constants.TagInfo;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;
import org.apache.sanselan.formats.tiff.fieldtypes.FieldType;

public class TiffOutputField implements TiffConstants
{
	public final int tag;
	public final TagInfo tagInfo;
	public final FieldType fieldType;
	public final int count;

	private byte bytes[];

	private final TiffOutputItem.Value separateValueItem;

	public TiffOutputField(TagInfo tagInfo, FieldType tagtype, int count,
			byte bytes[])
	{
		this(tagInfo.tag, tagInfo, tagtype, count, bytes);
	}

	public TiffOutputField(final int tag, TagInfo tagInfo, FieldType fieldType,
			int count, byte bytes[])
	{
		this.tag = tag;
		this.tagInfo = tagInfo;
		this.fieldType = fieldType;
		this.count = count;
		this.bytes = bytes;

		if (isLocalValue())
			separateValueItem = null;
		else
		{
			String name = "Field Seperate value (" + tagInfo.getDescription()
					+ ")";
			separateValueItem = new TiffOutputItem.Value(name, bytes);
		}
	}

	private int sortHint = -1;

	public static TiffOutputField create(TagInfo tagInfo, int byteOrder,
			Number number) throws ImageWriteException
	{
		if (tagInfo.dataTypes == null || tagInfo.dataTypes.length < 1)
			throw new ImageWriteException("Tag has no default data type.");
		FieldType fieldType = tagInfo.dataTypes[0];

		if (tagInfo.length != 1)
			throw new ImageWriteException("Tag does not expect a single value.");

		byte bytes[] = fieldType.writeData(number, byteOrder);

		return new TiffOutputField(tagInfo.tag, tagInfo, fieldType, 1, bytes);
	}

	public static TiffOutputField create(TagInfo tagInfo, int byteOrder,
			Number value[]) throws ImageWriteException
	{
		if (tagInfo.dataTypes == null || tagInfo.dataTypes.length < 1)
			throw new ImageWriteException("Tag has no default data type.");
		FieldType fieldType = tagInfo.dataTypes[0];

		if (tagInfo.length != value.length)
			throw new ImageWriteException("Tag does not expect a single value.");

		byte bytes[] = fieldType.writeData(value, byteOrder);

		return new TiffOutputField(tagInfo.tag, tagInfo, fieldType,
				value.length, bytes);
	}

	public static TiffOutputField create(TagInfo tagInfo, int byteOrder,
			String value) throws ImageWriteException
	{
		FieldType fieldType;
		if (tagInfo.dataTypes == null)
			fieldType = FIELD_TYPE_ASCII;
		else if (tagInfo.dataTypes == FIELD_TYPE_DESCRIPTION_ASCII)
			fieldType = FIELD_TYPE_ASCII;
		else if (tagInfo.dataTypes[0] == FIELD_TYPE_ASCII)
			fieldType = FIELD_TYPE_ASCII;
		else
			throw new ImageWriteException("Tag has unexpected data type.");

		byte bytes[] = fieldType.writeData(value, byteOrder);

		return new TiffOutputField(tagInfo.tag, tagInfo, fieldType, bytes.length, bytes);
	}

	protected static final TiffOutputField createOffsetField(TagInfo tagInfo,
			int byteOrder) throws ImageWriteException
	{
		return new TiffOutputField(tagInfo, FIELD_TYPE_LONG, 1, FIELD_TYPE_LONG
				.writeData(new int[] { 0, }, byteOrder));
	}

	protected void writeField(BinaryOutputStream bos) throws IOException,
			ImageWriteException
	{
		bos.write2Bytes(tag);
		bos.write2Bytes(fieldType.type);
		bos.write4Bytes(count);

		if (isLocalValue())
		{
			if (separateValueItem != null)
				throw new ImageWriteException("Unexpected separate value item.");
			if (bytes.length > 4)
				throw new ImageWriteException(
						"Local value has invalid length: " + bytes.length);

			bos.writeByteArray(bytes);
			int remainder = TIFF_ENTRY_MAX_VALUE_LENGTH - bytes.length;
			for (int i = 0; i < remainder; i++)
				bos.write(0);
		} else
		{
			if (separateValueItem == null)
				throw new ImageWriteException("Missing separate value item.");

			bos.write4Bytes(separateValueItem.getOffset());
		}
	}

	protected TiffOutputItem getSeperateValue()
	{
		return separateValueItem;
	}

	protected boolean isLocalValue()
	{
		return bytes.length <= TIFF_ENTRY_MAX_VALUE_LENGTH;
	}

	protected void setData(byte bytes[]) throws ImageWriteException
	{
		// if(tagInfo.isUnknown())
		// Debug.debug("unknown tag(0x" + Integer.toHexString(tag)
		// + ") setData", bytes);

		if (this.bytes.length != bytes.length)
			throw new ImageWriteException("Cannot change size of value.");

		// boolean wasLocalValue = isLocalValue();
		this.bytes = bytes;
		if (separateValueItem != null)
			separateValueItem.updateValue(bytes);
		// if (isLocalValue() != wasLocalValue)
		// throw new Error("Bug. Locality disrupted! "
		// + tagInfo.getDescription());
	}

	private static final String newline = System.getProperty("line.separator");

	public String toString()
	{
		return toString(null);
	}

	public String toString(String prefix)
	{
		if (prefix == null)
			prefix = "";
		StringBuffer result = new StringBuffer();

		result.append(prefix);
		result.append(tagInfo);
		result.append(newline);

		result.append(prefix);
		result.append("count: " + count);
		result.append(newline);

		result.append(prefix);
		result.append(fieldType);
		result.append(newline);

		return result.toString();
	}

	public int getSortHint()
	{
		return sortHint;
	}

	public void setSortHint(int sortHint)
	{
		this.sortHint = sortHint;
	}
}