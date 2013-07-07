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

import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.formats.tiff.TiffField;

public class FieldTypeASCII extends FieldType
{
	public FieldTypeASCII(int type, String name)
	{
		super(type, 1, name);
	}

	public Object getSimpleValue(TiffField entry) 
	{
		final byte[] rawBytes = getRawBytes(entry);
		return new String(rawBytes, 0, rawBytes.length-1); // strip '\0' character
	}

	public byte[] writeData(Object o, int byteOrder) throws ImageWriteException
	{
		final byte[] originalArray;
		if (o instanceof byte[]) {
			originalArray = (byte[])o;
		}
		else if (o instanceof String) {
			originalArray = ((String)o).getBytes();
		}
		else
			throw new ImageWriteException("Unknown data type: " + o);

		final byte[] retVal = new byte[originalArray.length+1]; // make sure '\0' char is added.
		System.arraycopy(originalArray, 0, retVal, 0, originalArray.length);
		return retVal;
	}

}