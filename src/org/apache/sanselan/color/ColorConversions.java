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
package org.apache.sanselan.color;

import org.apache.sanselan.util.Debug;

public abstract class ColorConversions
{
	public final static void main(String args[])
	{
		try
		{
			{
				for (int C = 0; C <= 256; C += 64)
					for (int M = 0; M <= 256; M += 64)
						for (int Y = 0; Y <= 256; Y += 64)
							for (int K = 0; K <= 256; K += 64)
							{

								int rgb1 = ColorConversions.convertCMYKtoRGB(
										Math.min(255, C), Math.min(255, M),
										Math.min(255, Y), Math.min(255, K));
								int rgb2 = ColorConversions
										.convertCMYKtoRGB_old(Math.min(255, C),
												Math.min(255, M), Math.min(255,
														Y), Math.min(255, K));

								if (rgb1 != rgb2)
								{
									Debug.debug();
									Debug.debug("C", C);
									Debug.debug("M", M);
									Debug.debug("Y", Y);
									Debug.debug("K", K);
									Debug.debug("rgb1", rgb1 + " ("
											+ Integer.toHexString(rgb1) + ")");
									Debug.debug("rgb2", rgb2 + " ("
											+ Integer.toHexString(rgb2) + ")");
								}
							}
			}
			int sample_rgbs[] = {
					0xffffffff, //	
					0xff000000, //	
					0xffff0000, //	
					0xff00ff00, //	
					0xff0000ff, //	
					0xffff00ff, //	
					0xfff0ff00, //	
					0xff00ffff, //	
					0x00000000, //	
					0xff7f7f7f, //	
			};
			for (int i = 0; i < sample_rgbs.length; i++)
			{
				int rgb = sample_rgbs[i];

				ColorXYZ xyz;
				{
					xyz = ColorConversions.convertRGBtoXYZ(rgb);
					int xyz_rgb = ColorConversions.convertXYZtoRGB(xyz);

					Debug.debug();
					Debug.debug("rgb", rgb + " (" + Integer.toHexString(rgb)
							+ ")");
					Debug.debug("xyz", xyz);
					Debug.debug("xyz_rgb", xyz_rgb + " ("
							+ Integer.toHexString(xyz_rgb) + ")");
					if ((0xffffff & xyz_rgb) != (0xffffff & rgb))
						Debug.debug("!!!!!!!!!!!!!!!!!!!!!!!");
				}
				ColorCIELab cielab;
				{
					cielab = ColorConversions.convertXYZtoCIELab(xyz);
					ColorXYZ cielab_xyz = ColorConversions
							.convertCIELabtoXYZ(cielab);
					int cielab_xyz_rgb = ColorConversions
							.convertXYZtoRGB(cielab_xyz);

					Debug.debug("cielab", cielab);
					Debug.debug("cielab_xyz", cielab_xyz);
					Debug.debug("cielab_xyz_rgb", cielab_xyz_rgb + " ("
							+ Integer.toHexString(cielab_xyz_rgb) + ")");
					if ((0xffffff & cielab_xyz_rgb) != (0xffffff & rgb))
						Debug.debug("!!!!!!!!!!!!!!!!!!!!!!!");
				}

				{
					ColorHunterLab hunterlab = ColorConversions
							.convertXYZtoHunterLab(xyz);
					ColorXYZ hunterlab_xyz = ColorConversions
							.convertHunterLabtoXYZ(hunterlab);
					int hunterlab_xyz_rgb = ColorConversions
							.convertXYZtoRGB(hunterlab_xyz);

					Debug.debug("hunterlab", hunterlab);
					Debug.debug("hunterlab_xyz", hunterlab_xyz);
					Debug.debug("hunterlab_xyz_rgb", hunterlab_xyz_rgb + " ("
							+ Integer.toHexString(hunterlab_xyz_rgb) + ")");
					if ((0xffffff & hunterlab_xyz_rgb) != (0xffffff & rgb))
						Debug.debug("!!!!!!!!!!!!!!!!!!!!!!!");
				}

				{
					ColorCMY cmy = ColorConversions.convertRGBtoCMY(rgb);
					ColorCMYK cmyk = ColorConversions.convertCMYtoCMYK(cmy);
					ColorCMY cmyk_cmy = ColorConversions.convertCMYKtoCMY(cmyk);
					int cmyk_cmy_rgb = ColorConversions
							.convertCMYtoRGB(cmyk_cmy);

					Debug.debug("cmy", cmy);
					Debug.debug("cmyk", cmyk);
					Debug.debug("cmyk_cmy", cmyk_cmy);
					Debug.debug("cmyk_cmy_rgb", cmyk_cmy_rgb + " ("
							+ Integer.toHexString(cmyk_cmy_rgb) + ")");
					if ((0xffffff & cmyk_cmy_rgb) != (0xffffff & rgb))
						Debug.debug("!!!!!!!!!!!!!!!!!!!!!!!");
				}

				{
					ColorHSL hsl = ColorConversions.convertRGBtoHSL(rgb);
					int hsl_rgb = ColorConversions.convertHSLtoRGB(hsl);

					Debug.debug("hsl", hsl);
					Debug.debug("hsl_rgb", hsl_rgb + " ("
							+ Integer.toHexString(hsl_rgb) + ")");
					if ((0xffffff & hsl_rgb) != (0xffffff & rgb))
						Debug.debug("!!!!!!!!!!!!!!!!!!!!!!!");
				}
				{
					ColorHSV hsv = ColorConversions.convertRGBtoHSV(rgb);
					int hsv_rgb = ColorConversions.convertHSVtoRGB(hsv);

					Debug.debug("hsv", hsv);
					Debug.debug("hsv_rgb", hsv_rgb + " ("
							+ Integer.toHexString(hsv_rgb) + ")");
					if ((0xffffff & hsv_rgb) != (0xffffff & rgb))
						Debug.debug("!!!!!!!!!!!!!!!!!!!!!!!");
				}

				{
					ColorCIELCH cielch = ColorConversions
							.convertCIELabtoCIELCH(cielab);
					ColorCIELab cielch_cielab = ColorConversions
							.convertCIELCHtoCIELab(cielch);

					Debug.debug("cielch", cielch);
					Debug.debug("cielch_cielab", cielch_cielab);
				}

				{
					ColorCIELuv cieluv = ColorConversions
							.convertXYZtoCIELuv(xyz);
					ColorXYZ cieluv_xyz = ColorConversions
							.convertCIELuvtoXYZ(cieluv);

					Debug.debug("cieluv", cieluv);
					Debug.debug("cieluv_xyz", cieluv_xyz);
				}

			}
		}
		catch (Throwable e)
		{
			Debug.debug(e);
		}
	}

