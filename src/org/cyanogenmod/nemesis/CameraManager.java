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

import java.io.IOException;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * This class is responsible for interacting with the Camera HAL.
 * It provides easy open/close, helper methods to set parameters or 
 * toggle features, etc. in an asynchronous fashion.
 */
public class CameraManager {
    private final static String TAG = "CameraManager";
    private static CameraManager mSingleton;
    
    private CameraPreview mPreview;
    private Camera mCamera;
    private int mCurrentFacing;
    private Point mTargetSize;
    private Camera.Parameters mParameters;
    private byte[] mPreviewFrameBuffer;
    
    private class AsyncParamRunnable implements Runnable {
        private String mKey;
        private String mValue;
        
        public AsyncParamRunnable(String key, String value) {
            mKey = key;
            mValue = value;
        }
        
        public void run() {
            Log.v(TAG, "Asynchronously setting parameter " + mKey + " to " + mValue);
            mParameters.set(mKey, mValue);
            mCamera.setParameters(mParameters);
        }
    };
    
    

    public static CameraManager getSingleton(Context context) {
        if (mSingleton == null) {
            mSingleton = new CameraManager(context);
        }
        
        return mSingleton;
    }
    
    private CameraManager(Context context) {
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
            mCurrentFacing = facing;
            mParameters = null;

            if (mTargetSize != null)
                setPreviewSize(mTargetSize.x, mTargetSize.y);
            else
                Log.e(TAG, "mTargetSize is null!");
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
        int previewWidth = mParameters.getPreviewSize().width;
        int previewHeight = mParameters.getPreviewSize().height;
        
        if (mPreviewFrameBuffer == null || mPreviewFrameBuffer.length != (previewWidth*previewHeight*3))
            mPreviewFrameBuffer = new byte[previewWidth*previewHeight*3];
        
        // Convert YUV420SP preview data to RGB
        Util.decodeYUV420SP(mPreviewFrameBuffer, data, previewWidth, previewHeight);
        
        // Decode the RGB data to a bitmap
        Bitmap output = BitmapFactory.decodeByteArray(mPreviewFrameBuffer, 0, mPreviewFrameBuffer.length);
        
        return output;
    }

    /**
     * Takes a snapshot
     */
    public void takeSnapshot(Camera.ShutterCallback shutterCallback, Camera.PictureCallback raw,
                             Camera.PictureCallback jpeg) {
        Log.e(TAG, "takePicture");
        mCamera.takePicture(shutterCallback, raw, jpeg);
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

            } catch (Exception e){
                Log.d(TAG, "Error starting camera preview: " + e.getMessage());
            }
        }

        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            mLastFrameBytes = data;
        }
    }
}
