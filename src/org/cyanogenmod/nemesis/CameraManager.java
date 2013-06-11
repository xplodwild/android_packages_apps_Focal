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
import android.graphics.Point;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.AutoFocusMoveCallback;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

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
    private int mCurrentFacing;
    private Point mTargetSize;
    private AutoFocusMoveCallback mAutoFocusMoveCallback;
    private Camera.Parameters mParameters;
    private int[] mPreviewFrameBuffer;
    private int mOrientation;
    private MediaRecorder mMediaRecorder;
    private PreviewPauseListener mPreviewPauseListener;

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

    private class AsyncParamRunnable implements Runnable {
        private String mKey;
        private String mValue;

        public AsyncParamRunnable(String key, String value) {
            mKey = key;
            mValue = value;
        }

        public void run() {
            Log.v(TAG, "Asynchronously setting parameter " + mKey + " to " + mValue);
            Camera.Parameters params = getParameters();
            params.set(mKey, mValue);

            try {
                mCamera.setParameters(params);
            } catch (RuntimeException e) {
                Log.e(TAG, "Could not set parameter " + mKey + " to '" + mValue + "'", e);
            }
            // Read them from sensor next time
            mParameters = null;
        }
    }

    ;


    public CameraManager(Context context) {
        mPreview = new CameraPreview(context);
        mMediaRecorder = new MediaRecorder();
    }

    /**
     * Opens the camera and show its preview in the preview
     *
     * @param cameraId
     * @return true if the operation succeeded, false otherwise
     */
    public boolean open(int cameraId) {
        if (mCamera != null) {
            // Close the previous camera
            releaseCamera();
        }

        // Try to open the camera
        try {
            mCamera = Camera.open(cameraId);
            mCamera.setPreviewCallback(mPreview);
            mCurrentFacing = cameraId;
            mParameters = mCamera.getParameters();

            if (mTargetSize != null)
                setPreviewSize(mTargetSize.x, mTargetSize.y);

            if (mAutoFocusMoveCallback != null)
                mCamera.setAutoFocusMoveCallback(mAutoFocusMoveCallback);

            // Default focus mode to continuous
        } catch (Exception e) {
            Log.e(TAG, "Error while opening cameras: " + e.getMessage());
            return false;
        }

        // Update the preview surface holder with the new opened camera
        mPreview.notifyCameraChanged();

        return true;
    }

    public void setPreviewPauseListener(PreviewPauseListener listener) {
        mPreviewPauseListener = listener;
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
        if (mCamera == null)
            return null;

        if (mParameters == null)
            mParameters = mCamera.getParameters();

        return mParameters;
    }

    public void pause() {
        releaseCamera();
    }

    public void resume() {
        reconnectToCamera();
    }

    private void releaseCamera() {
        if (mCamera != null) {
            Log.v(TAG, "Releasing camera facing " + mCurrentFacing);
            mCamera.release();
        }
        mCamera = null;
        mParameters = null;
        mPreview.notifyCameraChanged();
    }

    private void reconnectToCamera() {
        open(mCurrentFacing);
    }

    public void setPreviewSize(int width, int height) {
        mTargetSize = new Point(width, height);

        if (mCamera != null) {
            mParameters.setPreviewSize(width, height);

            Log.v(TAG, "Preview size is " + width + "x" + height);

            mCamera.stopPreview();
            mCamera.setParameters(mParameters);
            mPreview.notifyPreviewSize(width, height);
            mCamera.startPreview();
        }
    }

    public void setParameterAsync(String key, String value) {
        AsyncParamRunnable run = new AsyncParamRunnable(key, value);
        new Thread(run).start();
    }

    /**
     * Locks the automatic settings of the camera device, like White balance and
     * exposure.
     * @param lock true to lock, false to unlock
     */
    public void setLockSetup(boolean lock) {
        Camera.Parameters params = getParameters();

        if (params.isAutoExposureLockSupported()) {
            params.setAutoExposureLock(lock);
        }

        if (params.isAutoWhiteBalanceLockSupported()) {
            params.setAutoWhiteBalanceLock(lock);
        }

        mCamera.setParameters(params);
    }

    /**
     * Returns the last frame of the preview surface
     * @return Bitmap
     */
    public Bitmap getLastPreviewFrame() {
        // Decode the last frame bytes
        byte[] data = mPreview.getLastFrameBytes();
        int previewWidth = getParameters().getPreviewSize().width;
        int previewHeight = getParameters().getPreviewSize().height;

        // Convert YUV420SP preview data to RGB
        if (data != null) {
            Bitmap output = Util.decodeYUV420SP(mPreview.getContext(),
                    data, previewWidth, previewHeight);
            return output;
        } else {
            return null;
        }
    }

    /**
     * Takes a snapshot
     */
    public void takeSnapshot(Camera.ShutterCallback shutterCallback, Camera.PictureCallback raw,
                             Camera.PictureCallback jpeg) {
        Log.v(TAG, "takePicture");
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
        mMediaRecorder.stop();
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
        try {
            mCamera.startPreview();
        } catch (Exception e) {
            // ignore
        }
    }

    public void setCameraMode(final int mode) {
        if (mPreviewPauseListener != null) {
            mPreviewPauseListener.onPreviewPause();
        }

        new Thread() {
            public void run() {
                // We voluntarily sleep the thread here for the animation being done
                // in the CameraActivity
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                Camera.Parameters params = getParameters();

                if (mode == CameraActivity.CAMERA_MODE_VIDEO) {
                    params.setRecordingHint(true);
                } else {
                    params.setRecordingHint(false);
                }


                mCamera.stopPreview();
                mCamera.setParameters(params);
                mCamera.startPreview();
                mPreview.postCallbackBuffer();

                if (mPreviewPauseListener != null) {
                    mPreviewPauseListener.onPreviewResume();
                }
            }
        }.start();
    }

    /**
     * Trigger the autofocus function of the device
     *
     * @param cb The AF callback
     * @return true if we could start the AF, false otherwise
     */
    public boolean doAutofocus(AutoFocusCallback cb) {
        if (mCamera != null) {
            try {
                // make sure our auto settings aren't locked
                setLockSetup(false);

                // trigger af
                mCamera.autoFocus(cb);
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

        if (params.getMaxNumMeteringAreas() > 0) {
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
     * The CameraPreview class handles the Camera preview feed
     * and setting the surface holder.
     */
    public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {
        private final static String TAG = "CameraManager.CameraPreview";

        private SurfaceHolder mHolder;
        private byte[] mLastFrameBytes;

        public CameraPreview(Context context) {
            super(context);

            // Install a SurfaceHolder.Callback so we get notified when the
            // underlying surface is created and destroyed.
            mHolder = getHolder();
            mHolder.addCallback(this);
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

        public void notifyCameraChanged() {
            if (mCamera != null) {
                setupCamera();

                try {
                    mCamera.setPreviewDisplay(mHolder);
                    mCamera.startPreview();
                    mPreview.postCallbackBuffer();
                } catch (IOException e) {
                    Log.e(TAG, "Error setting camera preview: " + e.getMessage());
                }
            }
        }

        public void surfaceCreated(SurfaceHolder holder) {
            // The Surface has been created, now tell the camera where to draw the preview.
            if (mCamera == null)
                return;

            try {
                mCamera.setPreviewDisplay(holder);
                mCamera.startPreview();
                mPreview.postCallbackBuffer();
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
            try {
                mCamera.stopPreview();
            } catch (Exception e) {
                // ignore: tried to stop a non-existent preview
            }

            setupCamera();

            // start preview with new settings
            try {
                mCamera.setPreviewDisplay(mHolder);
                mCamera.startPreview();


            } catch (Exception e) {
                Log.d(TAG, "Error starting camera preview: " + e.getMessage());
            }
        }

        public void postCallbackBuffer() {
            mCamera.addCallbackBuffer(mLastFrameBytes);
            mCamera.setPreviewCallbackWithBuffer(this);
        }

        private void setupCamera() {
            // Set device-specifics here
            try {
                Camera.Parameters params = mCamera.getParameters();

                if (getResources().getBoolean(R.bool.config_qualcommZslCameraMode))
                    params.set("camera-mode", 1);

                mCamera.setParameters(params);

                postCallbackBuffer();
            } catch (Exception e) {
                Log.e(TAG, "Could not set device specifics");
            }
        }

        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            if (mCamera != null) {
                mCamera.addCallbackBuffer(mLastFrameBytes);
            }
        }
    }
}
