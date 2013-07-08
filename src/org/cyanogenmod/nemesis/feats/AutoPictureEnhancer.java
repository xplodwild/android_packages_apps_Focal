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

package org.cyanogenmod.nemesis.feats;

import android.graphics.Bitmap;
import android.media.effect.Effect;
import android.media.effect.EffectContext;
import android.media.effect.EffectFactory;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Automatic photo enhancement
 */
public class AutoPictureEnhancer implements GLSurfaceView.Renderer {
    private int[] mTextures = new int[2];
    private EffectContext mEffectContext;
    private Effect mAutoFixEffect;
    private Effect mMinMaxEffect;
    private TextureRenderer mTexRenderer = new TextureRenderer();
    private int mImageWidth;
    private int mImageHeight;
    private Bitmap mBitmapToLoad;
    private boolean mInitialized = false;

    public AutoPictureEnhancer() {

    }

    public void setTexture(Bitmap bitmap) {
        mBitmapToLoad = bitmap;
    }

    private void loadTextureImpl(Bitmap bitmap) {
        // Generate textures
        GLES20.glGenTextures(2, mTextures, 0);

        // Load input bitmap
        mImageWidth = bitmap.getWidth();
        mImageHeight = bitmap.getHeight();
        mTexRenderer.updateTextureSize(mImageWidth, mImageHeight);

        // Upload to texture
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextures[0]);
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);

        // Set texture parameters
        GLToolbox.initTexParams();
    }

    private void initEffects() {
        EffectFactory effectFactory = mEffectContext.getFactory();
        if (mAutoFixEffect != null) {
            mAutoFixEffect.release();
        }

        mAutoFixEffect = effectFactory.createEffect(
                EffectFactory.EFFECT_AUTOFIX);
        mAutoFixEffect.setParameter("scale", 0.4f);

        mMinMaxEffect = effectFactory.createEffect(
                EffectFactory.EFFECT_BLACKWHITE);
        mMinMaxEffect.setParameter("black", .1f);
        mMinMaxEffect.setParameter("white", .8f);
    }

    private void applyEffects() {
        mMinMaxEffect.apply(mTextures[0], mImageWidth, mImageHeight, mTextures[1]);
        mAutoFixEffect.apply(mTextures[1], mImageWidth, mImageHeight, mTextures[1]);
    }

    private void renderResult() {
        mTexRenderer.renderTexture(mTextures[1]);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        if (!mInitialized) {
            //Only need to do this once
            mEffectContext = EffectContext.createWithCurrentGlContext();
            mTexRenderer.init();
            mInitialized = true;
        }

        if (mBitmapToLoad != null) {
            loadTextureImpl(mBitmapToLoad);
            mBitmapToLoad = null;
        }

        // Render the effect
        initEffects();
        applyEffects();
        renderResult();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        if (mTexRenderer != null) {
            mTexRenderer.updateViewSize(width, height);
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    }
}