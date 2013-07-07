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
import org.apache.sanselan.common.RationalNumber;
import org.apache.sanselan.common.RationalNumberUtilities;
import org.apache.sanselan.formats.tiff.TiffField;
import org.apache.sanselan.util.Debug;

public class FieldTypeRational extends FieldType
{
	public FieldTypeRational(int type, String name)
	{
		super(type, 8, name);
	}

	public Object getSimpleValue(TiffField entry)
	{
		if (entry.length == 1)
			return convertByteArrayToRational(name + " (" + entry.tagInfo.name
					+ ")", entry.oversizeValue, entry.byteOrder);

		return convertByteArrayToRationalArray(name + " (" + entry.tagInfo.name
				+ ")", getRawBytes(entry), 0, entry.length, entry.byteOrder);
	}

	public byte[] writeData(Object o, int byteOrder) throws ImageWriteException
	{
		if (o instanceof RationalNumber)
			return convertRationalToByteArray((RationalNumber) o, byteOrder);
		else if (o instanceof RationalNumber[])
		{
			return convertRationalArrayToByteArray((RationalNumber[]) o,
					byteOrder);
		}
		else if (o instanceof Number)
		{
			Number number = (Number) o;
			RationalNumber rationalNumber = RationalNumberUtilities
					.getRationalNumber(number.doubleValue());
			return convertRationalToByteArray(rationalNumber, byteOrder);
		}
		else if (o instanceof Number[])
		{
			Number numbers[] = (Number[]) o;
			RationalNumber rationalNumbers[] = new RationalNumber[numbers.length];
			for (int i = 0; i < numbers.length; i++)
			{
				Number number = numbers[i];
				rationalNumbers[i] = RationalNumberUtilities
						.getRationalNumber(number.doubleValue());
			}
			return convertRationalArrayToByteArray(rationalNumbers, byteOrder);
		}
		else if (o instanceof double[])
		{
			double numbers[] = (double[]) o;
			RationalNumber rationalNumbers[] = new RationalNumber[numbers.length];
			for (int i = 0; i < numbers.length; i++)
			{
				double number = numbers[i];
				rationalNumbers[i] = RationalNumberUtilities
						.getRationalNumber(number);
			}
			return convertRationalArrayToByteArray(rationalNumbers, byteOrder);
		}
		else
			throw new ImageWriteException("Invalid data: " + o + " ("
					+ Debug.getType(o) + ")");
	}

	public byte[] writeData(int numerator, int denominator, int byteOrder)
			throws ImageWriteException
	{
		return writeData(new int[]{
			numerator
		}, new int[]{
			denominator
		}, byteOrder);
	}

	public byte[] writeData(int numerators[], int denominators[], int byteOrder)
			throws ImageWriteException
	{
		return convertIntArrayToRationalArray(numerators, denominators,
				byteOrder);
	}
}