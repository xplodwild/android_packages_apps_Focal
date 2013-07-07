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

//import java.awt.Dimension;
//import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.sanselan.FormatCompliance;
import org.apache.sanselan.ImageFormat;
import org.apache.sanselan.ImageInfo;
import org.apache.sanselan.ImageParser;
import org.apache.sanselan.ImageReadException;
import org.apache.sanselan.common.IImageMetadata;
import org.apache.sanselan.common.byteSources.ByteSource;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;

public class TiffImageParser extends ImageParser implements TiffConstants
{
	public TiffImageParser()
	{
		// setDebug(true);
	}

	public String getName()
	{
		return "Tiff-Custom";
	}

	public String getDefaultExtension()
	{
		return DEFAULT_EXTENSION;
	}

	private static final String DEFAULT_EXTENSION = ".tif";

	private static final String ACCEPTED_EXTENSIONS[] = { ".tif", ".tiff", };

	protected String[] getAcceptedExtensions()
	{
		return ACCEPTED_EXTENSIONS;
	}

	protected ImageFormat[] getAcceptedTypes()
	{
		return new ImageFormat[] { ImageFormat.IMAGE_FORMAT_TIFF, //
		};
	}

	public byte[] getICCProfileBytes(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		TiffContents contents = new TiffReader(isStrict(params))
				.readFirstDirectory(byteSource, params, false, formatCompliance);
		TiffDirectory directory = (TiffDirectory) contents.directories.get(0);

		TiffField field = directory.findField(EXIF_TAG_ICC_PROFILE);
		if (null == field)
			return null;
		return field.oversizeValue;
	}

	public int[] getImageSize(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		TiffContents contents = new TiffReader(isStrict(params))
				.readFirstDirectory(byteSource, params, false, formatCompliance);
		TiffDirectory directory = (TiffDirectory) contents.directories.get(0);

		int width = directory.findField(TIFF_TAG_IMAGE_WIDTH).getIntValue();
		int height = directory.findField(TIFF_TAG_IMAGE_LENGTH).getIntValue();

		return new int[]{width, height};
	}

	public byte[] embedICCProfile(byte image[], byte profile[])
	{
		return null;
	}

	public boolean embedICCProfile(File src, File dst, byte profile[])
	{
		return false;
	}

	public IImageMetadata getMetadata(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		TiffContents contents = new TiffReader(isStrict(params)).readContents(
				byteSource, params, formatCompliance);

		ArrayList directories = contents.directories;

		TiffImageMetadata result = new TiffImageMetadata(contents);

		for (int i = 0; i < directories.size(); i++)
		{
			TiffDirectory dir = (TiffDirectory) directories.get(i);

			TiffImageMetadata.Directory metadataDirectory = new TiffImageMetadata.Directory(
					dir);

			ArrayList entries = dir.getDirectoryEntrys();

			for (int j = 0; j < entries.size(); j++)
			{
				TiffField entry = (TiffField) entries.get(j);
				metadataDirectory.add(entry);
			}

			result.add(metadataDirectory);
		}

		return result;
	}

