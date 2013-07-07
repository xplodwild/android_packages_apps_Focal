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
package org.apache.sanselan.util;

//import java.awt.Dimension;
//import java.awt.Point;
//import java.awt.Rectangle;
//import java.awt.color.ICC_Profile;
import java.io.File;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.Map;

public final class Debug
{

	public static void debug(String message)
	{
		System.out.println(message);
	}

	public static void debug(Object o)
	{
		System.out.println(o == null ? "null" : o.toString());
	}

	public static String getDebug(String message)
	{
		return message;
	}

	public static void debug()
	{
		newline();
	}

	public static void newline()
	{
		System.out.print(newline);
	}

	public static String getDebug(String message, int value)
	{
		return getDebug(message + ": " + (value));
	}

	public static String getDebug(String message, double value)
	{
		return getDebug(message + ": " + (value));
	}

	public static String getDebug(String message, String value)
	{
		return getDebug(message + " " + value);
	}

	public static String getDebug(String message, long value)
	{
		return getDebug(message + " " + Long.toString(value));
	}

	public static String getDebug(String message, int v[])
	{
		StringBuffer result = new StringBuffer();

		if (v == null)
			result.append(message + " (" + null + ")" + newline);
		else
		{
			result.append(message + " (" + v.length + ")" + newline);
			for (int i = 0; i < v.length; i++)
				result.append("\t" + v[i] + newline);
			result.append(newline);
		}
		return result.toString();
	}

	public static String getDebug(String message, byte v[])
	{
		final int max = 250;
		return getDebug(message, v, max);
	}

	public static String getDebug(String message, byte v[], int max)
	{

		StringBuffer result = new StringBuffer();

		if (v == null)
			result.append(message + " (" + null + ")" + newline);
		else
		{
			result.append(message + " (" + v.length + ")" + newline);
			for (int i = 0; i < max && i < v.length; i++)
			{
				int b = 0xff & v[i];

				char c;
				if (b == 0 || b == 10 || b == 11 || b == 13)
					c = ' ';
				else
					c = (char) b;

				result.append("\t" + i + ": " + b + " (" + c + ", 0x"
						+ Integer.toHexString(b) + ")" + newline);
			}
			if (v.length > max)
				result.append("\t" + "..." + newline);

			result.append(newline);
		}
		return result.toString();
	}

	public static String getDebug(String message, char v[])
	{
		StringBuffer result = new StringBuffer();

		if (v == null)
			result.append(getDebug(message + " (" + null + ")") + newline);
		else
		{
			result.append(getDebug(message + " (" + v.length + ")") + newline);
			for (int i = 0; i < v.length; i++)
				result.append(getDebug("\t" + v[i] + " (" + (0xff & v[i]))
						+ ")" + newline);
			result.append(newline);
		}
		return result.toString();
	}

	private static long counter = 0;

	public static String getDebug(String message, java.util.List v)
	{
		StringBuffer result = new StringBuffer();

		String suffix = " [" + counter++ + "]";

		result.append(getDebug(message + " (" + v.size() + ")" + suffix)
				+ newline);
		for (int i = 0; i < v.size(); i++)
			result.append(getDebug("\t" + v.get(i).toString() + suffix)
					+ newline);
		result.append(newline);

		return result.toString();
	}

	public static void debug(String message, Map map)
	{
		debug(getDebug(message, map));
	}

	public static String getDebug(String message, Map map)
	{
		StringBuffer result = new StringBuffer();

		if (map == null)
			return getDebug(message + " map: " + null);

		ArrayList keys = new ArrayList(map.keySet());
		result.append(getDebug(message + " map: " + keys.size()) + newline);
		for (int i = 0; i < keys.size(); i++)
		{
			Object key = keys.get(i);
			Object value = map.get(key);
			result.append(getDebug("\t" + i + ": '" + key + "' -> '" + value
					+ "'")
					+ newline);
		}

		result.append(newline);

		return result.toString();
	}

	public static boolean compare(String prefix, Map a, Map b)
	{
		return compare(prefix, a, b, null, null);
	}

	//	public static String newline = System.getProperty("line.separator");
	public static String newline = "\r\n";

	private static void log(StringBuffer buffer, String s)
	{
		Debug.debug(s);
		if (buffer != null)
			buffer.append(s + newline);
	}

