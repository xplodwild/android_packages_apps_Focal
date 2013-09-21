/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright (C) 2013 Guillaume Lesniak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.focal.pano;

import java.text.SimpleDateFormat;
import java.util.Date;

public class PanoUtil {
    public static String createName(String format, long dateTaken) {
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat = new SimpleDateFormat(format);
        return dateFormat.format(date);
    }

    // TODO: Add comments about the range of these two arguments.
    public static double calculateDifferenceBetweenAngles(
            double firstAngle, double secondAngle) {
        double difference1 = (secondAngle - firstAngle) % 360;
        if (difference1 < 0) {
            difference1 += 360;
        }

        double difference2 = (firstAngle - secondAngle) % 360;
        if (difference2 < 0) {
            difference2 += 360;
        }

        return Math.min(difference1, difference2);
    }

    public static void decodeYUV420SPQuarterRes(int[] rgb, byte[] yuv420sp, int width, int height) {
        final int frameSize = width * height;

        for (int j = 0, ypd = 0; j < height; j += 4) {
            int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
            for (int i = 0; i < width; i += 4, ypd++) {
                int y = (0xff & (yuv420sp[j * width + i])) - 16;
                if (y < 0) {
                    y = 0;
                }
                if ((i & 1) == 0) {
                    v = (0xff & yuv420sp[uvp++]) - 128;
                    u = (0xff & yuv420sp[uvp++]) - 128;
                    uvp += 2;  // Skip the UV values for the 4 pixels skipped in between
                }
                int y1192 = 1192 * y;
                int r = (y1192 + 1634 * v);
                int g = (y1192 - 833 * v - 400 * u);
                int b = (y1192 + 2066 * u);

                if (r < 0) {
                    r = 0;
                } else if (r > 262143) {
                    r = 262143;
                }
                if (g < 0) {
                    g = 0;
                } else if (g > 262143) {
                    g = 262143;
                }
                if (b < 0) {
                    b = 0;
                } else if (b > 262143) {
                    b = 262143;
                }

                rgb[ypd] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) |
                        ((b >> 10) & 0xff);
            }
        }
    }
}
