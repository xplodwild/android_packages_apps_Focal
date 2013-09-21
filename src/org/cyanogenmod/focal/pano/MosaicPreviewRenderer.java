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

import android.graphics.SurfaceTexture;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

public class MosaicPreviewRenderer {
    private static final String TAG = "MosaicPreviewRenderer";
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    private static final boolean DEBUG = false;

    private int mWidth; // width of the view in UI
    private int mHeight; // height of the view in UI

    private boolean mIsLandscape = true;
    private final float[] mTransformMatrix = new float[16];

    private ConditionVariable mEglThreadBlockVar = new ConditionVariable();
    private HandlerThread mEglThread;
    private EGLHandler mEglHandler;

    private EGLConfig mEglConfig;
    private EGLDisplay mEglDisplay;
    private EGLContext mEglContext;
    private EGLSurface mEglSurface;
    private SurfaceTexture mMosaicOutputSurfaceTexture;
    private SurfaceTexture mInputSurfaceTexture;
    private EGL10 mEgl;
    private GL10 mGl;

    private class EGLHandler extends Handler {
        public static final int MSG_INIT_EGL_SYNC = 0;
        public static final int MSG_SHOW_PREVIEW_FRAME_SYNC = 1;
        public static final int MSG_SHOW_PREVIEW_FRAME = 2;
        public static final int MSG_ALIGN_FRAME_SYNC = 3;
        public static final int MSG_RELEASE = 4;

