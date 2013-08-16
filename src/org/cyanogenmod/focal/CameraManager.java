/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

package org.cyanogenmod.focal;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.AutoFocusMoveCallback;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceView;

import org.apache.http.NameValuePair;
import org.apache.http.message.BasicNameValuePair;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.util.ArrayList;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This class is responsible for interacting with the Camera HAL.
 * It provides easy open/close, helper methods to set parameters or
 * toggle features, etc. in an asynchronous fashion.
 */
public class CameraManager {
    private final static String TAG = "CameraManager";

    private final static int FOCUS_WIDTH = 80;
    private final static int FOCUS_HEIGHT = 80;

    private CameraPreview mPreview;
    private Camera mCamera;
    private boolean mCameraReady;
    private int mCurrentFacing;
    private Point mTargetSize;
    private AutoFocusMoveCallback mAutoFocusMoveCallback;
    private Camera.Parameters mParameters;
    private int mOrientation;
    private MediaRecorder mMediaRecorder;
    private PreviewPauseListener mPreviewPauseListener;
    private CameraReadyListener mCameraReadyListener;
    private Handler mHandler;
    private Context mContext;
    private boolean mIsModeSwitching;
    private List<NameValuePair> mPendingParameters;
    private boolean mIsResuming;
    private CameraRenderer mRenderer;
    private boolean mIsRecordingHint;

    public interface PreviewPauseListener {
        /**
         * This method is called when the preview is about to pause.
         * This allows the CameraActivity to display an animation when the preview
         * has to stop.
         */
        public void onPreviewPause();

        /**
         * This method is called when the preview resumes
         */
        public void onPreviewResume();
    }

    public interface CameraReadyListener {
        /**
         * Called when a camera has been successfully opened. This allows the
         * main activity to continue setup operations while the camera
         * sets up in a different thread.
         */
        public void onCameraReady();

        /**
         * Called when the camera failed to initialize
         */
        public void onCameraFailed();
    }

    Thread mParametersThread = new Thread() {
        public void run() {
            while (true) {
                synchronized (this) {
                    try {
                        wait();
                    } catch (InterruptedException e) {
                        // Do nothing here
                    }

                    List<NameValuePair> copy = new ArrayList<NameValuePair>(mPendingParameters);
                    mPendingParameters.clear();

                    for (NameValuePair pair : copy) {
                        String key = pair.getName();
                        String val = pair.getValue();
                        Log.v(TAG, "Asynchronously setting parameter " + key+ " to " + val);
                        Camera.Parameters params = getParameters();
                        if (params == null) {
                            // The camera died, just forget about these settings
                            return;
                        }
                        String workingValue = params.get(key);
                        params.set(key, val);

                        try {
                            mCamera.setParameters(params);
                        } catch (RuntimeException e) {
                            Log.e(TAG, "Could not set parameter " + key
                                    + " to '" + val + "', restoring '"
                                    + workingValue + "'", e);

                            // Reset the parameter back in storage
                            SettingsStorage.storeCameraSetting(
                                    mContext, mCurrentFacing, key, workingValue);

                            // Reset the camera as it likely crashed if we reached here
                            open(mCurrentFacing);
                        }
                    }

                    // Read them from sensor
                    mParameters = null;// getParameters();
                }
            }
        }
    };

    public CameraManager(Context context) {
        mPreview = new CameraPreview(context);
        mMediaRecorder = new MediaRecorder();
        mCameraReady = true;
        mHandler = new Handler();
        mIsModeSwitching = false;
        mContext = context;
        mPendingParameters = new ArrayList<NameValuePair>();
        mParametersThread.start();
        mIsResuming = false;
        mIsRecordingHint = false;
        mRenderer = new CameraRenderer();
    }

