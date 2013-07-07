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

import org.apache.sanselan.ImageWriteException;
import org.apache.sanselan.formats.tiff.constants.TagInfo;
import org.apache.sanselan.formats.tiff.constants.TiffConstants;
import org.apache.sanselan.util.Debug;

public final class TiffOutputSet implements TiffConstants
{
	public final int byteOrder;
	private final ArrayList directories = new ArrayList();

	public TiffOutputSet()
	{
		this(TiffConstants.DEFAULT_TIFF_BYTE_ORDER);
	}

	public TiffOutputSet(final int byteOrder)
	{
		super();
		this.byteOrder = byteOrder;
	}

	protected List getOutputItems(TiffOutputSummary outputSummary)
			throws ImageWriteException
	{
		List result = new ArrayList();

		for (int i = 0; i < directories.size(); i++)
		{
			TiffOutputDirectory directory = (TiffOutputDirectory) directories
					.get(i);

			result.addAll(directory.getOutputItems(outputSummary));
		}

		return result;
	}

	public void addDirectory(TiffOutputDirectory directory)
			throws ImageWriteException
	{
		if (null != findDirectory(directory.type))
			throw new ImageWriteException(
					"Output set already contains a directory of that type.");
		directories.add(directory);
	}

	public List getDirectories()
	{
		return new ArrayList(directories);
	}

	public TiffOutputDirectory getRootDirectory()
	{
		return findDirectory(DIRECTORY_TYPE_ROOT);
	}

	public TiffOutputDirectory getExifDirectory()
	{
		return findDirectory(DIRECTORY_TYPE_EXIF);
	}

	public TiffOutputDirectory getOrCreateRootDirectory()
			throws ImageWriteException
	{
		TiffOutputDirectory result = findDirectory(DIRECTORY_TYPE_ROOT);
		if (null != result)
			return result;
		return addRootDirectory();
	}

	public TiffOutputDirectory getOrCreateExifDirectory()
			throws ImageWriteException
	{
		// EXIF directory requires root directory.
		getOrCreateRootDirectory();

		TiffOutputDirectory result = findDirectory(DIRECTORY_TYPE_EXIF);
		if (null != result)
			return result;
		return addExifDirectory();
	}

	public TiffOutputDirectory getOrCreateGPSDirectory()
			throws ImageWriteException
	{
		// GPS directory requires EXIF directory
		getOrCreateExifDirectory();

		TiffOutputDirectory result = findDirectory(DIRECTORY_TYPE_GPS);
		if (null != result)
			return result;
		return addGPSDirectory();
	}

	public TiffOutputDirectory getGPSDirectory()
	{
		return findDirectory(DIRECTORY_TYPE_GPS);
	}

	public TiffOutputDirectory getInteroperabilityDirectory()
	{
		return findDirectory(DIRECTORY_TYPE_INTEROPERABILITY);
	}

	public TiffOutputDirectory findDirectory(int directoryType)
	{
		for (int i = 0; i < directories.size(); i++)
		{
			TiffOutputDirectory directory = (TiffOutputDirectory) directories
					.get(i);
			if (directory.type == directoryType)
				return directory;
		}
		return null;
	}