	public static boolean compare(String prefix, Map a, Map b,
			ArrayList ignore, StringBuffer buffer)
	{
		if ((a == null) && (b == null))
		{
			log(buffer, prefix + " both maps null");
			return true;
		}
		if (a == null)
		{
			log(buffer, prefix + " map a: null, map b: map");
			return false;
		}
		if (b == null)
		{
			log(buffer, prefix + " map a: map, map b: null");
			return false;
		}

		ArrayList keys_a = new ArrayList(a.keySet());
		ArrayList keys_b = new ArrayList(b.keySet());

		if (ignore != null)
		{
			keys_a.removeAll(ignore);
			keys_b.removeAll(ignore);
		}

		boolean result = true;

		for (int i = 0; i < keys_a.size(); i++)
		{
			Object key = keys_a.get(i);
			if (!keys_b.contains(key))
			{
				log(buffer, prefix + "b is missing key '" + key + "' from a");
				result = false;
			}
			else
			{
				keys_b.remove(key);
				Object value_a = a.get(key);
				Object value_b = b.get(key);
				if (!value_a.equals(value_b))
				{
					log(buffer, prefix + "key(" + key + ") value a: " + value_a
							+ ") !=  b: " + value_b + ")");
					result = false;
				}
			}
		}
		for (int i = 0; i < keys_b.size(); i++)
		{
			Object key = keys_b.get(i);

			log(buffer, prefix + "a is missing key '" + key + "' from b");
			result = false;
		}

		if (result)
			log(buffer, prefix + "a is the same as  b");

		return result;
	}

	private static final String byteQuadToString(int bytequad)
	{
		byte b1 = (byte) ((bytequad >> 24) & 0xff);
		byte b2 = (byte) ((bytequad >> 16) & 0xff);
		byte b3 = (byte) ((bytequad >> 8) & 0xff);
		byte b4 = (byte) ((bytequad >> 0) & 0xff);

		char c1 = (char) b1;
		char c2 = (char) b2;
		char c3 = (char) b3;
		char c4 = (char) b4;
		//		return new String(new char[] { c1, c2, c3, c4 });
		StringBuffer fStringBuffer = new StringBuffer();
		fStringBuffer.append(new String(new char[]{
				c1, c2, c3, c4
		}));
		fStringBuffer.append(" bytequad: " + bytequad);
		fStringBuffer.append(" b1: " + b1);
		fStringBuffer.append(" b2: " + b2);
		fStringBuffer.append(" b3: " + b3);
		fStringBuffer.append(" b4: " + b4);

		return fStringBuffer.toString();
	}

//	public static String getDebug(String message)
//	{
//
//		StringBuffer result = new StringBuffer();
//
//		result.append(getDebug("ICC_Profile " + message + ": "
//				+ ((value == null) ? "null" : value.toString()))
//				+ newline);
//		if (value != null)
//		{
//			result.append(getDebug("\t getProfileClass: "
//					+ byteQuadToString(value.getProfileClass()))
//					+ newline);
//			result.append(getDebug("\t getPCSType: "
//					+ byteQuadToString(value.getPCSType()))
//					+ newline);
//			result.append(getDebug("\t getColorSpaceType() : "
//					+ byteQuadToString(value.getColorSpaceType()))
//					+ newline);
//		}
//
//		return result.toString();
//
//	}

	public static String getDebug(String message, boolean value)
	{
		return getDebug(message + " " + ((value) ? ("true") : ("false")));
	}

	public static String getDebug(String message, File file)
	{
		return getDebug(message + ": "
				+ ((file == null) ? "null" : file.getPath()));
	}

	public static String getDebug(String message, Date value)
	{
		DateFormat df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
		return getDebug(message, (value == null) ? "null" : df.format(value));
	}

	public static String getDebug(String message, Calendar value)
	{
		DateFormat df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
		return getDebug(message, (value == null) ? "null" : df.format(value
				.getTime()));
	}

	public static void debug(String message, Object value)
	{
		if (value == null)
			debug(message, "null");
		else if (value instanceof char[])
			debug(message, (char[]) value);
		else if (value instanceof byte[])
			debug(message, (byte[]) value);
		else if (value instanceof int[])
			debug(message, (int[]) value);
		else if (value instanceof String)
			debug(message, (String) value);
		else if (value instanceof java.util.List)
			debug(message, (java.util.List) value);
		else if (value instanceof Map)
			debug(message, (Map) value);
		//		else if (value instanceof Object)
		//			debug(message, (Object) value);
//		else if (value instanceof ICC_Profile)
//			debug(message, (ICC_Profile) value);
		else if (value instanceof File)
			debug(message, (File) value);
		else if (value instanceof Date)
			debug(message, (Date) value);
		else if (value instanceof Calendar)
			debug(message, (Calendar) value);
		else
			debug(message, value.toString());
	}

