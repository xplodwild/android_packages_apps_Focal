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

package org.cyanogenmod.focal.feats;

import android.app.ActivityManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLSurfaceView;
import android.util.Log;

import fr.xplod.focal.R;
import org.cyanogenmod.focal.Util;

import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import static javax.microedition.khronos.egl.EGL10.EGL_ALPHA_SIZE;
import static javax.microedition.khronos.egl.EGL10.EGL_BLUE_SIZE;
import static javax.microedition.khronos.egl.EGL10.EGL_DEFAULT_DISPLAY;
import static javax.microedition.khronos.egl.EGL10.EGL_DEPTH_SIZE;
import static javax.microedition.khronos.egl.EGL10.EGL_GREEN_SIZE;
import static javax.microedition.khronos.egl.EGL10.EGL_HEIGHT;
import static javax.microedition.khronos.egl.EGL10.EGL_LARGEST_PBUFFER;
import static javax.microedition.khronos.egl.EGL10.EGL_NONE;
import static javax.microedition.khronos.egl.EGL10.EGL_NO_CONTEXT;
import static javax.microedition.khronos.egl.EGL10.EGL_RED_SIZE;
import static javax.microedition.khronos.egl.EGL10.EGL_STENCIL_SIZE;
import static javax.microedition.khronos.egl.EGL10.EGL_SUCCESS;
import static javax.microedition.khronos.egl.EGL10.EGL_WIDTH;
import static javax.microedition.khronos.opengles.GL10.GL_RGBA;
import static javax.microedition.khronos.opengles.GL10.GL_UNSIGNED_BYTE;

/**
 * Offscreen OpenGL renderer
 */
public class PixelBuffer {
    final static String TAG = "PixelBuffer";
    final static boolean LIST_CONFIGS = false;
    final static private int EGL_OPENGL_ES2_BIT = 4;
    final static private int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    final static private int EGL_BIND_TO_TEXTURE_RGB = 0x3039;
    final static private int EGL_BIND_TO_TEXTURE_RGBA = 0x303A;
    final static private int EGL_TEXTURE_FORMAT = 0x3080;
    final static private int EGL_TEXTURE_TARGET = 0x3081;
    final static private int EGL_TEXTURE_RGB = 0x305D;
    final static private int EGL_TEXTURE_RGBA = 0x305E;
    final static private int EGL_TEXTURE_2D = 0x305F;

    GLSurfaceView.Renderer mRenderer; // Borrow this interface
    int mWidth, mHeight;
    Bitmap mBitmap;

    EGL10 mEGL;
    EGLDisplay mEGLDisplay;
    EGLConfig[] mEGLConfigs;
    EGLConfig mEGLConfig;
    EGLContext mEGLContext;
    EGLSurface mEGLSurface;
    GL10 mGL;
    Context mContext;

    String mThreadOwner;

    public PixelBuffer(Context context, int width, int height) {
        mWidth = width;
        mHeight = height;
        mContext = context;

        int[] version = new int[2];
        int[] attribList = null;
        final int mMaxTextureSize =
                mContext.getResources().getInteger(R.integer.config_maxTextureSize);

        if (mWidth < mMaxTextureSize && mHeight < mMaxTextureSize) {
            // The texture is smaller than the maximum supported size, use it directly.
            attribList = new int[] {
                    EGL_WIDTH, mWidth,
                    EGL_HEIGHT, mHeight,
                    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
                    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
                    EGL_NONE
            };
        } else {
            // Use the maximum supported texture size.
            attribList = new int[] {
                    EGL_WIDTH, mMaxTextureSize,
                    EGL_HEIGHT, mMaxTextureSize,
                    EGL_LARGEST_PBUFFER, 1,
                    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
                    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
                    EGL_NONE
            };
        }

        // No error checking performed, minimum required code to elucidate logic
        mEGL = (EGL10) EGLContext.getEGL();
        mEGLDisplay = mEGL.eglGetDisplay(EGL_DEFAULT_DISPLAY);
        mEGL.eglInitialize(mEGLDisplay, version);
        mEGLConfig = chooseConfig(); // Choosing a config is a little more complicated

        // Make sure you run in OpenGL ES 2.0, as everything in Nemesis uses a shader pipeline
        mEGLContext = mEGL.eglCreateContext(mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, new int[] {
                EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE
        });
        mEGLSurface = mEGL.eglCreatePbufferSurface(mEGLDisplay, mEGLConfig,  attribList);
        mEGL.eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext);
        mGL = (GL10) mEGLContext.getGL();

