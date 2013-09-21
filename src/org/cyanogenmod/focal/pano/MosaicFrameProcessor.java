/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright (C) 2013 Guillaume Lesniak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.focal.pano;

import android.util.Log;

/**
 * Class to handle the processing of each frame by Mosaicer.
 */
public class MosaicFrameProcessor {
    private static final String TAG = "MosaicFrameProcessor";
    private static final int NUM_FRAMES_IN_BUFFER = 2;
    private static final int MAX_NUMBER_OF_FRAMES = 200;
    private static final int MOSAIC_RET_CODE_INDEX = 10;
    private static final int FRAME_COUNT_INDEX = 9;
    private static final int X_COORD_INDEX = 2;
    private static final int Y_COORD_INDEX = 5;
    private static final int HR_TO_LR_DOWNSAMPLE_FACTOR = 4;
    private static final int WINDOW_SIZE = 3;

    private Mosaic mMosaicer;
    private boolean mIsMosaicMemoryAllocated = false;
    private float mTranslationLastX;
    private float mTranslationLastY;

    private int mFillIn = 0;
    private int mTotalFrameCount = 0;
    private int mLastProcessFrameIdx = -1;
    private int mCurrProcessFrameIdx = -1;
    private boolean mFirstRun;

    // Panning rate is in unit of percentage of image content translation per
    // frame. Use moving average to calculate the panning rate.
    private float mPanningRateX;
    private float mPanningRateY;

    private float[] mDeltaX = new float[WINDOW_SIZE];
    private float[] mDeltaY = new float[WINDOW_SIZE];
    private int mOldestIdx = 0;
    private float mTotalTranslationX = 0f;
    private float mTotalTranslationY = 0f;

    private ProgressListener mProgressListener;

    private int mPreviewWidth;
    private int mPreviewHeight;
    private int mPreviewBufferSize;

    private static MosaicFrameProcessor sMosaicFrameProcessor; // singleton

    public interface ProgressListener {
        public void onProgress(boolean isFinished, float panningRateX, float panningRateY,
                float progressX, float progressY);
    }

    public static MosaicFrameProcessor getInstance() {
        if (sMosaicFrameProcessor == null) {
            sMosaicFrameProcessor = new MosaicFrameProcessor();
        }
        return sMosaicFrameProcessor;
    }

    private MosaicFrameProcessor() {
        mMosaicer = new Mosaic();
    }

    public void setProgressListener(ProgressListener listener) {
        mProgressListener = listener;
    }

    public int reportProgress(boolean hires, boolean cancel) {
        return mMosaicer.reportProgress(hires, cancel);
    }

    public void initialize(int previewWidth, int previewHeight, int bufSize) {
        mPreviewWidth = previewWidth;
        mPreviewHeight = previewHeight;
        mPreviewBufferSize = bufSize;
        setupMosaicer(mPreviewWidth, mPreviewHeight, mPreviewBufferSize);
        setStripType(Mosaic.STRIPTYPE_WIDE);
        reset();
    }

    public void clear() {
        if (mIsMosaicMemoryAllocated) {
            mMosaicer.freeMosaicMemory();
            mIsMosaicMemoryAllocated = false;
        }
        synchronized (this) {
            notify();
        }
    }

    public boolean isMosaicMemoryAllocated() {
        return mIsMosaicMemoryAllocated;
    }

    public void setStripType(int type) {
        mMosaicer.setStripType(type);
    }

    private void setupMosaicer(int previewWidth, int previewHeight, int bufSize) {
        Log.v(TAG, "setupMosaicer w, h=" + previewWidth + ',' + previewHeight + ',' + bufSize);

        if (mIsMosaicMemoryAllocated) {
            throw new RuntimeException("MosaicFrameProcessor in use!");
        }
        mIsMosaicMemoryAllocated = true;
        mMosaicer.allocateMosaicMemory(previewWidth, previewHeight);
    }

    public void reset() {
        // reset() can be called even if MosaicFrameProcessor is not initialized.
        // Only counters will be changed.
        mFirstRun = true;
        mTotalFrameCount = 0;
        mFillIn = 0;
        mTotalTranslationX = 0;
        mTranslationLastX = 0;
        mTotalTranslationY = 0;
        mTranslationLastY = 0;
        mPanningRateX = 0;
        mPanningRateY = 0;
        mLastProcessFrameIdx = -1;
        mCurrProcessFrameIdx = -1;
        for (int i = 0; i < WINDOW_SIZE; ++i) {
            mDeltaX[i] = 0f;
            mDeltaY[i] = 0f;
        }
        mMosaicer.reset();
    }