	public static final ColorCIELab convertXYZtoCIELab(ColorXYZ xyz)
	{
		return convertXYZtoCIELab(xyz.X, xyz.Y, xyz.Z);
	}

	private static final double ref_X = 95.047;
	private static final double ref_Y = 100.000;
	private static final double ref_Z = 108.883;

	public static final ColorCIELab convertXYZtoCIELab(double X, double Y,
			double Z)
	{

		double var_X = X / ref_X; //ref_X =  95.047  Observer= 2, Illuminant= D65
		double var_Y = Y / ref_Y; //ref_Y = 100.000
		double var_Z = Z / ref_Z; //ref_Z = 108.883

		if (var_X > 0.008856)
			var_X = Math.pow(var_X, (1 / 3.0));
		else
			var_X = (7.787 * var_X) + (16 / 116.0);
		if (var_Y > 0.008856)
			var_Y = Math.pow(var_Y, 1 / 3.0);
		else
			var_Y = (7.787 * var_Y) + (16 / 116.0);
		if (var_Z > 0.008856)
			var_Z = Math.pow(var_Z, 1 / 3.0);
		else
			var_Z = (7.787 * var_Z) + (16 / 116.0);

		double L = (116 * var_Y) - 16;
		double a = 500 * (var_X - var_Y);
		double b = 200 * (var_Y - var_Z);
		return new ColorCIELab(L, a, b);
	}

	public static final ColorXYZ convertCIELabtoXYZ(ColorCIELab cielab)
	{
		return convertCIELabtoXYZ(cielab.L, cielab.a, cielab.b);
	}

