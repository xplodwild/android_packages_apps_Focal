/**
 * Copyright (C) 2013 The CyanogenMod Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.cyanogenmod.nemesis;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.cyanogenmod.nemesis.ui.ExposureHudRing;
import org.cyanogenmod.nemesis.ui.FocusHudRing;
import org.cyanogenmod.nemesis.ui.Notifier;
import org.cyanogenmod.nemesis.ui.PreviewFrameLayout;
import org.cyanogenmod.nemesis.ui.ReviewDrawer;
import org.cyanogenmod.nemesis.ui.SavePinger;
import org.cyanogenmod.nemesis.ui.ShutterButton;
import org.cyanogenmod.nemesis.ui.SideBar;
import org.cyanogenmod.nemesis.ui.SwitchRingPad;
import org.cyanogenmod.nemesis.ui.ThumbnailFlinger;
import org.cyanogenmod.nemesis.ui.WidgetRenderer;

public class CameraActivity extends Activity implements CameraManager.CameraReadyListener {
    public final static String TAG = "CameraActivity";

    public final static int CAMERA_MODE_PHOTO = 1;
    public final static int CAMERA_MODE_VIDEO = 2;
    public final static int CAMERA_MODE_PANO = 3;
    public final static int CAMERA_MODE_PICSPHERE = 4;

    private static int mCameraMode = CAMERA_MODE_PHOTO;

    private CameraManager mCamManager;
    private SnapshotManager mSnapshotManager;
    private MainSnapshotListener mSnapshotListener;
    private FocusManager mFocusManager;
    private CameraOrientationEventListener mOrientationListener;
    private GestureDetector mGestureDetector;
    private Handler mHandler;

    private int mOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;
    ;
    private int mOrientationCompensation = 0;

    private SideBar mSideBar;
    private WidgetRenderer mWidgetRenderer;
    private FocusHudRing mFocusHudRing;
    private ExposureHudRing mExposureHudRing;
    private SwitchRingPad mSwitchRingPad;
    private ShutterButton mShutterButton;
    private SavePinger mSavePinger;
    private ViewGroup mRecTimerContainer;
    private Notifier mNotifier;
    private ReviewDrawer mReviewDrawer;

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
        mSavePinger = (SavePinger) findViewById(R.id.save_pinger);

        mSwitchRingPad = (SwitchRingPad) findViewById(R.id.switch_ring_pad);
        mSwitchRingPad.setListener(new MainRingPadListener());

        mRecTimerContainer = (ViewGroup) findViewById(R.id.recording_timer_container);
        mNotifier = (Notifier) findViewById(R.id.notifier_container);

        mReviewDrawer = (ReviewDrawer) findViewById(R.id.review_drawer);

        // Create orientation listener. This should be done first because it
        // takes some time to get first orientation.
        mOrientationListener = new CameraOrientationEventListener(this);
        mOrientationListener.enable();

        // Create sensor listener, to detect shake gestures
        SensorManager sensorMgr = (SensorManager) getSystemService(SENSOR_SERVICE);
        sensorMgr.registerListener(new CameraSensorListener(), sensorMgr.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
                SensorManager.SENSOR_DELAY_NORMAL);

        mHandler = new Handler();

        // Setup the camera hardware and preview
        setupCamera();

        SoundManager.getSingleton().preload(this);

        // Setup HUDs
        mFocusHudRing = (FocusHudRing) findViewById(R.id.hud_ring_focus);

        mExposureHudRing = (ExposureHudRing) findViewById(R.id.hud_ring_exposure);
        mExposureHudRing.setManagers(mCamManager);

        // Setup shutter button
        mShutterButton = (ShutterButton) findViewById(R.id.btn_shutter);
        mShutterButton.setOnClickListener(new MainShutterClickListener());
        mShutterButton.setSlideListener(new MainShutterSlideListener());

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

        // Use SavePinger to animate a bit while we open the camera device
        mSavePinger.setPingOnly(true);
        mSavePinger.startSaving();


        updateCapabilities();
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

    /**
     * Returns the mode of the activity
     * See CameraActivity.CAMERA_MODE_*
     *
     * @return int
     */
    public static int getCameraMode() {
        return mCameraMode;
    }

    /**
     * Sets the mode of the activity
     * See CameraActivity.CAMERA_MODE_*
     *
     * @param newMode
     */
    public void setCameraMode(int newMode) {
        if (mCameraMode == newMode) return;

        mCameraMode = newMode;

        if (newMode == CAMERA_MODE_PHOTO) {
            mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_photo));
        } else if (newMode == CAMERA_MODE_VIDEO) {
            mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_video));
            mNotifier.notify(getString(R.string.double_tap_to_snapshot), 2500);
        } else if (newMode == CAMERA_MODE_PICSPHERE) {
            // PicSphere <3
        } else if (newMode == CAMERA_MODE_PANO) {

        }

        mCamManager.setCameraMode(mCameraMode);
        updateCapabilities();
    }

    /**
     * Updates the orientation of the whole UI (in place)
     * based on the calculations given by the orientation listener
     */
    public void updateInterfaceOrientation() {
        setViewRotation(mShutterButton, mOrientationCompensation);
        setViewRotation(mRecTimerContainer, mOrientationCompensation);
        setViewRotation(mNotifier, mOrientationCompensation);
        mCamManager.setOrientation(mOrientationCompensation);
        mSideBar.notifyOrientationChanged(mOrientationCompensation);
        mWidgetRenderer.notifyOrientationChanged(mOrientationCompensation);
        mSwitchRingPad.notifyOrientationChanged(mOrientationCompensation);
        mSavePinger.notifyOrientationChanged(mOrientationCompensation);
    }

    public void updateCapabilities() {
        // Populate the sidebar buttons a little later (so we have camera parameters)
        mHandler.post(new Runnable() {
            public void run() {
                Camera.Parameters params = mCamManager.getParameters();

                // We don't have the camera parameters yet, retry later
                if (params == null) {
                    mHandler.postDelayed(this, 100);
                } else {
                    // Update sidebar
                    mSideBar.checkCapabilities(mCamManager, (ViewGroup) findViewById(R.id.widgets_container));

                    // Update focus/exposure ring support
                    mFocusHudRing.setVisibility(mCamManager.isFocusAreaSupported() ? View.VISIBLE : View.GONE);
                    mExposureHudRing.setVisibility(mCamManager.isExposureAreaSupported() ? View.VISIBLE : View.GONE);
                }
            }
        });
    }

    protected void setupCamera() {
        // Setup the Camera hardware and preview surface
        mCamManager = new CameraManager(this);

        // Add the preview surface to its container
        final PreviewFrameLayout layout = (PreviewFrameLayout) findViewById(R.id.camera_preview_container);

        // if we resumed the activity, the preview surface will already be attached
        if (mCamManager.getPreviewSurface().getParent() != null) {
            ((ViewGroup) mCamManager.getPreviewSurface().getParent())
                    .removeView(mCamManager.getPreviewSurface());
        }

        layout.addView(mCamManager.getPreviewSurface());
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mCamManager.getPreviewSurface().getLayoutParams();
        params.addRule(RelativeLayout.CENTER_IN_PARENT);
        params.width = RelativeLayout.LayoutParams.WRAP_CONTENT;
        params.height = RelativeLayout.LayoutParams.WRAP_CONTENT;

        mCamManager.getPreviewSurface().setLayoutParams(params);
        mCamManager.setPreviewPauseListener(new CameraPreviewListener());
        mCamManager.setCameraReadyListener(this);

        mCamManager.open(Camera.CameraInfo.CAMERA_FACING_BACK);
    }

    @Override
    public void onCameraReady() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Camera.Size sz = Util.getOptimalPreviewSize(CameraActivity.this, mCamManager.getParameters().getSupportedPreviewSizes(), 1.33f);
                mCamManager.setPreviewSize(sz.width, sz.height);

                final PreviewFrameLayout layout = (PreviewFrameLayout) findViewById(R.id.camera_preview_container);
                layout.setAspectRatio((double) sz.width / sz.height);
                layout.setPreviewSize(sz.width, sz.height);

                if (mFocusManager == null) {
                    mFocusManager = new FocusManager(mCamManager);
                    mFocusManager.setListener(new MainFocusListener());
                    mFocusHudRing.setManagers(mCamManager, mFocusManager);
                }

                if (mSnapshotManager == null) {
                    mSnapshotManager = new SnapshotManager(mCamManager, mFocusManager, CameraActivity.this);
                    mSnapshotListener = new MainSnapshotListener();
                    mSnapshotManager.addListener(mSnapshotListener);
                }

                // Hide sidebar after start
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mSideBar.slideClose();
                    }
                }, 1500);


                mSavePinger.stopSaving();
            }
        });
    }

    public void onCameraFailed() {
        Log.e(TAG, "Could not open camera HAL");
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(CameraActivity.this,
                        getResources().getString(R.string.cannot_connect_hal),
                        Toast.LENGTH_LONG).show();
            }
        });
    }

    /**
     * Recursively rotates the Views of ViewGroups
     *
     * @param vg       the root ViewGroup
     * @param rotation the angle to which rotate the views
     */
    public static void setViewGroupRotation(ViewGroup vg, float rotation) {
        final int childCount = vg.getChildCount();

        for (int i = 0; i < childCount; i++) {
            View child = vg.getChildAt(i);

            if (child instanceof ViewGroup) {
                setViewGroupRotation((ViewGroup) child, rotation);
            } else {
                setViewRotation(child, rotation);
            }
        }
    }

    public static void setViewRotation(View v, float rotation) {
        v.animate().rotation(rotation).setDuration(200)
                .setInterpolator(new DecelerateInterpolator()).start();
    }

    /**
     * Listener that is called when the preview pauses or resumes
     */
    private class CameraPreviewListener implements CameraManager.PreviewPauseListener {

        @Override
        public void onPreviewPause() {
            final Bitmap preview = mCamManager.getLastPreviewFrame();

            if (preview == null) {
                return;
            }

            final PreviewFrameLayout container = (PreviewFrameLayout)
                    findViewById(R.id.camera_preview_overlay_container);

            final ImageView iv = (ImageView) findViewById(R.id.camera_preview_overlay);

            new Thread() {
                public void run() {
                    long time = System.currentTimeMillis();
                    final Bitmap blurredPreview = BitmapFilter.getSingleton()
                            .getBlur(CameraActivity.this, preview, 16);
                    long elapsed = System.currentTimeMillis()-time;

                    Log.v(TAG, "Took " + elapsed + "ms to blur");

                    runOnUiThread(new Runnable() {
                        public void run() {
                            container.setAspectRatio((float) preview.getWidth() / preview.getHeight());
                            container.setPreviewSize(preview.getWidth(), preview.getHeight());
                            iv.setImageBitmap(blurredPreview);
                            iv.setAlpha(1.0f);

                        }
                    });
                }
            }.start();
        }

        @Override
        public void onPreviewResume() {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    ImageView iv = (ImageView) findViewById(R.id.camera_preview_overlay);
                    iv.animate().setDuration(300).alpha(0.0f).start();
                }
            }, 100);

        }
    }

    /**
     * Listener that is called when a ring pad button is activated (finger release above)
     */
    private class MainRingPadListener implements SwitchRingPad.RingPadListener {
        @Override
        public void onButtonActivated(int eventId) {
            switch (eventId) {
                case SwitchRingPad.BUTTON_CAMERA:
                    setCameraMode(CAMERA_MODE_PHOTO);
                    break;

                case SwitchRingPad.BUTTON_VIDEO:
                    setCameraMode(CAMERA_MODE_VIDEO);
                    break;


                case SwitchRingPad.BUTTON_SWITCHCAM:
                    if (mCamManager.getCurrentFacing() == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                        mCamManager.open(Camera.CameraInfo.CAMERA_FACING_BACK);
                    } else {
                        mCamManager.open(Camera.CameraInfo.CAMERA_FACING_FRONT);
                    }
                    updateCapabilities();
                    break;
            }
        }
    }

    /**
     * Listener that is called when shutter button is slided, to open ring pad view
     */
    private class MainShutterSlideListener implements ShutterButton.ShutterSlideListener {

        @Override
        public void onSlideOpen() {
            mSwitchRingPad.animateOpen();
        }

        @Override
        public void onSlideClose() {
            mSwitchRingPad.animateClose();
        }

        @Override
        public boolean onMotionEvent(MotionEvent ev) {
            return mSwitchRingPad.onTouchEvent(ev);
        }

        @Override
        public void onShutterButtonPressed() {
            // Animate the ring pad
            mSwitchRingPad.animateHint();

            // Make the review drawer super translucent if it is open
            mReviewDrawer.setTemporaryHide(true);

            // Lock automatic settings
            mCamManager.setLockSetup(true);
        }

    }

    /**
     * When the shutter button is pressed
     */
    public class MainShutterClickListener implements OnClickListener {
        @Override
        public void onClick(View v) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mReviewDrawer.setTemporaryHide(false);
                }
            }, 500);

            if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
                // XXX: Check for HDR, exposure, burst shots, timer, ...
                mSnapshotManager.queueSnapshot(true, 0);
            } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
                if (!mSnapshotManager.isRecording()) {
                    mSnapshotManager.startVideo();
                    mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_stop));
                } else {
                    mSnapshotManager.stopVideo();
                    mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_video));
                }
            }
        }
    }


    /**
     * Focus listener to animate the focus HUD ring from FocusManager events
     */
    private class MainFocusListener implements FocusManager.FocusListener {

        @Override
        public void onFocusStart(final boolean smallAdjust) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mFocusHudRing.animateWorking(smallAdjust ? 200 : 1500);
                }
            });

        }

        @Override
        public void onFocusReturns(final boolean smallAdjust, final boolean success) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mFocusHudRing.animatePressUp();

                    if (!smallAdjust) {
                        mFocusHudRing.setFocusImage(success);
                    } else {
                        mFocusHudRing.setFocusImage(true);
                    }
                }
            });
        }
    }

    /**
     * Snapshot listener for when snapshots are taken, in SnapshotManager
     */
    private class MainSnapshotListener implements SnapshotManager.SnapshotListener {
        private long mRecordingStartTimestamp;
        private TextView mTimerTv;
        private boolean mIsRecording;

        private Runnable mUpdateTimer = new Runnable() {
            @Override
            public void run() {
                long recordingDurationMs = System.currentTimeMillis() - mRecordingStartTimestamp;
                int minutes = (int) Math.floor(recordingDurationMs / 60000.0);
                int seconds = (int) recordingDurationMs / 1000 - minutes * 60;

                mTimerTv.setText(String.format("%02d:%02d", minutes, seconds));

                // Loop infinitely until recording stops
                if (mIsRecording) {
                    mHandler.postDelayed(this, 500);
                }
            }
        };

        @Override
        public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {
            FrameLayout layout = (FrameLayout) findViewById(R.id.thumb_flinger_container);

            // Fling the preview
            ThumbnailFlinger flinger = new ThumbnailFlinger(CameraActivity.this);
            layout.addView(flinger);
            flinger.setImageBitmap(info.mThumbnail);
            flinger.doAnimation();

            // Unlock camera auto settings
            mCamManager.setLockSetup(false);
        }

        @Override
        public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

        }

        @Override
        public void onSnapshotProcessed(SnapshotManager.SnapshotInfo info) {

        }

        @Override
        public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {
            String uriStr = info.mUri.toString();

            // Add the new image to the gallery and the review drawer
            int originalImageId = Integer.parseInt(uriStr.substring(uriStr.lastIndexOf("/") + 1, uriStr.length()));
            Log.v(TAG, "Adding snapshot to gallery: " + originalImageId);
            mReviewDrawer.addImageToList(originalImageId);
            mReviewDrawer.setPreviewedImage(originalImageId);
        }

        @Override
        public void onMediaSavingStart() {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mSavePinger.setPingOnly(false);
                    mSavePinger.startSaving();
                }
            });
        }

        @Override
        public void onMediaSavingDone() {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mSavePinger.stopSaving();
                }
            });
        }

        @Override
        public void onVideoRecordingStart() {
            mTimerTv = (TextView) findViewById(R.id.recording_timer_text);
            mRecordingStartTimestamp = System.currentTimeMillis();
            mIsRecording = true;

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mHandler.post(mUpdateTimer);
                    mRecTimerContainer.setVisibility(View.VISIBLE);
                }
            });
        }

        @Override
        public void onVideoRecordingStop() {
            mIsRecording = false;

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mRecTimerContainer.setVisibility(View.GONE);
                }
            });
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

    private class CameraSensorListener implements SensorEventListener {
        public final static int SHAKE_CLOSE_THRESHOLD = 15;
        public final static int SHAKE_OPEN_THRESHOLD = -15;

        private float mGravity[] = new float[3];
        private float mAccel[] = new float[3];

        private long mLastShakeTimestamp = 0;

        @Override
        public void onSensorChanged(SensorEvent sensorEvent) {
            // alpha is calculated as t / (t + dT)
            // with t, the low-pass filter's time-constant
            // and dT, the event delivery rate
            final float alpha = 0.8f;

            mGravity[0] = alpha * mGravity[0] + (1 - alpha) * sensorEvent.values[0];
            mGravity[1] = alpha * mGravity[1] + (1 - alpha) * sensorEvent.values[1];
            mGravity[2] = alpha * mGravity[2] + (1 - alpha) * sensorEvent.values[2];

            mAccel[0] = sensorEvent.values[0] - mGravity[0];
            mAccel[1] = sensorEvent.values[1] - mGravity[1];
            mAccel[2] = sensorEvent.values[2] - mGravity[2];

            // If we aren't just opening the sidebar
            if ((System.currentTimeMillis() - mLastShakeTimestamp) < 1000) {
                return;
            }

            if (mAccel[0] > SHAKE_CLOSE_THRESHOLD && mSideBar.isOpen()) {
                mSideBar.slideClose();

                if (mWidgetRenderer.getWidgetsCount() > 0) {
                    mWidgetRenderer.hideWidgets();
                }

                mLastShakeTimestamp = System.currentTimeMillis();
            } else if (mAccel[0] < SHAKE_OPEN_THRESHOLD && !mSideBar.isOpen()) {
                mSideBar.slideOpen();

                if (mWidgetRenderer.getWidgetsCount() > 0) {
                    mWidgetRenderer.restoreWidgets();
                }

                mLastShakeTimestamp = System.currentTimeMillis();
            }

        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int i) {

        }
    }

    /**
     * Handles the swipe and tap gestures on the lower layer of the screen
     * (ie. the preview surface)
     *
     * @note Remember that the default orientation of the screen is landscape, thus
     * the side bar is at the BOTTOM of the screen, and is swiped UP/DOWN.
     */
    public class GestureListener extends GestureDetector.SimpleOnGestureListener {
        //private final static String TAG = "GestureListener";

        private static final int SWIPE_MIN_DISTANCE = 10;
        private static final int SWIPE_MAX_OFF_PATH = 250;
        private static final int SWIPE_THRESHOLD_VELOCITY = 200;
        // allow to drag the side bar up to half of the screen
        private static final int SIDEBAR_THRESHOLD_FACTOR = 2;

        private boolean mCancelSwipe = false;

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            // A single tap equals to touch-to-focus in all modes
            mFocusHudRing.setPosition(e.getRawX(), e.getRawY());
            mFocusManager.refocus();

            return super.onSingleTapConfirmed(e);
        }

        @Override
        public boolean onDoubleTap(MotionEvent e) {
            // In VIDEO mode, a double tap focuses
            if (mCameraMode == CAMERA_MODE_VIDEO) {
                mSnapshotManager.queueSnapshot(true, 0);
            }

            return super.onDoubleTap(e);
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,
                                float distanceY) {
            // Detect drag of the side bar
            if (Math.abs(e1.getX() - e2.getX()) > SWIPE_MAX_OFF_PATH) {
                // Finger drifted from the path
                return false;
            } else if (e1.getRawY() > Util.getScreenSize(CameraActivity.this).y / SIDEBAR_THRESHOLD_FACTOR) {
                if (e1.getY() - e2.getY() > SWIPE_MIN_DISTANCE ||
                        e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE) {
                    mSideBar.slide(-distanceY);
                    mWidgetRenderer.notifySidebarSlideStatus(-distanceY);
                    mCancelSwipe = true;
                }

                return true;
            }

            return false;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            try {
                if (Math.abs(e1.getX() - e2.getX()) < SWIPE_MAX_OFF_PATH) {
                    // swipes to open/close the sidebar and/or hide/restore the widgets
                    if (e1.getY() - e2.getY() > SWIPE_MIN_DISTANCE
                            && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                        if (mWidgetRenderer.isHidden() && mWidgetRenderer.getWidgetsCount() > 0) {
                            mWidgetRenderer.restoreWidgets();
                        } else {
                            mSideBar.slideOpen();
                            mWidgetRenderer.notifySidebarSlideOpen();
                        }
                    } else if (e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE
                            && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                        if (mSideBar.isOpen()) {
                            mSideBar.slideClose();
                            mWidgetRenderer.notifySidebarSlideClose();
                        } else if (!mWidgetRenderer.isHidden()
                                && mWidgetRenderer.getWidgetsCount() > 0
                                && !mCancelSwipe) {
                            mWidgetRenderer.hideWidgets();
                        }
                    }
                } else if (Math.abs(e1.getY() - e2.getY()) < SWIPE_MAX_OFF_PATH) {
                    // swipes up/down to open/close the review drawer
                    if (e1.getX() - e2.getX() > SWIPE_MIN_DISTANCE && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
                        mReviewDrawer.close();
                    } else if (e2.getX() - e1.getX() > SWIPE_MIN_DISTANCE && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
                        mReviewDrawer.open();
                    }
                }
            } catch (Exception e) {
                // nothing
            }

            mCancelSwipe = false;

            return false;
        }
    }


}