    /**
     * Opens the camera and show its preview in the preview
     *
     * @param cameraId The facing of the camera
     * @return true if the operation succeeded, false otherwise
     */
    public boolean open(final int cameraId) {
        if (mCamera != null) {
            if (mPreviewPauseListener != null) {
                mPreviewPauseListener.onPreviewPause();
            }

            // Close the previous camera
            releaseCamera();
        }

        mCameraReady = false;

        // Try to open the camera
        new Thread() {
            public void run() {
                try {
                    if (mCamera != null) {
                        Log.e(TAG, "Previous camera not closed! Not opening");
                        return;
                    }
                    mCamera = Camera.open(cameraId);
                    Log.v(TAG, "Camera is open");

                    mCamera.enableShutterSound(false);
                    mCamera.setPreviewCallback(mPreview);
                    mCurrentFacing = cameraId;
                    mParameters = mCamera.getParameters();

                    String params = mCamera.getParameters().flatten();
                    final int step = params.length() > 256 ? 256 : params.length();
                    for (int i = 0; i < params.length(); i += step) {
                        Log.d(TAG, params);
                        params = params.substring(step);
                    }

                    if (mAutoFocusMoveCallback != null) {
                        setAutoFocusMoveCallback(mAutoFocusMoveCallback);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Error while opening cameras: " + e.getMessage(), e);

                    if (mCameraReadyListener != null) {
                        mCameraReadyListener.onCameraFailed();
                    }

                    return;
                }

                // Update the preview surface holder with the new opened camera
                mPreview.notifyCameraChanged(false);

                if (mCameraReadyListener != null) {
                    mCameraReadyListener.onCameraReady();
                }
                if (mPreviewPauseListener != null) {
                    mPreviewPauseListener.onPreviewResume();
                }

                mPreview.setPauseCopyFrame(false);

                mCameraReady = true;
            }
        }.start();

        return true;
    }

    public void setPreviewPauseListener(PreviewPauseListener listener) {
        mPreviewPauseListener = listener;
    }

    public void setCameraReadyListener(CameraReadyListener listener) {
        mCameraReadyListener = listener;
    }

    /**
     * Returns the preview surface used to display the Camera's preview
     *
     * @return CameraPreview
     */
    public CameraPreview getPreviewSurface() {
        return mPreview;
    }

    /**
     * @return The GLES20-compatible renderer for the camera preview
     */
    public CameraRenderer getRenderer() {
        return mRenderer;
    }

    /**
     * @return The facing of the current open camera
     */
    public int getCurrentFacing() {
        return mCurrentFacing;
    }

    /**
     * Returns the parameters structure of the current running camera
     *
     * @return Camera.Parameters
     */
    public Camera.Parameters getParameters() {
        synchronized (mParametersThread) {
            if (mCamera == null) {
                Log.w(TAG, "getParameters when camera is null");
                return null;
            }

            if (mParameters == null) {
                try {
                    mParameters = mCamera.getParameters();
                } catch (RuntimeException e) {
                    Log.e(TAG, "Error while getting parameters: ", e);
                    return null;
                }
            }
        }

        return mParameters;
    }

    public void pause() {
        mPreview.setPauseCopyFrame(true);
        releaseCamera();
        //mParametersThread.interrupt();
    }

    public void resume() {
        mIsResuming = true;
        reconnectToCamera();
        //mParametersThread.start();
    }

    private void releaseCamera() {
        if (mCamera != null && mCameraReady) {
            Log.v(TAG, "Releasing camera facing " + mCurrentFacing);
            mCamera.release();
            mCamera = null;
            mParameters = null;
            mPreview.notifyCameraChanged(false);
            mCameraReady = true;
        }
    }

    private void reconnectToCamera() {
        if (mCameraReady) {
            open(mCurrentFacing);
        } else {
            Log.e(TAG, "reconnectToCamera but camera not ready!");
        }
    }

    public void setPreviewSize(int width, int height) {
        mTargetSize = new Point(width, height);

        if (mCamera != null) {
            Camera.Parameters params = getParameters();
            params.setPreviewSize(width, height);

            Log.v(TAG, "Preview size is " + width + "x" + height);

            if (!mIsModeSwitching) {
                synchronized (mParametersThread) {
                    try {
                        mCamera.stopPreview();
                        mParameters = params;
                        mCamera.setParameters(mParameters);
                        mPreview.notifyPreviewSize(width, height);

                        if (mIsResuming) {
                            updateDisplayOrientation();
                            mCamera.startPreview();
                            mIsResuming = false;
                        }

                        mPreview.setPauseCopyFrame(false);
                    } catch (RuntimeException ex) {
                        Log.e(TAG, "Unable to set Preview Size", ex);
                    }
                }
            }
        }
    }

    public void setParameterAsync(String key, String value) {
        synchronized (mParametersThread) {
            mPendingParameters.add(new BasicNameValuePair(key, value));
            mParametersThread.notifyAll();
        }
    }

    /**
     * Sets a parameters class in a synchronous way. Use with caution, prefer setParameterAsync.
     * @param params Parameters
     */
    public void setParameters(Camera.Parameters params) {
        synchronized (mParametersThread) {
            mCamera.setParameters(params);
        }
    }

    /**
     * Locks the automatic settings of the camera device, like White balance and
     * exposure.
     *
     * @param lock true to lock, false to unlock
     */
    public void setLockSetup(boolean lock) {
        final Camera.Parameters params = getParameters();

        if (params == null) {
            // Params might be null if we pressed or swipe the shutter button
            // while the camera is not ready
            return;
        }

        if (params.isAutoExposureLockSupported()) {
            params.setAutoExposureLock(lock);
        }

        if (params.isAutoWhiteBalanceLockSupported()) {
            params.setAutoWhiteBalanceLock(lock);
        }

        new Thread() {
            public void run() {
                synchronized (mParametersThread) {
                    try {
                        mCamera.setParameters(params);
                    } catch (RuntimeException e) {
                        // Do nothing here
                    }
                }
            }
        }.start();
    }

    /**
     * Returns the last frame of the preview surface
     *
     * @return Bitmap
     */
    public Bitmap getLastPreviewFrame() {
        // Decode the last frame bytes
        byte[] data = mPreview.getLastFrameBytes();
        Camera.Parameters params = getParameters();

        if (params == null) {
            return null;
        }

        Camera.Size previewSize = params.getPreviewSize();
        if (previewSize == null) {
            return null;
        }

        int previewWidth = previewSize.width;
        int previewHeight = previewSize.height;

        // Convert YUV420SP preview data to RGB
        if (data != null && data.length > 8) {
            Bitmap bitmap = Util.decodeYUV420SP(mContext, data, previewWidth, previewHeight);
            if (mCurrentFacing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                // Frontcam has the image flipped, flip it back to not look weird in portrait
                Matrix m = new Matrix();
                m.preScale(-1, 1);
                Bitmap dst = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(),
                        bitmap.getHeight(), m, false);
                bitmap.recycle();
                bitmap = dst;
            }

            return bitmap;
        } else {
            return null;
        }
    }