        // Record thread owner of OpenGL context
        mThreadOwner = Thread.currentThread().getName();
    }

    public void setRenderer(GLSurfaceView.Renderer renderer) {
        mRenderer = renderer;

        // Does this thread own the OpenGL context?
        if (!Thread.currentThread().getName().equals(mThreadOwner)) {
            Log.e(TAG, "setRenderer: This thread does not own the OpenGL context.");
            return;
        }

        // Call the renderer initialization routines
        mRenderer.onSurfaceCreated(mGL, mEGLConfig);
        mRenderer.onSurfaceChanged(mGL, mWidth, mHeight);
    }

    public Bitmap getBitmap() {
        // Do we have a renderer?
        if (mRenderer == null) {
            Log.e(TAG, "getBitmap: Renderer was not set.");
            return null;
        }

        // Does this thread own the OpenGL context?
        if (!Thread.currentThread().getName().equals(mThreadOwner)) {
            Log.e(TAG, "getBitmap: This thread does not own the OpenGL context.");
            return null;
        }

        // Call the renderer draw routine
        mRenderer.onDrawFrame(mGL);
        convertToBitmap();
        return mBitmap;
    }

    private EGLConfig chooseConfig() {
        int[] attribList = new int[] {
                EGL_DEPTH_SIZE, 0,
                EGL_STENCIL_SIZE, 0,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_BIND_TO_TEXTURE_RGBA, 1,
                EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT,
                EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_NONE
        };

        // No error checking performed, minimum required code to elucidate logic
        // Expand on this logic to be more selective in choosing a configuration
        int[] numConfig = new int[1];
        mEGL.eglChooseConfig(mEGLDisplay, attribList, null, 0, numConfig);
        int configSize = numConfig[0];
        mEGLConfigs = new EGLConfig[configSize];
        mEGL.eglChooseConfig(mEGLDisplay, attribList, mEGLConfigs, configSize, numConfig);

        int error = mEGL.eglGetError();
        if (error != EGL_SUCCESS) {
            Log.e(TAG, "eglChooseConfig: " + error);
        }

        if (LIST_CONFIGS) {
            listConfig();
        }

        return mEGLConfigs[0];  // Best match is probably the first configuration
    }

    private void listConfig() {
        Log.i(TAG, "Config List {");

        for (EGLConfig config : mEGLConfigs) {
            int d, s, r, g, b, a;

            // Expand on this logic to dump other attributes
            d = getConfigAttrib(config, EGL_DEPTH_SIZE);
            s = getConfigAttrib(config, EGL_STENCIL_SIZE);
            r = getConfigAttrib(config, EGL_RED_SIZE);
            g = getConfigAttrib(config, EGL_GREEN_SIZE);
            b = getConfigAttrib(config, EGL_BLUE_SIZE);
            a = getConfigAttrib(config, EGL_ALPHA_SIZE);
            Log.i(TAG, "    <d,s,r,g,b,a> = <" + d + "," + s + "," +
                    r + "," + g + "," + b + "," + a + ">");
        }

        Log.i(TAG, "}");
    }

    private int getConfigAttrib(EGLConfig config, int attribute) {
        int[] value = new int[1];
        return mEGL.eglGetConfigAttrib(mEGLDisplay, config,
                attribute, value)? value[0] : 0;
    }

    private void convertToBitmap() {
        System.gc();
        Runtime.getRuntime().gc();

        final int mMaxTextureSize =
                mContext.getResources().getInteger(R.integer.config_maxTextureSize);
        boolean isScaled = (mWidth > mMaxTextureSize || mHeight > mMaxTextureSize);

        int scaledWidth = isScaled ? mMaxTextureSize : mWidth;
        int scaledHeight = isScaled ? mMaxTextureSize : mHeight;

        IntBuffer ib = IntBuffer.allocate(scaledWidth*scaledHeight);
        IntBuffer ibt = IntBuffer.allocate(scaledWidth*scaledHeight);
        mGL.glReadPixels(0, 0, scaledWidth, scaledHeight, GL_RGBA, GL_UNSIGNED_BYTE, ib);

        // Convert upside down mirror-reversed image to right-side up normal image.
        for (int i = 0; i < scaledHeight; i++) {
            for (int j = 0; j < scaledWidth; j++) {
                ibt.put((scaledHeight-i-1)*scaledWidth + j, ib.get(i*scaledWidth + j));
            }
        }

        mBitmap = Bitmap.createBitmap(scaledWidth, scaledHeight, Bitmap.Config.ARGB_8888);
        mBitmap.copyPixelsFromBuffer(ibt);

        // Release IntBuffers memory
        ibt = null;
        ib = null;

        if (isScaled) {
            ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
            ActivityManager activityManager =
                    (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
            activityManager.getMemoryInfo(mi);
            long availableMegs = mi.availMem / 1048576L;
            Log.d(TAG, "Available memory: " + availableMegs + "MB");
            while (availableMegs < 200) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                Log.w(TAG, "Waiting for more memory! (Free: " + availableMegs + "MB)");
                // We're going to need some memory
                System.gc();
                Runtime.getRuntime().gc();

                activityManager.getMemoryInfo(mi);
                availableMegs = mi.availMem / 1048576L;
            }

            // Image was converted to a power of two texture, scale it back
            Log.v(TAG, "Image was scaled, scaling back to " + mWidth + "x" + mHeight);
            Bitmap scaled = Bitmap.createScaledBitmap(mBitmap, mWidth, mHeight, true);
            mBitmap.recycle();
            mBitmap = scaled;
        }
    }
}