	public ImageInfo getImageInfo(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		TiffContents contents = new TiffReader(isStrict(params))
				.readDirectories(byteSource, false, formatCompliance);
		TiffDirectory directory = (TiffDirectory) contents.directories.get(0);

		TiffField widthField = directory.findField(TIFF_TAG_IMAGE_WIDTH, true);
		TiffField heightField = directory
				.findField(TIFF_TAG_IMAGE_LENGTH, true);

		if ((widthField == null) || (heightField == null))
			throw new ImageReadException("TIFF image missing size info.");

		int height = heightField.getIntValue();
		int width = widthField.getIntValue();

		// -------------------

		TiffField resolutionUnitField = directory
				.findField(TIFF_TAG_RESOLUTION_UNIT);
		int resolutionUnit = 2; // Inch
		if ((resolutionUnitField != null)
				&& (resolutionUnitField.getValue() != null))
			resolutionUnit = resolutionUnitField.getIntValue();

		double unitsPerInch = -1;
		switch (resolutionUnit)
		{
		case 1:
			break;
		case 2: // Inch
			unitsPerInch = 1.0;
			break;
		case 3: // Meter
			unitsPerInch = 0.0254;
			break;
		default:
			break;

		}
		TiffField xResolutionField = directory.findField(TIFF_TAG_XRESOLUTION);
		TiffField yResolutionField = directory.findField(TIFF_TAG_YRESOLUTION);

		int physicalWidthDpi = -1;
		float physicalWidthInch = -1;
		int physicalHeightDpi = -1;
		float physicalHeightInch = -1;

		if (unitsPerInch > 0)
		{
			if ((xResolutionField != null)
					&& (xResolutionField.getValue() != null))
			{
				double XResolutionPixelsPerUnit = xResolutionField
						.getDoubleValue();
				physicalWidthDpi = (int) (XResolutionPixelsPerUnit / unitsPerInch);
				physicalWidthInch = (float) (width / (XResolutionPixelsPerUnit * unitsPerInch));
			}
			if ((yResolutionField != null)
					&& (yResolutionField.getValue() != null))
			{
				double YResolutionPixelsPerUnit = yResolutionField
						.getDoubleValue();
				physicalHeightDpi = (int) (YResolutionPixelsPerUnit / unitsPerInch);
				physicalHeightInch = (float) (height / (YResolutionPixelsPerUnit * unitsPerInch));
			}
		}

		// -------------------

		TiffField bitsPerSampleField = directory
				.findField(TIFF_TAG_BITS_PER_SAMPLE);

		int bitsPerSample = -1;
		if ((bitsPerSampleField != null)
				&& (bitsPerSampleField.getValue() != null))
			bitsPerSample = bitsPerSampleField.getIntValueOrArraySum();

		int bitsPerPixel = bitsPerSample; // assume grayscale;
		// dunno if this handles colormapped images correctly.

		// -------------------

		ArrayList comments = new ArrayList();
		ArrayList entries = directory.entries;
		for (int i = 0; i < entries.size(); i++)
		{
			TiffField field = (TiffField) entries.get(i);
			String comment = field.toString();
			comments.add(comment);
		}

		ImageFormat format = ImageFormat.IMAGE_FORMAT_TIFF;
		String formatName = "TIFF Tag-based Image File Format";
		String mimeType = "image/tiff";
		int numberOfImages = contents.directories.size();
		// not accurate ... only reflects first
		boolean isProgressive = false;
		// is TIFF ever interlaced/progressive?

		String formatDetails = "Tiff v." + contents.header.tiffVersion;

		boolean isTransparent = false; // TODO: wrong
		boolean usesPalette = false;
		TiffField colorMapField = directory.findField(TIFF_TAG_COLOR_MAP);
		if (colorMapField != null)
			usesPalette = true;

		int colorType = ImageInfo.COLOR_TYPE_RGB;

		int compression = directory.findField(TIFF_TAG_COMPRESSION)
				.getIntValue();
		String compressionAlgorithm;

		switch (compression)
		{
		case TIFF_COMPRESSION_UNCOMPRESSED_1:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_NONE;
			break;
		case TIFF_COMPRESSION_CCITT_1D:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_CCITT_1D;
			break;
		case TIFF_COMPRESSION_CCITT_GROUP_3:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_CCITT_GROUP_3;
			break;
		case TIFF_COMPRESSION_CCITT_GROUP_4:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_CCITT_GROUP_4;
			break;
		case TIFF_COMPRESSION_LZW:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_LZW;
			break;
		case TIFF_COMPRESSION_JPEG:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_JPEG;
			break;
		case TIFF_COMPRESSION_UNCOMPRESSED_2:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_NONE;
			break;
		case TIFF_COMPRESSION_PACKBITS:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_PACKBITS;
			break;
		default:
			compressionAlgorithm = ImageInfo.COMPRESSION_ALGORITHM_UNKNOWN;
			break;
		}

		ImageInfo result = new ImageInfo(formatDetails, bitsPerPixel, comments,
				format, formatName, height, mimeType, numberOfImages,
				physicalHeightDpi, physicalHeightInch, physicalWidthDpi,
				physicalWidthInch, width, isProgressive, isTransparent,
				usesPalette, colorType, compressionAlgorithm);

		return result;
	}

	public String getXmpXml(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		TiffContents contents = new TiffReader(isStrict(params))
				.readDirectories(byteSource, false, formatCompliance);
		TiffDirectory directory = (TiffDirectory) contents.directories.get(0);

		TiffField xmpField = directory.findField(TIFF_TAG_XMP, false);
		if (xmpField == null)
			return null;

		byte bytes[] = xmpField.getByteArrayValue();

		try
		{
			// segment data is UTF-8 encoded xml.
			String xml = new String(bytes, "utf-8");
			return xml;
		} catch (UnsupportedEncodingException e)
		{
			throw new ImageReadException("Invalid JPEG XMP Segment.");
		}
	}

