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
package org.apache.sanselan.common.byteSources;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;

import org.apache.sanselan.util.Debug;

public class ByteSourceFile extends ByteSource
{
	private final File file;

	public ByteSourceFile(File file)
	{
		super(file.getName());
		this.file = file;
	}

	public InputStream getInputStream() throws IOException
	{
		FileInputStream is = null;
		BufferedInputStream bis = null;
		is = new FileInputStream(file);
		bis = new BufferedInputStream(is);
		return bis;
	}

	public byte[] getBlock(int start, int length) throws IOException
	{
		RandomAccessFile raf = null;
		try
		{
			raf = new RandomAccessFile(file, "r");

			return getRAFBytes(raf, start, length,
					"Could not read value from file");
		}
		finally
		{
			try
			{
				raf.close();
			}
			catch (Exception e)
			{
				Debug.debug(e);
			}

		}
	}

	public long getLength()
	{
		return file.length();
	}

	public byte[] getAll() throws IOException
	{
		ByteArrayOutputStream baos = new ByteArrayOutputStream();

		InputStream is = null;
		try
		{
			is = new FileInputStream(file);
			is = new BufferedInputStream(is);
			byte buffer[] = new byte[1024];
			int read;
			while ((read = is.read(buffer)) > 0)
			{
				baos.write(buffer, 0, read);
			}
			return baos.toByteArray();
		}
		finally
		{
			try
			{
				if (null != is)
					is.close();
			}
			catch (IOException e)
			{
				//				Debug.d
			}
		}
	}

	public String getDescription()
	{
		return "File: '" + file.getAbsolutePath() + "'";
	}

}