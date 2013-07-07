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

import java.text.DecimalFormat;
import java.text.NumberFormat;

public class RationalNumber extends Number
{
	private static final long serialVersionUID = -1;

	public final int numerator;
	public final int divisor;

	public RationalNumber(int numerator, int divisor)
	{
		this.numerator = numerator;
		this.divisor = divisor;
	}

	public static final RationalNumber factoryMethod(long n, long d)
	{
		// safer than constructor - handles values outside min/max range.
		// also does some simple finding of common denominators.

		if (n > Integer.MAX_VALUE || n < Integer.MIN_VALUE
				|| d > Integer.MAX_VALUE || d < Integer.MIN_VALUE)
		{
			while ((n > Integer.MAX_VALUE || n < Integer.MIN_VALUE
					|| d > Integer.MAX_VALUE || d < Integer.MIN_VALUE)
					&& (Math.abs(n) > 1) && (Math.abs(d) > 1))
			{
				// brutal, inprecise truncation =(
				// use the sign-preserving right shift operator.
				n >>= 1;
				d >>= 1;
			}

			if (d == 0)
				throw new NumberFormatException("Invalid value, numerator: "
						+ n + ", divisor: " + d);
		}

		long gcd = gcd(n, d);
		d = d / gcd;
		n = n / gcd;

		return new RationalNumber((int) n, (int) d);
	}

	/**
	 * Return the greatest common divisor
	 */
	private static long gcd(long a, long b)
	{

		if (b == 0)
			return a;
		else
			return gcd(b, a % b);
	}

	public RationalNumber negate()
	{
		return new RationalNumber(-numerator, divisor);
	}

	public double doubleValue()
	{
		return (double) numerator / (double) divisor;
	}

	public float floatValue()
	{
		return (float) numerator / (float) divisor;
	}

	public int intValue()
	{
		return (int) numerator / (int) divisor;
	}

	public long longValue()
	{
		return (long) numerator / (long) divisor;
	}

	public boolean isValid()
	{
		return divisor != 0;
	}

	private static final NumberFormat nf = DecimalFormat.getInstance();

	public String toString()
	{
		if (divisor == 0)
			return "Invalid rational (" + numerator + "/" + divisor + ")";
		if ((numerator % divisor) == 0)
			return nf.format(numerator / divisor);
		return numerator + "/" + divisor + " ("
				+ nf.format((double) numerator / divisor) + ")";
	}

	public String toDisplayString()
	{
		if ((numerator % divisor) == 0)
			return "" + (numerator / divisor);
		NumberFormat nf = DecimalFormat.getInstance();
		nf.setMaximumFractionDigits(3);
		return nf.format((double) numerator / (double) divisor);
	}
}