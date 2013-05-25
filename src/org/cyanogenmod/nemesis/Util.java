package org.cyanogenmod.nemesis;

import android.app.Activity;
import android.content.Context;
import android.graphics.Point;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.WindowManager;

public class Util {
    // Orientation hysteresis amount used in rounding, in degrees
    public static final int ORIENTATION_HYSTERESIS = 5;

    // Screen size holder
    private static Point mScreenSize = new Point();

    /**
     * Returns the orientation of the display
     * In our case, since we're locked in Landscape, it should always
     * be 90
     * @param activity
     * @return Orientation angle of the display
     */
    public static int getDisplayRotation(Activity activity) {
        int rotation = activity.getWindowManager().getDefaultDisplay()
                .getRotation();
        switch (rotation) {
        case Surface.ROTATION_0: return 0;
        case Surface.ROTATION_90: return 90;
        case Surface.ROTATION_180: return 180;
        case Surface.ROTATION_270: return 270;
        }
        return 0;
    }

    /**
     * Rounds the orientation so that the UI doesn't rotate if the user
     * holds the device towards the floor or the sky
     * @param orientation New orientation
     * @param orientationHistory Previous orientation
     * @return Rounded orientation
     */
    public static int roundOrientation(int orientation, int orientationHistory) {
        boolean changeOrientation = false;
        if (orientationHistory == OrientationEventListener.ORIENTATION_UNKNOWN) {
            changeOrientation = true;
        } else {
            int dist = Math.abs(orientation - orientationHistory);
            dist = Math.min( dist, 360 - dist );
            changeOrientation = (dist >= 45 + ORIENTATION_HYSTERESIS);
        }
        if (changeOrientation) {
            return ((orientation + 45) / 90 * 90) % 360;
        }
        return orientationHistory;
    }

    /**
     * Returns the size of the screen
     * @param activity
     * @return Point where x=width and y=height
     */
    public static Point getScreenSize(Activity activity) {
        WindowManager service = (WindowManager)activity.getSystemService(Context.WINDOW_SERVICE);
        service.getDefaultDisplay().getSize(mScreenSize);
        return mScreenSize;
    }


}