	public static final ColorXYZ convertCIELabtoXYZ(double L, double a, double b)
	{
		double var_Y = (L + 16) / 116.0;
		double var_X = a / 500 + var_Y;
		double var_Z = var_Y - b / 200.0;

		if (Math.pow(var_Y, 3) > 0.008856)
			var_Y = Math.pow(var_Y, 3);
		else
			var_Y = (var_Y - 16 / 116.0) / 7.787;
		if (Math.pow(var_X, 3) > 0.008856)
			var_X = Math.pow(var_X, 3);
		else
			var_X = (var_X - 16 / 116.0) / 7.787;
		if (Math.pow(var_Z, 3) > 0.008856)
			var_Z = Math.pow(var_Z, 3);
		else
			var_Z = (var_Z - 16 / 116.0) / 7.787;

		double X = ref_X * var_X; //ref_X =  95.047  Observer= 2, Illuminant= D65
		double Y = ref_Y * var_Y; //ref_Y = 100.000
		double Z = ref_Z * var_Z; //ref_Z = 108.883

		return new ColorXYZ(X, Y, Z);
	}

	public static final ColorHunterLab convertXYZtoHunterLab(ColorXYZ xyz)
	{
		return convertXYZtoHunterLab(xyz.X, xyz.Y, xyz.Z);
	}

	public static final ColorHunterLab convertXYZtoHunterLab(double X,
			double Y, double Z)
	{
		double L = 10 * Math.sqrt(Y);
		double a = 17.5 * (((1.02 * X) - Y) / Math.sqrt(Y));
		double b = 7 * ((Y - (0.847 * Z)) / Math.sqrt(Y));

		return new ColorHunterLab(L, a, b);
	}

	public static final ColorXYZ convertHunterLabtoXYZ(ColorHunterLab cielab)
	{
		return convertHunterLabtoXYZ(cielab.L, cielab.a, cielab.b);
	}

	public static final ColorXYZ convertHunterLabtoXYZ(double L, double a,
			double b)
	{
		double var_Y = L / 10;
		double var_X = a / 17.5 * L / 10;
		double var_Z = b / 7 * L / 10;

		double Y = Math.pow(var_Y, 2);
		double X = (var_X + Y) / 1.02;
		double Z = -(var_Z - Y) / 0.847;

		return new ColorXYZ(X, Y, Z);
	}

	public static final int convertXYZtoRGB(ColorXYZ xyz)
	{
		return convertXYZtoRGB(xyz.X, xyz.Y, xyz.Z);
	}

	public static final int convertXYZtoRGB(double X, double Y, double Z)
	{
		//Observer = 2, Illuminant = D65
		double var_X = X / 100.0; //Where X = 0   95.047
		double var_Y = Y / 100.0; //Where Y = 0  100.000
		double var_Z = Z / 100.0; //Where Z = 0  108.883

		double var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
		double var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
		double var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

		if (var_R > 0.0031308)
			var_R = 1.055 * Math.pow(var_R, (1 / 2.4)) - 0.055;
		else
			var_R = 12.92 * var_R;
		if (var_G > 0.0031308)
			var_G = 1.055 * Math.pow(var_G, (1 / 2.4)) - 0.055;
		else
			var_G = 12.92 * var_G;
		if (var_B > 0.0031308)
			var_B = 1.055 * Math.pow(var_B, (1 / 2.4)) - 0.055;
		else
			var_B = 12.92 * var_B;

		double R = (var_R * 255);
		double G = (var_G * 255);
		double B = (var_B * 255);

		return convertRGBtoRGB(R, G, B);
	}

	public static final ColorXYZ convertRGBtoXYZ(int rgb)
	{
		int r = 0xff & (rgb >> 16);
		int g = 0xff & (rgb >> 8);
		int b = 0xff & (rgb >> 0);

		double var_R = r / 255.0; //Where R = 0  255
		double var_G = g / 255.0; //Where G = 0  255
		double var_B = b / 255.0; //Where B = 0  255

		if (var_R > 0.04045)
			var_R = Math.pow((var_R + 0.055) / 1.055, 2.4);
		else
			var_R = var_R / 12.92;
		if (var_G > 0.04045)
			var_G = Math.pow((var_G + 0.055) / 1.055, 2.4);
		else
			var_G = var_G / 12.92;
		if (var_B > 0.04045)
			var_B = Math.pow((var_B + 0.055) / 1.055, 2.4);
		else
			var_B = var_B / 12.92;

		var_R = var_R * 100;
		var_G = var_G * 100;
		var_B = var_B * 100;

		//		Observer. = 2�, Illuminant = D65
		double X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
		double Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
		double Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

		return new ColorXYZ(X, Y, Z);
	}

