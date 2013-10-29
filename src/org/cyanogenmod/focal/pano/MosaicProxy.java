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

package org.cyanogenmod.focal.pano;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.TextureView;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;

import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.Storage;
import org.cyanogenmod.focal.Util;
import org.cyanogenmod.focal.feats.CaptureTransformer;
import org.cyanogenmod.focal.ui.PanoProgressBar;
import org.cyanogenmod.focal.ui.ShutterButton;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;

import fr.xplod.focal.R;

/**
 * Nemesis interface to interact with Google's mosaic interface
 * Strongly inspired from AOSP's PanoramaModule
 */
public class MosaicProxy extends CaptureTransformer
        implements SurfaceTexture.OnFrameAvailableListener, TextureView.SurfaceTextureListener {
    private static final String TAG = "CAM PanoModule";

    public static final int DEFAULT_SWEEP_ANGLE = 360;
    // The unit of speed is degrees per frame.
    private static final float PANNING_SPEED_THRESHOLD = 2.5f;

    private static final int MSG_LOW_RES_FINAL_MOSAIC_READY = 1;
    private static final int MSG_GENERATE_FINAL_MOSAIC_ERROR = 2;
    private static final int MSG_RESET_TO_PREVIEW = 3;
    private static final int MSG_CLEAR_SCREEN_DELAY = 4;

    private static final int CAPTURE_STATE_VIEWFINDER = 0;
    private static final int CAPTURE_STATE_MOSAIC = 1;
    private final int mIndicatorColor;
    private final int mIndicatorColorFast;

    private Runnable mOnFrameAvailableRunnable;
    private MosaicFrameProcessor mMosaicFrameProcessor;
    private MosaicPreviewRenderer mMosaicPreviewRenderer;
    private boolean mMosaicFrameProcessorInitialized;
    private float mHorizontalViewAngle;
    private float mVerticalViewAngle;

    private ShutterButton mShutterButton;
    private int mCaptureState;
    private FrameLayout mGLRootView;
    private TextureView mGLSurfaceView;
    private CameraActivity mActivity;
    private Handler mMainHandler;
    private SurfaceTexture mCameraTexture;
    private SurfaceTexture mMosaicTexture;
    private boolean mCancelComputation;
    private long mTimeTaken;
    private PanoProgressBar mPanoProgressBar;
    private Matrix mProgressDirectionMatrix = new Matrix();
    private float[] mProgressAngle = new float[2];
    private int mPreviewWidth;
    private int mPreviewHeight;
    private boolean mThreadRunning;
    final private Object mWaitObject = new Object();
    private int mCurrentOrientation;

    private class MosaicJpeg {
        public MosaicJpeg(byte[] data, int width, int height) {
            this.data = data;
            this.width = width;
            this.height = height;
            this.isValid = true;
        }

        public MosaicJpeg() {
            this.data = null;
            this.width = 0;
            this.height = 0;
            this.isValid = false;
        }

        public final byte[] data;
        public final int width;
        public final int height;
        public final boolean isValid;
    }

    public MosaicProxy(CameraActivity activity) {
        super(activity.getCamManager(), activity.getSnapManager());
        mActivity = activity;
        mCaptureState = CAPTURE_STATE_VIEWFINDER;
        mPanoProgressBar = activity.getPanoProgressBar();


        mGLRootView = (FrameLayout) mActivity.findViewById(R.id.gl_renderer_container);
        mGLSurfaceView = new TextureView(mActivity);
        mGLRootView.addView(mGLSurfaceView);

        mGLSurfaceView.setSurfaceTextureListener(this);

        mMosaicFrameProcessor = MosaicFrameProcessor.getInstance();
        Resources appRes = mActivity.getResources();
        mIndicatorColor = appRes.getColor(R.color.pano_progress_indication);
        mIndicatorColorFast = appRes.getColor(R.color.pano_progress_indication_fast);
        mPanoProgressBar.setBackgroundColor(appRes.getColor(R.color.pano_progress_empty));
        mPanoProgressBar.setDoneColor(appRes.getColor(R.color.pano_progress_done));
        mPanoProgressBar.setIndicatorColor(mIndicatorColor);
        mPanoProgressBar.setOnDirectionChangeListener(
                new PanoProgressBar.OnDirectionChangeListener () {
                    @Override
                    public void onDirectionChange(int direction) {
                        if (mCaptureState == CAPTURE_STATE_MOSAIC) {
                            showDirectionIndicators(direction);
                        }
                    }
                });

        // This runs in UI thread.
        mOnFrameAvailableRunnable = new Runnable() {
            @Override
            public void run() {
                // Frames might still be available after the activity is paused.
                // If we call onFrameAvailable after pausing, the GL thread will crash.
                // if (mPaused) return;

                if (mGLRootView.getVisibility() != View.VISIBLE) {
                    mMosaicPreviewRenderer.showPreviewFrameSync();
                    mGLRootView.setVisibility(View.VISIBLE);
                } else {
                    if (mCaptureState == CAPTURE_STATE_VIEWFINDER) {
                        mMosaicPreviewRenderer.showPreviewFrame();
                    } else {
                        mMosaicPreviewRenderer.alignFrameSync();
                        mMosaicFrameProcessor.processFrame();
                    }
                }
            }
        };

        mMainHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_LOW_RES_FINAL_MOSAIC_READY:
                        //onBackgroundThreadFinished();
                        //showFinalMosaic((Bitmap) msg.obj);
                        Util.fadeOut(mGLRootView);
                        mActivity.displayOverlayBitmap((Bitmap) msg.obj);
                        saveHighResMosaic();
                        break;
                    case MSG_GENERATE_FINAL_MOSAIC_ERROR:
                        CameraActivity.notify(
                                mActivity.getString(R.string.pano_panorama_rendering_failed), 2000);
                        resetToPreview();
                        break;
                    case MSG_RESET_TO_PREVIEW:
                        resetToPreview();
                        mThreadRunning = false;
                        break;
                    case MSG_CLEAR_SCREEN_DELAY:
                        mActivity.getWindow().clearFlags(WindowManager.LayoutParams.
                                FLAG_KEEP_SCREEN_ON);
                        break;
                }
            }
        };

        // Initialization
        Camera.Parameters params = mActivity.getCamManager().getParameters();
        if (params != null) {
            mHorizontalViewAngle = params.getHorizontalViewAngle();
            mVerticalViewAngle = params.getVerticalViewAngle();
        } else {
            mHorizontalViewAngle = 50;
            mHorizontalViewAngle = 30;
        }

        int pixels = mActivity.getResources().getInteger(R.integer.config_panoramaDefaultWidth)
                * mActivity.getResources().getInteger(R.integer.config_panoramaDefaultHeight);

        if (params != null) {
            Point size = Util.findBestPanoPreviewSize(
                    params.getSupportedPreviewSizes(), true, true, pixels);
            mPreviewWidth = size.y;
            mPreviewHeight = size.x;
        } else {
            mPreviewWidth = 480;
            mPreviewHeight = 640;
        }

        FrameLayout.LayoutParams layoutParams =
                (FrameLayout.LayoutParams) mGLSurfaceView.getLayoutParams();
        layoutParams.width = FrameLayout.LayoutParams.WRAP_CONTENT;
        layoutParams.height = FrameLayout.LayoutParams.WRAP_CONTENT;
        layoutParams.gravity = Gravity.CENTER;
        mGLSurfaceView.setLayoutParams(layoutParams);

        layoutParams = (FrameLayout.LayoutParams) mGLRootView.getLayoutParams();
        layoutParams.width = FrameLayout.LayoutParams.WRAP_CONTENT;
        layoutParams.height = FrameLayout.LayoutParams.WRAP_CONTENT;
        layoutParams.gravity = Gravity.CENTER;
        mGLSurfaceView.setLayoutParams(layoutParams);
    }

    // This function will be called upon the first camera frame is available.
    private void resetToPreview() {
        mCaptureState = CAPTURE_STATE_VIEWFINDER;
        if (mShutterButton != null) {
            mShutterButton.setImageResource(R.drawable.btn_shutter_photo);
        }

        mCamManager.setLockSetup(false);
        Util.fadeIn(mShutterButton);
        Util.fadeIn(mGLRootView);
        mActivity.hideOverlayBitmap();
        Util.fadeOut(mPanoProgressBar);
        mMosaicFrameProcessor.reset();
        mCameraTexture.setOnFrameAvailableListener(this);
        setupProgressDirectionMatrix();
        mCamManager.setRenderToTexture(mCameraTexture);
    }


    /**
     * Call this when you're done using MosaicProxy, to remove views added and shutdown
     * threads.
     */
    public void tearDown() {
        mGLRootView.removeView(mGLSurfaceView);
        mMosaicFrameProcessor.clear();
    }

    private void configMosaicPreview() {
        boolean isLandscape = false;
        Log.d(TAG, "isLandscape ? " + isLandscape +
                " (orientation: " + mCamManager.getOrientation() + ")");

        int viewWidth = mGLRootView.getMeasuredWidth();
        int viewHeight = mGLRootView.getMeasuredHeight();

        mMosaicPreviewRenderer = new MosaicPreviewRenderer(mMosaicTexture, viewWidth,
                viewHeight, isLandscape);

        mCameraTexture = mMosaicPreviewRenderer.getInputSurfaceTexture();
        mCameraTexture.setDefaultBufferSize(mPreviewWidth, mPreviewHeight);
        mCameraTexture.setOnFrameAvailableListener(this);
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                mActivity.getCamManager().setRenderToTexture(mCameraTexture);
            }
        });
        mMosaicTexture.setDefaultBufferSize(viewWidth, viewHeight);
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        /* This function may be called by some random thread,
         * so let's be safe and jump back to ui thread.
         * No OpenGL calls can be done here. */
        mActivity.runOnUiThread(mOnFrameAvailableRunnable);
    }

    @Override
    public void onShutterButtonClicked(ShutterButton button) {
        mShutterButton = button;

        if (mCaptureState == CAPTURE_STATE_MOSAIC) {
            stopCapture(false);
            Util.fadeOut(button);
        } else {
            startCapture();
            button.setImageResource(R.drawable.btn_shutter_stop);
        }
    }

    @Override
    public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotProcessing(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onMediaSavingStart() {

    }

    @Override
    public void onMediaSavingDone() {

    }

    @Override
    public void onVideoRecordingStart() {

    }

    @Override
    public void onVideoRecordingStop() {

    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int i, int i2) {
        mMosaicTexture = surfaceTexture;
        initMosaicFrameProcessorIfNeeded();
        configMosaicPreview();
    }

    public int getPreviewBufSize() {
        PixelFormat pixelInfo = new PixelFormat();
        PixelFormat.getPixelFormatInfo(PixelFormat.RGB_888, pixelInfo);
        // TODO: remove this extra 32 byte after the driver bug is fixed.
        return (mPreviewWidth * mPreviewHeight * pixelInfo.bitsPerPixel / 8) + 32;
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int i, int i2) {

    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {

    }

    public void startCapture() {
        Log.v(TAG, "Starting Panorama capture");

        // Reset values so we can do this again.
        mCancelComputation = false;
        mTimeTaken = System.currentTimeMillis();
        // mShutterButton.setImageResource(R.drawable.btn_shutter_recording);
        mCaptureState = CAPTURE_STATE_MOSAIC;
        //mCaptureIndicator.setVisibility(View.VISIBLE);
        //showDirectionIndicators(PanoProgressBar.DIRECTION_NONE);
        mPanoProgressBar.setDoneColor(mActivity.getResources().getColor(R.color.pano_progress_done));
        mPanoProgressBar.setIndicatorColor(mIndicatorColor);

        boolean isLandscape = (mCamManager.getOrientation() != -90);
        Log.d(TAG, "isLandscape ? " + isLandscape + " (orientation: "
                + mCamManager.getOrientation() + ")");
        mMosaicPreviewRenderer.setLandscape(isLandscape);

        mCurrentOrientation = mActivity.getCamManager().getOrientation();
        if (mCurrentOrientation == -90) {
            mCurrentOrientation = 90;
        }
        if (mCurrentOrientation < 0) {
            mCurrentOrientation += 360;
        }

        mMosaicFrameProcessor.setProgressListener(new MosaicFrameProcessor.ProgressListener() {
            @Override
            public void onProgress(boolean isFinished, float panningRateX, float panningRateY,
                    float progressX, float progressY) {
                float accumulatedHorizontalAngle = progressX * mHorizontalViewAngle;
                float accumulatedVerticalAngle = progressY * mVerticalViewAngle;
                if (isFinished
                        || (Math.abs(accumulatedHorizontalAngle) >= DEFAULT_SWEEP_ANGLE)
                        || (Math.abs(accumulatedVerticalAngle) >= DEFAULT_SWEEP_ANGLE)) {
                    Util.fadeOut(mShutterButton);
                    stopCapture(false);
                } else {
                    float panningRateXInDegree = panningRateX * mHorizontalViewAngle;
                    float panningRateYInDegree = panningRateY * mVerticalViewAngle;
                    updateProgress(panningRateXInDegree, panningRateYInDegree,
                            accumulatedHorizontalAngle, accumulatedVerticalAngle);
                }
            }
        });

        mPanoProgressBar.reset();
        // TODO: calculate the indicator width according to different devices to reflect the actual
        // angle of view of the camera device.
        mPanoProgressBar.setIndicatorWidth(20);
        mPanoProgressBar.setMaxProgress(DEFAULT_SWEEP_ANGLE);
        mPanoProgressBar.setVisibility(View.VISIBLE);
        Util.fadeIn(mPanoProgressBar);
        //mDeviceOrientationAtCapture = mDeviceOrientation;
        //keepScreenOn();
        //mActivity.getOrientationManager().lockOrientation();
        setupProgressDirectionMatrix();
    }

    private void stopCapture(boolean aborted) {
        mCaptureState = CAPTURE_STATE_VIEWFINDER;
        //mCaptureIndicator.setVisibility(View.GONE);
        //hideTooFastIndication();
        //hideDirectionIndicators();

        mMosaicFrameProcessor.setProgressListener(null);
        //stopCameraPreview();

        mCameraTexture.setOnFrameAvailableListener(null);

        mPanoProgressBar.setDoneColor(mActivity.getResources().getColor(R.color.pano_saving_done));
        mPanoProgressBar.setIndicatorColor(mActivity.getResources().getColor(R.color.pano_saving_indication));

        if (!aborted && !mThreadRunning) {
            //mRotateDialog.showWaitingDialog(mPreparePreviewString);
            // Hide shutter button, shutter icon, etc when waiting for
            // panorama to stitch
            //mActivity.hideUI();
            runInBackground(new Thread() {
                @Override
                public void run() {
                    MosaicJpeg jpeg = generateFinalMosaic(false);

                    if (jpeg != null && jpeg.isValid) {
                        Bitmap bitmap = null;
                        bitmap = BitmapFactory.decodeByteArray(jpeg.data, 0, jpeg.data.length);
                        mMainHandler.sendMessage(mMainHandler.obtainMessage(
                                MSG_LOW_RES_FINAL_MOSAIC_READY, bitmap));
                    } else {
                        CameraActivity.notify(
                                mActivity.getString(R.string.pano_panorama_rendering_failed), 2000);
                        mMainHandler.sendMessage(mMainHandler.obtainMessage(
                                MSG_RESET_TO_PREVIEW));
                    }
                }
            });
        }
    }

    /**
     * Generate the final mosaic image.
     *
     * @param highRes flag to indicate whether we want to get a high-res version.
     * @return a MosaicJpeg with its isValid flag set to true if successful; null if the generation
     *         process is cancelled; and a MosaicJpeg with its isValid flag set to false if there
     *         is an error in generating the final mosaic.
     */
    public MosaicJpeg generateFinalMosaic(boolean highRes) {
        int mosaicReturnCode = mMosaicFrameProcessor.createMosaic(highRes);
        if (mosaicReturnCode == Mosaic.MOSAIC_RET_CANCELLED) {
            return null;
        } else if (mosaicReturnCode == Mosaic.MOSAIC_RET_ERROR) {
            return new MosaicJpeg();
        }

        byte[] imageData = mMosaicFrameProcessor.getFinalMosaicNV21();
        if (imageData == null) {
            Log.e(TAG, "getFinalMosaicNV21() returned null.");
            return new MosaicJpeg();
        }

        int len = imageData.length - 8;
        int width = (imageData[len + 0] << 24) + ((imageData[len + 1] & 0xFF) << 16)
                + ((imageData[len + 2] & 0xFF) << 8) + (imageData[len + 3] & 0xFF);
        int height = (imageData[len + 4] << 24) + ((imageData[len + 5] & 0xFF) << 16)
                + ((imageData[len + 6] & 0xFF) << 8) + (imageData[len + 7] & 0xFF);
        Log.v(TAG, "ImLength = " + (len) + ", W = " + width + ", H = " + height);

        if (width <= 0 || height <= 0) {
            CameraActivity.notify(
                    mActivity.getString(R.string.pano_panorama_rendering_failed), 2000);
            Log.e(TAG, "width|height <= 0!!, len = " + (len) + ", W = " + width + ", H = " +
                    height);
            return new MosaicJpeg();
        }

        YuvImage yuvimage = new YuvImage(imageData, ImageFormat.NV21, width, height, null);
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        yuvimage.compressToJpeg(new Rect(0, 0, width, height), 100, out);
        try {
            out.close();
        } catch (Exception e) {
            Log.e(TAG, "Exception in storing final mosaic", e);
            CameraActivity.notify(
                    mActivity.getString(R.string.pano_panorama_rendering_failed), 2000);
            return new MosaicJpeg();
        }
        return new MosaicJpeg(out.toByteArray(), width, height);
    }

    void setupProgressDirectionMatrix() {
        int degrees = Util.getDisplayRotation(mActivity);
        int cameraId = 0; //CameraHolder.instance().getBackCameraId();
        int orientation = 0; // TODO //Util.getDisplayOrientation(degrees, cameraId);
        mProgressDirectionMatrix.reset();
        mProgressDirectionMatrix.postRotate(orientation);
    }

    public void saveHighResMosaic() {
        CameraActivity.notify(mActivity.getString(R.string.pano_panorama_rendering), 3000);
        runInBackground(new Thread() {
            @Override
            public void run() {
                //mPartialWakeLock.acquire();
                MosaicJpeg jpeg;
                try {
                    jpeg = generateFinalMosaic(true);
                } finally {
                    //mPartialWakeLock.release();
                }

                if (jpeg == null) {  // Cancelled by user.
                    mMainHandler.sendEmptyMessage(MSG_RESET_TO_PREVIEW);
                } else if (!jpeg.isValid) {  // Error when generating mosaic.
                    mMainHandler.sendEmptyMessage(MSG_GENERATE_FINAL_MOSAIC_ERROR);
                } else {
                    Uri uri = savePanorama(jpeg.data, jpeg.width, jpeg.height, mCurrentOrientation);
                    if (uri != null) {
                        Util.broadcastNewPicture(mActivity, uri);
                        mActivity.getReviewDrawer().updateFromGallery(true, 0);
                    }

                    mMainHandler.sendMessage(
                            mMainHandler.obtainMessage(MSG_RESET_TO_PREVIEW));
                }
            }
        });
        reportProgress();
    }

    private Uri savePanorama(byte[] jpegData, int width, int height, int orientation) {
        if (jpegData != null) {
            String filename = PanoUtil.createName(
                    mActivity.getResources().getString(R.string.pano_file_name_format), mTimeTaken);
            String filePath = Storage.getStorage().writeFile(filename, jpegData);

            // Add Exif tags.
            try {
                ExifInterface exif = new ExifInterface(filePath);
                /*exif.setAttribute(ExifInterface.TAG_GPS_DATESTAMP,
                        mGPSDateStampFormat.format(mTimeTaken));
                exif.setAttribute(ExifInterface.TAG_GPS_TIMESTAMP,
                        mGPSTimeStampFormat.format(mTimeTaken));
                exif.setAttribute(ExifInterface.TAG_DATETIME,
                        mDateTimeStampFormat.format(mTimeTaken));*/
                exif.setAttribute(ExifInterface.TAG_ORIENTATION,
                        getExifOrientation(orientation));
                exif.saveAttributes();
            } catch (IOException e) {
                Log.e(TAG, "Cannot set EXIF for " + filePath, e);
            }

            int jpegLength = (int) (new File(filePath).length());
            return Storage.getStorage().addImage(mActivity.getContentResolver(), filename,
                    mTimeTaken, null, orientation, jpegLength, filePath, width, height);
        }
        return null;
    }

    private static String getExifOrientation(int orientation) {
        orientation = (orientation + 360) % 360;

        switch (orientation) {
            case 0:
                return String.valueOf(ExifInterface.ORIENTATION_NORMAL);
            case 90:
                return String.valueOf(ExifInterface.ORIENTATION_ROTATE_90);
            case 180:
                return String.valueOf(ExifInterface.ORIENTATION_ROTATE_180);
            case 270:
                return String.valueOf(ExifInterface.ORIENTATION_ROTATE_270);
            default:
                throw new AssertionError("invalid: " + orientation);
        }
    }

    private void initMosaicFrameProcessorIfNeeded() {
        //if (mPaused || mThreadRunning) return;
        if (!mMosaicFrameProcessorInitialized) {
            mMosaicFrameProcessor.initialize(
                    mPreviewWidth, mPreviewHeight, getPreviewBufSize());
            mMosaicFrameProcessorInitialized = true;
        }
    }

    private void updateProgress(float panningRateXInDegree, float panningRateYInDegree,
            float progressHorizontalAngle, float progressVerticalAngle) {
        mGLRootView.invalidate();

        if ((Math.abs(panningRateXInDegree) > PANNING_SPEED_THRESHOLD)
                || (Math.abs(panningRateYInDegree) > PANNING_SPEED_THRESHOLD)) {
            showTooFastIndication();
        } else {
            hideTooFastIndication();
        }

        // progressHorizontalAngle and progressVerticalAngle are relative to the
        // camera. Convert them to UI direction.
        mProgressAngle[0] = progressHorizontalAngle;
        mProgressAngle[1] = -progressVerticalAngle;
        mProgressDirectionMatrix.mapPoints(mProgressAngle);

        int angleInMajorDirection =
                (Math.abs(mProgressAngle[0]) > Math.abs(mProgressAngle[1]))
                        ? (int) mProgressAngle[0]
                        : (int) mProgressAngle[1];
        mPanoProgressBar.setProgress((angleInMajorDirection));
    }

    /**
     * Report saving progress
     */
    public void reportProgress() {
        mPanoProgressBar.reset();
        mPanoProgressBar.setRightIncreasing(true);
        mPanoProgressBar.setMaxProgress(100);
        Thread t = new Thread() {
            @Override
            public void run() {
                while (mThreadRunning) {
                    final int progress = mMosaicFrameProcessor.reportProgress(
                            true, mCancelComputation);

                    try {
                        synchronized (mWaitObject) {
                            mWaitObject.wait(50);
                        }
                    } catch (InterruptedException e) {
                        throw new RuntimeException("Panorama reportProgress failed", e);
                    }
                    // Update the progress bar
                    mActivity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mPanoProgressBar.setProgress(progress);
                        }
                    });
                }
            }
        };
        t.start();
    }

    private void runInBackground(Thread t) {
        mThreadRunning = true;
        t.start();
    }

    private void showTooFastIndication() {
        //mTooFastPrompt.setVisibility(View.VISIBLE);
        // The PreviewArea also contains the border for "too fast" indication.
        //mPreviewArea.setVisibility(View.VISIBLE);
        mPanoProgressBar.setIndicatorColor(mIndicatorColorFast);
        //mLeftIndicator.setEnabled(true);
        //mRightIndicator.setEnabled(true);
    }

    private void hideTooFastIndication() {
        //mTooFastPrompt.setVisibility(View.GONE);
        // We set "INVISIBLE" instead of "GONE" here because we need mPreviewArea to have layout
        // information so we can know the size and position for mCameraScreenNail.
        //mPreviewArea.setVisibility(View.INVISIBLE);
        mPanoProgressBar.setIndicatorColor(mIndicatorColor);
        //mLeftIndicator.setEnabled(false);
        //mRightIndicator.setEnabled(false);
    }

    private void hideDirectionIndicators() {
        /*mLeftIndicator.setVisibility(View.GONE);
        mRightIndicator.setVisibility(View.GONE);*/
    }

    private void showDirectionIndicators(int direction) {
        /*switch (direction) {
            case PanoProgressBar.DIRECTION_NONE:
                mLeftIndicator.setVisibility(View.VISIBLE);
                mRightIndicator.setVisibility(View.VISIBLE);
                break;
            case PanoProgressBar.DIRECTION_LEFT:
                mLeftIndicator.setVisibility(View.VISIBLE);
                mRightIndicator.setVisibility(View.GONE);
                break;
            case PanoProgressBar.DIRECTION_RIGHT:
                mLeftIndicator.setVisibility(View.GONE);
                mRightIndicator.setVisibility(View.VISIBLE);
                break;
        }*/
    }
}
