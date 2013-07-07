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

//import java.awt.Dimension;
//import java.awt.color.ICC_Profile;
//import java.awt.image.BufferedImage;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.apache.sanselan.common.IImageMetadata;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.common.byteSources.ByteSourceArray;
import org.apache.sanselan.common.byteSources.ByteSourceFile;
import org.apache.sanselan.common.byteSources.ByteSourceInputStream;
//import org.apache.sanselan.icc.IccProfileParser;
import org.apache.sanselan.util.Debug;

/**
 * The primary interface to the sanselan library.
 * <p>
 * Almost all of the Sanselan library's core functionality can be accessed
 * through it's methods.
 * <p>
 * All of Sanselan's methods are static.
 * <p>
 * See the source of the SampleUsage class and other classes in the
 * org.apache.sanselan.sampleUsage package for examples.
 * 
 * @see org.apache.sanselan.sampleUsage.SampleUsage
 */
public abstract class Sanselan implements SanselanConstants {

	/**
	 * Tries to guess whether a file contains an image based on its file
	 * extension.
	 * <p>
	 * Returns true if the file has a file extension associated with a file
	 * format, such as .jpg or .gif.
	 * <p>
	 * 
	 * @param file
	 *            File which may contain an image.
	 * @return true if the file has an image format file extension.
	 */
//	public static boolean hasImageFileExtension(File file) {
//		if (!file.isFile())
//			return false;
//		return hasImageFileExtension(file.getName());
//	}

	/**
	 * Tries to guess whether a filename represents an image based on its file
	 * extension.
	 * <p>
	 * Returns true if the filename has a file extension associated with a file
	 * format, such as .jpg or .gif.
	 * <p>
	 * 
	 * @param filename
	 *            String representing name of file which may contain an image.
	 * @return true if the filename has an image format file extension.
	 */
//	public static boolean hasImageFileExtension(String filename) {
//		filename = filename.toLowerCase();
//
//		ImageParser imageParsers[] = ImageParser.getAllImageParsers();
//		for (int i = 0; i < imageParsers.length; i++) {
//			ImageParser imageParser = imageParsers[i];
//			String exts[] = imageParser.getAcceptedExtensions();
//
//			for (int j = 0; j < exts.length; j++) {
//				String ext = exts[j];
//				if (filename.endsWith(ext.toLowerCase()))
//					return true;
//			}
//		}
//
//		return false;
//	}

	/**
	 * Tries to guess what the image type (if any) of data based on the file's
	 * "magic numbers," the first bytes of the data.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return An ImageFormat, such as ImageFormat.IMAGE_FORMAT_JPEG. Returns
	 *         ImageFormat.IMAGE_FORMAT_UNKNOWN if the image type cannot be
	 *         guessed.
	 */
//	public static ImageFormat guessFormat(byte bytes[])
//			throws ImageReadException, IOException {
//		return guessFormat(new ByteSourceArray(bytes));
//	}

	/**
	 * Tries to guess what the image type (if any) of a file based on the file's
	 * "magic numbers," the first bytes of the file.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return An ImageFormat, such as ImageFormat.IMAGE_FORMAT_JPEG. Returns
	 *         ImageFormat.IMAGE_FORMAT_UNKNOWN if the image type cannot be
	 *         guessed.
	 */
//	public static ImageFormat guessFormat(File file) throws ImageReadException,
//			IOException {
//		return guessFormat(new ByteSourceFile(file));
//	}