	public static final ColorCMY convertRGBtoCMY(int rgb)
	{
		int R = 0xff & (rgb >> 16);
		int G = 0xff & (rgb >> 8);
		int B = 0xff & (rgb >> 0);

		//		RGB values = 0 � 255
		//		CMY values = 0 � 1

		double C = 1 - (R / 255.0);
		double M = 1 - (G / 255.0);
		double Y = 1 - (B / 255.0);

		return new ColorCMY(C, M, Y);
	}

	public static final int convertCMYtoRGB(ColorCMY cmy)
	{
		//		CMY values = 0 � 1
		//		RGB values = 0 � 255

		double R = (1 - cmy.C) * 255.0;
		double G = (1 - cmy.M) * 255.0;
		double B = (1 - cmy.Y) * 255.0;

		return convertRGBtoRGB(R, G, B);
	}

	public static final ColorCMYK convertCMYtoCMYK(ColorCMY cmy)
	{
		//		Where CMYK and CMY values = 0  1

		double C = cmy.C;
		double M = cmy.M;
		double Y = cmy.Y;

		double var_K = 1.0;

		if (C < var_K)
			var_K = C;
		if (M < var_K)
			var_K = M;
		if (Y < var_K)
			var_K = Y;
		if (var_K == 1)
		{ //Black
			C = 0;
			M = 0;
			Y = 0;
		}
		else
		{
			C = (C - var_K) / (1 - var_K);
			M = (M - var_K) / (1 - var_K);
			Y = (Y - var_K) / (1 - var_K);
		}
		return new ColorCMYK(C, M, Y, var_K);
	}

	public static final ColorCMY convertCMYKtoCMY(ColorCMYK cmyk)
	{
		return convertCMYKtoCMY(cmyk.C, cmyk.M, cmyk.Y, cmyk.K);
	}

	public static final ColorCMY convertCMYKtoCMY(double C, double M, double Y,
			double K)
	{
		//	Where CMYK and CMY values =  1

		C = (C * (1 - K) + K);
		M = (M * (1 - K) + K);
		Y = (Y * (1 - K) + K);

		return new ColorCMY(C, M, Y);
	}

	public static final int convertCMYKtoRGB(int c, int m, int y, int k)
	//	throws ImageReadException, IOException
	{
		double C = c / 255.0;
		double M = m / 255.0;
		double Y = y / 255.0;
		double K = k / 255.0;

		return convertCMYtoRGB(convertCMYKtoCMY(C, M, Y, K));
	}

	public static final ColorHSL convertRGBtoHSL(int rgb)
	{

		int R = 0xff & (rgb >> 16);
		int G = 0xff & (rgb >> 8);
		int B = 0xff & (rgb >> 0);

		double var_R = (R / 255.0); //Where RGB values = 0  255
		double var_G = (G / 255.0);
		double var_B = (B / 255.0);

		double var_Min = Math.min(var_R, Math.min(var_G, var_B)); //Min. value of RGB
		double var_Max = Math.max(var_R, Math.max(var_G, var_B)); //Max. value of RGB
		double del_Max = var_Max - var_Min; //Delta RGB value

		double L = (var_Max + var_Min) / 2.0;

		double H, S;
		//		Debug.debug("del_Max", del_Max);
		if (del_Max == 0) //This is a gray, no chroma...
		{
			H = 0; //HSL results = 0  1
			S = 0;
		}
		else
		//Chromatic data...
		{
			//			Debug.debug("L", L);

			if (L < 0.5)
				S = del_Max / (var_Max + var_Min);
			else
				S = del_Max / (2 - var_Max - var_Min);

			//			Debug.debug("S", S);

			double del_R = (((var_Max - var_R) / 6) + (del_Max / 2)) / del_Max;
			double del_G = (((var_Max - var_G) / 6) + (del_Max / 2)) / del_Max;
			double del_B = (((var_Max - var_B) / 6) + (del_Max / 2)) / del_Max;

			if (var_R == var_Max)
				H = del_B - del_G;
			else if (var_G == var_Max)
				H = (1 / 3.0) + del_R - del_B;
			else if (var_B == var_Max)
				H = (2 / 3.0) + del_G - del_R;
			else
			{
				Debug.debug("uh oh");
				H = 0; // cmc
			}

			//			Debug.debug("H1", H);

			if (H < 0)
				H += 1;
			if (H > 1)
				H -= 1;

			//			Debug.debug("H2", H);
		}

		return new ColorHSL(H, S, L);
	}

