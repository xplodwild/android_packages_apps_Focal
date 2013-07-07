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
import org.apache.sanselan.formats.tiff.TiffField;
import org.apache.sanselan.util.Debug;

public class FieldTypeShort extends FieldType
{
	public FieldTypeShort(int type, String name)
	{
		super(type, 2, name);
	}

	//	public Object[] getValueArray(TiffField entry)
	//	{
	//		if(isLocalValue(entry))
	//			return convertByteArrayToShortArray(name + " (" + entry.tagInfo.name
	//					+ ")", entry.valueOffsetBytes, 0, entry.length, entry.byteOrder);
	//
	////		return new Integer(convertByteArrayToShort(name + " ("
	////				+ entry.tagInfo.name + ")", entry.valueOffsetBytes,
	////				entry.byteOrder));
	//
	//		return convertByteArrayToShortArray(name + " (" + entry.tagInfo.name
	//				+ ")", getRawBytes(entry), 0, entry.length, entry.byteOrder);
	//	}

	public Object getSimpleValue(TiffField entry) throws ImageReadException
	{
		if (entry.length == 1)
			return new Integer(convertByteArrayToShort(name + " ("
					+ entry.tagInfo.name + ")", entry.valueOffsetBytes,
					entry.byteOrder));

		return convertByteArrayToShortArray(name + " (" + entry.tagInfo.name
				+ ")", getRawBytes(entry), 0, entry.length, entry.byteOrder);
	}

	public byte[] writeData(Object o, int byteOrder) throws ImageWriteException
	{
		if (o instanceof Integer)
			return convertShortArrayToByteArray(new int[]{
				((Integer) o).intValue(),
			}, byteOrder);
		else if (o instanceof int[])
		{
			int numbers[] = (int[]) o;
			return convertShortArrayToByteArray(numbers, byteOrder);
		}
		else if (o instanceof Integer[])
		{
			Integer numbers[] = (Integer[]) o;
			int values[] = new int[numbers.length];
			for (int i = 0; i < values.length; i++)
				values[i] = numbers[i].intValue();
			return convertShortArrayToByteArray(values, byteOrder);
		}
		else
			throw new ImageWriteException("Invalid data: " + o + " ("
					+ Debug.getType(o) + ")");
	}

}