	public static ImageFormat guessFormat(ByteSource byteSource)
			throws ImageReadException, IOException {
		InputStream is = null;

		try {
			is = byteSource.getInputStream();

			int i1 = is.read();
			int i2 = is.read();
			if ((i1 < 0) || (i2 < 0))
				throw new ImageReadException(
						"Couldn't read magic numbers to guess format.");

			int b1 = i1 & 0xff;
			int b2 = i2 & 0xff;

			if (b1 == 0x47 && b2 == 0x49) {
				return ImageFormat.IMAGE_FORMAT_GIF;
			}
			// else if (b1 == 0x00 && b2 == 0x00) // too similar to tga
			// {
			// return ImageFormat.IMAGE_FORMAT_ICO;
			// }
			else if (b1 == 0x89 && b2 == 0x50) {
				return ImageFormat.IMAGE_FORMAT_PNG;
			} else if (b1 == 0xff && b2 == 0xd8) {
				return ImageFormat.IMAGE_FORMAT_JPEG;
			} else if (b1 == 0x42 && b2 == 0x4d) {
				return ImageFormat.IMAGE_FORMAT_BMP;
			} else if (b1 == 0x4D && b2 == 0x4D) // Motorola byte order TIFF
			{
				return ImageFormat.IMAGE_FORMAT_TIFF;
			} else if (b1 == 0x49 && b2 == 0x49) // Intel byte order TIFF
			{
				return ImageFormat.IMAGE_FORMAT_TIFF;
			} else if (b1 == 0x38 && b2 == 0x42) {
				return ImageFormat.IMAGE_FORMAT_PSD;
			} else if (b1 == 0x50 && b2 == 0x31) {
				return ImageFormat.IMAGE_FORMAT_PBM;
			} else if (b1 == 0x50 && b2 == 0x34) {
				return ImageFormat.IMAGE_FORMAT_PBM;
			} else if (b1 == 0x50 && b2 == 0x32) {
				return ImageFormat.IMAGE_FORMAT_PGM;
			} else if (b1 == 0x50 && b2 == 0x35) {
				return ImageFormat.IMAGE_FORMAT_PGM;
			} else if (b1 == 0x50 && b2 == 0x33) {
				return ImageFormat.IMAGE_FORMAT_PPM;
			} else if (b1 == 0x50 && b2 == 0x36) {
				return ImageFormat.IMAGE_FORMAT_PPM;
			} else if (b1 == 0x97 && b2 == 0x4A) {

				int i3 = is.read();
				int i4 = is.read();
				if ((i3 < 0) || (i4 < 0))
					throw new ImageReadException(
							"Couldn't read magic numbers to guess format.");

				int b3 = i3 & 0xff;
				int b4 = i4 & 0xff;

				if (b3 == 0x42 && b4 == 0x32)
					return ImageFormat.IMAGE_FORMAT_JBIG2;
			}

			return ImageFormat.IMAGE_FORMAT_UNKNOWN;
		} finally {
			if (is != null) {
				try {
					is.close();

				} catch (IOException e) {
					Debug.debug(e);

				}
			}
		}
	}

	/**
	 * Extracts an ICC Profile (if present) from JPEG, PNG, PSD (Photoshop) and
	 * TIFF images.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return An instance of ICC_Profile or null if the image contains no ICC
	 *         profile..
	 */
//	public static ICC_Profile getICCProfile(byte bytes[])
//			throws ImageReadException, IOException {
//		return getICCProfile(bytes, null);
//	}

	/**
	 * Extracts an ICC Profile (if present) from JPEG, PNG, PSD (Photoshop) and
	 * TIFF images.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ICC_Profile or null if the image contains no ICC
	 *         profile..
	 */
//	public static ICC_Profile getICCProfile(byte bytes[], Map params)
//			throws ImageReadException, IOException {
//		return getICCProfile(new ByteSourceArray(bytes), params);
//	}

	/**
	 * Extracts an ICC Profile (if present) from JPEG, PNG, PSD (Photoshop) and
	 * TIFF images.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @return An instance of ICC_Profile or null if the image contains no ICC
	 *         profile..
	 */
//	public static ICC_Profile getICCProfile(InputStream is, String filename)
//			throws ImageReadException, IOException {
//		return getICCProfile(is, filename, null);
//	}

	/**
	 * Extracts an ICC Profile (if present) from JPEG, PNG, PSD (Photoshop) and
	 * TIFF images.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ICC_Profile or null if the image contains no ICC
	 *         profile..
	 */
//	public static ICC_Profile getICCProfile(InputStream is, String filename,
//			Map params) throws ImageReadException, IOException {
//		return getICCProfile(new ByteSourceInputStream(is, filename), params);
//	}

	/**
	 * Extracts an ICC Profile (if present) from JPEG, PNG, PSD (Photoshop) and
	 * TIFF images.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return An instance of ICC_Profile or null if the image contains no ICC
	 *         profile..
	 */
//	public static ICC_Profile getICCProfile(File file)
//			throws ImageReadException, IOException {
//		return getICCProfile(file, null);
//	}