	public static int convertHSLtoRGB(ColorHSL hsl)
	{
		return convertHSLtoRGB(hsl.H, hsl.S, hsl.L);
	}

	public static int convertHSLtoRGB(double H, double S, double L)
	{
		double R, G, B;

		if (S == 0) //HSL values = 0  1
		{
			R = L * 255; //RGB results = 0  255
			G = L * 255;
			B = L * 255;
		}
		else
		{
			double var_2;

			if (L < 0.5)
				var_2 = L * (1 + S);
			else
				var_2 = (L + S) - (S * L);

			double var_1 = 2 * L - var_2;

			R = 255 * convertHuetoRGB(var_1, var_2, H + (1 / 3.0));
			G = 255 * convertHuetoRGB(var_1, var_2, H);
			B = 255 * convertHuetoRGB(var_1, var_2, H - (1 / 3.0));
		}

		return convertRGBtoRGB(R, G, B);
	}

	private static double convertHuetoRGB(double v1, double v2, double vH) //Function Hue_2_RGB
	{
		if (vH < 0)
			vH += 1;
		if (vH > 1)
			vH -= 1;
		if ((6 * vH) < 1)
			return (v1 + (v2 - v1) * 6 * vH);
		if ((2 * vH) < 1)
			return (v2);
		if ((3 * vH) < 2)
			return (v1 + (v2 - v1) * ((2 / 3.0) - vH) * 6);
		return (v1);
	}

	public static final ColorHSV convertRGBtoHSV(int rgb)
	{
		int R = 0xff & (rgb >> 16);
		int G = 0xff & (rgb >> 8);
		int B = 0xff & (rgb >> 0);

		double var_R = (R / 255.0); //RGB values = 0  255
		double var_G = (G / 255.0);
		double var_B = (B / 255.0);

		double var_Min = Math.min(var_R, Math.min(var_G, var_B)); //Min. value of RGB
		double var_Max = Math.max(var_R, Math.max(var_G, var_B)); //Max. value of RGB
		double del_Max = var_Max - var_Min; //Delta RGB value

		double V = var_Max;

		double H, S;
		if (del_Max == 0) //This is a gray, no chroma...
		{
			H = 0; //HSV results = 0  1
			S = 0;
		}
		else
		//Chromatic data...
		{
			S = del_Max / var_Max;

			double del_R = (((var_Max - var_R) / 6) + (del_Max / 2)) / del_Max;
			double del_G = (((var_Max - var_G) / 6) + (del_Max / 2)) / del_Max;
			double del_B = (((var_Max - var_B) / 6) + (del_Max / 2)) / del_Max;

			if (var_R == var_Max)
				H = del_B - del_G;
			else if (var_G == var_Max)
				H = (1 / 3.0) + del_R - del_B;
			else if (var_B == var_Max)
				H = (2 / 3.0) + del_G - del_R;
			else
			{
				Debug.debug("uh oh");
				H = 0; // cmc;
			}

			if (H < 0)
				H += 1;
			if (H > 1)
				H -= 1;
		}

		return new ColorHSV(H, S, V);
	}

	public static int convertHSVtoRGB(ColorHSV HSV)
	{
		return convertHSVtoRGB(HSV.H, HSV.S, HSV.V);
	}

