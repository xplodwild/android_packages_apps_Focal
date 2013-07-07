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
package org.apache.sanselan.formats.jpeg;

//import java.awt.image.BufferedImage;
import java.io.IOException;
import java.util.ArrayList;

import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.common.IImageMetadata;
import org.apache.sanselan.formats.tiff.TiffField;
//import org.apache.sanselan.formats.tiff.TiffImageData;
import org.apache.sanselan.formats.tiff.TiffImageMetadata;
import org.apache.sanselan.formats.tiff.constants.TagInfo;
import org.apache.sanselan.util.Debug;

public class JpegImageMetadata implements IImageMetadata {
//	private final JpegPhotoshopMetadata photoshop;
	private final TiffImageMetadata exif;

	public JpegImageMetadata(final /*JpegPhotoshopMetadata*/Object photoshop,
			final TiffImageMetadata exif) {
//		this.photoshop = photoshop;
		this.exif = exif;
	}

	public TiffImageMetadata getExif() {
		return exif;
	}

//	public JpegPhotoshopMetadata getPhotoshop() {
//		return photoshop;
//	}

	public TiffField findEXIFValue(TagInfo tagInfo) {
		ArrayList items = getItems();
		for (int i = 0; i < items.size(); i++) {
			Object o = items.get(i);
			if (!(o instanceof TiffImageMetadata.Item))
				continue;

			TiffImageMetadata.Item item = (TiffImageMetadata.Item) o;
			TiffField field = item.getTiffField();
			if (field.tag == tagInfo.tag)
				return field;
		}

		return null;
	}

	public Object getEXIFThumbnail() throws ImageReadException,
			IOException {
//		ArrayList dirs = exif.getDirectories();
//		for (int i = 0; i < dirs.size(); i++) {
//			TiffImageMetadata.Directory dir = (TiffImageMetadata.Directory) dirs
//					.get(i);
//			// Debug.debug("dir", dir);
//			BufferedImage image = dir.getThumbnail();
//			if (null != image)
//				return image;
//		}

		return null;
	}

//	public TiffImageData getRawImageData() {
//		ArrayList dirs = exif.getDirectories();
//		for (int i = 0; i < dirs.size(); i++) {
//			TiffImageMetadata.Directory dir = (TiffImageMetadata.Directory) dirs
//					.get(i);
//			// Debug.debug("dir", dir);
//			TiffImageData rawImageData = dir.getTiffImageData();
//			if (null != rawImageData)
//				return rawImageData;
//		}
//
//		return null;
//	}
//
	public ArrayList getItems() {
		ArrayList result = new ArrayList();

		if (null != exif)
			result.addAll(exif.getItems());

//		if (null != photoshop)
//			result.addAll(photoshop.getItems());

		return result;
	}

	private static final String newline = System.getProperty("line.separator");

	public String toString() {
		return toString(null);
	}

	public String toString(String prefix) {
		if (prefix == null)
			prefix = "";

		StringBuffer result = new StringBuffer();

		result.append(prefix);
		if (null == exif)
			result.append("No Exif metadata.");
		else {
			result.append("Exif metadata:");
			result.append(newline);
			result.append(exif.toString("\t"));
		}

		// if (null != exif && null != photoshop)
		result.append(newline);

		result.append(prefix);
//		if (null == photoshop)
			result.append("No Photoshop (IPTC) metadata.");
//		else {
//			result.append("Photoshop (IPTC) metadata:");
//			result.append(newline);
//			result.append(photoshop.toString("\t"));
//		}

		return result.toString();
	}

	public void dump() {
		Debug.debug(this.toString());
	}

}