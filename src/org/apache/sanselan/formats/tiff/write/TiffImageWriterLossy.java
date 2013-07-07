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
import java.io.OutputStream;
import java.util.List;

import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.common.BinaryOutputStream;

public class TiffImageWriterLossy extends TiffImageWriterBase
{

	public TiffImageWriterLossy()
	{
	}

	public TiffImageWriterLossy(int byteOrder)
	{
		super(byteOrder);
	}

	public void write(OutputStream os, TiffOutputSet outputSet)
			throws IOException, ImageWriteException
	{
		TiffOutputSummary outputSummary = validateDirectories(outputSet);

		List outputItems = outputSet.getOutputItems(outputSummary);

		updateOffsetsStep(outputItems);

		outputSummary.updateOffsets(byteOrder);

		BinaryOutputStream bos = new BinaryOutputStream(os, byteOrder);

		writeStep(bos, outputItems);
	}

	private void updateOffsetsStep(List outputItems) throws IOException,
			ImageWriteException
	{
		int offset = TIFF_HEADER_SIZE;

		for (int i = 0; i < outputItems.size(); i++)
		{
			TiffOutputItem outputItem = (TiffOutputItem) outputItems.get(i);

			outputItem.setOffset(offset);
			int itemLength = outputItem.getItemLength();
			offset += itemLength;

			int remainder = imageDataPaddingLength(itemLength);
			offset += remainder;
		}
	}

	private void writeStep(BinaryOutputStream bos, List outputItems)
			throws IOException, ImageWriteException
	{
		writeImageFileHeader(bos);

		for (int i = 0; i < outputItems.size(); i++)
		{
			TiffOutputItem outputItem = (TiffOutputItem) outputItems.get(i);

			outputItem.writeItem(bos);

			int length = outputItem.getItemLength();

			int remainder = imageDataPaddingLength(length);
			for (int j = 0; j < remainder; j++)
				bos.write(0);
		}

	}
}