	public static void debug(String message, Object value[])
	{
		if (value == null)
			debug(message, "null");

		debug(message, value.length);
		final int max = 10;
		for (int i = 0; i < value.length && i < max; i++)
			debug("\t" + i, value[i]);
		if (value.length > max)
			debug("\t...");
		debug();
	}

	public static String getDebug(String message, Object value)
	{
		if (value == null)
			return getDebug(message, "null");
		else if (value instanceof Calendar)
			return getDebug(message, (Calendar) value);
		else if (value instanceof Date)
			return getDebug(message, (Date) value);
		else if (value instanceof File)
			return getDebug(message, (File) value);
//		else if (value instanceof ICC_Profile)
//			return getDebug(message, (ICC_Profile) value);
		else if (value instanceof Map)
			return getDebug(message, (Map) value);
		else if (value instanceof Map)
			return getDebug(message, (Map) value); //
		//		else if (value instanceof Object) // getDebug(message, (Object) value);
		else if (value instanceof String)
			return getDebug(message, (String) value);
		else if (value instanceof byte[])
			return getDebug(message, (byte[]) value);
		else if (value instanceof char[])
			return getDebug(message, (char[]) value);
		else if (value instanceof int[])
			return getDebug(message, (int[]) value);
		else if (value instanceof java.util.List)
			return getDebug(message, (java.util.List) value);
		else
			return getDebug(message, value.toString());
	}

	public static String getType(Object value)
	{
		if (value == null)
			return "null";
		else if (value instanceof Object[])
			return "[Object[]: " + ((Object[]) value).length + "]";
		else if (value instanceof char[])
			return "[char[]: " + ((char[]) value).length + "]";
		else if (value instanceof byte[])
			return "[byte[]: " + ((byte[]) value).length + "]";
		else if (value instanceof short[])
			return "[short[]: " + ((short[]) value).length + "]";
		else if (value instanceof int[])
			return "[int[]: " + ((int[]) value).length + "]";
		else if (value instanceof long[])
			return "[long[]: " + ((long[]) value).length + "]";
		else if (value instanceof float[])
			return "[float[]: " + ((float[]) value).length + "]";
		else if (value instanceof double[])
			return "[double[]: " + ((double[]) value).length + "]";
		else if (value instanceof boolean[])
			return "[boolean[]: " + ((boolean[]) value).length + "]";
		else
			return value.getClass().getName();
	}

	public static boolean isArray(Object value)
	{
		if (value == null)
			return false;
		else if (value instanceof Object[])
			return true;
		else if (value instanceof char[])
			return true;
		else if (value instanceof byte[])
			return true;
		else if (value instanceof short[])
			return true;
		else if (value instanceof int[])
			return true;
		else if (value instanceof long[])
			return true;
		else if (value instanceof float[])
			return true;
		else if (value instanceof double[])
			return true;
		else if (value instanceof boolean[])
			return true;
		else
			return false;
	}

	public static String getDebug(String message, Object value[])
	{
		StringBuffer result = new StringBuffer();

		if (value == null)
			result.append(getDebug(message, "null") + newline);

		result.append(getDebug(message, value.length));
		final int max = 10;
		for (int i = 0; i < value.length && i < max; i++)
			result.append(getDebug("\t" + i, value[i]) + newline);
		if (value.length > max)
			result.append(getDebug("\t...") + newline);
		result.append(newline);

		return result.toString();
	}

	public static String getDebug(Class fClass, Throwable e)
	{
		return getDebug(fClass == null ? "[Unknown]" : fClass.getName(), e);
	}

	public static void debug(Class fClass, Throwable e)
	{
		debug(fClass.getName(), e);
	}

	private static final SimpleDateFormat timestamp = new SimpleDateFormat(
			"yyyy-MM-dd kk:mm:ss:SSS");

	public static void debug(String message, boolean value)
	{
		debug(message + " " + ((value) ? ("true") : ("false")));
	}

	public static void debug(String message, byte v[])
	{
		debug(getDebug(message, v));
	}

	public static void debug(String message, char v[])
	{
		debug(getDebug(message, v));
	}

	public static void debug(String message, Calendar value)
	{
		DateFormat df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
		debug(message, (value == null) ? "null" : df.format(value.getTime()));
	}

	public static void debug(String message, Date value)
	{
		DateFormat df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
		debug(message, (value == null) ? "null" : df.format(value));
	}

	public static void debug(String message, double value)
	{
		debug(message + ": " + (value));
	}

	public static void debug(String message, File file)
	{
		debug(message + ": " + ((file == null) ? "null" : file.getPath()));
	}