	/**
	 * Extracts an ICC Profile (if present) from JPEG, PNG, PSD (Photoshop) and
	 * TIFF images.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ICC_Profile or null if the image contains no ICC
	 *         profile..
	 */
//	public static ICC_Profile getICCProfile(File file, Map params)
//			throws ImageReadException, IOException {
//		return getICCProfile(new ByteSourceFile(file), params);
//	}

//	protected static ICC_Profile getICCProfile(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException {
//		byte bytes[] = getICCProfileBytes(byteSource, params);
//		if (bytes == null)
//			return null;
//
//		IccProfileParser parser = new IccProfileParser();
//		IccProfileInfo info = parser.getICCProfileInfo(bytes);
//		if (info.issRGB())
//			return null;
//
//		ICC_Profile icc = ICC_Profile.getInstance(bytes);
//		return icc;
//	}

	/**
	 * Extracts the raw bytes of an ICC Profile (if present) from JPEG, PNG, PSD
	 * (Photoshop) and TIFF images.
	 * <p>
	 * To parse the result use IccProfileParser or
	 * ICC_Profile.getInstance(bytes).
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return A byte array.
	 * @see IccProfileParser
	 * @see ICC_Profile
	 */
//	public static byte[] getICCProfileBytes(byte bytes[])
//			throws ImageReadException, IOException {
//		return getICCProfileBytes(bytes, null);
//	}

	/**
	 * Extracts the raw bytes of an ICC Profile (if present) from JPEG, PNG, PSD
	 * (Photoshop) and TIFF images.
	 * <p>
	 * To parse the result use IccProfileParser or
	 * ICC_Profile.getInstance(bytes).
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return A byte array.
	 * @see IccProfileParser
	 * @see ICC_Profile
	 */
//	public static byte[] getICCProfileBytes(byte bytes[], Map params)
//			throws ImageReadException, IOException {
//		return getICCProfileBytes(new ByteSourceArray(bytes), params);
//	}

	/**
	 * Extracts the raw bytes of an ICC Profile (if present) from JPEG, PNG, PSD
	 * (Photoshop) and TIFF images.
	 * <p>
	 * To parse the result use IccProfileParser or
	 * ICC_Profile.getInstance(bytes).
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return A byte array.
	 * @see IccProfileParser
	 * @see ICC_Profile
	 */
//	public static byte[] getICCProfileBytes(File file)
//			throws ImageReadException, IOException {
//		return getICCProfileBytes(file, null);
//	}

	/**
	 * Extracts the raw bytes of an ICC Profile (if present) from JPEG, PNG, PSD
	 * (Photoshop) and TIFF images.
	 * <p>
	 * To parse the result use IccProfileParser or
	 * ICC_Profile.getInstance(bytes).
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return A byte array.
	 * @see IccProfileParser
	 * @see ICC_Profile
	 */
//	public static byte[] getICCProfileBytes(File file, Map params)
//			throws ImageReadException, IOException {
//		return getICCProfileBytes(new ByteSourceFile(file), params);
//	}
//
//	private static byte[] getICCProfileBytes(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		return imageParser.getICCProfileBytes(byteSource, params);
//	}

	/**
	 * Parses the "image info" of an image.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param filename
	 *            String.
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(String filename, byte bytes[],
//			Map params) throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceArray(filename, bytes), params);
//	}

	/**
	 * Parses the "image info" of an image.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param filename
	 *            String.
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(String filename, byte bytes[])
//			throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceArray(filename, bytes), null);
//	}

	/**
	 * Parses the "image info" of an image.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(InputStream is, String filename)
//			throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceInputStream(is, filename), null);
//	}

	/**
	 * Parses the "image info" of an image.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(InputStream is, String filename,
//			Map params) throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceInputStream(is, filename), params);
//	}

	/**
	 * Parses the "image info" of an image.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(byte bytes[])
//			throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceArray(bytes), null);
//	}

	/**
	 * Parses the "image info" of an image.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(byte bytes[], Map params)
//			throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceArray(bytes), params);
//	}

	/**
	 * Parses the "image info" of an image file.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(File file, Map params)
//			throws ImageReadException, IOException {
//		return getImageInfo(new ByteSourceFile(file), params);
//	}

	/**
	 * Parses the "image info" of an image file.
	 * <p>
	 * "Image info" is a summary of basic information about the image such as:
	 * width, height, file format, bit depth, color type, etc.
	 * <p>
	 * Not to be confused with "image metadata."
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return An instance of ImageInfo.
	 * @see ImageInfo
	 */
//	public static ImageInfo getImageInfo(File file) throws ImageReadException,
//			IOException {
//		return getImageInfo(file, null);
//	}
//
//	private static ImageInfo getImageInfo(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		ImageInfo imageInfo = imageParser.getImageInfo(byteSource, params);
//
//		return imageInfo;
//	}

