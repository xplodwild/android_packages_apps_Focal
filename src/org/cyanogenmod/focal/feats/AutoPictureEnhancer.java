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

import android.content.Context;
import android.graphics.Bitmap;
import android.media.effect.Effect;
import android.media.effect.EffectContext;
import android.media.effect.EffectFactory;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.util.Log;

import fr.xplod.focal.R;
import org.cyanogenmod.focal.Util;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Automatic photo enhancement
 */
public class AutoPictureEnhancer implements GLSurfaceView.Renderer {
    public final static String TAG = "AutoPictureEnhancer";

    private int[] mTextures = new int[2];
    private EffectContext mEffectContext;
    private Effect mAutoFixEffect;
    private Effect mMinMaxEffect;
    private TextureRenderer mTexRenderer = new TextureRenderer();
    private int mImageWidth;
    private int mImageHeight;
    private Bitmap mBitmapToLoad;
    private boolean mInitialized = false;
    private Context mContext;

    public AutoPictureEnhancer(Context context) {
        mContext = context;
    }

    public void setTexture(Bitmap bitmap) {
        mBitmapToLoad = bitmap;
    }

    public static Bitmap scale(Bitmap bmp, int w, int h) {
        Bitmap b = null;
        if (bmp != null) {
            Log.v(TAG, "Scaling bitmap from " + bmp.getWidth() + "x"
                    + bmp.getHeight() + " to " + w + "x" + h);

            // We're going to need some memory
            System.gc();
            Runtime.getRuntime().gc();

            // Scale it!
            b=Bitmap.createScaledBitmap(bmp, w, h, true);

            // Release original bitmap
            bmp.recycle();
        }
        return b;
    }

    private void loadTextureImpl(Bitmap bitmap) {
        // Generate textures
        GLES20.glGenTextures(1, mTextures, 0);

        final int mMaxTextureSize =
                mContext.getResources().getInteger(R.integer.config_maxTextureSize);

        // Load input bitmap
        if (bitmap.getWidth() > mMaxTextureSize || bitmap.getHeight() > mMaxTextureSize) {
            mImageWidth = mMaxTextureSize;
            mImageHeight = mMaxTextureSize;
        } else {
            mImageWidth = bitmap.getWidth();
            mImageHeight = bitmap.getHeight();
        }

        mTexRenderer.updateTextureSize(mImageWidth, mImageHeight);

        // Upload to texture
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextures[0]);
        if (mImageWidth != bitmap.getWidth() || mImageHeight != bitmap.getHeight()) {
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, scale(bitmap,
                    mImageWidth, mImageHeight), 0);
        } else {
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
        }

        bitmap.recycle();
        System.gc();
        Runtime.getRuntime().gc();

        // Set texture parameters
        GLToolbox.initTexParams();
    }

    private void initEffects() {
        EffectFactory effectFactory = mEffectContext.getFactory();
        if (mAutoFixEffect != null) {
            mAutoFixEffect.release();
        }

        mAutoFixEffect = effectFactory.createEffect( EffectFactory.EFFECT_AUTOFIX);
        mAutoFixEffect.setParameter("scale", 0.4f);

        mMinMaxEffect = effectFactory.createEffect( EffectFactory.EFFECT_BLACKWHITE);
        mMinMaxEffect.setParameter("black", .1f);
        mMinMaxEffect.setParameter("white", .8f);
    }

    private void applyEffects() {
        mMinMaxEffect.apply(mTextures[0], mImageWidth, mImageHeight, mTextures[0]);
        mAutoFixEffect.apply(mTextures[0], mImageWidth, mImageHeight, mTextures[0]);
    }

    private void renderResult() {
        mTexRenderer.renderTexture(mTextures[0]);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        if (!mInitialized) {
            // Only need to do this once
            mEffectContext = EffectContext.createWithCurrentGlContext();
            mTexRenderer.init();
            mInitialized = true;
        }

        if (mBitmapToLoad != null) {
            loadTextureImpl(mBitmapToLoad);
            mBitmapToLoad = null;
        } else {
            Log.e(TAG, "Bitmap to load is null");
        }

        // Render the effect
        initEffects();
        applyEffects();
        renderResult();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        final int mMaxTextureSize =
                mContext.getResources().getInteger(R.integer.config_maxTextureSize);
        if (mTexRenderer != null) {
            if (width > mMaxTextureSize || height > mMaxTextureSize) {
                mTexRenderer.updateViewSize(mMaxTextureSize, mMaxTextureSize);
            } else {
                mTexRenderer.updateViewSize(width, height);
            }
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    }
}
