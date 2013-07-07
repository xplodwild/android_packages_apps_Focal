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

public abstract class RationalNumberUtilities extends Number
{

	private static class Option
	{
		public final RationalNumber rationalNumber;
		public final double error;

		private Option(final RationalNumber rationalNumber, final double error)
		{
			this.rationalNumber = rationalNumber;
			this.error = error;
		}

		public static final Option factory(final RationalNumber rationalNumber,
				final double value)
		{
			return new Option(rationalNumber, Math.abs(rationalNumber
					.doubleValue()
					- value));
		}

		public String toString()
		{
			return rationalNumber.toString();
		}
	}

	// int-precision tolerance
	private static final double TOLERANCE = 1E-8;

	//
	// calculate rational number using successive approximations.
	// 
	public static final RationalNumber getRationalNumber(double value)
	{
		if (value >= Integer.MAX_VALUE)
			return new RationalNumber(Integer.MAX_VALUE, 1);
		else if (value <= -Integer.MAX_VALUE)
			return new RationalNumber(-Integer.MAX_VALUE, 1);

		boolean negative = false;
		if (value < 0)
		{
			negative = true;
			value = Math.abs(value);
		}

		Option low;
		Option high;
		{
			RationalNumber l, h;

			if (value == 0)
				return new RationalNumber(0, 1);
			else if (value >= 1)
			{
				int approx = (int) value;
				if (approx < value)
				{
					l = new RationalNumber(approx, 1);
					h = new RationalNumber(approx + 1, 1);
				}
				else
				{
					l = new RationalNumber(approx - 1, 1);
					h = new RationalNumber(approx, 1);
				}
			}
			else
			{
				int approx = (int) (1.0 / value);
				if ((1.0 / approx) < value)
				{
					l = new RationalNumber(1, approx);
					h = new RationalNumber(1, approx - 1);
				}
				else
				{
					l = new RationalNumber(1, approx + 1);
					h = new RationalNumber(1, approx);
				}
			}
			low = Option.factory(l, value);
			high = Option.factory(h, value);
		}

		Option bestOption = (low.error < high.error) ? low : high;

		final int MAX_ITERATIONS = 100; // value is quite high, actually.  shouldn't matter.
		for (int count = 0; bestOption.error > TOLERANCE
				&& count < MAX_ITERATIONS; count++)
		{
			//			Debug.debug("bestOption: " + bestOption + ", left: " + low
			//					+ ", right: " + high + ", value: " + value + ", error: "
			//					+ bestOption.error);

			RationalNumber mediant = RationalNumber.factoryMethod(
					(long) low.rationalNumber.numerator
							+ (long) high.rationalNumber.numerator,
					(long) low.rationalNumber.divisor
							+ (long) high.rationalNumber.divisor);
			Option mediantOption = Option.factory(mediant, value);

			if (value < mediant.doubleValue())
			{
				if (high.error <= mediantOption.error)
					break;

				high = mediantOption;
			}
			else
			{
				if (low.error <= mediantOption.error)
					break;

				low = mediantOption;
			}

			if (mediantOption.error < bestOption.error)
				bestOption = mediantOption;
		}

		return negative
				? bestOption.rationalNumber.negate()
				: bestOption.rationalNumber;
	}

}