	private static final ImageParser getImageParser(ByteSource byteSource)
			throws ImageReadException, IOException {
		ImageFormat format = guessFormat(byteSource);
		if (!format.equals(ImageFormat.IMAGE_FORMAT_UNKNOWN)) {

			ImageParser imageParsers[] = ImageParser.getAllImageParsers();

			for (int i = 0; i < imageParsers.length; i++) {
				ImageParser imageParser = imageParsers[i];

				if (imageParser.canAcceptType(format))
					return imageParser;
			}
		}

		String filename = byteSource.getFilename();
		if (filename != null) {
			ImageParser imageParsers[] = ImageParser.getAllImageParsers();

			for (int i = 0; i < imageParsers.length; i++) {
				ImageParser imageParser = imageParsers[i];

				if (imageParser.canAcceptExtension(filename))
					return imageParser;
			}
		}

		throw new ImageReadException("Can't parse this format.");
	}


	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @return The width and height of the image.
	 */
//	public static Dimension getImageSize(InputStream is, String filename)
//			throws ImageReadException, IOException {
//		return getImageSize(is, filename, null);
//	}

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return The width and height of the image.
	 */
//	public static Dimension getImageSize(InputStream is, String filename,
//			Map params) throws ImageReadException, IOException {
//		return getImageSize(new ByteSourceInputStream(is, filename), params);
//	}

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return The width and height of the image.
	 */
//	public static Dimension getImageSize(byte bytes[])
//			throws ImageReadException, IOException {
//		return getImageSize(bytes, null);
//	}

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return The width and height of the image.
	 */
//	public static Dimension getImageSize(byte bytes[], Map params)
//			throws ImageReadException, IOException {
//		return getImageSize(new ByteSourceArray(bytes), params);
//	}

	/**
	 * Determines the width and height of an image file.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return The width and height of the image.
	 */
//	public static Dimension getImageSize(File file) throws ImageReadException,
//			IOException {
//		return getImageSize(file, null);
//	}