        public EGLHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_INIT_EGL_SYNC:
                    doInitGL();
                    mEglThreadBlockVar.open();
                    break;
                case MSG_SHOW_PREVIEW_FRAME_SYNC:
                    doShowPreviewFrame();
                    mEglThreadBlockVar.open();
                    break;
                case MSG_SHOW_PREVIEW_FRAME:
                    doShowPreviewFrame();
                    break;
                case MSG_ALIGN_FRAME_SYNC:
                    doAlignFrame();
                    mEglThreadBlockVar.open();
                    break;
                case MSG_RELEASE:
                    doRelease();
                    break;
            }
        }

        private void doAlignFrame() {
            mInputSurfaceTexture.updateTexImage();
            mInputSurfaceTexture.getTransformMatrix(mTransformMatrix);

            MosaicRenderer.setWarping(true);
            // Call preprocess to render it to low-res and high-res RGB textures.
            MosaicRenderer.preprocess(mTransformMatrix);
            // Now, transfer the textures from GPU to CPU memory for processing
            MosaicRenderer.transferGPUtoCPU();
            MosaicRenderer.updateMatrix();
            draw();
            mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);
        }

        private void doShowPreviewFrame() {
            mInputSurfaceTexture.updateTexImage();
            mInputSurfaceTexture.getTransformMatrix(mTransformMatrix);

            MosaicRenderer.setWarping(false);
            // Call preprocess to render it to low-res and high-res RGB textures.
            MosaicRenderer.preprocess(mTransformMatrix);
            MosaicRenderer.updateMatrix();
            draw();
            mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);
        }

        private void doInitGL() {
            // These are copied from GLSurfaceView
            mEgl = (EGL10) EGLContext.getEGL();
            mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
            if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
                throw new RuntimeException("eglGetDisplay failed");
            }
            int[] version = new int[2];
            if (!mEgl.eglInitialize(mEglDisplay, version)) {
                throw new RuntimeException("eglInitialize failed");
            } else {
                Log.v(TAG, "EGL version: " + version[0] + '.' + version[1]);
            }
            int[] attribList = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
            mEglConfig = chooseConfig(mEgl, mEglDisplay);
            mEglContext = mEgl.eglCreateContext(mEglDisplay, mEglConfig, EGL10.EGL_NO_CONTEXT,
                    attribList);

            if (mEglContext == null || mEglContext == EGL10.EGL_NO_CONTEXT) {
                throw new RuntimeException("failed to createContext");
            }

            mEglSurface = mEgl.eglCreateWindowSurface(
                    mEglDisplay, mEglConfig, mMosaicOutputSurfaceTexture, null);
            if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE) {
                throw new RuntimeException("failed to createWindowSurface");
            }

            if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
                throw new RuntimeException("failed to eglMakeCurrent");
            }

            mGl = (GL10) mEglContext.getGL();

            mInputSurfaceTexture = new SurfaceTexture(MosaicRenderer.init());
            MosaicRenderer.reset(mWidth, mHeight, true);
        }

        private void doRelease() {
            mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
            mEgl.eglDestroyContext(mEglDisplay, mEglContext);
            mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE,
                    EGL10.EGL_NO_CONTEXT);
            mEgl.eglTerminate(mEglDisplay);
            mEglSurface = null;
            mEglContext = null;
            mEglDisplay = null;
            releaseSurfaceTexture(mInputSurfaceTexture);
            mEglThread.quit();
        }

        private void releaseSurfaceTexture(SurfaceTexture st) {
            st.release();
        }

        // Should be called from other thread.
        public void sendMessageSync(int msg) {
            mEglThreadBlockVar.close();
            sendEmptyMessage(msg);
            mEglThreadBlockVar.block();
        }

    }

    public MosaicPreviewRenderer(SurfaceTexture tex, int w, int h, boolean isLandscape) {
        mMosaicOutputSurfaceTexture = tex;
        mWidth = w;
        mHeight = h;
        mIsLandscape = isLandscape;

        mEglThread = new HandlerThread("PanoramaRealtimeRenderer");
        mEglThread.start();
        mEglHandler = new EGLHandler(mEglThread.getLooper());

        // We need to sync this because the generation of surface texture for input is
        // done here and the client will continue with the assumption that the
        // generation is completed.
        mEglHandler.sendMessageSync(EGLHandler.MSG_INIT_EGL_SYNC);
    }

    public void release() {
        mEglHandler.sendEmptyMessage(EGLHandler.MSG_RELEASE);
    }

    public void setLandscape(boolean landscape) {
        MosaicRenderer.setIsLandscape(landscape);
    }

    public void showPreviewFrameSync() {
        mEglHandler.sendMessageSync(EGLHandler.MSG_SHOW_PREVIEW_FRAME_SYNC);
    }

    public void showPreviewFrame() {
        mEglHandler.sendEmptyMessage(EGLHandler.MSG_SHOW_PREVIEW_FRAME);
    }

    public void alignFrameSync() {
        mEglHandler.sendMessageSync(EGLHandler.MSG_ALIGN_FRAME_SYNC);
    }

    public SurfaceTexture getInputSurfaceTexture() {
        return mInputSurfaceTexture;
    }

    private void draw() {
        MosaicRenderer.step();
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e(TAG, String.format("%s: EGL error: 0x%x", prompt, error));
        }
    }

    private static final int EGL_OPENGL_ES2_BIT = 4;
    private static final int[] CONFIG_SPEC = new int[] {
            EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL10.EGL_RED_SIZE, 8,
            EGL10.EGL_GREEN_SIZE, 8,
            EGL10.EGL_BLUE_SIZE, 8,
            EGL10.EGL_NONE
    };

    private static EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
        int[] numConfig = new int[1];
        if (!egl.eglChooseConfig(display, CONFIG_SPEC, null, 0, numConfig)) {
            throw new IllegalArgumentException("eglChooseConfig failed");
        }

        int numConfigs = numConfig[0];
        if (numConfigs <= 0) {
            throw new IllegalArgumentException("No configs match configSpec");
        }

        EGLConfig[] configs = new EGLConfig[numConfigs];
        if (!egl.eglChooseConfig(
                display, CONFIG_SPEC, configs, numConfigs, numConfig)) {
            throw new IllegalArgumentException("eglChooseConfig#2 failed");
        }

        return configs[0];
    }
}
