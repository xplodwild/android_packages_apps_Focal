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

import java.util.Comparator;

public abstract class TiffElement
{
	public final int offset;
	public final int length;

	public TiffElement(int offset, int length)
	{
		this.offset = offset;
		this.length = length;
	}

	public String getElementDescription()
	{
		return getElementDescription(false);
	}

	public abstract String getElementDescription(boolean verbose);

	public static final Comparator COMPARATOR = new Comparator()
	{
		public int compare(Object o1, Object o2)
		{
			TiffElement e1 = (TiffElement) o1;
			TiffElement e2 = (TiffElement) o2;
			return e1.offset - e2.offset;
		}
	};

	public static abstract class DataElement extends TiffElement
	{
		public final byte data[];

		public DataElement(int offset, int length, final byte data[])
		{
			super(offset, length);

			this.data = data;
		}

	}

	public static final class Stub extends TiffElement
	{
		public Stub(int offset, int length)
		{
			super(offset, length);
		}

		public String getElementDescription(boolean verbose)
		{
			return "Element, offset: " + offset + ", length: " + length
					+ ", last: " + (offset + length) + "";
		}

	}
}