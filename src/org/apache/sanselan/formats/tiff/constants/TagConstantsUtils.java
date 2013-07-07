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
package org.apache.sanselan.formats.tiff.constants;

public class TagConstantsUtils implements TiffDirectoryConstants
{

	public static TagInfo[] mergeTagLists(TagInfo lists[][])
	{
		int count = 0;
		for (int i = 0; i < lists.length; i++)
			count += lists[i].length;

		TagInfo result[] = new TagInfo[count];

		int index = 0;
		for (int i = 0; i < lists.length; i++)
		{
			System.arraycopy(lists[i], 0, result, index, lists[i].length);
			index += lists[i].length;
		}

		return result;
	}

	public static ExifDirectoryType getExifDirectoryType(int type)
	{
		for (int i = 0; i < EXIF_DIRECTORIES.length; i++)
			if (EXIF_DIRECTORIES[i].directoryType == type)
				return EXIF_DIRECTORIES[i];
		return EXIF_DIRECTORY_UNKNOWN;
	}

}