    public int createMosaic(boolean highRes) {
        return mMosaicer.createMosaic(highRes);
    }

    public byte[] getFinalMosaicNV21() {
        return mMosaicer.getFinalMosaicNV21();
    }

    // Processes the last filled image frame through the mosaicer and
    // updates the UI to show progress.
    // When done, processes and displays the final mosaic.
    public void processFrame() {
        if (!mIsMosaicMemoryAllocated) {
            // clear() is called and buffers are cleared, stop computation.
            // This can happen when the onPause() is called in the activity, but still some frames
            // are not processed yet and thus the callback may be invoked.
            return;
        }

        mCurrProcessFrameIdx = mFillIn;
        mFillIn = ((mFillIn + 1) % NUM_FRAMES_IN_BUFFER);

        // Check that we are trying to process a frame different from the
        // last one processed (useful if this class was running asynchronously)
        if (mCurrProcessFrameIdx != mLastProcessFrameIdx) {
            mLastProcessFrameIdx = mCurrProcessFrameIdx;

            // TODO: make the termination condition regarding reaching
            // MAX_NUMBER_OF_FRAMES solely determined in the library.
            if (mTotalFrameCount < MAX_NUMBER_OF_FRAMES) {
                // If we are still collecting new frames for the current mosaic,
                // process the new frame.
                calculateTranslationRate();

                // Publish progress of the ongoing processing
                if (mProgressListener != null) {
                    mProgressListener.onProgress(false, mPanningRateX, mPanningRateY,
                            mTranslationLastX * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewWidth,
                            mTranslationLastY * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewHeight);
                }
            } else {
                if (mProgressListener != null) {
                    mProgressListener.onProgress(true, mPanningRateX, mPanningRateY,
                            mTranslationLastX * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewWidth,
                            mTranslationLastY * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewHeight);
                }
            }
        }
    }

    public void calculateTranslationRate() {
        float[] frameData = mMosaicer.setSourceImageFromGPU();
        int ret_code = (int) frameData[MOSAIC_RET_CODE_INDEX];
        mTotalFrameCount  = (int) frameData[FRAME_COUNT_INDEX];
        float translationCurrX = frameData[X_COORD_INDEX];
        float translationCurrY = frameData[Y_COORD_INDEX];

        if (mFirstRun) {
            // First time: no need to update delta values.
            mTranslationLastX = translationCurrX;
            mTranslationLastY = translationCurrY;
            mFirstRun = false;
            return;
        }

        // Moving average: remove the oldest translation/deltaTime and
        // add the newest translation/deltaTime in
        int idx = mOldestIdx;
        mTotalTranslationX -= mDeltaX[idx];
        mTotalTranslationY -= mDeltaY[idx];
        mDeltaX[idx] = Math.abs(translationCurrX - mTranslationLastX);
        mDeltaY[idx] = Math.abs(translationCurrY - mTranslationLastY);
        mTotalTranslationX += mDeltaX[idx];
        mTotalTranslationY += mDeltaY[idx];

        // The panning rate is measured as the rate of the translation percentage in
        // image width/height. Take the horizontal panning rate for example, the image width
        // used in finding the translation is (PreviewWidth / HR_TO_LR_DOWNSAMPLE_FACTOR).
        // To get the horizontal translation percentage, the horizontal translation,
        // (translationCurrX - mTranslationLastX), is divided by the
        // image width. We then get the rate by dividing the translation percentage with the
        // number of frames.
        mPanningRateX = mTotalTranslationX /
                (mPreviewWidth / HR_TO_LR_DOWNSAMPLE_FACTOR) / WINDOW_SIZE;
        mPanningRateY = mTotalTranslationY /
                (mPreviewHeight / HR_TO_LR_DOWNSAMPLE_FACTOR) / WINDOW_SIZE;

        mTranslationLastX = translationCurrX;
        mTranslationLastY = translationCurrY;
        mOldestIdx = (mOldestIdx + 1) % WINDOW_SIZE;
    }
}
