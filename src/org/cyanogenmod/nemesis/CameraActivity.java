package org.cyanogenmod.nemesis;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;

public class CameraActivity extends Activity {    
	public final static String TAG = "CameraActivity";

	private CameraManager mCamManager;
	private CameraOrientationEventListener mOrientationListener;
	
	private int mOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;;
	// The orientation compensation for icons and thumbnails. Ex: if the value
    // is 90, the UI components should be rotated 90 degrees counter-clockwise.
    private int mOrientationCompensation = 0;

	/**
	 * Event: Activity created
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_camera);

		// Create orientation listener. This should be done first because it
        // takes some time to get first orientation.
        mOrientationListener = new CameraOrientationEventListener(this);
        mOrientationListener.enable();
        
		// Setup the camera hardware and preview
		setupCamera();
	}


	public void updateInterfaceOrientation() {
		final ViewGroup root = (ViewGroup) findViewById(R.id.shutter_button_container);
		setViewRotation(root, mOrientationCompensation);
		
		root.bringToFront();
	}

	protected void setupCamera() {
		// Setup the Camera hardware and preview surface
		mCamManager = new CameraManager(this);

		// Add the preview surface to its container
		final FrameLayout layout = (FrameLayout) findViewById(R.id.camera_preview_container);
		layout.addView(mCamManager.getPreviewSurface());

		if (mCamManager.open(0)) {
			Log.e(TAG, "Camera 0  open");
		} else {
			Log.e(TAG, "Could not open cameras");
		}
	}

	/**
	 * Recursively rotates the Views of ViewGroups
	 * @param vg the root ViewGroup
	 * @param rotation the angle to which rotate the views
	 */
	public static void setViewRotation(ViewGroup vg, float rotation) {
		final int childCount = vg.getChildCount();

		for (int i = 0; i < childCount; i++) {
			View child = vg.getChildAt(i);

			if (child instanceof ViewGroup) {
				setViewRotation((ViewGroup) child, rotation);
			} else {
				child.animate().rotation(rotation).setDuration(200).setInterpolator(new DecelerateInterpolator()).start();
			}
		}
	}


	private class CameraOrientationEventListener
	extends OrientationEventListener {
		public CameraOrientationEventListener(Context context) {
			super(context);
		}

		@Override
		public void onOrientationChanged(int orientation) {
			// We keep the last known orientation. So if the user first orient
			// the camera then point the camera to floor or sky, we still have
			// the correct orientation.
			if (orientation == ORIENTATION_UNKNOWN) return;
			mOrientation = Util.roundOrientation(orientation, mOrientation);
			
			int orientationCompensation = mOrientation + 90;
			if (orientationCompensation == 90)
				orientationCompensation += 180;
			else if (orientationCompensation == 270)
				orientationCompensation -= 180;
			
			

			if (mOrientationCompensation != orientationCompensation) {
				// Avoid turning all around
				float angleDelta = orientationCompensation - mOrientationCompensation;
				if (angleDelta >= 270)
					orientationCompensation -= 360;
				
				mOrientationCompensation = orientationCompensation;
				updateInterfaceOrientation();
			}
		}
	}

}