    public Context getContext() {
        return mContext;
    }

    /**
     * Defines a new size for the recorded picture
     * XXX: Should it update preview size?!
     *
     * @param sz The new picture size
     */
    public void setPictureSize(Camera.Size sz) {
        Camera.Parameters params = getParameters();
        params.setPictureSize(sz.width, sz.height);
        mCamera.setParameters(params);
    }

    /**
     * Defines a new size for the recorded picture
     * @param sz The size string in format widthxheight
     */
    public void setPictureSize(String sz) {
        String[] splat = sz.split("x");
        int width = Integer.parseInt(splat[0]);
        int height = Integer.parseInt(splat[1]);

        setParameterAsync("picture-size", Integer.toString(width)
                + "x" + Integer.toString(height));
    }

    /**
     * Takes a snapshot
     */
    public void takeSnapshot(Camera.ShutterCallback shutterCallback,
                             Camera.PictureCallback raw, Camera.PictureCallback jpeg) {
        Log.v(TAG, "takePicture");
        SoundManager.getSingleton().play(SoundManager.SOUND_SHUTTER);

        if (mCamera != null) {
            mCamera.takePicture(shutterCallback, raw, jpeg);
        }
    }

    /**
     * Prepares the MediaRecorder to record a video. This must be called before
     * startVideoRecording to setup the recording environment.
     *
     * @param fileName Target file path
     * @param profile  Target profile (quality)
     */
    public void prepareVideoRecording(String fileName, CamcorderProfile profile) {
        // Unlock the camera for use with MediaRecorder
        mCamera.unlock();

        mMediaRecorder.setCamera(mCamera);
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);


