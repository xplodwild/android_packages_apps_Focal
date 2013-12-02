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
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.hardware.Camera;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.ScaleGestureDetector;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import org.cyanogenmod.focal.feats.CaptureTransformer;
import org.cyanogenmod.focal.feats.SoftwareHdrCapture;
import org.cyanogenmod.focal.pano.MosaicProxy;
import org.cyanogenmod.focal.picsphere.PicSphereCaptureTransformer;
import org.cyanogenmod.focal.picsphere.PicSphereManager;
import org.cyanogenmod.focal.ui.CircleTimerView;
import org.cyanogenmod.focal.ui.ExposureHudRing;
import org.cyanogenmod.focal.ui.FocusHudRing;
import org.cyanogenmod.focal.ui.Notifier;
import org.cyanogenmod.focal.ui.PanoProgressBar;
import org.cyanogenmod.focal.ui.ReviewDrawer;
import org.cyanogenmod.focal.ui.SavePinger;
import org.cyanogenmod.focal.ui.ShutterButton;
import org.cyanogenmod.focal.ui.SideBar;
import org.cyanogenmod.focal.ui.SwitchRingPad;
import org.cyanogenmod.focal.ui.ThumbnailFlinger;
import org.cyanogenmod.focal.ui.WidgetRenderer;
import org.cyanogenmod.focal.ui.showcase.ShowcaseView;

import fr.xplod.focal.R;

