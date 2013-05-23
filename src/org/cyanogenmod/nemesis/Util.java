package org.cyanogenmod.nemesis;

import android.app.Activity;
import android.view.OrientationEventListener;
import android.view.Surface;

public class Util {
	// Orientation hysteresis amount used in rounding, in degrees
    public static final int ORIENTATION_HYSTERESIS = 5;
    
    public static int getDisplayRotation(Activity activity) {
        int rotation = activity.getWindowManager().getDefaultDisplay()
                .getRotation();
        switch (rotation) {
            case Surface.ROTATION_0: return 0;
            case Surface.ROTATION_90: return 90;
            case Surface.ROTATION_180: return -90;
            case Surface.ROTATION_270: return 180;
        }
        return 0;
    }
    
    
	public static int roundOrientation(int orientation, int orientationHistory) {
		boolean changeOrientation = false;
		if (orientationHistory == OrientationEventListener.ORIENTATION_UNKNOWN) {
			changeOrientation = true;
		} else {
			int dist = Math.abs(orientation - orientationHistory);
			dist = Math.min( dist, 360 - dist );
			changeOrientation = ( dist >= 45 + ORIENTATION_HYSTERESIS );
		}
		if (changeOrientation) {
			return ((orientation + 45) / 90 * 90) % 360;
		}
		return orientationHistory;
	}
}
