/*
 * Copyright (C) 2013 The CyanogenMod Project
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.nemesis;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.AutoFocusMoveCallback;
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
            mCamera.setParameters(params);
            // Read them from sensor next time
            mParameters = null;
        }
    };
    

    public CameraManager(Context context) {
        mPreview = new CameraPreview(context);
    }

    /**
     * Opens the camera and show its preview in the preview
     * @param facing
     * @return true if the operation succeeded, false otherwise
     */
    public boolean open(int facing) {
        if (mCamera != null) {
            // Close the previous camera
            releaseCamera();
        }

        // Try to open the camera
        try {
            mCamera = Camera.open(facing);
            mCamera.setPreviewCallback(mPreview);
            mCurrentFacing = facing;
            mParameters = mCamera.getParameters();

            if (mTargetSize != null)
                setPreviewSize(mTargetSize.x, mTargetSize.y);
            
            if (mAutoFocusMoveCallback != null)
                mCamera.setAutoFocusMoveCallback(mAutoFocusMoveCallback);

            // Default focus mode to continuous
        }
        catch (Exception e) {
            Log.e(TAG, "Error while opening cameras: " + e.getMessage());
            return false;
        }

        // Update the preview surface holder with the new opened camera
        mPreview.notifyCameraChanged();

        return true;
    }

    /**
     * Returns the preview surface used to display the Camera's preview
     * @return CameraPreview
     */
    public CameraPreview getPreviewSurface() {
        return mPreview;
    }

    /**
     * Returns the parameters structure of the current running camera
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
        mCamera.release();
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
            mCamera.startPreview();
        }
    }
    
    public void setParameterAsync(String key, String value) {
        AsyncParamRunnable run = new AsyncParamRunnable(key, value);
        new Thread(run).start();
    }
    
    public Bitmap getLastPreviewFrame() {
        // Decode the last frame bytes
        byte[] data = mPreview.getLastFrameBytes();
        int previewWidth = getParameters().getPreviewSize().width;
        int previewHeight = getParameters().getPreviewSize().height;
        
        if (mPreviewFrameBuffer == null || mPreviewFrameBuffer.length != (previewWidth*previewHeight*3))
            mPreviewFrameBuffer = new int[previewWidth*previewHeight];
        
        // Convert YUV420SP preview data to RGB
        Log.e(TAG, "Preview is " + previewWidth + "x" + previewHeight);
        Util.decodeYUV420SP(mPreviewFrameBuffer, data, previewWidth, previewHeight);
        
        // Decode the RGB data to a bitmap
        Bitmap output = Bitmap.createBitmap(mPreviewFrameBuffer, previewWidth, previewHeight, Bitmap.Config.ARGB_8888);
        
        return output;
    }

    /**
     * Takes a snapshot
     */
    public void takeSnapshot(Camera.ShutterCallback shutterCallback, Camera.PictureCallback raw,
                             Camera.PictureCallback jpeg) {
        Log.v(TAG, "takePicture");
        mCamera.takePicture(shutterCallback, raw, jpeg);
    }

    public void restartPreviewIfNeeded() {
        try {
            mCamera.startPreview();
        } catch (Exception e) {
            // ignore
        }
    }
    
    /**
     * Trigger the autofocus function of the device
     * @param cb The AF callback
     * @return true if we could start the AF, false otherwise
     */
    public boolean doAutofocus(AutoFocusCallback cb) {
        if (mCamera != null) {
            mCamera.autoFocus(cb);
            return true;
        } else {
            return false;
        }
        
    }

    /**
     * Defines the main focus point of the camera to the provided x and y values.
     * Those values must between -1000 and 1000, where -1000 is the top/left, and 1000 the bottom/right
     * @param x The X position of the focus point
     * @param y The Y position of the focus point
     */
    public void setFocusPoint(int x, int y) {
        Camera.Parameters params = getParameters();

        if (params.getMaxNumFocusAreas() > 0) {
            List<Camera.Area> focusArea = new ArrayList<Camera.Area>();
            focusArea.add(new Camera.Area(new Rect(x, y, x + FOCUS_WIDTH, y + FOCUS_HEIGHT), 1000));

            Log.e(TAG, "Setting focus point to " + x + ";" + y);
            params.setFocusAreas(focusArea);

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

            mLastFrameBytes = new byte[2100000];

            // Install a SurfaceHolder.Callback so we get notified when the
            // underlying surface is created and destroyed.
            mHolder = getHolder();
            mHolder.addCallback(this);
        }
        
        public byte[] getLastFrameBytes() {
            return mLastFrameBytes;
        }

        public void notifyCameraChanged() {
            if (mCamera != null) {
                try {
                    mCamera.setPreviewDisplay(mHolder);
                    mCamera.startPreview();
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
            } catch (IOException e) {
                Log.e(TAG, "Error setting camera preview: " + e.getMessage());
            }
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
            // empty. Take care of releasing the Camera preview in your activity.
        }

        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            if (mHolder.getSurface() == null){
                // preview surface does not exist
                return;
            }

            // stop preview before making changes
            try {
                mCamera.stopPreview();
            } catch (Exception e){
                // ignore: tried to stop a non-existent preview
            }

            // Set device-specifics here
            try {
                Camera.Parameters params = mCamera.getParameters();
                
                if (getResources().getBoolean(R.bool.config_qualcommZslCameraMode))
                    params.set("camera-mode", 1);

                mCamera.setParameters(params);
            }
            catch (Exception e) {
                Log.e(TAG, "Could not set device specifics");
            }

            // start preview with new settings
            try {
                mCamera.setPreviewDisplay(mHolder);
                mCamera.startPreview();

                mCamera.addCallbackBuffer(mLastFrameBytes);
                mCamera.setPreviewCallbackWithBuffer(this);

            } catch (Exception e){
                Log.d(TAG, "Error starting camera preview: " + e.getMessage());
            }
        }

        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            mCamera.addCallbackBuffer(mLastFrameBytes);
        }
    }
}
