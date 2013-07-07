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
package org.apache.sanselan.formats.tiff.fieldtypes;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryFileFunctions;
import org.apache.sanselan.formats.tiff.TiffField;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;

public abstract class FieldType extends BinaryFileFunctions implements
		TiffConstants
{
	public final int type, length;
	public final String name;

	public FieldType(int type, int length, String name)
	{
		this.type = type;
		this.length = length;
		this.name = name;
	}

	public boolean isLocalValue(TiffField entry)
	{
		return ((length > 0) && ((length * entry.length) <= TIFF_ENTRY_MAX_VALUE_LENGTH));
	}

	public int getBytesLength(TiffField entry) throws ImageReadException
	{
		if (length < 1)
			throw new ImageReadException("Unknown field type");

		return length * entry.length;
	}

	// public static final byte[] STUB_LOCAL_VALUE = new
	// byte[TIFF_ENTRY_MAX_VALUE_LENGTH];

	public static final byte[] getStubLocalValue()
	{
		return new byte[TIFF_ENTRY_MAX_VALUE_LENGTH];
	}

	public final byte[] getStubValue(int count)
	{
		return new byte[count * length];
	}

	public String getDisplayValue(TiffField entry) throws ImageReadException
	{
		Object o = getSimpleValue(entry);
		if (o == null)
			return "NULL";
		return o.toString();
	}

	public final byte[] getRawBytes(TiffField entry)
	{
		if (isLocalValue(entry))
		{
			int rawLength = length * entry.length;
			byte result[] = new byte[rawLength];
			System.arraycopy(entry.valueOffsetBytes, 0, result, 0, rawLength);
			return result;
//			return readBytearray(name, entry.valueOffsetBytes, 0, length
//					* entry.length);
			// return getBytearrayHead(name + " (" + entry.tagInfo.name + ")",
			// entry.valueOffsetBytes, length * entry.length);
		}

		return entry.oversizeValue;
	}

	public abstract Object getSimpleValue(TiffField entry)
			throws ImageReadException;

	// public final Object getSimpleValue(TiffField entry)
	// {
	// Object array[] = getValueArray(entry);
	// if (null == array)
	// return null;
	// if (array.length == 1)
	// return array[0];
	// return array;
	// }
	//
	// public abstract Object[] getValueArray(TiffField entry);

	public String toString()
	{
		return "[" + getClass().getName() + ". type: " + type + ", name: "
				+ name + ", length: " + length + "]";
	}

	public abstract byte[] writeData(Object o, int byteOrder)
			throws ImageWriteException;

}