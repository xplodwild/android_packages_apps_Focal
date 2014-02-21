/*
 * Copyright (C) 2013 Guillaume Lesniak
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

package org.cyanogenmod.focal;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.hardware.Camera.Size;
import android.net.Uri;
import android.os.Build;
import android.provider.BaseColumns;
import android.provider.MediaStore;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RSIllegalArgumentException;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicYuvToRGB;
import android.renderscript.Type;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

public class Util {
    public final static String TAG = "Nemesis.Util";

    // Orientation hysteresis amount used in rounding, in degrees
    public static final int ORIENTATION_HYSTERESIS = 5;

    public static final String REVIEW_ACTION = "com.android.camera.action.REVIEW";
    // See android.hardware.Camera.ACTION_NEW_PICTURE.
    public static final String ACTION_NEW_PICTURE = "android.hardware.action.NEW_PICTURE";
    // See android.hardware.Camera.ACTION_NEW_VIDEO.
    public static final String ACTION_NEW_VIDEO = "android.hardware.action.NEW_VIDEO";

    // Screen size holder
    private static Point mScreenSize = new Point();
    private static int mRotation = 90;

    private static DateFormat mJpegDateFormat = new SimpleDateFormat("yyyyMMdd_HHmmssSSS");

    /**
     * Returns the orientation of the display
     * In our case, since we're locked in Landscape, it should always
     * be 90
     *
     * @param activity
     * @return Orientation angle of the display
     */
    public static int getDisplayRotation(Activity activity) {
        if (activity != null) {
            int rotation = activity.getWindowManager().getDefaultDisplay()
                    .getRotation();
            switch (rotation) {
                case Surface.ROTATION_0:
                    mRotation = 0;
                    break;
                case Surface.ROTATION_90:
                    mRotation = 90;
                    break;
                case Surface.ROTATION_180:
                    mRotation = 180;
                    break;
                case Surface.ROTATION_270:
                    mRotation = 270;
                    break;
            }
        }

        return mRotation;
    }

    /**
     * Rounds the orientation so that the UI doesn't rotate if the user
     * holds the device towards the floor or the sky
     *
     * @param orientation        New orientation
     * @param orientationHistory Previous orientation
     * @return Rounded orientation
     */
    public static int roundOrientation(int orientation, int orientationHistory) {
        boolean changeOrientation = false;
        if (orientationHistory == OrientationEventListener.ORIENTATION_UNKNOWN) {
            changeOrientation = true;
        } else {
            int dist = Math.abs(orientation - orientationHistory);
            dist = Math.min(dist, 360 - dist);
            changeOrientation = (dist >= 45 + ORIENTATION_HYSTERESIS);
        }
        if (changeOrientation) {
            return ((orientation + 45) / 90 * 90) % 360;
        }
        return orientationHistory;
    }

    /**
     * Returns the size of the screen
     *
     * @param activity
     * @return Point where x=width and y=height
     */
    public static Point getScreenSize(Activity activity) {
        if (activity != null) {
            WindowManager service =
                    (WindowManager) activity.getSystemService(Context.WINDOW_SERVICE);
            service.getDefaultDisplay().getSize(mScreenSize);
        }
        return mScreenSize;
    }

    /**
     * Returns the optimal preview size for photo shots
     *
     * @param currentActivity
     * @param sizes
     * @param targetRatio
     * @return
     */
    public static Size getOptimalPreviewSize(Activity currentActivity,
            List<Size> sizes, double targetRatio) {
        // Use a very small tolerance because we want an exact match.
        final double ASPECT_TOLERANCE = 0.01;
        if (sizes == null) {
            return null;
        }

        Size optimalSize = null;
        double minDiff = Double.MAX_VALUE;

        // Because of bugs of overlay and layout, we sometimes will try to
        // layout the viewfinder in the portrait orientation and thus get the
        // wrong size of preview surface. When we change the preview size, the
        // new overlay will be created before the old one closed, which causes
        // an exception. For now, just get the screen size.
        Point point = getScreenSize(currentActivity);
        int targetHeight = Math.min(point.x, point.y);
        // Try to find an size match aspect ratio and size
        for (Size size : sizes) {
            double ratio = (double) size.width / size.height;
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE) {
                continue;
            }
            if (Math.abs(size.height - targetHeight) < minDiff) {
                optimalSize = size;
                minDiff = Math.abs(size.height - targetHeight);
            }
        }
        // Cannot find the one match the aspect ratio. This should not happen.
        // Ignore the requirement.
        if (optimalSize == null) {
            Log.w(TAG, "No preview size match the aspect ratio");
            minDiff = Double.MAX_VALUE;
            for (Size size : sizes) {
                if (Math.abs(size.height - targetHeight) < minDiff) {
                    optimalSize = size;
                    minDiff = Math.abs(size.height - targetHeight);
                }
            }
        }
        return optimalSize;
    }

    /**
     * Fade in a view from startAlpha to endAlpha during duration milliseconds
     * @param view
     * @param startAlpha
     * @param endAlpha
     * @param duration
     */
    public static void fadeIn(View view, float startAlpha, float endAlpha, long duration) {
        if (view.getVisibility() == View.VISIBLE) {
            return;
        }

        view.setVisibility(View.VISIBLE);
        Animation animation = new AlphaAnimation(startAlpha, endAlpha);
        animation.setDuration(duration);
        view.startAnimation(animation);
    }

    /**
     * Fade in a view with the default time
     * @param view
     */
    public static void fadeIn(View view) {
        fadeIn(view, 0F, 1F, 400);

        // We disabled the button in fadeOut(), so enable it here.
        view.setEnabled(true);
    }

    public static void fadeOut(View view) {
        if (view.getVisibility() != View.VISIBLE) return;

        // Since the button is still clickable before fade-out animation
        // ends, we disable the button first to block click.
        view.setEnabled(false);
        Animation animation = new AlphaAnimation(1F, 0F);
        animation.setDuration(400);
        view.startAnimation(animation);
        view.setVisibility(View.GONE);
    }

    /**
     * Converts the provided byte array from YUV420SP into an RGBA bitmap.
     * @param context
     * @param yuv420sp The YUV420SP data
     * @param width Width of the data's picture
     * @param height Height of the data's picture
     * @return A decoded bitmap
     * @throws NullPointerException
     * @throws IllegalArgumentException
     */
    public static Bitmap decodeYUV420SP(Context context, byte[] yuv420sp, int width, int height)
            throws NullPointerException, IllegalArgumentException {

        Bitmap bmp = null;

        if (Build.VERSION.SDK_INT >= 17) {
            final RenderScript rs = RenderScript.create(context);
            final ScriptIntrinsicYuvToRGB script = ScriptIntrinsicYuvToRGB
                    .create(rs, Element.RGBA_8888(rs));
            Type.Builder tb = new Type.Builder(rs, Element.RGBA_8888(rs));
            tb.setX(width);
            tb.setY(height);

            Allocation allocationOut = Allocation.createTyped(rs, tb.create());
            Allocation allocationIn = Allocation.createSized(rs, Element.U8(rs),
                    (height * width) + ((height) * (width) * 2));

            script.setInput(allocationIn);

            bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            try {
                allocationIn.copyFrom(yuv420sp);
                script.forEach(allocationOut);
                allocationOut.copyTo(bmp);
            } catch (RSIllegalArgumentException ex) {
                Log.e(TAG, "Cannot copy YUV420SP data", ex);
            }
        } else {
            final int frameSize = width * height;
            int[] rgb = new int[frameSize];

            for (int j = 0, yp = 0; j < height; j++) {
                int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
                for (int i = 0; i < width; i++, yp++) {
                    int y = (0xff & ((int) yuv420sp[yp])) - 16;
                    if (y < 0) {
                        y = 0;
                    }
                    if ((i & 1) == 0) {
                        v = (0xff & yuv420sp[uvp++]) - 128;
                        u = (0xff & yuv420sp[uvp++]) - 128;
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

                    rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00)
                            | ((b >> 10) & 0xff);
                }
            }

            bmp = Bitmap.createBitmap(rgb, width, height, Bitmap.Config.ARGB_8888);
        }

        return bmp;
    }

    public static Bitmap decodeYUV422P(byte[] yuv422p, int width, int height)
            throws NullPointerException, IllegalArgumentException {
        final int frameSize = width * height;
        int[] rgb = new int[frameSize];
        for (int j = 0, yp = 0; j < height; j++) {
            int up = frameSize + (j * (width/2)), u = 0, v = 0;
            int vp = ((int)(frameSize*1.5) + (j*(width/2)));
            for (int i = 0; i < width; i++, yp++) {
                int y = (0xff & ((int) yuv422p[yp])) - 16;
                if (y < 0) {
                    y = 0;
                }

                if ((i & 1) == 0) {
                    u = (0xff & yuv422p[up++]) - 128;
                    v = (0xff & yuv422p[vp++]) - 128;
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

                rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00)
                        | ((b >> 10) & 0xff);
            }
        }

        return Bitmap.createBitmap(rgb, width, height, Bitmap.Config.ARGB_8888);
    }

    public static String createJpegName(long dateTaken) {
        return "IMG_" + mJpegDateFormat.format(new Date(dateTaken));
    }

    public static String createVideoName(long dateTaken) {
        return "VID_" + mJpegDateFormat.format(new Date(dateTaken));
    }

    /**
     * Broadcast an intent to notify of a new picture in the Gallery
     * @param context
     * @param uri
     */
    public static void broadcastNewPicture(Context context, Uri uri) {
        context.sendBroadcast(new Intent(ACTION_NEW_PICTURE, uri));
        // Keep compatibility
        context.sendBroadcast(new Intent("com.android.camera.NEW_PICTURE", uri));
    }

    /**
     * Removes an image from the gallery
     * @param cr
     * @param id
     */
    public static void removeFromGallery(ContentResolver cr, long id) {
        cr.delete(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                BaseColumns._ID + "=" + Long.toString(id), null);
    }

    /**
     * Converts the specified DP to PIXELS according to current screen density
     * @param context
     * @param dp
     * @return
     */
    public static float dpToPx(Context context, float dp) {
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        return (int) ((dp * displayMetrics.density) + 0.5);
    }

    /**
     * Returns the physical path (on emmc/sd) of the provided URI from MediaGallery
     * @param context
     * @param contentURI
     * @return
     */
    public static String getRealPathFromURI(Context context, Uri contentURI) {
        Cursor cursor = context.getContentResolver().query(contentURI, null, null, null, null);
        if (cursor == null) { // Source is Dropbox or other similar local file path
            return contentURI.getPath();
        } else {
            cursor.moveToFirst();
            int idx = cursor.getColumnIndex(MediaStore.Images.ImageColumns.DATA);
            return cursor.getString(idx);
        }
    }


    /**
     * Returns the best Panorama preview size
     * @param supportedSizes
     * @param need4To3
     * @param needSmaller
     * @return
     */
    public static Point findBestPanoPreviewSize(List<Size> supportedSizes, boolean need4To3,
                                                boolean needSmaller, int defaultPixels) {
        Point output = null;
        int pixelsDiff = defaultPixels;

        for (Size size : supportedSizes) {
            int h = size.height;
            int w = size.width;
            // we only want 4:3 format.
            int d = defaultPixels - h * w;
            if (needSmaller && d < 0) { // no bigger preview than 960x720.
                continue;
            }
            if (need4To3 && (h * 4 != w * 3)) {
                continue;
            }
            d = Math.abs(d);
            if (d < pixelsDiff) {
                output = new Point(w, h);
                pixelsDiff = d;
            }
        }
        return output;
    }

    /**
     * Returns the best PicSphere picture size. The reference size is 2048x1536 from Nexus 4.
     * @param supportedSizes Supported picture size
     * @param needSmaller If a larger image size is accepted
     * @return A Point where X and Y corresponds to Width and Height
     */
    public static Point findBestPicSpherePictureSize(List<Size> supportedSizes, boolean needSmaller) {
        Point output = null;
        final int defaultPixels = 2048*1536;

        int pixelsDiff = defaultPixels;

        for (Size size : supportedSizes) {
            int h = size.height;
            int w = size.width;
            int d = defaultPixels - h * w;

            if (needSmaller && d < 0) { // no bigger preview than 960x720.
                continue;
            }

            // we only want 4:3 format.
            if ((h * 4 != w * 3)) {
                continue;
            }
            d = Math.abs(d);
            if (d < pixelsDiff) {
                output = new Point(w, h);
                pixelsDiff = d;
            }
        }

        if (output == null) {
            // Fail-safe, default to 640x480 is nothing suitable is found
            output = new Point(640, 480);
        }

        return output;
    }

    /**
     * Older devices need to stop preview before taking a shot
     * (example: galaxy S, galaxy S2, etc)
     * @return true if the device is an old one
     */
    public static boolean deviceNeedsStopPreviewToShoot() {
        String[] oldDevices = {"smdk4210", "aries"};

        boolean needs = Arrays.asList(oldDevices).contains(Build.BOARD);

        Log.e(TAG, "Device " + Build.BOARD + (needs ? " needs ": " doesn't need ") + "to stop preview");
        return needs;
    }

    /**
     * Disable ZSL mode on certain Qualcomm models
     * @return true if the device needs ZSL disabled
     */
    public static boolean deviceNeedsDisableZSL() {
        String[] noZSL = {"apexqtmo", "expressatt", "aegis2vzw"};

        boolean needs = Arrays.asList(noZSL).contains(Build.PRODUCT);

        Log.e(TAG, "Device " + Build.PRODUCT + (needs ? " needs ": " doesn't need ") + "to disable ZSL");
        return needs;
    }

}