        mMediaRecorder.setProfile(profile);
        mMediaRecorder.setOutputFile(fileName);
        // Set maximum file size.
        long maxFileSize = Storage.getStorage().getAvailableSpace()
                - Storage.LOW_STORAGE_THRESHOLD;
        mMediaRecorder.setMaxFileSize(maxFileSize);
        mMediaRecorder.setMaxDuration(0); // infinite

        try {
            mMediaRecorder.prepare();
        } catch (IllegalStateException e) {
            Log.e(TAG, "Cannot prepare MediaRecorder", e);
        } catch (IOException e) {
            Log.e(TAG, "Cannot prepare MediaRecorder", e);
        }

        mPreview.postCallbackBuffer();
    }

    public void startVideoRecording() {
        Log.v(TAG, "startVideoRecording");
        mMediaRecorder.start();
        mPreview.postCallbackBuffer();
    }

    public void stopVideoRecording() {
        Log.v(TAG, "stopVideoRecording");
        try {
            mMediaRecorder.stop();
        } catch (Exception e) {
            Log.e(TAG, "Cannot stop MediaRecorder", e);
        }
        mCamera.lock();
        mMediaRecorder.reset();
        mPreview.postCallbackBuffer();
    }

    /**
     * @return The orientation of the device
     */
    public int getOrientation() {
        return mOrientation;
    }

    /**
     * Sets the current orientation of the device
     * @param orientation The orientation, in degrees
     */
    public void setOrientation(int orientation) {
        mOrientation = orientation;

        // Rotate the pictures accordingly (display is kept at 90 degrees)
        Camera.CameraInfo info =
                new android.hardware.Camera.CameraInfo();
        Camera.getCameraInfo(mCurrentFacing, info);
        //orientation = (360 - orientation + 45) / 90 * 90;
        int rotation = 0;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            rotation = (info.orientation - orientation + 360) % 360;
        } else {  // back-facing camera
            rotation = (info.orientation + orientation) % 360;
        }

        setParameterAsync("rotation", Integer.toString(rotation));
    }

    public void restartPreviewIfNeeded() {
        new Thread() {
            public void run() {
                synchronized (mParametersThread) {
                    try {
                        mCamera.startPreview();
                        mPreview.setPauseCopyFrame(false);
                    } catch (Exception e) {
                        // ignore
                    }
                }
            }
        }.start();
    }

    public void setCameraMode(final int mode) {
        if (mPreviewPauseListener != null) {
            mPreviewPauseListener.onPreviewPause();
        }

        // Unlock any exposure/stab lock that was caused by
        // swiping the ring
        setLockSetup(false);

        new Thread() {
            public void run() {
                mIsModeSwitching = true;
                synchronized (mParametersThread) {
                    Camera.Parameters params = getParameters();

                    if (params == null) {
                        // We're likely in the middle of a transient state.
                        // Just do that again shortly when the camera will
                        // be available.
                        return;
                    }

                    boolean shouldStartPreview = false;

                    if (mode == CameraActivity.CAMERA_MODE_VIDEO) {
                        if (!mIsRecordingHint) {
                            params.setRecordingHint(true);
                            mIsRecordingHint = true;
                            mCamera.stopPreview();
                            shouldStartPreview = true;
                        }
                    } else {
                        if (mIsRecordingHint) {
                            params.setRecordingHint(false);
                            mIsRecordingHint = false;
                            mCamera.stopPreview();
                            shouldStartPreview = true;
                        }
                    }

                    if (mode == CameraActivity.CAMERA_MODE_PANO) {
                        // Apply special settings for panorama mode
                        initializePanoramaMode();
                    } else {
                        // Make sure the Infinity mode from panorama is gone
                        params.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                    }

                    if (mode == CameraActivity.CAMERA_MODE_PICSPHERE) {
                        // If we are in PicSphere mode, set pic size to 640x480 to avoid using
                        // all the phone's resources when rendering, as well as not take 15 min
                        // to render. This value can be changed in the settings widget once
                        // in PicSphere mode, so this value will be changed again when the
                        // widget loads.
                        params.setPictureSize(640, 480);
                        params.setPreviewSize(640, 480);

                        // Set focus mode to infinity
                        setInfinityFocus(params);
                    } else {
                        params.setPreviewSize(mTargetSize.x, mTargetSize.y);
                    }

                    mCamera.setParameters(params);
                    mParameters = mCamera.getParameters();

                    if (shouldStartPreview) {
                        mCamera.startPreview();
                    }
                }

                mPreview.setPauseCopyFrame(false);
                mIsModeSwitching = false;

                if (mPreviewPauseListener != null) {
                    mPreviewPauseListener.onPreviewResume();
                }
            }
        }.start();
    }

    /**
     * Updates the orientation of the display
     */
    public void updateDisplayOrientation() {
        android.hardware.Camera.CameraInfo info =
                new android.hardware.Camera.CameraInfo();
        android.hardware.Camera.getCameraInfo(mCurrentFacing, info);
        int degrees = Util.getDisplayRotation(null);

        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;  // compensate the mirror
        } else {  // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        mCamera.setDisplayOrientation(result);
    }

    /**
     * Initializes the Panorama (mosaic) mode
     */
    private void initializePanoramaMode() {
        Camera.Parameters parameters = getParameters();

        int pixels = mContext.getResources().getInteger(R.integer.config_panoramaDefaultWidth)
                * mContext.getResources().getInteger(R.integer.config_panoramaDefaultHeight);

        List<Camera.Size> supportedSizes = parameters.getSupportedPreviewSizes();
        Point previewSize = Util.findBestPanoPreviewSize(supportedSizes, false, false, pixels);

        Log.v(TAG, "preview h = " + previewSize.y + " , w = " + previewSize.x);
        parameters.setPreviewSize(previewSize.x, previewSize.y);
        mTargetSize = previewSize;

        List<int[]> frameRates = parameters.getSupportedPreviewFpsRange();
        if (frameRates != null) {
            int last = frameRates.size() - 1;
            int minFps = (frameRates.get(last))[Camera.Parameters.PREVIEW_FPS_MIN_INDEX];
            int maxFps = (frameRates.get(last))[Camera.Parameters.PREVIEW_FPS_MAX_INDEX];
            parameters.setPreviewFpsRange(minFps, maxFps);
            Log.v(TAG, "preview fps: " + minFps + ", " + maxFps);
        }

        setInfinityFocus(parameters);

        parameters.set("recording-hint", "false");
        mParameters = parameters;
    }

    private void setInfinityFocus(Camera.Parameters parameters) {
        List<String> supportedFocusModes = parameters.getSupportedFocusModes();
        if (supportedFocusModes != null
                && supportedFocusModes.indexOf(Camera.Parameters.FOCUS_MODE_INFINITY) >= 0) {
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
        } else {
            // Use the default focus mode and log a message
            Log.w(TAG, "Cannot set the focus mode to "
                    + Camera.Parameters.FOCUS_MODE_INFINITY
                    + " because the mode is not supported.");
        }
    }

    /**
     * Trigger the autofocus function of the device
     *
     * @param cb The AF callback
     * @return true if we could start the AF, false otherwise
     */
    public boolean doAutofocus(final AutoFocusCallback cb) {
        if (mCamera != null) {
            try {
                // Make sure our auto settings aren't locked
                setLockSetup(false);

                // Trigger af
                mCamera.cancelAutoFocus();

                mHandler.post(new Runnable() {
                    public void run() {
                        try {
                            mCamera.autoFocus(cb);
                        } catch (Exception e) {
                            // Do nothing here
                        }
                    }
                });

            } catch (Exception e) {
                return false;
            }
            return true;
        } else {
            return false;
        }
    }

    /**
     * Returns whether or not the current open camera device supports
     * focus area (focus ring)
     *
     * @return true if supported
     */
    public boolean isFocusAreaSupported() {
        if (mCamera != null) {
            try {
                return (getParameters().getMaxNumFocusAreas() > 0);
            } catch (Exception e) {
                return false;
            }
        } else {
            return false;
        }
    }

    /**
     * Returns whether or not the current open camera device supports
     * exposure metering area (exposure ring)
     *
     * @return true if supported
     */
    public boolean isExposureAreaSupported() {
        if (mCamera != null) {
            try {
                return (getParameters().getMaxNumMeteringAreas() > 0);
            } catch (Exception e) {
                return false;
            }
        } else {
            return false;
        }
    }

    /**
     * Defines the main focus point of the camera to the provided x and y values.
     * Those values must between -1000 and 1000, where -1000 is the top/left, and 1000 the bottom/right
     *
     * @param x The X position of the focus point
     * @param y The Y position of the focus point
     */
    public void setFocusPoint(int x, int y) {
        Camera.Parameters params = getParameters();

        if (params.getMaxNumFocusAreas() > 0) {
            List<Camera.Area> focusArea = new ArrayList<Camera.Area>();
            focusArea.add(new Camera.Area(new Rect(x, y, x + FOCUS_WIDTH, y + FOCUS_HEIGHT), 1000));

            params.setFocusAreas(focusArea);

            try {
                mCamera.setParameters(params);
            } catch (Exception e) {
                // Ignore, we might be setting it too
                // fast since previous attempt
            }
        }
    }

    /**
     * Defines the exposure metering point of the camera to the provided x and y values.
     * Those values must between -1000 and 1000, where -1000 is the top/left, and 1000 the bottom/right
     *
     * @param x The X position of the exposure metering point
     * @param y The Y position of the exposure metering point
     */
    public void setExposurePoint(int x, int y) {
        Camera.Parameters params = getParameters();

        if (params != null && params.getMaxNumMeteringAreas() > 0) {
            List<Camera.Area> exposureArea = new ArrayList<Camera.Area>();
            exposureArea.add(new Camera.Area(new Rect(x, y, x + FOCUS_WIDTH, y + FOCUS_HEIGHT), 1000));

            params.setMeteringAreas(exposureArea);

            try {
                mCamera.setParameters(params);
            } catch (Exception e) {
                // Ignore, we might be setting it too
                // fast since previous attempt
            }
        }
    }

    public void setAutoFocusMoveCallback(AutoFocusMoveCallback cb) {
        mAutoFocusMoveCallback = cb;

        List<String> focusModes = mParameters.getSupportedFocusModes();
        if (mCamera != null && focusModes != null && focusModes.contains(
                Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
            mCamera.setAutoFocusMoveCallback(cb);
        }
    }

    /**
     * Enable the device image stabilization system.
     * @param enabled True to stabilize
     */
    public void setStabilization(boolean enabled) {
        Camera.Parameters params = getParameters();
        if (params == null) {
            return;
        }

        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
            // In wrappers: sony has sony-is, HTC has ois_mode, etc.
            if (params.get("image-stabilization") != null) {
                params.set("image-stabilization", enabled ? "on" : "off");
            }
        } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            if (params.isVideoStabilizationSupported()) {
                params.setVideoStabilization(enabled);
            }
        }

        try {
            mCamera.setParameters(params);
        } catch (Exception e) {
            // Do nothing here
        }
    }

    /**
     * Sets the camera device to render to a texture rather than a SurfaceHolder
     * @param texture The texture to which render
     */
    public void setRenderToTexture(SurfaceTexture texture) {
        mPreview.setRenderToTexture(texture);
        Log.i(TAG, "Needs to render to texture, rebooting preview");
        mPreview.notifyCameraChanged(true);
    }

    /**
     * The CameraPreview class handles the Camera preview feed
     * and setting the surface holder.
     */
    public class CameraPreview extends SurfaceView implements Camera.PreviewCallback {
        private final static String TAG = "CameraManager.CameraPreview";

        private SurfaceTexture mTexture;
        private byte[] mLastFrameBytes;
        private boolean mPauseCopyFrame;

        public CameraPreview(Context context) {
            super(context);
        }

        /**
         * Sets that the camera preview should rather render to a texture than the default
         * SurfaceHolder.
         * Note that you have to restart camera preview manually after setting this.
         * Set to null to reset render to the SurfaceHolder.
         *
         * @param texture The target texture
         */
        public void setRenderToTexture(SurfaceTexture texture) {
            mTexture = texture;
        }

        public void setPauseCopyFrame(boolean pause) {
            mPauseCopyFrame = pause;

            if (!pause && mCamera != null) {
                postCallbackBuffer();
            }
        }

        public void notifyPreviewSize(int width, int height) {
            mLastFrameBytes = new byte[(int) (width * height * 1.5 + 0.5)];
            requestLayout();
        }

        public byte[] getLastFrameBytes() {
            return mLastFrameBytes;
        }

        public void notifyCameraChanged(boolean startPreview) {
            synchronized (mParametersThread) {
                if (mCamera != null) {
                    if (startPreview) {
                        mCamera.stopPreview();
                    }

                    setupCamera();

                    try {
                        mCamera.setPreviewTexture(mTexture);

                        if (startPreview) {
                            updateDisplayOrientation();
                            mCamera.startPreview();
                            postCallbackBuffer();
                        }
                    } catch (IOException e) {
                        Log.e(TAG, "Error setting camera preview: " + e.getMessage());
                    }
                }
            }
        }

        public void postCallbackBuffer() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mCamera != null) {
                        mCamera.addCallbackBuffer(mLastFrameBytes);
                        mCamera.setPreviewCallbackWithBuffer(CameraPreview.this);
                    }
                }
            });
        }

        private void setupCamera() {
            // Set device-specifics here
            try {
                Camera.Parameters params = mCamera.getParameters();
                Resources res = getResources();

                if (res != null) {
                    if (res.getBoolean(R.bool.config_qualcommZslCameraMode)) {
                        if (res.getBoolean(R.bool.config_useSamsungZSL)) {
                            mCamera.sendRawCommand(1508, 0, 0);
                        }
                        params.set("camera-mode", 1);
                    }
                }

                mCamera.setParameters(params);

                postCallbackBuffer();
            } catch (Exception e) {
                Log.e(TAG, "Could not set device specifics");

            }
        }

        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            if (mCamera != null && !mPauseCopyFrame) {
                mCamera.addCallbackBuffer(mLastFrameBytes);
            }
        }
    }

    public class CameraRenderer implements GLSurfaceView.Renderer {
        private final float[] mTransformMatrix;
        int mTexture;
        private SurfaceTexture mSurface;

        private final static String VERTEX_SHADER =
                "attribute vec4 vPosition;\n" +
                        "attribute vec2 a_texCoord;\n" +
                        "varying vec2 v_texCoord;\n" +
                        "uniform mat4 u_xform;\n" +
                        "void main() {\n" +
                        "  gl_Position = vPosition;\n" +
                        "  v_texCoord = vec2(u_xform * vec4(a_texCoord, 1.0, 1.0));\n" +
                        "}\n";

        private final static String FRAGMENT_SHADER =
                "#extension GL_OES_EGL_image_external : require\n" +
                        "precision mediump float;\n" +
                        "uniform samplerExternalOES s_texture;\n" +
                        "varying vec2 v_texCoord;\n" +
                        "void main() {\n" +
                        "  gl_FragColor = texture2D(s_texture, v_texCoord);\n" +
                        "}\n";

        private FloatBuffer vertexBuffer, textureVerticesBuffer;
        private int mProgram;
        private int mPositionHandle;
        private int mTextureCoordHandle;
        private int mTransformHandle;

        // Number of coordinates per vertex in this array
        static final int COORDS_PER_VERTEX = 2;
        static final float RATIO = 4.0f/3.0f;

        private final float squareVertices[] =
                { 1.0f * RATIO, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f * RATIO, -1.0f };

        private final float textureVertices[] =
                { 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

        public CameraRenderer() {
            mTransformMatrix = new float[16];
        }

        public void onSurfaceCreated(GL10 unused, EGLConfig config) {
            mTexture = createTexture();
            mSurface = new SurfaceTexture(mTexture);
            GLES20.glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

            setRenderToTexture(mSurface);

            // Setup vertex buffer
            ByteBuffer bb = ByteBuffer.allocateDirect(squareVertices.length * 4);
            bb.order(ByteOrder.nativeOrder());
            vertexBuffer = bb.asFloatBuffer();
            vertexBuffer.put(squareVertices);
            vertexBuffer.position(0);

            ByteBuffer bb2 = ByteBuffer.allocateDirect(textureVertices.length * 4);
            bb2.order(ByteOrder.nativeOrder());
            textureVerticesBuffer = bb2.asFloatBuffer();
            textureVerticesBuffer.put(textureVertices);
            textureVerticesBuffer.position(0);

            // Load shaders
            int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, VERTEX_SHADER);
            int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, FRAGMENT_SHADER);

            mProgram = GLES20.glCreateProgram();             // create empty OpenGL ES Program
            GLES20.glAttachShader(mProgram, vertexShader);   // add the vertex shader to program
            GLES20.glAttachShader(mProgram, fragmentShader); // add the fragment shader to program
            GLES20.glLinkProgram(mProgram);

            // Since we only use one program/texture/vertex array, we bind them once here
            // and then we only draw what we need in onDrawFrame
            GLES20.glUseProgram(mProgram);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mTexture);

            mPositionHandle = GLES20.glGetAttribLocation(mProgram, "vPosition");
            GLES20.glVertexAttribPointer(mPositionHandle, COORDS_PER_VERTEX, GLES20.GL_FLOAT,
                    false, 0, vertexBuffer);
            GLES20.glEnableVertexAttribArray(mPositionHandle);

            mTextureCoordHandle = GLES20.glGetAttribLocation(mProgram, "a_texCoord");
            GLES20.glVertexAttribPointer(mTextureCoordHandle, COORDS_PER_VERTEX, GLES20.GL_FLOAT,
                    false, 0, textureVerticesBuffer);
            GLES20.glEnableVertexAttribArray(mTextureCoordHandle);

            mTransformHandle = GLES20.glGetUniformLocation(mProgram, "u_xform");
        }

        public void onDrawFrame(GL10 unused) {
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

            if (mSurface != null) {
                mSurface.updateTexImage();
                mSurface.getTransformMatrix(mTransformMatrix);
                GLES20.glUniformMatrix4fv(mTransformHandle, 1, false, mTransformMatrix, 0);
            }

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_FAN, 0, 4);
        }

        public void onSurfaceChanged(GL10 unused, int width, int height) {
            GLES20.glViewport(0, 0, width, height);
        }

        public int loadShader(int type, String shaderCode) {
            int shader = GLES20.glCreateShader(type);

            GLES20.glShaderSource(shader, shaderCode);
            GLES20.glCompileShader(shader);

            return shader;
        }

        private int createTexture() {
            int[] texture = new int[1];

            GLES20.glGenTextures(1,texture, 0);
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, texture[0]);

            return texture[0];
        }
    }
}
