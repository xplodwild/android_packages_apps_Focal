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

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;

/**
 * ImageInfo represents a collection of basic properties of an image, such as
 * width, height, format, bit depth, etc.
 */
public class ImageInfo
{
	private final String formatDetails; // ie version

	private final int bitsPerPixel;
	private final ArrayList comments;

	private final ImageFormat format;
	private final String formatName;
	private final int height;
	private final String mimeType;

	private final int numberOfImages;
	private final int physicalHeightDpi;
	private final float physicalHeightInch;
	private final int physicalWidthDpi;
	private final float physicalWidthInch;
	private final int width;
	private final boolean isProgressive;
	private final boolean isTransparent;

	private final boolean usesPalette;

	public static final int COLOR_TYPE_BW = 0;
	public static final int COLOR_TYPE_GRAYSCALE = 1;
	public static final int COLOR_TYPE_RGB = 2;
	public static final int COLOR_TYPE_CMYK = 3;
	public static final int COLOR_TYPE_OTHER = -1;
	public static final int COLOR_TYPE_UNKNOWN = -2;

	private final int colorType;

	public static final String COMPRESSION_ALGORITHM_UNKNOWN = "Unknown";
	public static final String COMPRESSION_ALGORITHM_NONE = "None";
	public static final String COMPRESSION_ALGORITHM_LZW = "LZW";
	public static final String COMPRESSION_ALGORITHM_PACKBITS = "PackBits";
	public static final String COMPRESSION_ALGORITHM_JPEG = "JPEG";
	public static final String COMPRESSION_ALGORITHM_RLE = "RLE: Run-Length Encoding";
	public static final String COMPRESSION_ALGORITHM_PSD = "Photoshop";
	public static final String COMPRESSION_ALGORITHM_PNG_FILTER = "PNG Filter";
	public static final String COMPRESSION_ALGORITHM_CCITT_GROUP_3 = "CCITT Group 3 1-Dimensional Modified Huffman run-length encoding.";
	public static final String COMPRESSION_ALGORITHM_CCITT_GROUP_4 = "CCITT Group 4";
	public static final String COMPRESSION_ALGORITHM_CCITT_1D = "CCITT 1D";

	private final String compressionAlgorithm;

	public ImageInfo(String formatDetails, int bitsPerPixel,
			ArrayList comments, ImageFormat format, String formatName,
			int height, String mimeType, int numberOfImages,
			int physicalHeightDpi, float physicalHeightInch,
			int physicalWidthDpi, float physicalWidthInch, int width,
			boolean isProgressive, boolean isTransparent, boolean usesPalette,
			int colorType, String compressionAlgorithm)
	{
		this.formatDetails = formatDetails;

		this.bitsPerPixel = bitsPerPixel;
		this.comments = comments;

		this.format = format;
		this.formatName = formatName;
		this.height = height;
		this.mimeType = mimeType;

		this.numberOfImages = numberOfImages;
		this.physicalHeightDpi = physicalHeightDpi;
		this.physicalHeightInch = physicalHeightInch;
		this.physicalWidthDpi = physicalWidthDpi;
		this.physicalWidthInch = physicalWidthInch;
		this.width = width;
		this.isProgressive = isProgressive;

		this.isTransparent = isTransparent;
		this.usesPalette = usesPalette;

		this.colorType = colorType;
		this.compressionAlgorithm = compressionAlgorithm;
	}

	/**
	 * Returns the bits per pixel of the image data.
	 */
	public int getBitsPerPixel()
	{
		return bitsPerPixel;
	}

	/**
	 * Returns a list of comments from the image file. <p/> This is mostly
	 * obsolete.
	 */
	public ArrayList getComments()
	{
		return new ArrayList(comments);
	}

	/**
	 * Returns the image file format, ie. ImageFormat.IMAGE_FORMAT_PNG. <p/>
	 * Returns ImageFormat.IMAGE_FORMAT_UNKNOWN if format is unknown.
	 * 
	 * @return A constant defined in ImageFormat.
	 * @see ImageFormat
	 */
	public ImageFormat getFormat()
	{
		return format;
	}

	/**
	 * Returns a string with the name of the image file format.
	 * 
	 * @see #getFormat()
	 */
	public String getFormatName()
	{
		return formatName;
	}

	/**
	 * Returns the height of the image in pixels.
	 * 
	 * @see #getWidth()
	 */
	public int getHeight()
	{
		return height;
	}

	/**
	 * Returns the MIME type of the image.
	 * 
	 * @see #getFormat()
	 */
	public String getMimeType()
	{
		return mimeType;
	}