	public boolean dumpImageFile(PrintWriter pw, ByteSource byteSource)
			throws ImageReadException, IOException
	{
		try
		{
			pw.println("tiff.dumpImageFile");

			{
				ImageInfo imageData = getImageInfo(byteSource);
				if (imageData == null)
					return false;

				imageData.toString(pw, "");
			}

			pw.println("");

			// try
			{
				FormatCompliance formatCompliance = FormatCompliance
						.getDefault();
				Map params = null;
				TiffContents contents = new TiffReader(true).readContents(
						byteSource, params, formatCompliance);

				ArrayList directories = contents.directories;

				if (directories == null)
					return false;

				for (int d = 0; d < directories.size(); d++)
				{
					TiffDirectory directory = (TiffDirectory) directories
							.get(d);

					ArrayList entries = directory.entries;

					if (entries == null)
						return false;

					// Debug.debug("directory offset", directory.offset);

					for (int i = 0; i < entries.size(); i++)
					{
						TiffField field = (TiffField) entries.get(i);

						field.dump(pw, d + "");
					}
				}

				pw.println("");
			}
			// catch (Exception e)
			// {
			// Debug.debug(e);
			// pw.println("");
			// return false;
			// }

			return true;
		} finally
		{
			pw.println("");
		}
	}

	public FormatCompliance getFormatCompliance(ByteSource byteSource)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		Map params = null;
		new TiffReader(isStrict(params)).readContents(byteSource, params,
				formatCompliance);
		return formatCompliance;
	}

	public List collectRawImageData(ByteSource byteSource, Map params)
			throws ImageReadException, IOException
	{
		FormatCompliance formatCompliance = FormatCompliance.getDefault();
		TiffContents contents = new TiffReader(isStrict(params))
				.readDirectories(byteSource, true, formatCompliance);

		List result = new ArrayList();
		for (int i = 0; i < contents.directories.size(); i++)
		{
			TiffDirectory directory = (TiffDirectory) contents.directories
					.get(i);
			List dataElements = directory.getTiffRawImageDataElements();
			for (int j = 0; j < dataElements.size(); j++)
			{
				TiffDirectory.ImageDataElement element = (TiffDirectory.ImageDataElement) dataElements
						.get(j);
				byte bytes[] = byteSource.getBlock(element.offset,
						element.length);
				result.add(bytes);
			}
		}
		return result;
	}

//	public BufferedImage getBufferedImage(ByteSource byteSource, Map params)
//			throws ImageReadException, IOException
//	{
//		FormatCompliance formatCompliance = FormatCompliance.getDefault();
//		TiffContents contents = new TiffReader(isStrict(params))
//				.readFirstDirectory(byteSource, params, true, formatCompliance);
//		TiffDirectory directory = (TiffDirectory) contents.directories.get(0);
//		BufferedImage result = directory.getTiffImage(params);
//		if (null == result)
//			throw new ImageReadException("TIFF does not contain an image.");
//		return result;
//	}

//	protected BufferedImage getBufferedImage(TiffDirectory directory, Map params)
//			throws ImageReadException, IOException
//	{
//		ArrayList entries = directory.entries;
//
//		if (entries == null)
//			throw new ImageReadException("TIFF missing entries");
//
//		int photometricInterpretation = directory.findField(
//				TIFF_TAG_PHOTOMETRIC_INTERPRETATION, true).getIntValue();
//		int compression = directory.findField(TIFF_TAG_COMPRESSION, true)
//				.getIntValue();
//		int width = directory.findField(TIFF_TAG_IMAGE_WIDTH, true)
//				.getIntValue();
//		int height = directory.findField(TIFF_TAG_IMAGE_LENGTH, true)
//				.getIntValue();
//		int samplesPerPixel = directory.findField(TIFF_TAG_SAMPLES_PER_PIXEL,
//				true).getIntValue();
//		int bitsPerSample[] = directory.findField(TIFF_TAG_BITS_PER_SAMPLE,
//				true).getIntArrayValue();
//		// TODO: why are we using bits per sample twice? because one is a sum.
//		int bitsPerPixel = directory.findField(TIFF_TAG_BITS_PER_SAMPLE, true)
//				.getIntValueOrArraySum();
//
//		// int bitsPerPixel = getTagAsValueOrArraySum(entries,
//		// TIFF_TAG_BITS_PER_SAMPLE);
//
//		int predictor = -1;
//		{
//			// dumpOptionalNumberTag(entries, TIFF_TAG_FILL_ORDER);
//			// dumpOptionalNumberTag(entries, TIFF_TAG_FREE_BYTE_COUNTS);
//			// dumpOptionalNumberTag(entries, TIFF_TAG_FREE_OFFSETS);
//			// dumpOptionalNumberTag(entries, TIFF_TAG_ORIENTATION);
//			// dumpOptionalNumberTag(entries, TIFF_TAG_PLANAR_CONFIGURATION);
//			TiffField predictorField = directory.findField(TIFF_TAG_PREDICTOR);
//			if (null != predictorField)
//				predictor = predictorField.getIntValueOrArraySum();
//		}
//
//		if (samplesPerPixel != bitsPerSample.length)
//			throw new ImageReadException("Tiff: samplesPerPixel ("
//					+ samplesPerPixel + ")!=fBitsPerSample.length ("
//					+ bitsPerSample.length + ")");
//
//		boolean hasAlpha = false;
//		BufferedImage result = getBufferedImageFactory(params)
//				.getColorBufferedImage(width, height, hasAlpha);
//
//		PhotometricInterpreter photometricInterpreter = getPhotometricInterpreter(
//				directory, photometricInterpretation, bitsPerPixel,
//				bitsPerSample, predictor, samplesPerPixel, width, height);
//
//		TiffImageData imageData = directory.getTiffImageData();
//
//		DataReader dataReader = imageData.getDataReader(entries,
//				photometricInterpreter, bitsPerPixel, bitsPerSample, predictor,
//				samplesPerPixel, width, height, compression);
//
//		dataReader.readImageData(result);
//
//		photometricInterpreter.dumpstats();
//
//		return result;
//	}

