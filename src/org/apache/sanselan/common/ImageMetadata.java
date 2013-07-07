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
package org.apache.sanselan.common;

import java.util.ArrayList;

public class ImageMetadata implements IImageMetadata
{

	private final ArrayList items = new ArrayList();

	public void add(String keyword, String text)
	{
		add(new Item(keyword, text));
	}

	public void add(IImageMetadataItem item)
	{
		items.add(item);
	}

	public ArrayList getItems()
	{
		return new ArrayList(items);
	}

	protected static final String newline = System
			.getProperty("line.separator");

	public String toString()
	{
		return toString(null);
	}

	public String toString(String prefix)
	{
		if (null == prefix)
			prefix = "";

		StringBuffer result = new StringBuffer();
		for (int i = 0; i < items.size(); i++)
		{
			if (i > 0)
				result.append(newline);
			//			if (null != prefix)
			//				result.append(prefix);

			ImageMetadata.IImageMetadataItem item = (ImageMetadata.IImageMetadataItem) items
					.get(i);
			result.append(item.toString(prefix + "\t"));

			//			Debug.debug("prefix", prefix);
			//			Debug.debug("item", items.get(i));
			//			Debug.debug();
		}
		return result.toString();
	}

	public static class Item implements IImageMetadataItem
	{
		private final String keyword, text;

		public Item(String keyword, String text)
		{
			this.keyword = keyword;
			this.text = text;
		}

		public String getKeyword()
		{
			return keyword;
		}

		public String getText()
		{
			return text;
		}

		public String toString()
		{
			return toString(null);
		}

		public String toString(String prefix)
		{
			String result = keyword + ": " + text;
			if (null != prefix)
				result = prefix + result;
			return result;
		}
	}

}