	/**
	 * Determines the width and height of an image file.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return The width and height of the image.
	 */
//	public static Dimension getImageSize(File file, Map params)
//			throws ImageReadException, IOException {
//		return getImageSize(new ByteSourceFile(file), params);
//	}
//
//	public static Dimension getImageSize(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		return imageParser.getImageSize(byteSource, params);
//	}
	

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(InputStream is, String filename)
//			throws ImageReadException, IOException {
//		return getXmpXml(is, filename, null);
//	}

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(InputStream is, String filename,
//			Map params) throws ImageReadException, IOException {
//		return getXmpXml(new ByteSourceInputStream(is, filename), params);
//	}

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(byte bytes[])
//			throws ImageReadException, IOException {
//		return getXmpXml(bytes, null);
//	}

	/**
	 * Determines the width and height of an image.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(byte bytes[], Map params)
//			throws ImageReadException, IOException {
//		return getXmpXml(new ByteSourceArray(bytes), params);
//	}

	/**
	 * Extracts embedded XML metadata as XML string.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(File file) throws ImageReadException,
//			IOException {
//		return getXmpXml(file, null);
//	}

	/**
	 * Extracts embedded XML metadata as XML string.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(File file, Map params)
//			throws ImageReadException, IOException {
//		return getXmpXml(new ByteSourceFile(file), params);
//	}

	/**
	 * Extracts embedded XML metadata as XML string.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return Xmp Xml as String, if present.  Otherwise, returns null..
	 */
//	public static String getXmpXml(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		return imageParser.getXmpXml(byteSource, params);
//	}

	/**
	 * Parses the metadata of an image. This metadata depends on the format of
	 * the image.
	 * <p>
	 * JPEG/JFIF files may contain EXIF and/or IPTC metadata. PNG files may
	 * contain comments. TIFF files may contain metadata.
	 * <p>
	 * The instance of IImageMetadata returned by getMetadata() should be upcast
	 * (depending on image format).
	 * <p>
	 * Not to be confused with "image info."
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return An instance of IImageMetadata.
	 * @see IImageMetadata
	 */
	public static IImageMetadata getMetadata(byte bytes[])
			throws ImageReadException, IOException {
		return getMetadata(bytes, null);
	}

	/**
	 * Parses the metadata of an image. This metadata depends on the format of
	 * the image.
	 * <p>
	 * JPEG/JFIF files may contain EXIF and/or IPTC metadata. PNG files may
	 * contain comments. TIFF files may contain metadata.
	 * <p>
	 * The instance of IImageMetadata returned by getMetadata() should be upcast
	 * (depending on image format).
	 * <p>
	 * Not to be confused with "image info."
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of IImageMetadata.
	 * @see IImageMetadata
	 */
	public static IImageMetadata getMetadata(byte bytes[], Map params)
			throws ImageReadException, IOException {
		return getMetadata(new ByteSourceArray(bytes), params);
	}

	/**
	 * Parses the metadata of an image file. This metadata depends on the format
	 * of the image.
	 * <p>
	 * JPEG/JFIF files may contain EXIF and/or IPTC metadata. PNG files may
	 * contain comments. TIFF files may contain metadata.
	 * <p>
	 * The instance of IImageMetadata returned by getMetadata() should be upcast
	 * (depending on image format).
	 * <p>
	 * Not to be confused with "image info."
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @return An instance of IImageMetadata.
	 * @see IImageMetadata
	 */
	public static IImageMetadata getMetadata(InputStream is, String filename)
			throws ImageReadException, IOException {
		return getMetadata(is, filename, null);
	}

	/**
	 * Parses the metadata of an image file. This metadata depends on the format
	 * of the image.
	 * <p>
	 * JPEG/JFIF files may contain EXIF and/or IPTC metadata. PNG files may
	 * contain comments. TIFF files may contain metadata.
	 * <p>
	 * The instance of IImageMetadata returned by getMetadata() should be upcast
	 * (depending on image format).
	 * <p>
	 * Not to be confused with "image info."
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of IImageMetadata.
	 * @see IImageMetadata
	 */
	public static IImageMetadata getMetadata(InputStream is, String filename,
			Map params) throws ImageReadException, IOException {
		return getMetadata(new ByteSourceInputStream(is, filename), params);
	}

	/**
	 * Parses the metadata of an image file. This metadata depends on the format
	 * of the image.
	 * <p>
	 * JPEG/JFIF files may contain EXIF and/or IPTC metadata. PNG files may
	 * contain comments. TIFF files may contain metadata.
	 * <p>
	 * The instance of IImageMetadata returned by getMetadata() should be upcast
	 * (depending on image format).
	 * <p>
	 * Not to be confused with "image info."
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return An instance of IImageMetadata.
	 * @see IImageMetadata
	 */
	public static IImageMetadata getMetadata(File file)
			throws ImageReadException, IOException {
		return getMetadata(file, null);
	}

	/**
	 * Parses the metadata of an image file. This metadata depends on the format
	 * of the image.
	 * <p>
	 * JPEG/JFIF files may contain EXIF and/or IPTC metadata. PNG files may
	 * contain comments. TIFF files may contain metadata.
	 * <p>
	 * The instance of IImageMetadata returned by getMetadata() should be upcast
	 * (depending on image format).
	 * <p>
	 * Not to be confused with "image info."
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return An instance of IImageMetadata.
	 * @see IImageMetadata
	 */
	public static IImageMetadata getMetadata(File file, Map params)
			throws ImageReadException, IOException {
		return getMetadata(new ByteSourceFile(file), params);
	}

	private static IImageMetadata getMetadata(ByteSource byteSource, Map params)
			throws ImageReadException, IOException {
		ImageParser imageParser = getImageParser(byteSource);

		return imageParser.getMetadata(byteSource, params);
	}

	/**
	 * Returns a description of the image's structure.
	 * <p>
	 * Useful for exploring format-specific details of image files.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return A description of the image file's structure.
	 */
//	public static String dumpImageFile(byte bytes[]) throws ImageReadException,
//			IOException {
//		return dumpImageFile(new ByteSourceArray(bytes));
//	}

	/**
	 * Returns a description of the image file's structure.
	 * <p>
	 * Useful for exploring format-specific details of image files.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return A description of the image file's structure.
	 */
//	public static String dumpImageFile(File file) throws ImageReadException,
//			IOException {
//		return dumpImageFile(new ByteSourceFile(file));
//	}

//	private static String dumpImageFile(ByteSource byteSource)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		return imageParser.dumpImageFile(byteSource);
//	}
//
//	public static FormatCompliance getFormatCompliance(byte bytes[])
//			throws ImageReadException, IOException {
//		return getFormatCompliance(new ByteSourceArray(bytes));
//	}
//
//	public static FormatCompliance getFormatCompliance(File file)
//			throws ImageReadException, IOException {
//		return getFormatCompliance(new ByteSourceFile(file));
//	}
//
//	private static FormatCompliance getFormatCompliance(ByteSource byteSource)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		return imageParser.getFormatCompliance(byteSource);
//	}

	/**
	 * Returns all images contained in an image.
	 * <p>
	 * Useful for image formats such as GIF and ICO in which a single file may
	 * contain multiple images.
	 * <p>
	 * 
	 * @param is
	 *            InputStream from which to read image data.
	 * @param filename
	 *            Filename associated with image data (optional).
	 * @return A vector of BufferedImages.
	 */
//	public static ArrayList getAllBufferedImages(InputStream is, String filename)
//			throws ImageReadException, IOException {
//		return getAllBufferedImages(new ByteSourceInputStream(is, filename));
//	}

	/**
	 * Returns all images contained in an image.
	 * <p>
	 * Useful for image formats such as GIF and ICO in which a single file may
	 * contain multiple images.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return A vector of BufferedImages.
	 */
//	public static ArrayList getAllBufferedImages(byte bytes[])
//			throws ImageReadException, IOException {
//		return getAllBufferedImages(new ByteSourceArray(bytes));
//	}

	/**
	 * Returns all images contained in an image file.
	 * <p>
	 * Useful for image formats such as GIF and ICO in which a single file may
	 * contain multiple images.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return A vector of BufferedImages.
	 */
//	public static ArrayList getAllBufferedImages(File file)
//			throws ImageReadException, IOException {
//		return getAllBufferedImages(new ByteSourceFile(file));
//	}
//
//	private static ArrayList getAllBufferedImages(ByteSource byteSource)
//			throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//
//		return imageParser.getAllBufferedImages(byteSource);
//	}
//
	// public static boolean extractImages(byte bytes[], File dstDir,
	// String dstRoot, ImageParser encoder) throws ImageReadException,
	// IOException, ImageWriteException
	// {
	// return extractImages(new ByteSourceArray(bytes), dstDir, dstRoot,
	// encoder);
	// }
	//
	// public static boolean extractImages(File file, File dstDir, String
	// dstRoot,
	// ImageParser encoder) throws ImageReadException, IOException,
	// ImageWriteException
	// {
	// return extractImages(new ByteSourceFile(file), dstDir, dstRoot, encoder);
	// }
	//
	// public static boolean extractImages(ByteSource byteSource, File dstDir,
	// String dstRoot, ImageParser encoder) throws ImageReadException,
	// IOException, ImageWriteException
	// {
	// ImageParser imageParser = getImageParser(byteSource);
	//
	// return imageParser.extractImages(byteSource, dstDir, dstRoot, encoder);
	// }

	/**
	 * Reads the first image from an InputStream as a BufferedImage.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param is
	 *            InputStream to read image data from.
	 * @return A BufferedImage.
	 * @see SanselanConstants
	 */
//	public static BufferedImage getBufferedImage(InputStream is)
//			throws ImageReadException, IOException {
//		return getBufferedImage(is, null);
//	}

	/**
	 * Reads the first image from an InputStream as a BufferedImage.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param is
	 *            InputStream to read image data from.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return A BufferedImage.
	 * @see SanselanConstants
	 */
//	public static BufferedImage getBufferedImage(InputStream is, Map params)
//			throws ImageReadException, IOException {
//		String filename = null;
//		if (params != null && params.containsKey(PARAM_KEY_FILENAME))
//			filename = (String) params.get(PARAM_KEY_FILENAME);
//		return getBufferedImage(new ByteSourceInputStream(is, filename), params);
//	}

	/**
	 * Reads the first image from an image file as a BufferedImage.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @return A BufferedImage.
	 * @see SanselanConstants
	 */
//	public static BufferedImage getBufferedImage(byte bytes[])
//			throws ImageReadException, IOException {
//		return getBufferedImage(new ByteSourceArray(bytes), null);
//	}

	/**
	 * Reads the first image from an image file as a BufferedImage.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param bytes
	 *            Byte array containing an image file.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return A BufferedImage.
	 * @see SanselanConstants
	 */
//	public static BufferedImage getBufferedImage(byte bytes[], Map params)
//			throws ImageReadException, IOException {
//		return getBufferedImage(new ByteSourceArray(bytes), params);
//	}

	/**
	 * Reads the first image from an image file as a BufferedImage.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @return A BufferedImage.
	 * @see SanselanConstants
	 */
//	public static BufferedImage getBufferedImage(File file)
//			throws ImageReadException, IOException {
//		return getBufferedImage(new ByteSourceFile(file), null);
//	}

	/**
	 * Reads the first image from an image file as a BufferedImage.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param file
	 *            File containing image data.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return A BufferedImage.
	 * @see SanselanConstants
	 */
//	public static BufferedImage getBufferedImage(File file, Map params)
//			throws ImageReadException, IOException {
//		return getBufferedImage(new ByteSourceFile(file), params);
//	}

//	private static BufferedImage getBufferedImage(ByteSource byteSource,
//			Map params) throws ImageReadException, IOException {
//		ImageParser imageParser = getImageParser(byteSource);
//		if (null == params)
//			params = new HashMap();
//
//		return imageParser.getBufferedImage(byteSource, params);
//	}

	/**
	 * Writes a BufferedImage to a file.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param src
	 *            The BufferedImage to be written.
	 * @param file
	 *            File to write to.
	 * @param format
	 *            The ImageFormat to use.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @see SanselanConstants
	 */
//	public static void writeImage(BufferedImage src, File file,
//			ImageFormat format, Map params) throws ImageWriteException,
//			IOException {
//		OutputStream os = null;
//
//		try {
//			os = new FileOutputStream(file);
//			os = new BufferedOutputStream(os);
//
//			writeImage(src, os, format, params);
//		} finally {
//			try {
//				if (os != null)
//					os.close();
//			} catch (Exception e) {
//				Debug.debug(e);
//			}
//		}
//	}

	/**
	 * Writes a BufferedImage to a byte array.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param src
	 *            The BufferedImage to be written.
	 * @param format
	 *            The ImageFormat to use.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @return A byte array containing the image file.
	 * @see SanselanConstants
	 */
//	public static byte[] writeImageToBytes(BufferedImage src,
//			ImageFormat format, Map params) throws ImageWriteException,
//			IOException {
//		ByteArrayOutputStream os = new ByteArrayOutputStream();
//
//		writeImage(src, os, format, params);
//
//		return os.toByteArray();
//	}

	/**
	 * Writes a BufferedImage to an OutputStream.
	 * <p>
	 * (TODO: elaborate here.)
	 * <p>
	 * Sanselan can only read image info, metadata and ICC profiles from all
	 * image formats. However, note that the library cannot currently read or
	 * write JPEG image data. PSD (Photoshop) files can only be partially read
	 * and cannot be written. All other formats (PNG, GIF, TIFF, BMP, etc.) are
	 * fully supported.
	 * <p>
	 * 
	 * @param src
	 *            The BufferedImage to be written.
	 * @param os
	 *            The OutputStream to write to.
	 * @param format
	 *            The ImageFormat to use.
	 * @param params
	 *            Map of optional parameters, defined in SanselanConstants.
	 * @see SanselanConstants
	 */
//	public static void writeImage(BufferedImage src, OutputStream os,
//			ImageFormat format, Map params) throws ImageWriteException,
//			IOException {
//		ImageParser imageParsers[] = ImageParser.getAllImageParsers();
//
//		// make sure params are non-null
//		if (params == null)
//			params = new HashMap();
//
//		params.put(PARAM_KEY_FORMAT, format);
//
//		for (int i = 0; i < imageParsers.length; i++) {
//			ImageParser imageParser = imageParsers[i];
//
//			if (!imageParser.canAcceptType(format))
//				continue;
//
//			imageParser.writeImage(src, os, params);
//			return;
//		}
//
//		throw new ImageWriteException("Unknown Format: " + format);
//	}
}