	//	public static void debug(String message, Object value)
	//	{
	//		debug("Unknown Object " + message + ": "
	//				+ ((value == null) ? "null" : value.toString()));
	//	}

//	public static void debug(String message, ICC_Profile value)
//	{
//		debug("ICC_Profile " + message + ": "
//				+ ((value == null) ? "null" : value.toString()));
//		if (value != null)
//		{
//			debug("\t getProfileClass: "
//					+ byteQuadToString(value.getProfileClass()));
//			debug("\t getPCSType: " + byteQuadToString(value.getPCSType()));
//			debug("\t getColorSpaceType() : "
//					+ byteQuadToString(value.getColorSpaceType()));
//		}
//	}

	public static void debug(String message, int value)
	{
		debug(message + ": " + (value));
	}

	public static void debug(String message, int v[])
	{
		debug(getDebug(message, v));
	}

	public static void debug(String message, byte v[], int max)
	{
		debug(getDebug(message, v, max));
	}

	public static void debug(String message, java.util.List v)
	{
		String suffix = " [" + counter++ + "]";

		debug(message + " (" + v.size() + ")" + suffix);
		for (int i = 0; i < v.size(); i++)
			debug("\t" + v.get(i).toString() + suffix);
		debug();
	}

	public static void debug(String message, long value)
	{
		debug(message + " " + Long.toString(value));
	}

//	public static void debug(String prefix, Point p)
//	{
//		System.out.println(prefix + ": "
//				+ ((p == null) ? "null" : (p.x + ", " + p.y)));
//	}
//
//	public static void debug(String prefix, Rectangle r)
//	{
//		debug(getDebug(prefix, r));
//	}

	public static void debug(String message, String value)
	{
		debug(message + " " + value);
	}

	public static void debug(String message, Throwable e)
	{
		debug(getDebug(message, e));
	}

	public static void debug(Throwable e)
	{
		debug(getDebug(e));
	}

	public static void debug(Throwable e, int value)
	{
		debug(getDebug(e, value));
	}

	public static void dumpStack()
	{
		debug(getStackTrace(new Exception("Stack trace"), -1, 1));
	}

	public static void dumpStack(int limit)
	{
		debug(getStackTrace(new Exception("Stack trace"), limit, 1));
	}

	public static String getDebug(String message, Throwable e)
	{
		return message + newline + getDebug(e);
	}

	public static String getDebug(Throwable e)
	{
		return getDebug(e, -1);
	}

	public static String getDebug(Throwable e, int max)
	{
		StringBuffer result = new StringBuffer();

		String datetime = timestamp.format(new Date()).toLowerCase();

		result.append(newline);
		result.append("Throwable: "
				+ ((e == null) ? "" : ("(" + e.getClass().getName() + ")"))
				+ ":" + datetime + newline);
		result.append("Throwable: "
				+ ((e == null) ? "null" : e.getLocalizedMessage()) + newline);
		result.append(newline);

		result.append(getStackTrace(e, max));

		result.append("Caught here:" + newline);
		result.append(getStackTrace(new Exception(), max, 1));
		//		Debug.dumpStack();
		result.append(newline);
		return result.toString();
	}

	public static String getStackTrace(Throwable e)
	{
		return getStackTrace(e, -1);
	}

	public static String getStackTrace(Throwable e, int limit)
	{
		return getStackTrace(e, limit, 0);
	}

	public static String getStackTrace(Throwable e, int limit, int skip)
	{
		StringBuffer result = new StringBuffer();

		if (e != null)
		{
			StackTraceElement stes[] = e.getStackTrace();
			if (stes != null)
			{
				for (int i = skip; i < stes.length && (limit < 0 || i < limit); i++)
				{
					StackTraceElement ste = stes[i];

					result.append("\tat " + ste.getClassName() + "."
							+ ste.getMethodName() + "(" + ste.getFileName()
							+ ":" + ste.getLineNumber() + ")" + newline);
				}
				if (limit >= 0 && stes.length > limit)
					result.append("\t..." + newline);
			}

			//			e.printStackTrace(System.out);
			result.append(newline);
		}

		return result.toString();
	}

	public static void debugByteQuad(String message, int i)
	{
		int alpha = (i >> 24) & 0xff;
		int red = (i >> 16) & 0xff;
		int green = (i >> 8) & 0xff;
		int blue = (i >> 0) & 0xff;

		System.out.println(message + ": " + "alpha: " + alpha + ", " + "red: "
				+ red + ", " + "green: " + green + ", " + "blue: " + blue);
	}

	public static void debugIPQuad(String message, int i)
	{
		int b1 = (i >> 24) & 0xff;
		int b2 = (i >> 16) & 0xff;
		int b3 = (i >> 8) & 0xff;
		int b4 = (i >> 0) & 0xff;

		System.out.println(message + ": " + "b1: " + b1 + ", " + "b2: " + b2
				+ ", " + "b3: " + b3 + ", " + "b4: " + b4);
	}

