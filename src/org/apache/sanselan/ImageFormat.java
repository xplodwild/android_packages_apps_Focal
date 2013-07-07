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
package org.apache.sanselan;

public class ImageFormat
{
	public final String name;
	public final String extension;
	public final boolean actual;

	private ImageFormat(String name, boolean actual)
	{
		this.name = name;
		this.extension = name;
		this.actual = actual;
	}

	private ImageFormat(String name)
	{
		this.name = name;
		this.extension = name;
		actual = true;
	}

	public boolean equals(Object o)
	{
		if (!(o instanceof ImageFormat))
			return false;

		ImageFormat other = (ImageFormat) o;

		return other.name.equals(name);

	}

	public String toString()
	{
		return "{" + name + ": " + extension + "}";
	}

	public int hashCode()
	{
		return name.hashCode();
	}

	public static final ImageFormat IMAGE_FORMAT_UNKNOWN = new ImageFormat(
			"UNKNOWN", false);
	public static final ImageFormat IMAGE_FORMAT_PNG = new ImageFormat("PNG");
	public static final ImageFormat IMAGE_FORMAT_GIF = new ImageFormat("GIF");
	public static final ImageFormat IMAGE_FORMAT_ICO = new ImageFormat("ICO");
	public static final ImageFormat IMAGE_FORMAT_TIFF = new ImageFormat("TIFF");
	public static final ImageFormat IMAGE_FORMAT_JPEG = new ImageFormat("JPEG");
	public static final ImageFormat IMAGE_FORMAT_BMP = new ImageFormat("BMP");
	public static final ImageFormat IMAGE_FORMAT_PSD = new ImageFormat("PSD");
	public static final ImageFormat IMAGE_FORMAT_PBM = new ImageFormat("PBM");
	public static final ImageFormat IMAGE_FORMAT_PGM = new ImageFormat("PGM");
	public static final ImageFormat IMAGE_FORMAT_PPM = new ImageFormat("PPM");
	public static final ImageFormat IMAGE_FORMAT_PNM = new ImageFormat("PNM");
	public static final ImageFormat IMAGE_FORMAT_TGA = new ImageFormat("TGA");
	public static final ImageFormat IMAGE_FORMAT_JBIG2 = new ImageFormat("JBig2");

	public static final ImageFormat[] getAllFormats()
	{
		ImageFormat result[] = {
				IMAGE_FORMAT_UNKNOWN, IMAGE_FORMAT_PNG, IMAGE_FORMAT_GIF,
				IMAGE_FORMAT_TIFF, IMAGE_FORMAT_JPEG, IMAGE_FORMAT_BMP,
				IMAGE_FORMAT_PSD, IMAGE_FORMAT_PBM, IMAGE_FORMAT_PGM,
				IMAGE_FORMAT_PPM, IMAGE_FORMAT_PNM, IMAGE_FORMAT_TGA,
				IMAGE_FORMAT_JBIG2,
		};

		return result;
	}

}