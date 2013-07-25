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

import android.content.Context;
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
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import org.apache.http.NameValuePair;
import org.apache.http.message.BasicNameValuePair;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

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
    private long mLastPreviewFrameTime;
    private boolean mIsModeSwitching;
    private List<NameValuePair> mPendingParameters;
    private boolean mIsResuming;

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
                        // do nothing
                    }

                    List<NameValuePair> copy = new ArrayList<NameValuePair>(mPendingParameters);
                    mPendingParameters.clear();

                    for (NameValuePair pair : copy) {
                        String key = pair.getName();
                        String val = pair.getValue();
                        Log.v(TAG, "Asynchronously setting parameter " + key+ " to " + val);
                        Camera.Parameters params = getParameters();
                        String workingValue = params.get(key);
                        params.set(key, val);

                        try {
                            mCamera.setParameters(params);
                        } catch (RuntimeException e) {
                            Log.e(TAG, "Could not set parameter " + key + " to '" + val + "', restoring '"
                                    + workingValue + "'", e);

                            // Reset the parameter back in storage
                            SettingsStorage.storeCameraSetting(mContext, mCurrentFacing, key, workingValue);

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
    }

    /**
     * Opens the camera and show its preview in the preview
     *
     * @param cameraId
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
                        mCamera.setAutoFocusMoveCallback(mAutoFocusMoveCallback);
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
        if (mCamera == null) {
            Log.w(TAG, "getParameters when camera is null");
            return null;
        }

        synchronized (mParametersThread) {
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
     * @param params
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
        if (data != null) {
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

    /**
     * Returns the system time of when the last preview frame was received
     *
     * @return long
     */
    public long getLastPreviewFrameTime() {
        return mLastPreviewFrameTime;
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

        setParameterAsync("picture-size", Integer.toString(width) + "x" + Integer.toString(height));
    }

    /**
     * Takes a snapshot
     */
    public void takeSnapshot(Camera.ShutterCallback shutterCallback, Camera.PictureCallback raw,
                             Camera.PictureCallback jpeg) {
        Log.v(TAG, "takePicture");
        SoundManager.getSingleton().play(SoundManager.SOUND_SHUTTER);
        mCamera.takePicture(shutterCallback, raw, jpeg);
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
        long maxFileSize = Storage.getStorage().getAvailableSpace() - Storage.LOW_STORAGE_THRESHOLD;
        mMediaRecorder.setMaxFileSize(maxFileSize);
        mMediaRecorder.setMaxDuration(0); // infinite


        mMediaRecorder.setPreviewDisplay(mPreview.getSurfaceHolder().getSurface());

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
        } catch (Exception e) { }
        mCamera.lock();
        mMediaRecorder.reset();
        mPreview.postCallbackBuffer();
    }

    /**
     * Returns the orientation of the device
     *
     * @return
     */
    public int getOrientation() {
        return mOrientation;
    }

    public void setOrientation(int orientation) {
        mOrientation = orientation;
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
                        // We're likely in the middle of a transient state. Just do that again
                        // shortly when the camera will be available.
                        return;
                    }

                    if (mode == CameraActivity.CAMERA_MODE_VIDEO) {
                        params.setRecordingHint(true);
                    } else {
                        params.setRecordingHint(false);
                    }

                    mCamera.stopPreview();

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
                        // to render. Eventually, we could put up a setting somewhere to let users
                        // rendering super high quality pictures.
                        params.setPictureSize(640, 480);
                        params.setPreviewSize(640, 480);

                        // Set focus mode to infinity
                        setInfinityFocus(params);
                    } else {
                        params.setPreviewSize(mTargetSize.x, mTargetSize.y);
                    }

                    mCamera.setParameters(params);
                    mParameters = mCamera.getParameters();
                    try {
                        // PicSphere and Pano renders to a texture, so the preview will be started
                        // once the SurfaceTexture is ready to receive frames
                        if (mode != CameraActivity.CAMERA_MODE_PICSPHERE
                                && mode != CameraActivity.CAMERA_MODE_PANO) {
                            mCamera.startPreview();
                        }

                        mPreview.notifyPreviewSize(mTargetSize.x, mTargetSize.y);
                    } catch (RuntimeException e) {
                        Log.e(TAG, "Unable to start preview", e);
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

        List<Camera.Size> supportedPicSizes = parameters.getSupportedPictureSizes();
        if (supportedPicSizes.contains(mTargetSize)) {
            parameters.setPictureSize(mTargetSize.x, mTargetSize.y);
        }

        List<int[]> frameRates = parameters.getSupportedPreviewFpsRange();
        int last = frameRates.size() - 1;
        int minFps = (frameRates.get(last))[Camera.Parameters.PREVIEW_FPS_MIN_INDEX];
        int maxFps = (frameRates.get(last))[Camera.Parameters.PREVIEW_FPS_MAX_INDEX];
        parameters.setPreviewFpsRange(minFps, maxFps);
        Log.v(TAG, "preview fps: " + minFps + ", " + maxFps);

        setInfinityFocus(parameters);

        parameters.set("recording-hint", "false");
        mParameters = parameters;
    }

    private void setInfinityFocus(Camera.Parameters parameters) {
        List<String> supportedFocusModes = parameters.getSupportedFocusModes();
        if (supportedFocusModes.indexOf(Camera.Parameters.FOCUS_MODE_INFINITY) >= 0) {
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
        } else {
            // Use the default focus mode and log a message
            Log.w(TAG, "Cannot set the focus mode to " + Camera.Parameters.FOCUS_MODE_INFINITY +
                    " because the mode is not supported.");
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
                // make sure our auto settings aren't locked
                setLockSetup(false);

                // trigger af
                mCamera.cancelAutoFocus();

                mHandler.post(new Runnable() {
                    public void run() {
                        try {
                            mCamera.autoFocus(cb);
                        } catch (Exception e) { }
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
                // ignore, we might be setting it too fast since previous attempt
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
                // ignore, we might be setting it too fast since previous attempt
            }
        }
    }

    public void setAutoFocusMoveCallback(AutoFocusMoveCallback cb) {
        mAutoFocusMoveCallback = cb;

        if (mCamera != null)
            mCamera.setAutoFocusMoveCallback(cb);
    }

    /**
     * Enable the device image stabilization system.
     * @param enabled
     */
    public void setStabilization(boolean enabled) {
        Camera.Parameters params = getParameters();
        if (params == null) return;

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
        } catch (Exception e) { }
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
     * Redirect the camera preview callbacks to the specified preview callback rather than the
     * default preview callback.
     * @param cb The callback
     */
    public void deviatePreviewCallbackWithBuffer(Camera.PreviewCallback cb) {
        mCamera.setPreviewCallbackWithBuffer(cb);
    }

    /**
     * The CameraPreview class handles the Camera preview feed
     * and setting the surface holder.
     */
    public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {
        private final static String TAG = "CameraManager.CameraPreview";

        private SurfaceHolder mHolder;
        private SurfaceTexture mTexture;
        private byte[] mLastFrameBytes;
        private boolean mPauseCopyFrame;

        public CameraPreview(Context context) {
            super(context);

            // Install a SurfaceHolder.Callback so we get notified when the
            // underlying surface is created and destroyed.
            mHolder = getHolder();
            mHolder.addCallback(this);
        }

        /**
         * Sets that the camera preview should rather render to a texture than the default
         * SurfaceHolder.
         * Note that you have to restart camera preview manually after setting this.
         * Set to null to reset render to the SurfaceHolder.
         *
         * @param texture
         */
        public void setRenderToTexture(SurfaceTexture texture) {
            mTexture = texture;
        }

        public SurfaceTexture getSurfaceTexture() {
            return mTexture;
        }

        public void setPauseCopyFrame(boolean pause) {
            mPauseCopyFrame = pause;

            if (!pause && mCamera != null) {
                postCallbackBuffer();
            }
        }

        public void notifyPreviewSize(int width, int height) {
            mLastFrameBytes = new byte[(int) (width * height * 1.5 + 0.5)];
        }

        public byte[] getLastFrameBytes() {
            return mLastFrameBytes;
        }

        public SurfaceHolder getSurfaceHolder() {
            return mHolder;
        }

        public void notifyCameraChanged(boolean startPreview) {
            synchronized (mParametersThread) {
                if (mCamera != null) {
                    if (startPreview) {
                        mCamera.stopPreview();
                    }

                    setupCamera();

                    try {
                        if (mTexture != null) {
                            mCamera.setPreviewTexture(mTexture);
                        } else {
                            mCamera.setPreviewDisplay(mHolder);
                        }

                        if (startPreview) {
                            mCamera.startPreview();
                            postCallbackBuffer();
                        }
                    } catch (IOException e) {
                        Log.e(TAG, "Error setting camera preview: " + e.getMessage());
                    }
                }
            }
        }

        public void surfaceCreated(SurfaceHolder holder) {
            // The Surface has been created, now tell the camera where to draw the preview.
            if (mCamera == null)
                return;

            try {
                if (mTexture != null) {
                    mCamera.setPreviewTexture(mTexture);
                } else {
                    mCamera.setPreviewDisplay(mHolder);
                }
                mCamera.startPreview();
                postCallbackBuffer();
            } catch (IOException e) {
                Log.e(TAG, "Error setting camera preview: " + e.getMessage());
            }
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
            // empty. Take care of releasing the Camera preview in your activity.
        }

        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            if (mHolder.getSurface() == null) {
                // preview surface does not exist
                return;
            }

            // stop preview before making changes
            synchronized (mParametersThread) {
                try {
                    mCamera.stopPreview();
                } catch (Exception e) {
                    // ignore: tried to stop a non-existent preview
                }

                setupCamera();

                // start preview with new settings
                try {
                    if (mTexture != null) {
                        mCamera.setPreviewTexture(mTexture);
                    } else {
                        mCamera.setPreviewDisplay(mHolder);
                    }
                    mCamera.startPreview();

                    mParameters = mCamera.getParameters();
                    postCallbackBuffer();
                } catch (Exception e) {
                    Log.d(TAG, "Error starting camera preview: " + e.getMessage());
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

                if (getResources().getBoolean(R.bool.config_qualcommZslCameraMode)) {
                    params.set("camera-mode", 1);
                }

                mCamera.setParameters(params);

                postCallbackBuffer();
            } catch (Exception e) {
                Log.e(TAG, "Could not set device specifics");
            }
        }

        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            mLastPreviewFrameTime = System.currentTimeMillis();
            if (mCamera != null && !mPauseCopyFrame) {
                mCamera.addCallbackBuffer(mLastFrameBytes);
            }
        }
    }
}
