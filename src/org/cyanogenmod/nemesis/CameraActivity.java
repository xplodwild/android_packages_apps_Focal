package org.cyanogenmod.nemesis;

import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
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
import android.widget.Toast;

import org.cyanogenmod.nemesis.ui.PreviewFrameLayout;
import org.cyanogenmod.nemesis.ui.ShutterButton;
import org.cyanogenmod.nemesis.ui.SideBar;
import org.cyanogenmod.nemesis.ui.ThumbnailFlinger;
import org.cyanogenmod.nemesis.ui.WidgetRenderer;

public class CameraActivity extends Activity {
    public final static String TAG = "CameraActivity";

    private CameraManager mCamManager;
    private SnapshotManager mSnapshotManager;
    private MainSnapshotListener mSnapshotListener;
    private FocusManager mFocusManager;
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

        getWindow().getDecorView()
                .setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE);

        mSideBar = (SideBar) findViewById(R.id.sidebar_scroller);
        mWidgetRenderer = (WidgetRenderer) findViewById(R.id.widgets_container);

        // Create orientation listener. This should be done first because it
        // takes some time to get first orientation.
        mOrientationListener = new CameraOrientationEventListener(this);
        mOrientationListener.enable();

        mHandler = new Handler();

        // Setup the camera hardware and preview
        setupCamera();

        SoundManager.getSingleton().preload(this);

        ShutterButton shutterButton = (ShutterButton) findViewById(R.id.btn_shutter);
        shutterButton.setOnClickListener(new ShutterButton.ClickListener(mSnapshotManager));

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
                Camera.Parameters params = mCamManager.getParameters();

                // We don't have the camera parameters yet, retry later
                if (params == null) {
                    Log.w(TAG, "No camera params yet, posting again");
                    mHandler.postDelayed(this, 100);
                } else {
                    mSideBar.checkCapabilities(mCamManager, (ViewGroup) findViewById(R.id.widgets_container));
                }
            }
        });
    }


    @Override
    protected void onPause() {
        // Pause the camera preview
        mCamManager.pause();

        super.onPause();
    }

    @Override
    protected void onResume() {
        // Restore the camera preview
        mCamManager.resume();

        super.onResume();
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
        mSnapshotManager = new SnapshotManager(mCamManager, this);
        mSnapshotListener = new MainSnapshotListener();
        mSnapshotManager.addListener(mSnapshotListener);
        mFocusManager = new FocusManager(mCamManager);

        // Add the preview surface to its container
        final PreviewFrameLayout layout = (PreviewFrameLayout) findViewById(R.id.camera_preview_container);

        // if we resumed the activity, the preview surface will already be attached
        if (mCamManager.getPreviewSurface().getParent() != null)
            ((ViewGroup)mCamManager.getPreviewSurface().getParent()).removeView(mCamManager.getPreviewSurface());

        layout.addView(mCamManager.getPreviewSurface());


        if (!mCamManager.open(0)) {
            Log.e(TAG, "Could not open camera HAL");
            Toast.makeText(this, getResources().getString(R.string.cannot_connect_hal), Toast.LENGTH_LONG).show();
            return;
        }

        Camera.Size sz = Util.getOptimalPreviewSize(this, mCamManager.getParameters().getSupportedPreviewSizes(), 1.33f);
        mCamManager.setPreviewSize(sz.width, sz.height);

        layout.setAspectRatio((double) sz.width / sz.height);
        layout.setPreviewSize(sz.width, sz.height);
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
     * Snapshot listener for when snapshots are taken, in SnapshotManager
     */
    private class MainSnapshotListener implements SnapshotManager.SnapshotListener {

        @Override
        public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {
            FrameLayout layout = (FrameLayout) findViewById(R.id.thumb_flinger_container);
            ThumbnailFlinger flinger = new ThumbnailFlinger(CameraActivity.this);
            layout.addView(flinger);
            flinger.setImageBitmap(info.mThumbnail);
            flinger.doAnimation();
        }

        @Override
        public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

        }

        @Override
        public void onSnapshotProcessed(SnapshotManager.SnapshotInfo info) {

        }

        @Override
        public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {

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
                    mWidgetRenderer.notifySidebarSlideStatus(-distanceY);
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
                    mWidgetRenderer.notifySidebarSlideOpen();
                }  else if (e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                    mSideBar.slideClose();
                    mWidgetRenderer.notifySidebarSlideClose();
                }
            } catch (Exception e) {
                // nothing
            }
            return false;
        }
    }


}