	public static int convertHSVtoRGB(double H, double S, double V)
	{
		double R, G, B;

		if (S == 0) //HSV values = 0 � 1
		{
			R = V * 255;
			G = V * 255;
			B = V * 255;
		}
		else
		{
			double var_h = H * 6;
			if (var_h == 6)
				var_h = 0; //H must be < 1
			double var_i = Math.floor(var_h); //Or ... var_i = floor( var_h )
			double var_1 = V * (1 - S);
			double var_2 = V * (1 - S * (var_h - var_i));
			double var_3 = V * (1 - S * (1 - (var_h - var_i)));

			double var_r, var_g, var_b;

			if (var_i == 0)
			{
				var_r = V;
				var_g = var_3;
				var_b = var_1;
			}
			else if (var_i == 1)
			{
				var_r = var_2;
				var_g = V;
				var_b = var_1;
			}
			else if (var_i == 2)
			{
				var_r = var_1;
				var_g = V;
				var_b = var_3;
			}
			else if (var_i == 3)
			{
				var_r = var_1;
				var_g = var_2;
				var_b = V;
			}
			else if (var_i == 4)
			{
				var_r = var_3;
				var_g = var_1;
				var_b = V;
			}
			else
			{
				var_r = V;
				var_g = var_1;
				var_b = var_2;
			}

			R = var_r * 255; //RGB results = 0 � 255
			G = var_g * 255;
			B = var_b * 255;
		}

		return convertRGBtoRGB(R, G, B);
	}

	public static final int convertCMYKtoRGB_old(int sc, int sm, int sy, int sk)
	//	throws ImageReadException, IOException
	{
		int red = 255 - (sc + sk);
		int green = 255 - (sm + sk);
		int blue = 255 - (sy + sk);

		return convertRGBtoRGB(red, green, blue);
	}

	private static double cube(double f)
	{
		return f * f * f;
	}

	private static double square(double f)
	{
		return f * f;
	}

	public static final int convertCIELabtoARGBTest(int cieL, int cieA, int cieB)
	{
		double X, Y, Z;

		{

			double var_Y = (((double) cieL * 100.0 / 255.0) + 16.0) / 116.0;
			double var_X = cieA / 500.0 + var_Y;
			double var_Z = var_Y - cieB / 200.0;

			double var_x_cube = cube(var_X);
			double var_y_cube = cube(var_Y);
			double var_z_cube = cube(var_Z);

			if (var_y_cube > 0.008856)
				var_Y = var_y_cube;
			else
				var_Y = (var_Y - 16 / 116.0) / 7.787;

			if (var_x_cube > 0.008856)
				var_X = var_x_cube;
			else
				var_X = (var_X - 16 / 116.0) / 7.787;

			if (var_z_cube > 0.008856)
				var_Z = var_z_cube;
			else
				var_Z = (var_Z - 16 / 116.0) / 7.787;

			//			double ref_X = 95.047;
			//			double ref_Y = 100.000;
			//			double ref_Z = 108.883;

			X = ref_X * var_X; //ref_X =  95.047  Observer= 2�, Illuminant= D65
			Y = ref_Y * var_Y; //ref_Y = 100.000
			Z = ref_Z * var_Z; //ref_Z = 108.883

		}

		double R, G, B;
		{
			double var_X = X / 100; //X = From 0 to ref_X
			double var_Y = Y / 100; //Y = From 0 to ref_Y
			double var_Z = Z / 100; //Z = From 0 to ref_Y

			double var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
			double var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
			double var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

			if (var_R > 0.0031308)
				var_R = 1.055 * Math.pow(var_R, (1 / 2.4)) - 0.055;
			else
				var_R = 12.92 * var_R;
			if (var_G > 0.0031308)
				var_G = 1.055 * Math.pow(var_G, (1 / 2.4)) - 0.055;
			else
				var_G = 12.92 * var_G;

			if (var_B > 0.0031308)
				var_B = 1.055 * Math.pow(var_B, (1 / 2.4)) - 0.055;
			else
				var_B = 12.92 * var_B;

			R = (var_R * 255);
			G = (var_G * 255);
			B = (var_B * 255);
		}

		return convertRGBtoRGB(R, G, B);
	}

	private static final int convertRGBtoRGB(double R, double G, double B)
	{
		int red = (int) Math.round(R);
		int green = (int) Math.round(G);
		int blue = (int) Math.round(B);

		red = Math.min(255, Math.max(0, red));
		green = Math.min(255, Math.max(0, green));
		blue = Math.min(255, Math.max(0, blue));

		int alpha = 0xff;
		int rgb = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);

