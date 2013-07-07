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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;

class TiffOutputSummary implements TiffConstants
{
	public final int byteOrder;
	public final TiffOutputDirectory rootDirectory;
	public final Map directoryTypeMap;

	public TiffOutputSummary(final int byteOrder,
			final TiffOutputDirectory rootDirectory, final Map directoryTypeMap)
	{
		this.byteOrder = byteOrder;
		this.rootDirectory = rootDirectory;
		this.directoryTypeMap = directoryTypeMap;
	}

	private static class OffsetItem
	{
		public final TiffOutputItem item;
		public final TiffOutputField itemOffsetField;

		public OffsetItem(final TiffOutputItem item,
				final TiffOutputField itemOffsetField)
		{
			super();
			this.itemOffsetField = itemOffsetField;
			this.item = item;
		}
	}

	private List offsetItems = new ArrayList();

	public void add(final TiffOutputItem item,
			final TiffOutputField itemOffsetField)
	{
		offsetItems.add(new OffsetItem(item, itemOffsetField));
	}

	public void updateOffsets(int byteOrder) throws ImageWriteException
	{
		for (int i = 0; i < offsetItems.size(); i++)
		{
			OffsetItem offset = (OffsetItem) offsetItems.get(i);

			byte value[] = FIELD_TYPE_LONG.writeData(new int[]{
				offset.item.getOffset(),
			}, byteOrder);
			offset.itemOffsetField.setData(value);
		}

		for (int i = 0; i < imageDataItems.size(); i++)
		{
			ImageDataOffsets imageDataInfo = (ImageDataOffsets) imageDataItems
					.get(i);

			for (int j = 0; j < imageDataInfo.outputItems.length; j++)
			{
				TiffOutputItem item = imageDataInfo.outputItems[j];
				imageDataInfo.imageDataOffsets[j] = item.getOffset();
			}

			imageDataInfo.imageDataOffsetsField.setData(FIELD_TYPE_LONG
					.writeData(imageDataInfo.imageDataOffsets, byteOrder));
		}
	}

	private List imageDataItems = new ArrayList();

	public void addTiffImageData(final ImageDataOffsets imageDataInfo)
	{
		imageDataItems.add(imageDataInfo);
	}

}