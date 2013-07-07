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
//import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Map;

import org.apache.sanselan.common.BinaryFileParser;
import org.apache.sanselan.common.IImageMetadata;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.common.byteSources.ByteSourceArray;
import org.apache.sanselan.common.byteSources.ByteSourceFile;
//import org.apache.sanselan.formats.bmp.BmpImageParser;
//import org.apache.sanselan.formats.gif.GifImageParser;
//import org.apache.sanselan.formats.ico.IcoImageParser;
import org.apache.sanselan.formats.jpeg.JpegImageParser;
import org.apache.sanselan.formats.png.PngImageParser;
//import org.apache.sanselan.formats.pnm.PNMImageParser;
//import org.apache.sanselan.formats.psd.PsdImageParser;
import org.apache.sanselan.formats.tiff.TiffImageParser;

public abstract class ImageParser extends BinaryFileParser implements
		SanselanConstants
{

	public static final ImageParser[] getAllImageParsers()
	{
		ImageParser result[] = { new JpegImageParser(), new TiffImageParser(),
				new PngImageParser() 
//				new BmpImageParser(),
//				new GifImageParser(), new PsdImageParser(),
//				new PNMImageParser(), new IcoImageParser(),
		// new JBig2ImageParser(),
		// new TgaImageParser(),
		};

		return result;
	}

	public final IImageMetadata getMetadata(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		return getMetadata(byteSource, null);
	}

	public abstract IImageMetadata getMetadata(ByteSource byteSource, Map params)
			throws ImageReadException, IOException;

	public final IImageMetadata getMetadata(byte bytes[])
			throws ImageReadException, IOException
	{
		return getMetadata(bytes);
	}

	public final IImageMetadata getMetadata(byte bytes[], Map params)
			throws ImageReadException, IOException
	{
		return getMetadata(new ByteSourceArray(bytes), params);
	}

	public final IImageMetadata getMetadata(File file)
			throws ImageReadException, IOException
	{
		return getMetadata(file, null);
	}

	public final IImageMetadata getMetadata(File file, Map params)
			throws ImageReadException, IOException
	{
		if (debug)
			System.out.println(getName() + ".getMetadata" + ": "
					+ file.getName());

		if (!canAcceptExtension(file))
			return null;

		return getMetadata(new ByteSourceFile(file), params);
	}

	public abstract ImageInfo getImageInfo(ByteSource byteSource, Map params)
			throws ImageReadException, IOException;

	public final ImageInfo getImageInfo(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		return getImageInfo(byteSource, null);
	}

	public final ImageInfo getImageInfo(byte bytes[], Map params)
			throws ImageReadException, IOException
	{
		return getImageInfo(new ByteSourceArray(bytes), params);
	}

	public final ImageInfo getImageInfo(File file, Map params)
			throws ImageReadException, IOException
	{
		if (!canAcceptExtension(file))
			return null;

		return getImageInfo(new ByteSourceFile(file), params);
	}

	public FormatCompliance getFormatCompliance(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		return null;
	}

	public final FormatCompliance getFormatCompliance(byte bytes[])
			throws ImageReadException, IOException
	{
		return getFormatCompliance(new ByteSourceArray(bytes));
	}

	public final FormatCompliance getFormatCompliance(File file)
			throws ImageReadException, IOException
	{
		if (!canAcceptExtension(file))
			return null;

		return getFormatCompliance(new ByteSourceFile(file));
	}

//	public ArrayList getAllBufferedImages(ByteSource byteSource)
//			throws ImageReadException, IOException
//	{
//		BufferedImage bi = getBufferedImage(byteSource, null);
//
//		ArrayList result = new ArrayList();
//
//		result.add(bi);
//
//		return result;
//	}

//	public final ArrayList getAllBufferedImages(byte bytes[])
//			throws ImageReadException, IOException
//	{
//		return getAllBufferedImages(new ByteSourceArray(bytes));
//	}
//
//	public final ArrayList getAllBufferedImages(File file)
//			throws ImageReadException, IOException
//	{
//		if (!canAcceptExtension(file))
//			return null;
//
//		return getAllBufferedImages(new ByteSourceFile(file));
//	}

	// public boolean extractImages(ByteSource byteSource, File dstDir,
	// String dstRoot, ImageParser encoder) throws ImageReadException,
	// IOException, ImageWriteException
	// {
	// ArrayList v = getAllBufferedImages(byteSource);
	//
	// if (v == null)
	// return false;
	//
	// for (int i = 0; i < v.size(); i++)
	// {
	// BufferedImage image = (BufferedImage) v.get(i);
	// File file = new File(dstDir, dstRoot + "_" + i
	// + encoder.getDefaultExtension());
	// encoder.writeImage(image, new FileOutputStream(file), null);
	// }
	//
	// return false;
	// }
	//
	// public final boolean extractImages(byte bytes[], File dstDir,
	// String dstRoot, ImageParser encoder)
	//
	// throws ImageReadException, IOException, ImageWriteException
	// {
	// return extractImages(new ByteSourceArray(bytes), dstDir, dstRoot,
	// encoder);
	// }
	//
	// public final boolean extractImages(File file, File dstDir,
	// String dstRoot, ImageParser encoder)
	//
	// throws ImageReadException, IOException, ImageWriteException
	// {
	// if (!canAcceptExtension(file))
	// return false;
	//
	// return extractImages(new ByteSourceFile(file), dstDir, dstRoot,
	// encoder);
	// }

//	public abstract BufferedImage getBufferedImage(ByteSource byteSource,
//			Map params) throws ImageReadException, IOException;
//
//	public final BufferedImage getBufferedImage(byte bytes[], Map params)
//			throws ImageReadException, IOException
//	{
//		return getBufferedImage(new ByteSourceArray(bytes), params);
//	}
//
//	public final BufferedImage getBufferedImage(File file, Map params)
//			throws ImageReadException, IOException
//	{
//		if (!canAcceptExtension(file))
//			return null;
//
//		return getBufferedImage(new ByteSourceFile(file), params);
//	}
//
//	public void writeImage(BufferedImage src, OutputStream os, Map params)
//			throws ImageWriteException, IOException
//	{
//		try
//		{
//			os.close(); // we are obligated to close stream.
//		} catch (Exception e)
//		{
//			Debug.debug(e);
//		}
//
//		throw new ImageWriteException("This image format (" + getName()
//				+ ") cannot be written.");
//	}
//
//	public final Dimension getImageSize(byte bytes[])
//			throws ImageReadException, IOException
//	{
//		return getImageSize(bytes, null);
//	}
//
//	public final Dimension getImageSize(byte bytes[], Map params)
//			throws ImageReadException, IOException
//	{
//		return getImageSize(new ByteSourceArray(bytes), params);
//	}
//
//	public final Dimension getImageSize(File file) throws ImageReadException,
//			IOException
//	{
//
//		return getImageSize(file, null);
//	}
//
//	public final Dimension getImageSize(File file, Map params)
//			throws ImageReadException, IOException
//	{
//
//		if (!canAcceptExtension(file))
//			return null;
//
//		return getImageSize(new ByteSourceFile(file), params);
//	}
//
//	public abstract Dimension getImageSize(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException;

	public abstract String getXmpXml(ByteSource byteSource, Map params)
			throws ImageReadException, IOException;

	public final byte[] getICCProfileBytes(byte bytes[])
			throws ImageReadException, IOException
	{
		return getICCProfileBytes(bytes, null);
	}

	public final byte[] getICCProfileBytes(byte bytes[], Map params)
			throws ImageReadException, IOException
	{
		return getICCProfileBytes(new ByteSourceArray(bytes), params);
	}

	public final byte[] getICCProfileBytes(File file)
			throws ImageReadException, IOException
	{
		return getICCProfileBytes(file, null);
	}

	public final byte[] getICCProfileBytes(File file, Map params)
			throws ImageReadException, IOException
	{
		if (!canAcceptExtension(file))
			return null;

		if (debug)
			System.out.println(getName() + ": " + file.getName());

		return getICCProfileBytes(new ByteSourceFile(file), params);
	}

	public abstract byte[] getICCProfileBytes(ByteSource byteSource, Map params)
			throws ImageReadException, IOException;

	public final String dumpImageFile(byte bytes[]) throws ImageReadException,
			IOException
	{
		return dumpImageFile(new ByteSourceArray(bytes));
	}

	public final String dumpImageFile(File file) throws ImageReadException,
			IOException
	{
		if (!canAcceptExtension(file))
			return null;

		if (debug)
			System.out.println(getName() + ": " + file.getName());

		return dumpImageFile(new ByteSourceFile(file));
	}

	public final String dumpImageFile(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		StringWriter sw = new StringWriter();
		PrintWriter pw = new PrintWriter(sw);

		dumpImageFile(pw, byteSource);

		pw.flush();

		return sw.toString();
	}

	public boolean dumpImageFile(PrintWriter pw, ByteSource byteSource)
			throws ImageReadException, IOException
	{
		return false;
	}

	public abstract boolean embedICCProfile(File src, File dst, byte profile[]);

	public abstract String getName();

	public abstract String getDefaultExtension();

	protected abstract String[] getAcceptedExtensions();

	protected abstract ImageFormat[] getAcceptedTypes();

	public boolean canAcceptType(ImageFormat type)
	{
		ImageFormat types[] = getAcceptedTypes();

		for (int i = 0; i < types.length; i++)
			if (types[i].equals(type))
				return true;
		return false;
	}

	protected final boolean canAcceptExtension(File file)
	{
		return canAcceptExtension(file.getName());
	}

	protected final boolean canAcceptExtension(String filename)
	{
		String exts[] = getAcceptedExtensions();
		if (exts == null)
			return true;

		int index = filename.lastIndexOf('.');
		if (index >= 0)
		{
			String ext = filename.substring(index);
			ext = ext.toLowerCase();

			for (int i = 0; i < exts.length; i++)
				if (exts[i].toLowerCase().equals(ext))
					return true;
		}
		return false;
	}

//	protected IBufferedImageFactory getBufferedImageFactory(Map params)
//	{
//		if (params == null)
//			return new SimpleBufferedImageFactory();
//
//		IBufferedImageFactory result = (IBufferedImageFactory) params
//				.get(SanselanConstants.BUFFERED_IMAGE_FACTORY);
//
//		if (null != result)
//			return result;
//
//		return new SimpleBufferedImageFactory();
//	}

	public static final boolean isStrict(Map params)
	{
		if (params == null || !params.containsKey(PARAM_KEY_STRICT))
			return false;
		return ((Boolean) params.get(PARAM_KEY_STRICT)).booleanValue();
	}
}