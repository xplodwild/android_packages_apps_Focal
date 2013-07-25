/**
 * Copyright (C) 2009 The Android Open Source Project
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

package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;

/**
 * A layout which handles the preview aspect ratio.
 */
public class PreviewFrameLayout extends RelativeLayout {

    public static final String TAG = "CAM_preview";

    /**
     * A callback to be invoked when the preview frame's size changes.
     */
    public interface OnSizeChangedListener {
        public void onSizeChanged(int width, int height);
    }

    private double mAspectRatio;
    private int mPreviewWidth;
    private int mPreviewHeight;
    private OnSizeChangedListener mListener;


    public PreviewFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        setAspectRatio(4.0 / 3.0);
    }

    public void setAspectRatio(double ratio) {
        if (ratio <= 0.0) throw new IllegalArgumentException();

        if (getResources().getConfiguration().orientation
                == Configuration.ORIENTATION_PORTRAIT) {
            ratio = 1 / ratio;
        }

        if (mAspectRatio != ratio) {
            mAspectRatio = ratio;
            requestLayout();
        }
    }

    public void setPreviewSize(int width, int height) {
        mPreviewWidth = width;
        mPreviewHeight = height;
        requestLayout();
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        // Scale the preview while keeping the aspect ratio
        int fullWidth = MeasureSpec.getSize(widthSpec);
        int fullHeight = MeasureSpec.getSize(heightSpec);

        // Ask children to follow the new preview dimension.
        super.onMeasure(MeasureSpec.makeMeasureSpec(fullWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(fullHeight, MeasureSpec.EXACTLY));
    }

    @Override
    protected void onDraw(Canvas canvas) {
        int previewWidth = mPreviewWidth;
        int previewHeight = mPreviewHeight;

        // Scale the preview while keeping the aspect ratio
        int fullWidth = getWidth();
        int fullHeight = getHeight();

        float ratio = Math.min((float) fullWidth / (float) previewWidth, (float) fullHeight / (float) previewHeight);
        previewWidth *= ratio;
        previewHeight *= ratio;

        canvas.save();
        canvas.scale(ratio, ratio);

        super.onDraw(canvas);
        canvas.restore();
    }

    public void setOnSizeChangedListener(OnSizeChangedListener listener) {
        mListener = listener;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        if (mListener != null) mListener.onSizeChanged(w, h);
    }
}