	/**
	 * A convenience method to update GPS values in EXIF metadata.
	 * 
	 * @param longitude Longitude in degrees E, negative values are W.
	 * @param latitude latitude in degrees N, negative values are S.
	 * @throws ImageWriteException
	 */
	public void setGPSInDegrees(double longitude, double latitude)
			throws ImageWriteException
	{
		TiffOutputDirectory gpsDirectory = getOrCreateGPSDirectory();

		String longitudeRef = longitude < 0 ? "W" : "E";
		longitude = Math.abs(longitude);
		String latitudeRef = latitude < 0 ? "S" : "N";
		latitude = Math.abs(latitude);

		{
			TiffOutputField longitudeRefField = TiffOutputField.create(
					TiffConstants.GPS_TAG_GPS_LONGITUDE_REF, byteOrder,
					longitudeRef);
			gpsDirectory.removeField(TiffConstants.GPS_TAG_GPS_LONGITUDE_REF);
			gpsDirectory.add(longitudeRefField);
		}

		{
			TiffOutputField latitudeRefField = TiffOutputField.create(
					TiffConstants.GPS_TAG_GPS_LATITUDE_REF, byteOrder,
					latitudeRef);
			gpsDirectory.removeField(TiffConstants.GPS_TAG_GPS_LATITUDE_REF);
			gpsDirectory.add(latitudeRefField);
		}

		{
			double value = longitude;
			double longitudeDegrees = (long) value;
			value %= 1;
			value *= 60.0;
			double longitudeMinutes = (long) value;
			value %= 1;
			value *= 60.0;
			double longitudeSeconds = value;
			Double values[] = {
					new Double(longitudeDegrees), new Double(longitudeMinutes),
					new Double(longitudeSeconds),
			};

			TiffOutputField longitudeField = TiffOutputField.create(
					TiffConstants.GPS_TAG_GPS_LONGITUDE, byteOrder, values);
			gpsDirectory.removeField(TiffConstants.GPS_TAG_GPS_LONGITUDE);
			gpsDirectory.add(longitudeField);
		}

		{
			double value = latitude;
			double latitudeDegrees = (long) value;
			value %= 1;
			value *= 60.0;
			double latitudeMinutes = (long) value;
			value %= 1;
			value *= 60.0;
			double latitudeSeconds = value;
			Double values[] = {
					new Double(latitudeDegrees), new Double(latitudeMinutes),
					new Double(latitudeSeconds),
			};

			TiffOutputField latitudeField = TiffOutputField.create(
					TiffConstants.GPS_TAG_GPS_LATITUDE, byteOrder, values);
			gpsDirectory.removeField(TiffConstants.GPS_TAG_GPS_LATITUDE);
			gpsDirectory.add(latitudeField);
		}

	}

	public void removeField(TagInfo tagInfo)
	{
		removeField(tagInfo.tag);
	}

	public void removeField(int tag)
	{
		for (int i = 0; i < directories.size(); i++)
		{
			TiffOutputDirectory directory = (TiffOutputDirectory) directories
					.get(i);
			directory.removeField(tag);
		}
	}

	public TiffOutputField findField(TagInfo tagInfo)
	{
		return findField(tagInfo.tag);
	}

	public TiffOutputField findField(int tag)
	{
		for (int i = 0; i < directories.size(); i++)
		{
			TiffOutputDirectory directory = (TiffOutputDirectory) directories
					.get(i);
			TiffOutputField field = directory.findField(tag);
			if (null != field)
				return field;
		}
		return null;
	}

	public TiffOutputDirectory addRootDirectory() throws ImageWriteException
	{
		TiffOutputDirectory result = new TiffOutputDirectory(
				DIRECTORY_TYPE_ROOT);
		addDirectory(result);
		return result;
	}

	public TiffOutputDirectory addExifDirectory() throws ImageWriteException
	{
		TiffOutputDirectory result = new TiffOutputDirectory(
				DIRECTORY_TYPE_EXIF);
		addDirectory(result);
		return result;
	}

	public TiffOutputDirectory addGPSDirectory() throws ImageWriteException
	{
		TiffOutputDirectory result = new TiffOutputDirectory(DIRECTORY_TYPE_GPS);
		addDirectory(result);
		return result;
	}

	public TiffOutputDirectory addInteroperabilityDirectory()
			throws ImageWriteException
	{
		getOrCreateExifDirectory();

		TiffOutputDirectory result = new TiffOutputDirectory(
				DIRECTORY_TYPE_INTEROPERABILITY);
		addDirectory(result);
		return result;
	}

	private static final String newline = System.getProperty("line.separator");

	public String toString()
	{
		return toString(null);
	}

	public String toString(String prefix)
	{
		if (prefix == null)
			prefix = "";

		StringBuffer result = new StringBuffer();

		result.append(prefix);
		result.append("TiffOutputSet {");
		result.append(newline);

		result.append(prefix);
		result.append("byteOrder: " + byteOrder);
		result.append(newline);

		for (int i = 0; i < directories.size(); i++)
		{
			TiffOutputDirectory directory = (TiffOutputDirectory) directories
					.get(i);
			result.append(prefix);
			result.append("\t" + "directory " + i + ": "
					+ directory.description() + " (" + directory.type + ")");
			result.append(newline);

			ArrayList fields = directory.getFields();
			for (int j = 0; j < fields.size(); j++)
			{
				TiffOutputField field = (TiffOutputField) fields.get(j);
				result.append(prefix);
				result.append("\t\t" + "field " + i + ": " + field.tagInfo);
				result.append(newline);
			}
		}

		result.append(prefix);
		result.append("}");
		result.append(newline);

		return result.toString();
	}

	public void dump()
	{
		Debug.debug(this.toString());
	}

}