		return rgb;
	}

	private static final int convertRGBtoRGB(int red, int green, int blue)
	{
		red = Math.min(255, Math.max(0, red));
		green = Math.min(255, Math.max(0, green));
		blue = Math.min(255, Math.max(0, blue));

		int alpha = 0xff;
		int rgb = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);

		return rgb;
	}

	public static ColorCIELCH convertCIELabtoCIELCH(ColorCIELab cielab)
	{
		return convertCIELabtoCIELCH(cielab.L, cielab.a, cielab.b);
	}

	public static ColorCIELCH convertCIELabtoCIELCH(double L, double a, double b)
	{
		double var_H = Math.atan2(b, a); //Quadrant by signs

		if (var_H > 0)
			var_H = (var_H / Math.PI) * 180.0;
		else
			var_H = 360 - radian_2_degree(Math.abs(var_H));

		//		L = L;
		double C = Math.sqrt(square(a) + square(b));
		double H = var_H;

		return new ColorCIELCH(L, C, H);
	}

	public static ColorCIELab convertCIELCHtoCIELab(ColorCIELCH cielch)
	{
		return convertCIELCHtoCIELab(cielch.L, cielch.C, cielch.H);
	}

	public static ColorCIELab convertCIELCHtoCIELab(double L, double C, double H)
	{
		//		Where CIE-H� = 0 � 360�

		//		CIE-L* = CIE-L;
		double a = Math.cos(degree_2_radian(H)) * C;
		double b = Math.sin(degree_2_radian(H)) * C;

		return new ColorCIELab(L, a, b);
	}

	public static double degree_2_radian(double degree)
	{
		return degree * Math.PI / 180.0;
	}

	public static double radian_2_degree(double radian)
	{
		return radian * 180.0 / Math.PI;
	}

	public static ColorCIELuv convertXYZtoCIELuv(ColorXYZ xyz)
	{
		return convertXYZtoCIELuv(xyz.X, xyz.Y, xyz.Z);
	}

	public static ColorCIELuv convertXYZtoCIELuv(double X, double Y, double Z)
	{
		// problems here with div by zero

		double var_U = (4 * X) / (X + (15 * Y) + (3 * Z));
		double var_V = (9 * Y) / (X + (15 * Y) + (3 * Z));

		//		Debug.debug("var_U", var_U);
		//		Debug.debug("var_V", var_V);

		double var_Y = Y / 100.0;
		//		Debug.debug("var_Y", var_Y);

		if (var_Y > 0.008856)
			var_Y = Math.pow(var_Y, (1 / 3.0));
		else
			var_Y = (7.787 * var_Y) + (16 / 116.0);

		double ref_X = 95.047; //Observer= 2�, Illuminant= D65
		double ref_Y = 100.000;
		double ref_Z = 108.883;

		//		Debug.debug("var_Y", var_Y);

		double ref_U = (4 * ref_X) / (ref_X + (15 * ref_Y) + (3 * ref_Z));
		double ref_V = (9 * ref_Y) / (ref_X + (15 * ref_Y) + (3 * ref_Z));

		//		Debug.debug("ref_U", ref_U);
		//		Debug.debug("ref_V", ref_V);

		double L = (116 * var_Y) - 16;
		double u = 13 * L * (var_U - ref_U);
		double v = 13 * L * (var_V - ref_V);

		return new ColorCIELuv(L, u, v);
	}

	public static ColorXYZ convertCIELuvtoXYZ(ColorCIELuv cielch)
	{
		return convertCIELuvtoXYZ(cielch.L, cielch.u, cielch.v);
	}

	public static ColorXYZ convertCIELuvtoXYZ(double L, double u, double v)
	{
		// problems here with div by zero

		double var_Y = (L + 16) / 116;
		if (Math.pow(var_Y, 3) > 0.008856)
			var_Y = Math.pow(var_Y, 3);
		else
			var_Y = (var_Y - 16 / 116) / 7.787;

		double ref_X = 95.047; //Observer= 2�, Illuminant= D65
		double ref_Y = 100.000;
		double ref_Z = 108.883;

		double ref_U = (4 * ref_X) / (ref_X + (15 * ref_Y) + (3 * ref_Z));
		double ref_V = (9 * ref_Y) / (ref_X + (15 * ref_Y) + (3 * ref_Z));
		double var_U = u / (13 * L) + ref_U;
		double var_V = v / (13 * L) + ref_V;

		double Y = var_Y * 100;
		double X = -(9 * Y * var_U) / ((var_U - 4) * var_V - var_U * var_V);
		double Z = (9 * Y - (15 * var_V * Y) - (var_V * X)) / (3 * var_V);

		return new ColorXYZ(X, Y, Z);
	}
}