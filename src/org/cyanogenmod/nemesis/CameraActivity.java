package org.cyanogenmod.nemesis;

import org.cyanogenmod.nemesis.ui.SideBar;
import org.cyanogenmod.nemesis.ui.WidgetRenderer;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;

public class CameraActivity extends Activity {    
    public final static String TAG = "CameraActivity";

    private CameraManager mCamManager;
    private CameraOrientationEventListener mOrientationListener;
    private GestureDetector mGestureDetector;
    private Handler mHandler;

    private int mOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;;
    private int mOrientationCompensation = 0;

    private SideBar mSideBar;
    private WidgetRenderer mWidgetRenderer;

    /**
     * Event: Activity created
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_camera);

        mSideBar = (SideBar) findViewById(R.id.sidebar_scroller);
        mWidgetRenderer = (WidgetRenderer) findViewById(R.id.widgets_container);

        // Create orientation listener. This should be done first because it
        // takes some time to get first orientation.
        mOrientationListener = new CameraOrientationEventListener(this);
        mOrientationListener.enable();

        mHandler = new Handler();

        // Setup the camera hardware and preview
        setupCamera();

        // Setup gesture detection
        mGestureDetector = new GestureDetector(this, new GestureListener());
        View.OnTouchListener touchListener = new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent ev) {
                if (ev.getAction() == MotionEvent.ACTION_UP) {
                    mSideBar.clampSliding();
                }

                mGestureDetector.onTouchEvent(ev);
                return true;
            }
        };

        mCamManager.getPreviewSurface().setOnTouchListener(touchListener);
        mSideBar.setOnTouchListener(touchListener);

        // Populate the sidebar buttons a little later (so we have camera parameters)
        mHandler.post(new Runnable() {
            public void run() {
                mSideBar.checkCapabilities(mCamManager.getParameters(), 
                        (ViewGroup) findViewById(R.id.widgets_container));
            }
        });
    }


    public void updateInterfaceOrientation() {
        final ViewGroup root = (ViewGroup) findViewById(R.id.shutter_button_container);
        setViewRotation(root, mOrientationCompensation);
        mSideBar.notifyOrientationChanged(mOrientationCompensation);
        mWidgetRenderer.notifyOrientationChanged(mOrientationCompensation);
    }

    protected void setupCamera() {
        // Setup the Camera hardware and preview surface
        mCamManager = new CameraManager(this);

        // Add the preview surface to its container
        final FrameLayout layout = (FrameLayout) findViewById(R.id.camera_preview_container);
        layout.addView(mCamManager.getPreviewSurface());

        if (!mCamManager.open(0)) {
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


    /**
     * Handles the orientation changes without turning the actual activity
     */
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

    /**
     * Handles the swipe and tap gestures on the lower layer of the screen
     * (ie. the preview surface)
     * @note Remember that the default orientation of the screen is landscape, thus
     * 	     the side bar is at the BOTTOM of the screen, and is swiped UP/DOWN. 
     */
    public class GestureListener extends GestureDetector.SimpleOnGestureListener {
        //private final static String TAG = "GestureListener";

        private static final int SWIPE_MIN_DISTANCE = 10;
        private static final int SWIPE_MAX_OFF_PATH = 250;
        private static final int SWIPE_THRESHOLD_VELOCITY = 200;
        // allow to drag the side bar up to half of the screen
        private static final int SIDEBAR_THRESHOLD_FACTOR = 2;

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,
                float distanceY) {			
            // Detect drag of the side bar
            if (Math.abs(e1.getX() - e2.getX()) > SWIPE_MAX_OFF_PATH) {
                // Finger drifted from the path
                return false;
            } else if (e1.getRawY() > Util.getScreenSize(CameraActivity.this).y/SIDEBAR_THRESHOLD_FACTOR) {
                if (e1.getY() - e2.getY() > SWIPE_MIN_DISTANCE ||
                        e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE) {
                    mSideBar.slide(-distanceY);
                }

                return true;
            }

            return false;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            try {
                if (Math.abs(e1.getX() - e2.getX()) > SWIPE_MAX_OFF_PATH)
                    return false;

                // swipes to open/close the sidebar 
                if(e1.getY() - e2.getY() > SWIPE_MIN_DISTANCE && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                    mSideBar.slideOpen();
                }  else if (e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                    mSideBar.slideClose();
                }
            } catch (Exception e) {
                // nothing
            }
            return false;
        }
    }


}