//	private PhotometricInterpreter getPhotometricInterpreter(
//			TiffDirectory directory, int photometricInterpretation,
//			int bitsPerPixel, int bitsPerSample[], int predictor,
//			int samplesPerPixel, int width, int height) throws IOException,
//			ImageReadException
//	{
//		switch (photometricInterpretation)
//		{
//		case 0:
//		case 1:
//			boolean invert = photometricInterpretation == 0;
//
//			return new PhotometricInterpreterBiLevel(bitsPerPixel,
//					samplesPerPixel, bitsPerSample, predictor, width, height,
//					invert);
//		case 3: // Palette
//		{
//			int colorMap[] = directory.findField(TIFF_TAG_COLOR_MAP, true)
//					.getIntArrayValue();
//
//			int expected_colormap_size = 3 * (1 << bitsPerPixel);
//
//			if (colorMap.length != expected_colormap_size)
//				throw new ImageReadException("Tiff: fColorMap.length ("
//						+ colorMap.length + ")!=expected_colormap_size ("
//						+ expected_colormap_size + ")");
//
//			return new PhotometricInterpreterPalette(samplesPerPixel,
//					bitsPerSample, predictor, width, height, colorMap);
//		}
//		case 2: // RGB
//			return new PhotometricInterpreterRGB(samplesPerPixel,
//					bitsPerSample, predictor, width, height);
//		case 5: // CMYK
//			return new PhotometricInterpreterCMYK(samplesPerPixel,
//					bitsPerSample, predictor, width, height);
//		case 6: //
//		{
//			double yCbCrCoefficients[] = directory.findField(
//					TIFF_TAG_YCBCR_COEFFICIENTS, true).getDoubleArrayValue();
//
//			int yCbCrPositioning[] = directory.findField(
//					TIFF_TAG_YCBCR_POSITIONING, true).getIntArrayValue();
//			int yCbCrSubSampling[] = directory.findField(
//					TIFF_TAG_YCBCR_SUB_SAMPLING, true).getIntArrayValue();
//
//			double referenceBlackWhite[] = directory.findField(
//					TIFF_TAG_REFERENCE_BLACK_WHITE, true).getDoubleArrayValue();
//
//			return new PhotometricInterpreterYCbCr(yCbCrCoefficients,
//					yCbCrPositioning, yCbCrSubSampling, referenceBlackWhite,
//					samplesPerPixel, bitsPerSample, predictor, width, height);
//		}
//
//		case 8:
//			return new PhotometricInterpreterCIELAB(samplesPerPixel,
//					bitsPerSample, predictor, width, height);
//
//		case 32844:
//		case 32845: {
//			boolean yonly = (photometricInterpretation == 32844);
//			return new PhotometricInterpreterLogLUV(samplesPerPixel,
//					bitsPerSample, predictor, width, height, yonly);
//		}
//
//		default:
//			throw new ImageReadException(
//					"TIFF: Unknown fPhotometricInterpretation: "
//							+ photometricInterpretation);
//		}
//	}

//	public void writeImage(BufferedImage src, OutputStream os, Map params)
//			throws ImageWriteException, IOException
//	{
//		new TiffImageWriterLossy().writeImage(src, os, params);
//	}

}