	public static void debugIPQuad(String message, byte bytes[])
	{
		System.out.print(message + ": ");
		if (bytes == null)
			System.out.print("null");
		else
		{
			for (int i = 0; i < bytes.length; i++)
			{
				if (i > 0)
					System.out.print(".");
				System.out.print(0xff & bytes[i]);
			}
		}
		System.out.println();
	}

//	public static String getDebug(String prefix, Dimension r)
//	{
//		String s_ar1 = "null";
//		String s_ar2 = "null";
//
//		if (r != null)
//		{
//			double aspect_ratio = ((double) r.width) / ((double) r.height);
//			double aspect_ratio2 = 1.0 / aspect_ratio;
//
//			s_ar1 = "" + aspect_ratio;
//			s_ar2 = "" + aspect_ratio2;
//
//			if (s_ar1.length() > 7)
//				s_ar1 = s_ar1.substring(0, 7);
//			if (s_ar2.length() > 7)
//				s_ar2 = s_ar2.substring(0, 7);
//		}
//
//		return (prefix + ": "
//				+ ((r == null) ? "null" : (r.width + "x" + r.height))
//				+ " aspect_ratio: " + s_ar1 + " (" + s_ar2 + ")");
//	}

//	public static void debug(String prefix, Dimension r)
//	{
//		debug(getDebug(prefix, r));
//	}
//
//	public static String getDebug(String prefix, Rectangle r)
//	{
//		String s_ar1 = "null";
//		String s_ar2 = "null";
//
//		if (r != null)
//		{
//			double aspect_ratio = ((double) r.width) / ((double) r.height);
//			double aspect_ratio2 = 1.0 / aspect_ratio;
//
//			s_ar1 = "" + aspect_ratio;
//			s_ar2 = "" + aspect_ratio2;
//
//			if (s_ar1.length() > 7)
//				s_ar1 = s_ar1.substring(0, 7);
//			if (s_ar2.length() > 7)
//				s_ar2 = s_ar2.substring(0, 7);
//		}
//
//		return (prefix
//				+ ": "
//				+ ((r == null) ? "null" : (r.x + "x" + r.y + "," + r.width
//						+ "x" + r.height)) + " aspect_ratio: " + s_ar1 + " ("
//				+ s_ar2 + ")");
//	}
//
//	public static String getDebug(String prefix, Point p)
//	{
//		return (prefix + ": " + ((p == null) ? "null" : (p.x + ", " + p.y)));
//	}

	public static void dump(String prefix, Object value)
	{
		if (value == null)
			debug(prefix, "null");
		else if (value instanceof Object[])
		{
			Object[] array = (Object[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				dump(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof int[])
		{
			int[] array = (int[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				debug(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof char[])
		{
			char[] array = (char[]) value;
			debug(prefix, "[" + new String(array) + "]");
		}
		else if (value instanceof long[])
		{
			long[] array = (long[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				debug(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof boolean[])
		{
			boolean[] array = (boolean[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				debug(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof byte[])
		{
			byte[] array = (byte[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				debug(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof float[])
		{
			float[] array = (float[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				debug(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof byte[])
		{
			double[] array = (double[]) value;
			debug(prefix, array);
			for (int i = 0; i < array.length; i++)
				debug(prefix + "\t" + i + ": ", array[i]);
		}
		else if (value instanceof java.util.List)
		{
			java.util.List list = (java.util.List) value;
			debug(prefix, "list");
			for (int i = 0; i < list.size(); i++)
				dump(prefix + "\t" + "list: " + i + ": ", list.get(i));
		}
		else if (value instanceof Map)
		{
			java.util.Map map = (java.util.Map) value;
			debug(prefix, "map");
			ArrayList keys = new ArrayList(map.keySet());
			Collections.sort(keys);
			for (int i = 0; i < keys.size(); i++)
			{
				Object key = keys.get(i);
				dump(prefix + "\t" + "map: " + key + " -> ", map.get(key));
			}
		}
		//		else if (value instanceof String)
		//			debug(prefix, value);
		else
		{
			debug(prefix, value.toString());
			debug(prefix + "\t", value.getClass().getName());
		}
	}

	public static final void purgeMemory()
	{
		try
		{
			//			Thread.sleep(50);
			System.runFinalization();
			Thread.sleep(50);
			System.gc();
			Thread.sleep(50);
		}
		catch (Throwable e)
		{
			Debug.debug(e);
		}
	}

}
