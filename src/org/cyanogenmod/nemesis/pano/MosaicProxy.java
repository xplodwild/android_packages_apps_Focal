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

package org.cyanogenmod.nemesis.pano;

import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.os.Message;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import org.cyanogenmod.nemesis.CameraActivity;
import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.feats.CaptureTransformer;
import org.cyanogenmod.nemesis.ui.ShutterButton;

/**
 * Nemesis interface to interact with Google's mosaic interface
 * Strongly inspired from AOSP's PanoramaModule
 */
public class MosaicProxy extends CaptureTransformer
        implements SurfaceTexture.OnFrameAvailableListener {
    private static final String TAG = "CAM PanoModule";

    public static final int DEFAULT_SWEEP_ANGLE = 160;
    public static final int DEFAULT_BLEND_MODE = Mosaic.BLENDTYPE_HORIZONTAL;
    public static final int DEFAULT_CAPTURE_PIXELS = 960 * 720;

    private static final int MSG_LOW_RES_FINAL_MOSAIC_READY = 1;
    private static final int MSG_GENERATE_FINAL_MOSAIC_ERROR = 2;
    private static final int MSG_RESET_TO_PREVIEW = 3;
    private static final int MSG_CLEAR_SCREEN_DELAY = 4;

    private static final int PREVIEW_STOPPED = 0;
    private static final int PREVIEW_ACTIVE = 1;
    private static final int CAPTURE_STATE_VIEWFINDER = 0;
    private static final int CAPTURE_STATE_MOSAIC = 1;
    private final String mPreparePreviewString;
    private final String mDialogTitle;
    private final String mDialogOkString;
    private final String mDialogPanoramaFailedString;
    private final String mDialogWaitingPreviousString;

    private Runnable mOnFrameAvailableRunnable;
    private MosaicFrameProcessor mMosaicFrameProcessor;
    private MosaicPreviewRenderer mMosaicPreviewRenderer;
    private boolean mMosaicFrameProcessorInitialized;
    private float mHorizontalViewAngle;
    private float mVerticalViewAngle;

    private int mCaptureState;
    private ViewGroup mGLRootView;
    private TextureView mGLSurfaceView;
    private CameraActivity mActivity;
    private Handler mMainHandler;
    private SurfaceTexture mCameraTexture;
    private MosaicSurfaceTexture mMosaicTexture;

    public MosaicProxy(CameraActivity activity) {
        super(activity.getCamManager(), activity.getSnapManager());
        mActivity = activity;

        mGLRootView = (ViewGroup) mActivity.findViewById(R.id.gl_renderer_container);
        mGLSurfaceView = new TextureView(mActivity);
        mGLRootView.addView(mGLSurfaceView);

        mMosaicFrameProcessor = MosaicFrameProcessor.getInstance();
        Resources appRes = mActivity.getResources();
        mPreparePreviewString = appRes.getString(R.string.pano_dialog_prepare_preview);
        mDialogTitle = appRes.getString(R.string.pano_dialog_title);
        mDialogOkString = appRes.getString(R.string.OK);
        mDialogPanoramaFailedString = appRes.getString(R.string.pano_dialog_panorama_failed);
        mDialogWaitingPreviousString = appRes.getString(R.string.pano_dialog_waiting_previous);

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
                        /*onBackgroundThreadFinished();
                        showFinalMosaic((Bitmap) msg.obj);
                        saveHighResMosaic();*/
                        break;
                    case MSG_GENERATE_FINAL_MOSAIC_ERROR:
                        /*onBackgroundThreadFinished();
                        if (mPaused) {
                            resetToPreview();
                        } else {
                            mRotateDialog.showAlertDialog(
                                    mDialogTitle, mDialogPanoramaFailedString,
                                    mDialogOkString, new Runnable() {
                                @Override
                                public void run() {
                                    resetToPreview();
                                }},
                                    null, null);
                        }
                        clearMosaicFrameProcessorIfNeeded();*/
                        break;
                    case MSG_RESET_TO_PREVIEW:
                        /*onBackgroundThreadFinished();
                        resetToPreview();
                        clearMosaicFrameProcessorIfNeeded();*/
                        break;
                    case MSG_CLEAR_SCREEN_DELAY:
                        mActivity.getWindow().clearFlags(WindowManager.LayoutParams.
                                FLAG_KEEP_SCREEN_ON);
                        break;
                }
            }
        };

        Camera.Parameters params = mActivity.getCamManager().getParameters();
        mHorizontalViewAngle = params.getHorizontalViewAngle();
        mVerticalViewAngle = params.getVerticalViewAngle();

        int previewWidth = params.getPreviewSize().width;
        int previewHeight = params.getPreviewSize().height;

        PixelFormat pixelInfo = new PixelFormat();
        PixelFormat.getPixelFormatInfo(params.getPreviewFormat(), pixelInfo);
        // TODO: remove this extra 32 byte after the driver bug is fixed.
        int previewBufSize = (previewWidth * previewHeight * pixelInfo.bitsPerPixel / 8) + 32;

        mMosaicFrameProcessor.initialize(previewWidth, previewHeight, previewBufSize);
        mMosaicFrameProcessorInitialized = true;
        configMosaicPreview(previewWidth, previewHeight);
    }

    /**
     * Call this when you're done using MosaicProxy, to remove views added and shutdown
     * threads.
     */
    public void tearDown() {
        mGLRootView.removeView(mGLSurfaceView);
    }

    private void configMosaicPreview(int w, int h) {
        // TODO: Grab the actual REAL landscape mode from orientation sensors
        boolean isLandscape = (mActivity.getResources().getConfiguration().orientation
                == Configuration.ORIENTATION_LANDSCAPE);

        mMosaicTexture = (MosaicSurfaceTexture) mCamManager.getPreviewSurface().getSurfaceTexture();
        mMosaicTexture.initPreviewRenderer(w, h, isLandscape);
        mMosaicPreviewRenderer = mMosaicTexture.getPreviewRenderer();

        mCameraTexture = mMosaicPreviewRenderer.getInputSurfaceTexture();
        mGLSurfaceView.setSurfaceTexture(mMosaicTexture);
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
}