	/**
	 * Returns the number of images in the file.
	 * <p>
	 * Applies mostly to GIF and TIFF; reading PSD/Photoshop layers is not
	 * supported, and Jpeg/JFIF EXIF thumbnails are not included in this count.
	 */
	public int getNumberOfImages()
	{
		return numberOfImages;
	}

	/**
	 * Returns horizontal dpi of the image, if available.
	 * <p>
	 * Applies to TIFF (optional), BMP (always), GIF (constant: 72), Jpeg
	 * (optional), PNG (optional), PNM (constant: 72), PSD/Photoshop (constant:
	 * 72).
	 * 
	 * @return returns -1 if not present.
	 */
	public int getPhysicalHeightDpi()
	{
		return physicalHeightDpi;
	}

	/**
	 * Returns physical height of the image in inches, if available.
	 * <p>
	 * Applies to TIFF (optional), BMP (always), GIF (constant: 72), Jpeg
	 * (optional), PNG (optional), PNM (constant: 72), PSD/Photoshop (constant:
	 * 72).
	 * 
	 * @return returns -1 if not present.
	 */
	public float getPhysicalHeightInch()
	{
		return physicalHeightInch;
	}

	/**
	 * Returns vertical dpi of the image, if available.
	 * <p>
	 * Applies to TIFF (optional), BMP (always), GIF (constant: 72), Jpeg
	 * (optional), PNG (optional), PNM (constant: 72), PSD/Photoshop (constant:
	 * 72).
	 * 
	 * @return returns -1 if not present.
	 */
	public int getPhysicalWidthDpi()
	{
		return physicalWidthDpi;
	}

	/**
	 * Returns physical width of the image in inches, if available.
	 * <p>
	 * Applies to TIFF (optional), BMP (always), GIF (constant: 72), Jpeg
	 * (optional), PNG (optional), PNM (constant: 72), PSD/Photoshop (constant:
	 * 72).
	 * 
	 * @return returns -1 if not present.
	 */
	public float getPhysicalWidthInch()
	{
		return physicalWidthInch;
	}

	/**
	 * Returns the width of the image in pixels.
	 * 
	 * @see #getHeight()
	 */
	public int getWidth()
	{
		return width;
	}

	/**
	 * Returns true if the image is progressive or interlaced.
	 */
	public boolean getIsProgressive()
	{
		return isProgressive;
	}

	/**
	 * Returns the color type of the image, as a constant (ie.
	 * ImageFormat.COLOR_TYPE_CMYK).
	 * 
	 * @see #getColorTypeDescription()
	 */
	public int getColorType()
	{
		return colorType;
	}

	/**
	 * Returns a description of the color type of the image.
	 * 
	 * @see #getColorType()
	 */
	public String getColorTypeDescription()
	{
		switch (colorType)
		{
		case COLOR_TYPE_BW:
			return "Black and White";
		case COLOR_TYPE_GRAYSCALE:
			return "Grayscale";
		case COLOR_TYPE_RGB:
			return "RGB";
		case COLOR_TYPE_CMYK:
			return "CMYK";
		case COLOR_TYPE_OTHER:
			return "Other";
		case COLOR_TYPE_UNKNOWN:
			return "Unknown";

		default:
			return "Unknown";
		}

	}

	public void dump()
	{
		System.out.print(toString());
	}

	public String toString()
	{
		try
		{
			StringWriter sw = new StringWriter();
			PrintWriter pw = new PrintWriter(sw);

			toString(pw, "");
			pw.flush();

			return sw.toString();
		} catch (Exception e)
		{
			return "Image Data: Error";
		}
	}

	public void toString(PrintWriter pw, String prefix)
			throws ImageReadException, IOException
	{
		pw.println("Format Details: " + formatDetails);

		pw.println("Bits Per Pixel: " + bitsPerPixel);
		pw.println("Comments: " + comments.size());
		for (int i = 0; i < comments.size(); i++)
		{
			String s = (String) comments.get(i);
			pw.println("\t" + i + ": '" + s + "'");

		}
		pw.println("Format: " + format.name);
		pw.println("Format Name: " + formatName);
		pw.println("Compression Algorithm: " + compressionAlgorithm);
		pw.println("Height: " + height);
		pw.println("MimeType: " + mimeType);
		pw.println("Number Of Images: " + numberOfImages);
		pw.println("Physical Height Dpi: " + physicalHeightDpi);
		pw.println("Physical Height Inch: " + physicalHeightInch);
		pw.println("Physical Width Dpi: " + physicalWidthDpi);
		pw.println("Physical Width Inch: " + physicalWidthInch);
		pw.println("Width: " + width);
		pw.println("Is Progressive: " + isProgressive);
		pw.println("Is Transparent: " + isTransparent);

		pw.println("Color Type: " + getColorTypeDescription());
		pw.println("Uses Palette: " + usesPalette);

		pw.flush();

	}

}