public class CameraActivity extends Activity implements CameraManager.CameraReadyListener,
        ShowcaseView.OnShowcaseEventListener {
    public final static String TAG = "CameraActivity";

    public final static int CAMERA_MODE_PHOTO     = 1;
    public final static int CAMERA_MODE_VIDEO     = 2;
    public final static int CAMERA_MODE_PANO      = 3;
    public final static int CAMERA_MODE_PICSPHERE = 4;

    // whether or not to enable profiling
    private final static boolean DEBUG_PROFILE = true;

    private static int mCameraMode = CAMERA_MODE_PHOTO;

    private CameraManager mCamManager;
    private SnapshotManager mSnapshotManager;
    private MainSnapshotListener mSnapshotListener;
    private FocusManager mFocusManager;
    private PicSphereManager mPicSphereManager;
    private MosaicProxy mMosaicProxy;
    private CameraOrientationEventListener mOrientationListener;
    private GestureDetector mGestureDetector;
    private CaptureTransformer mCaptureTransformer;
    private Handler mHandler;
    private boolean mPaused;

    private int mOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;
    private int mOrientationCompensation = 0;

    private SideBar mSideBar;
    private WidgetRenderer mWidgetRenderer;
    private FocusHudRing mFocusHudRing;
    private ExposureHudRing mExposureHudRing;
    private SwitchRingPad mSwitchRingPad;
    private ShutterButton mShutterButton;
    private SavePinger mSavePinger;
    private PanoProgressBar mPanoProgressBar;
    private Button mPicSphereUndo;
    private CircleTimerView mTimerView;
    private ViewGroup mRecTimerContainer;
    private static Notifier mNotifier;
    private ReviewDrawer mReviewDrawer;
    private ScaleGestureDetector mZoomGestureDetector;
    private TextView mHelperText;
    private ShowcaseView mShowcaseView;
    private boolean mHasPinchZoomed;
    private boolean mCancelSideBarClose;
    private boolean mIsFocusButtonDown;
    private boolean mIsShutterButtonDown;
    private boolean mUserWantsExposureRing;
    private boolean mIsFullscreenShutter;
    private int mShowcaseIndex;
    private boolean mIsCamSwitching;
    private boolean mIsShutterLongClicked = false;
    private CameraPreviewListener mCamPreviewListener;
    private GLSurfaceView mGLSurfaceView;
    private boolean mIsFocusing = false;

    private final static int SHOWCASE_INDEX_WELCOME_1 = 0;
    private final static int SHOWCASE_INDEX_WELCOME_2 = 1;
    private final static int SHOWCASE_INDEX_PANORAMA  = 0;
    private final static int SHOWCASE_INDEX_PICSPHERE = 0;

    private final static String KEY_SHOWCASE_WELCOME = "SHOWCASE_WELCOME";
    private final static String KEY_SHOWCASE_PANORAMA = "SHOWCASE_PANORAMA";
    private final static String KEY_SHOWCASE_PICSPHERE = "SHOWCASE_PICSPHERE";

    /**
     * Gesture listeners to apply on camera previews views
     */
    private View.OnTouchListener mPreviewTouchListener =  new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent ev) {
            if (ev.getAction() == MotionEvent.ACTION_UP) {
                mSideBar.clampSliding();
                mReviewDrawer.clampSliding();
            }

            // Process HUD gestures only if we aren't pinching
            mHasPinchZoomed = false;
            mZoomGestureDetector.onTouchEvent(ev);

            if (!mHasPinchZoomed) {
                mGestureDetector.onTouchEvent(ev);
            }

            return true;
        }
    };

    /**
     * Event: Activity created
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);

        mPaused = false;
        mIsCamSwitching = false;

        getWindow().getDecorView()
                .setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE);

        mUserWantsExposureRing = true;
        mIsFullscreenShutter = false;

        mSideBar = (SideBar) findViewById(R.id.sidebar_scroller);
        mWidgetRenderer = (WidgetRenderer) findViewById(R.id.widgets_container);
        mSavePinger = (SavePinger) findViewById(R.id.save_pinger);
        mTimerView = (CircleTimerView) findViewById(R.id.timer_view);
        mHelperText = (TextView) findViewById(R.id.txt_helper);
        mPicSphereUndo = (Button) findViewById(R.id.btn_picsphere_undo);

        mSwitchRingPad = (SwitchRingPad) findViewById(R.id.switch_ring_pad);
        mSwitchRingPad.setListener(new MainRingPadListener());

        mPanoProgressBar = (PanoProgressBar) findViewById(R.id.panorama_progress_bar);
        mRecTimerContainer = (ViewGroup) findViewById(R.id.recording_timer_container);
        mNotifier = (Notifier) findViewById(R.id.notifier_container);

        mReviewDrawer = (ReviewDrawer) findViewById(R.id.review_drawer);

        // Create orientation listener. This should be done first because it
        // takes some time to get first orientation.
        mOrientationListener = new CameraOrientationEventListener(this);
        mOrientationListener.enable();

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
        MainShutterClickListener shutterClickListener = new MainShutterClickListener();
        mShutterButton.setOnClickListener(shutterClickListener);
        mShutterButton.setOnLongClickListener(shutterClickListener);
        mShutterButton.setOnTouchListener(shutterClickListener);
        mShutterButton.setSlideListener(new MainShutterSlideListener());

        // Setup gesture detection
        mGestureDetector = new GestureDetector(this, new GestureListener());
        mZoomGestureDetector = new ScaleGestureDetector(this, new ZoomGestureListener());

        findViewById(R.id.gl_renderer_container).setOnTouchListener(mPreviewTouchListener);

        // Use SavePinger to animate a bit while we open the camera device
        mSavePinger.setPingMode(SavePinger.PING_MODE_SIMPLE);
        mSavePinger.startSaving();

        // Hack because review drawer size might not be measured yet
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mReviewDrawer.open();
                mReviewDrawer.close();
            }
        }, 300);

        startShowcaseWelcome();
    }

    public int getOrientation() {
        return mOrientationCompensation;
    }

    public void startShowcaseWelcome() {
        if (SettingsStorage.getAppSetting(this, KEY_SHOWCASE_WELCOME, "0").equals("0")) {
            SettingsStorage.storeAppSetting(this, KEY_SHOWCASE_WELCOME, "1");
            ShowcaseView.ConfigOptions co = new ShowcaseView.ConfigOptions();
            co.hideOnClickOutside = true;
            mShowcaseView = ShowcaseView.insertShowcaseView(mSideBar,
                    this, getString(R.string.showcase_welcome_1_title),
                    getString(R.string.showcase_welcome_1_body), co);

            // Animate gesture
            Point size = new Point();
            getWindowManager().getDefaultDisplay().getSize(size);

            mShowcaseView.animateGesture(size.x/2, size.y*2.0f/3.0f, size.x/2, size.y/2.0f);
            mShowcaseView.setOnShowcaseEventListener(this);
            mShowcaseIndex = SHOWCASE_INDEX_WELCOME_1;
        }
    }

    public void startShowcasePanorama() {
        if (SettingsStorage.getAppSetting(this, KEY_SHOWCASE_PANORAMA, "0").equals("0")) {
            SettingsStorage.storeAppSetting(this, KEY_SHOWCASE_PANORAMA, "1");
            ShowcaseView.ConfigOptions co = new ShowcaseView.ConfigOptions();
            co.hideOnClickOutside = true;
            Point size = new Point();
            getWindowManager().getDefaultDisplay().getSize(size);
            mShowcaseView = ShowcaseView.insertShowcaseView(size.x/2, size.y - Util.dpToPx(this, 16),
                    this, getString(R.string.showcase_panorama_title),
                    getString(R.string.showcase_panorama_body), co);

            mShowcaseIndex = SHOWCASE_INDEX_PANORAMA;
        }
    }

    public void startShowcasePicSphere() {
        if (SettingsStorage.getAppSetting(this, KEY_SHOWCASE_PICSPHERE, "0").equals("0")) {
            SettingsStorage.storeAppSetting(this, KEY_SHOWCASE_PICSPHERE, "1");
            ShowcaseView.ConfigOptions co = new ShowcaseView.ConfigOptions();
            co.hideOnClickOutside = true;
            Point size = new Point();
            getWindowManager().getDefaultDisplay().getSize(size);

            mShowcaseView = ShowcaseView.insertShowcaseView(size.x / 2,
                    size.y - Util.dpToPx(this, 16), this, getString(R.string.showcase_picsphere_title),
                    getString(R.string.showcase_picsphere_body), co);

            mShowcaseIndex = SHOWCASE_INDEX_PICSPHERE;

            mShowcaseView.notifyOrientationChanged(mOrientationCompensation);
        }
    }

    @Override
    protected void onPause() {
        // Pause the camera preview
        mPaused = true;

        if (mCamManager != null) {
            mCamManager.pause();
        }

        if (mSnapshotManager != null) {
            mSnapshotManager.onPause();
        }

        if (mOrientationListener != null) {
            mOrientationListener.disable();
        }

        if (mPicSphereManager != null) {
            mPicSphereManager.onPause();
        }

        if (SoftwareHdrCapture.isServiceBound()) {
            try {
                unbindService(SoftwareHdrCapture.getServiceConnection());
            } catch (IllegalArgumentException e) {
                // Do nothing
            }
        }

        // Reset capture transformers on pause, if we are in
        // PicSphere mode
        if (mCameraMode == CAMERA_MODE_PICSPHERE) {
            mCaptureTransformer = null;
        }

        super.onPause();
    }

    @Override
    protected void onResume() {
        // Restore the camera preview
        mPaused = false;

        if (mCamManager != null) {
            mCamManager.resume();
        }

        super.onResume();

        if (mSnapshotManager != null) {
            mSnapshotManager.onResume();
        }

        if (mPicSphereManager != null) {
            mPicSphereManager.onResume();
        }

        mOrientationListener.enable();

        mReviewDrawer.close();
    }

    @Override
    public void onBackPressed() {
        if (mReviewDrawer.isOpen()) {
            mReviewDrawer.close();
        } else {
            super.onBackPressed();
        }
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
     * Notify, like a toast, but orientation aware
     * @param text The text to show
     * @param lengthMs The duration
     */
    public static void notify(String text, int lengthMs) {
        mNotifier.notify(text, lengthMs);
    }

    /**
     * Notify, like a toast, but orientation aware at the specified position
     * @param text The text to show
     * @param lengthMs The duration
     *
     */
    public static void notify(String text, int lengthMs, float x, float y) {
        mNotifier.notify(text, lengthMs, x, y);
    }

    /**
     * @return The Panorama Progress Bar view
     */
    public PanoProgressBar getPanoProgressBar() {
        return mPanoProgressBar;
    }

    public void displayOverlayBitmap(Bitmap bmp) {
        final ImageView iv = (ImageView) findViewById(R.id.camera_preview_overlay);
        iv.setImageBitmap(bmp);
        iv.setAlpha(1.0f);
        iv.setScaleType(ImageView.ScaleType.FIT_CENTER);
        Util.fadeIn(iv);
        iv.setVisibility(View.VISIBLE);
    }

    public void hideOverlayBitmap() {
        final ImageView iv = (ImageView) findViewById(R.id.camera_preview_overlay);
        Util.fadeOut(iv);
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                iv.setVisibility(View.GONE);
            }
        }, 300);
    }

    /**
     * Sets the mode of the activity
     * See CameraActivity.CAMERA_MODE_*
     *
     * @param newMode
     */
    public void setCameraMode(final int newMode) {
        if (mCameraMode == newMode) {
            return;
        }

        if (mCamManager.getParameters() == null) {
            mHandler.post(new Runnable() {
                public void run() {
                    setCameraMode(newMode);
                }
            });
        }

        if (mCamPreviewListener != null) {
            mCamPreviewListener.onPreviewPause();
        }

        setHelperText("");

        // Reset PicSphere 3D renderer if we were in PS mode
        if (mCameraMode == CAMERA_MODE_PICSPHERE) {
            resetPicSphere();
        } else if (mCameraMode == CAMERA_MODE_PANO) {
            resetPanorama();
        } 
        else if (mCameraMode == CAMERA_MODE_VIDEO){
            // must release the camera
            // to reset internals - at least on find5
            mCamManager.pause();
            mCamManager.resume();
        }

        mCameraMode = newMode;

        // Reset any capture transformer
        mCaptureTransformer = null;

        if (newMode == CAMERA_MODE_PHOTO) {
            mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_photo));
            mCamManager.setStabilization(false);
        } else if (newMode == CAMERA_MODE_VIDEO) {
            mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_video));
            mCamManager.setStabilization(true);
            mNotifier.notify(getString(R.string.double_tap_to_snapshot), 2500);
        } else if (newMode == CAMERA_MODE_PICSPHERE) {
            initializePicSphere();
            mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_photo));
            startShowcasePicSphere();
        } else if (newMode == CAMERA_MODE_PANO) {
            mShutterButton.setImageDrawable(getResources().getDrawable(R.drawable.btn_shutter_photo));
        }

        mCamManager.setCameraMode(mCameraMode);

        if (newMode == CAMERA_MODE_PANO) {
            initializePanorama();
            startShowcasePanorama();
        }

        // Reload pictures in the ReviewDrawer
        mReviewDrawer.updateFromGallery(newMode != CAMERA_MODE_VIDEO, 0);
        mHandler.post(new Runnable() {
            public void run() {
                updateCapabilities();
            }
        });
    }

    /**
     * Sets the active capture transformer. See {@link CaptureTransformer} for
     * more details on what's a capture transformer.
     *
     * @param transformer The new transformer to apply
     */
    public void setCaptureTransformer(CaptureTransformer transformer) {
        if (mCaptureTransformer != null) {
            mSnapshotManager.removeListener(mCaptureTransformer);
        }
        mCaptureTransformer = transformer;

        if (mCaptureTransformer != null && mSnapshotManager != null) {
            mSnapshotManager.addListener(transformer);
        }
    }

    /**
     * Updates the orientation of the whole UI (in place)
     * based on the calculations given by the orientation listener
     */
    public void updateInterfaceOrientation() {
        setViewRotation(mShutterButton, mOrientationCompensation);
        setViewRotation(mRecTimerContainer, mOrientationCompensation);
        setViewRotation(mPanoProgressBar, mOrientationCompensation);
        setViewRotation(mPicSphereUndo, mOrientationCompensation);
        setViewRotation(mHelperText, mOrientationCompensation);
        mNotifier.notifyOrientationChanged(mOrientationCompensation);
        mSideBar.notifyOrientationChanged(mOrientationCompensation);
        mWidgetRenderer.notifyOrientationChanged(mOrientationCompensation);
        mSwitchRingPad.notifyOrientationChanged(mOrientationCompensation);
        mSavePinger.notifyOrientationChanged(mOrientationCompensation);
        mReviewDrawer.notifyOrientationChanged(mOrientationCompensation);
    }

    public void updateCapabilities() {
        // Populate the sidebar buttons a little later (so we have camera parameters)
        mHandler.post(new Runnable() {
            public void run() {
                Camera.Parameters params = mCamManager.getParameters();

                // We don't have the camera parameters yet, retry later
                if (params == null) {
                    if (!mPaused) {
                        mHandler.postDelayed(this, 100);
                    }
                } else {
                    mCamManager.startParametersBatch();

                    // Close all widgets
                    mWidgetRenderer.closeAllWidgets();

                    // Update focus/exposure ring support
                    updateRingsVisibility();

                    // Update sidebar
                    mSideBar.checkCapabilities(CameraActivity.this,
                            (ViewGroup) findViewById(R.id.widgets_container));

                    // Set orientation
                    updateInterfaceOrientation();

                    mCamManager.stopParametersBatch();
                }
            }
        });
    }

    public void updateRingsVisibility() {
        // Rings logic:
        //    * PicSphere and panorama don't need it (infinity focus when possible)
        //    * Show focus all the time otherwise in photo and video
        //    * Show exposure ring in photo and video, if it's not toggled off
        //    * Fullscreen shutter hides all the rings
        if ((mCameraMode == CAMERA_MODE_PHOTO && !mIsFullscreenShutter)
                || mCameraMode == CAMERA_MODE_VIDEO) {
            mFocusHudRing.setVisibility(mCamManager.isFocusAreaSupported() ?
                    View.VISIBLE : View.GONE);
            mExposureHudRing.setVisibility(mCamManager.isExposureAreaSupported()
                    && mUserWantsExposureRing ? View.VISIBLE : View.GONE);
        } else {
            mFocusHudRing.setVisibility(View.GONE);
            mExposureHudRing.setVisibility(View.GONE);
        }
    }

    public boolean isExposureRingVisible() {
        return (mExposureHudRing.getVisibility() == View.VISIBLE);
    }

    public void setExposureRingVisible(boolean visible) {
        mUserWantsExposureRing = visible;
        updateRingsVisibility();

        // Internally reset the position of the exposure ring, while still
        // leaving it at its position so that if the user toggles it back
        // on, it will appear at its previous location
        mCamManager.setExposurePoint(0, 0);
    }

    public void startTimerCountdown(int timeMs) {
        mTimerView.animate().alpha(1.0f).setDuration(300).start();
        mTimerView.setIntervalTime(timeMs);
        mTimerView.startIntervalAnimation();
    }

    public void hideTimerCountdown() {
        mTimerView.animate().alpha(0.0f).setDuration(300).start();
    }

    protected void setupCamera() {
        // Setup the Camera hardware and preview
        mCamManager = new CameraManager(this);
        ((CameraApplication)getApplication()).setCameraManager(mCamManager);

        setGLRenderer(mCamManager.getRenderer());

        mCamPreviewListener = new CameraPreviewListener();
        mCamManager.setPreviewPauseListener(mCamPreviewListener);
        mCamManager.setCameraReadyListener(this);

        mCamManager.open(Camera.CameraInfo.CAMERA_FACING_BACK);
    }

    @Override
    public void onCameraReady() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Profiler.getDefault().start("OnCameraReady");
                Camera.Parameters params = mCamManager.getParameters();

                if (params == null) {
                    // Are we too fast? Let's try again.
                    mHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            onCameraReady();
                        }
                    }, 20);
                    return;
                }

                mCamManager.updateDisplayOrientation();
                Camera.Size picSize = params.getPictureSize();

                Camera.Size sz = Util.getOptimalPreviewSize(CameraActivity.this, params.getSupportedPreviewSizes(),
                        ((float) picSize.width / (float) picSize.height));
                if (sz == null) {
                    Log.e(TAG, "No preview size!! Something terribly wrong with camera!");
                    return;
                }
                //mCamManager.setPreviewSize(sz.width, sz.height);

                if (mIsCamSwitching) {
                    mCamManager.restartPreviewIfNeeded();
                    mIsCamSwitching = false;
                }

                if (mFocusManager == null) {
                    mFocusManager = new FocusManager(mCamManager);
                    mFocusManager.setListener(new MainFocusListener());
                }

                mFocusHudRing.setManagers(mCamManager, mFocusManager);

                if (mSnapshotManager == null) {
                    mSnapshotManager = new SnapshotManager(mCamManager, mFocusManager, CameraActivity.this);
                    mSnapshotListener = new MainSnapshotListener();
                    mSnapshotManager.addListener(mSnapshotListener);
                }

                // Hide sidebar after start
                mCancelSideBarClose = false;
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (!mCancelSideBarClose) {
                            mSideBar.slideClose();
                            mWidgetRenderer.notifySidebarSlideClose();
                        }
                    }
                }, 1500);

                Profiler.getDefault().start("OnCameraReady-updateCapa");
                updateCapabilities();
                Profiler.getDefault().logProfile("OnCameraReady-updateCapa");

                mSavePinger.stopSaving();
                Profiler.getDefault().logProfile("OnCameraReady");
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

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_FOCUS:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                // Use the volume down button as focus button
                if (!mIsFocusButtonDown) {
                    mCamManager.doAutofocus(mFocusManager);
                    mCamManager.setLockSetup(true);
                    mIsFocusButtonDown = true;
                }
                return true;
            case KeyEvent.KEYCODE_CAMERA:
            case KeyEvent.KEYCODE_VOLUME_UP:
                // Use the volume up button as shutter button (or snapshot button in video mode)
                if (!mIsShutterButtonDown) {
                    if (mCameraMode == CAMERA_MODE_VIDEO) {
                        mSnapshotManager.queueSnapshot(true, 0);
                    } else {
                        mShutterButton.performClick();
                    }
                    mIsShutterButtonDown = true;
                }
                return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_FOCUS:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                mIsFocusButtonDown = false;
                mCamManager.setLockSetup(false);
                break;

            case KeyEvent.KEYCODE_CAMERA:
            case KeyEvent.KEYCODE_VOLUME_UP:
                mIsShutterButtonDown = false;
                break;
        }

        return super.onKeyUp(keyCode, event);
    }

    public CameraManager getCamManager() {
        return mCamManager;
    }

    public SnapshotManager getSnapManager() {
        return mSnapshotManager;
    }

    public PicSphereManager getPicSphereManager() {
        return mPicSphereManager;
    }

    public ReviewDrawer getReviewDrawer() {
        return mReviewDrawer;
    }

    public void initializePicSphere() {
        // Check if device has a gyroscope and GLES2 support
        // XXX: Should we make a fallback for super super old devices?
        final ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        final boolean supportsEs2 = configurationInfo.reqGlEsVersion >= 0x20000;

        if (!supportsEs2) {
            mNotifier.notify(getString(R.string.no_gles20_support), 4000);
            return;
        }
        // Close widgets and slide sidebar to make room and focus on the sphere
        mSideBar.slideClose();
        mWidgetRenderer.closeAllWidgets();

        // Setup the 3D rendering
        if (mPicSphereManager == null) {
            mPicSphereManager = new PicSphereManager(this, mSnapshotManager);
        }
        setGLRenderer(mPicSphereManager.getRenderer());

        // Setup the capture transformer
        final PicSphereCaptureTransformer transformer =
                new PicSphereCaptureTransformer(this);
        setCaptureTransformer(transformer);

        mPicSphereUndo.setVisibility(View.VISIBLE);
        mPicSphereUndo.setAlpha(0.0f);
        mPicSphereUndo.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                transformer.removeLastPicture();
            }
        });

        // Notify how to start a sphere
        setHelperText(getString(R.string.picsphere_start_hint));
    }

    /**
     * Tear down the PicSphere mode and set the default renderer back on the preview
     * GL surface.
     */
    public void resetPicSphere() {
        // Reset the normal renderer
        setGLRenderer(mCamManager.getRenderer());

        // Tear down PicSphere capture system
        if (mPicSphereManager != null) {
            mPicSphereManager.tearDown();
        }
        setCaptureTransformer(null);

        if (mPicSphereUndo != null) {
            mPicSphereUndo.setVisibility(View.GONE);
        }
    }

    /**
     * Initializes the panorama (mosaic) subsystem
     */
    public void initializePanorama() {
        mMosaicProxy = new MosaicProxy(this);
        setCaptureTransformer(mMosaicProxy);
        mCamManager.setRenderToTexture(null);
        updateRingsVisibility();
    }

    /**
     * Turns off the panorama (mosaic) subsystem
     */
    public void resetPanorama() {
        if (mMosaicProxy != null) {
            mMosaicProxy.tearDown();
        }
        setGLRenderer(mCamManager.getRenderer());
    }

    public void setGLRenderer(GLSurfaceView.Renderer renderer) {
        final ViewGroup container = ((ViewGroup) findViewById(R.id.gl_renderer_container));
        // Delete the previous GL Surface View (if any)
        if (mGLSurfaceView != null) {
            container.removeView(mGLSurfaceView);
            mGLSurfaceView = null;
        }

        // Make a new GL view using the provided renderer
        mGLSurfaceView = new GLSurfaceView(this);
        mGLSurfaceView.setEGLContextClientVersion(2);
        mGLSurfaceView.setRenderer(renderer);

        container.addView(mGLSurfaceView);
    }

    /**
     * Toggles the fullscreen shutter that lets user take pictures by tapping on the screen
     */
    public void toggleFullscreenShutter() {
        if (mIsFullscreenShutter) {
            mIsFullscreenShutter = false;
            mShutterButton.animate().translationY(0).setDuration(400).start();
        } else {
            mIsFullscreenShutter = true;
            mShutterButton.animate().translationY(mShutterButton.getHeight()).setDuration(400).start();
            notify(getString(R.string.fullscreen_shutter_info), 2000);
        }
        updateRingsVisibility();
    }

    /**
     * Show a persistent helper text that indicates the user a required action
     * @param text The text to show, or empty/null to hide
     */
    public void setHelperText(final CharSequence text) {
        setHelperText(text, false);
    }

    /**
     * Show a persistent helper text that indicates the user a required action
     * @param text The text to show, or empty/null to hide
     * @param beware Show the text in red
     */
    public void setHelperText(final CharSequence text, final boolean beware) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (text == null || text.equals("")) {
                    // Hide it
                    Util.fadeOut(mHelperText);
                } else {
                    mHelperText.setText(text);
                    if (beware) {
                        mHelperText.setTextColor(getResources().getColor(R.color.clock_red));
                    } else {
                        mHelperText.setTextColor(0xFFFFFFFF);
                    }
                    Util.fadeIn(mHelperText);
                }
            }
        });
    }

    public void setPicSphereUndoVisible(final boolean visible) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (visible) {
                    mPicSphereUndo.setVisibility(View.VISIBLE);
                    mPicSphereUndo.setAlpha(1.0f);
                } else {
                    mPicSphereUndo.animate().alpha(0.0f).setDuration(200).start();
                }
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

    @Override
    public void onShowcaseViewHide(ShowcaseView showcaseView) {
        switch (mShowcaseIndex) {
            case SHOWCASE_INDEX_WELCOME_1:
                mShowcaseIndex = SHOWCASE_INDEX_WELCOME_2;

                Point size = new Point();
                getWindowManager().getDefaultDisplay().getSize(size);

                ShowcaseView.ConfigOptions co = new ShowcaseView.ConfigOptions();
                co.hideOnClickOutside = true;
                mShowcaseView = ShowcaseView.insertShowcaseView(size.x / 2,
                        size.y - Util.dpToPx(this, 16), this,
                        getString(R.string.showcase_welcome_2_title),
                        getString(R.string.showcase_welcome_2_body), co);

                // animate gesture
                mShowcaseView.animateGesture(size.x / 2,
                        size.y - Util.dpToPx(this, 16), size.x / 2, size.y / 2);
                mShowcaseView.setOnShowcaseEventListener(this);

                // ping the button
                mSwitchRingPad.animateHint();

                break;
        }
    }

    @Override
    public void onShowcaseViewShow(ShowcaseView showcaseView) {
        // Do nothing here
    }

    /**
     * Listener that is called when the preview pauses or resumes
     */
    private class CameraPreviewListener implements CameraManager.PreviewPauseListener {
        @Override
        public void onPreviewPause() {
            // XXX: Do a little animation
        }

        @Override
        public void onPreviewResume() {
            // XXX: Do a little animation
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
                case SwitchRingPad.BUTTON_PANO:
                    setCameraMode(CAMERA_MODE_PANO);
                    break;
                case SwitchRingPad.BUTTON_VIDEO:
                    setCameraMode(CAMERA_MODE_VIDEO);
                    break;
                case SwitchRingPad.BUTTON_PICSPHERE:
                    setCameraMode(CAMERA_MODE_PICSPHERE);
                    break;
                case SwitchRingPad.BUTTON_SWITCHCAM:
                    mIsCamSwitching = true;
                    if (mCamManager.getCurrentFacing() == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                        mCamManager.open(Camera.CameraInfo.CAMERA_FACING_BACK);
                    } else {
                        mCamManager.open(Camera.CameraInfo.CAMERA_FACING_FRONT);
                    }

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

            // Tapping the shutter button locked exposure/WB, so we unlock it if we slide our finger
            mCamManager.setLockSetup(false);

            // Cancel long-press action
            mIsShutterLongClicked = false;
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

            // Turn on stabilization
            mCamManager.setStabilization(true);
        }
    }

    /**
     * When the shutter button is pressed
     */
    public class MainShutterClickListener implements OnClickListener,
            View.OnLongClickListener, View.OnTouchListener {


        @Override
        public void onClick(View v) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mReviewDrawer.setTemporaryHide(false);
                }
            }, 500);

            if (mSnapshotManager == null) return;

            // If we have a capture transformer, apply it, otherwise use the default
            // behavior.
            if (mCaptureTransformer != null) {
                mCaptureTransformer.onShutterButtonClicked(mShutterButton);
            } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
                mSnapshotManager.queueSnapshot(true, 0);
            } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
                if (!mSnapshotManager.isRecording()) {
                    mSnapshotManager.startVideo();
                    mShutterButton.setImageDrawable(getResources()
                            .getDrawable(R.drawable.btn_shutter_stop));
                } else {
                    mSnapshotManager.stopVideo();
                    mShutterButton.setImageDrawable(getResources()
                            .getDrawable(R.drawable.btn_shutter_video));
                }
            } else {
                Log.e(TAG, "Unknown Camera Mode: " + mCameraMode + " ; No capture transformer");
            }
        }

        @Override
        public boolean onLongClick(View view) {
            if (mCaptureTransformer != null) {
                mCaptureTransformer.onShutterButtonLongPressed(mShutterButton);
            } else {
                mIsShutterLongClicked = true;
                if (mFocusManager != null) {
                    mFocusManager.checkFocus();
                }
            }
            return true;
        }

        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            // If we long-press the shutter button and no capture transformer handles it, we
            // will just have nothing happening. We register the long click event in here, and
            // trigger a snapshot once it's released.
            if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP && mIsShutterLongClicked) {
                mIsShutterLongClicked = false;
                onClick(view);
            }

            return view.onTouchEvent(motionEvent);
        }
    }


    /**
     * Focus listener to animate the focus HUD ring from FocusManager events
     */
    private class MainFocusListener implements FocusManager.FocusListener {
        @Override
        public void onFocusStart(final boolean smallAdjust) {
            mIsFocusing = true;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mFocusHudRing.animateWorking(smallAdjust ? 200 : 1500);
                }
            });
        }

        @Override
        public void onFocusReturns(final boolean smallAdjust, final boolean success) {
            mIsFocusing = false;
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
        public void onSnapshotShutter(final SnapshotManager.SnapshotInfo info) {
            final FrameLayout layout = (FrameLayout) findViewById(R.id.thumb_flinger_container);

            // Fling the preview
            final ThumbnailFlinger flinger = new ThumbnailFlinger(CameraActivity.this);
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    layout.addView(flinger);
                    flinger.setRotation(90);
                    flinger.setImageBitmap(info.mThumbnail);
                    flinger.doAnimation();
                }
            });

            // Unlock camera auto settings
            mCamManager.setLockSetup(false);
            mCamManager.setStabilization(false);
        }

        @Override
        public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {
            // Do nothing here
        }

        @Override
        public void onSnapshotProcessing(SnapshotManager.SnapshotInfo info) {
            runOnUiThread(new Runnable() {
                public void run() {
                    if (mSavePinger != null) {
                        mSavePinger.setPingMode(SavePinger.PING_MODE_ENHANCER);
                        mSavePinger.startSaving();
                    }
                }
            });
        }

        @Override
        public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {
            String uriStr = info.mUri.toString();

            // Add the new image to the gallery and the review drawer
            int originalImageId = Integer.parseInt(uriStr.substring(uriStr
                    .lastIndexOf("/") + 1, uriStr.length()));
            Log.v(TAG, "Adding snapshot to gallery: " + originalImageId);
            mReviewDrawer.addImageToList(originalImageId);
            mReviewDrawer.scrollToLatestImage();
        }

        @Override
        public void onMediaSavingStart() {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mSavePinger.setPingMode(SavePinger.PING_MODE_SAVE);
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
    private class CameraOrientationEventListener extends OrientationEventListener {
        public CameraOrientationEventListener(Context context) {
            super(context);
        }

        @Override
        public void onOrientationChanged(int orientation) {
            // We keep the last known orientation. So if the user first orient
            // the camera then point the camera to floor or sky, we still have
            // the correct orientation.
            if (orientation == ORIENTATION_UNKNOWN) {
                return;
            }
            mOrientation = Util.roundOrientation(orientation, mOrientation);

            // Notify camera of the raw orientation
            mCamManager.setOrientation(mOrientation);

            // Adjust orientationCompensation for the native orientation of the device.
            Configuration config = getResources().getConfiguration();
            int rotation = getWindowManager().getDefaultDisplay().getRotation();
            Util.getDisplayRotation(CameraActivity.this);

            boolean nativeLandscape = false;

            if (((rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180)
                    && config.orientation == Configuration.ORIENTATION_LANDSCAPE)
                    || ((rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270)
                    && config.orientation == Configuration.ORIENTATION_PORTRAIT)) {
                nativeLandscape = true;
            }

            int orientationCompensation = mOrientation; // + (nativeLandscape ? 0 : 90);
            if (orientationCompensation == 90) {
                orientationCompensation += 180;
            } else if (orientationCompensation == 270) {
                orientationCompensation -= 180;
            }

            // Avoid turning all around
            float angleDelta = orientationCompensation - mOrientationCompensation;
            if (angleDelta >= 270) {
                orientationCompensation -= 360;
            }

            if (mOrientationCompensation != orientationCompensation) {
                mOrientationCompensation = orientationCompensation;
                updateInterfaceOrientation();
            }
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
        private static final int SWIPE_MIN_DISTANCE = 10;
        private final float DRAG_MIN_DISTANCE = Util.dpToPx(CameraActivity.this, 5.0f);
        private static final int SWIPE_MAX_OFF_PATH = 80;
        private static final int SWIPE_THRESHOLD_VELOCITY = 800;

        // Allow to drag the side bar up to half of the screen
        private static final int SIDEBAR_THRESHOLD_FACTOR = 2;

        private boolean mCancelSwipe = false;

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            if (mPaused) return false;

            // A single tap equals to touch-to-focus in photo/video
            if ((mCameraMode == CAMERA_MODE_PHOTO && !mIsFullscreenShutter)
                    || mCameraMode == CAMERA_MODE_VIDEO) {
                if (mFocusManager != null) {
                    mFocusHudRing.setPosition(e.getRawX(), e.getRawY());
                    mFocusManager.refocus();
                }
            } else if (mCameraMode == CAMERA_MODE_PHOTO && mIsFullscreenShutter) {
                // We are in fullscreen shutter mode, so just take a picture
                mSnapshotManager.queueSnapshot(true, 0);
            }

            return super.onSingleTapConfirmed(e);
        }

        @Override
        public boolean onDoubleTap(MotionEvent e) {
            // In VIDEO mode, a double tap snapshots (or volume up)
            if (mCameraMode == CAMERA_MODE_VIDEO) {
                mSnapshotManager.queueSnapshot(true, 0);
            } else if (mCameraMode == CAMERA_MODE_PHOTO) {
                // Toggle fullscreen shutter
                toggleFullscreenShutter();
            }

            return super.onDoubleTap(e);
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            if (e1 == null || e2 == null) {
                return false;
            }

            // Detect drag of the side bar or review drawer
            if (Math.abs(e1.getY() - e2.getY()) < SWIPE_MAX_OFF_PATH) {
                if (e1.getRawX() < Util.getScreenSize(CameraActivity.this)
                        .x / SIDEBAR_THRESHOLD_FACTOR) {
                    if (e1.getX() - e2.getX() > SWIPE_MIN_DISTANCE ||
                            e2.getX() - e1.getX() > SWIPE_MIN_DISTANCE) {
                        mSideBar.slide(-distanceX);
                        mWidgetRenderer.notifySidebarSlideStatus(-distanceX);
                        mCancelSwipe = true;
                        mCancelSideBarClose = true;
                    }

                    return true;
                }
            } else if (Math.abs(e1.getY() - e2.getY()) > DRAG_MIN_DISTANCE) {
                mReviewDrawer.slide(-distanceY);
            }

            return true;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            try {
                if (Math.abs(e1.getY() - e2.getY()) < SWIPE_MAX_OFF_PATH) {
                    // swipes to open/close the sidebar and/or hide/restore the widgets
                    if (e2.getX() - e1.getX() > SWIPE_MIN_DISTANCE
                            && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
                        if (mWidgetRenderer.isHidden() && mWidgetRenderer.getWidgetsCount() > 0) {
                            mWidgetRenderer.restoreWidgets();
                        } else {
                            mSideBar.slideOpen();
                            mWidgetRenderer.notifySidebarSlideOpen();
                            mCancelSideBarClose = true;
                        }
                    } else if (e1.getX() - e2.getX() > SWIPE_MIN_DISTANCE
                            && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
                        if (mSideBar.isOpen()) {
                            mSideBar.slideClose();
                            mWidgetRenderer.notifySidebarSlideClose();
                            mCancelSideBarClose = true;
                        } else if (!mWidgetRenderer.isHidden()
                                && mWidgetRenderer.getWidgetsCount() > 0
                                && !mCancelSwipe) {
                            mWidgetRenderer.hideWidgets();
                        }
                    }
                }

                if (Math.abs(e1.getX() - e2.getX()) < SWIPE_MAX_OFF_PATH) {
                    // swipes up/down to open/close the review drawer
                    if (e1.getY() - e2.getY() > SWIPE_MIN_DISTANCE
                            && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                        mReviewDrawer.close();
                    } else if (e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE
                            && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
                        mReviewDrawer.open();
                    }
                }
            } catch (Exception e) {
                // Do nothing here
            }

            mCancelSwipe = false;
            return true;
        }
    }


    /**
     * Handles the pinch-to-zoom gesture
     */
    private class ZoomGestureListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            Camera.Parameters params = mCamManager.getParameters();

            if (params == null) return false;

            if (!mIsFocusing) {
                if (detector.getScaleFactor() > 1.0f) {
                    params.setZoom(Math.min(params.getZoom() + 1, params.getMaxZoom()));
                } else if (detector.getScaleFactor() < 1.0f) {
                    params.setZoom(Math.max(params.getZoom() - 1, 0));
                } else {
                    return false;
                }

                mHasPinchZoomed = true;
                mCamManager.setParameters(params);
            }

            return true;
        }
    }
}
