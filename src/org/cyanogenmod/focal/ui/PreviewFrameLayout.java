/*
 * Copyright (C) 2009 The Android Open Source Project
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

package org.cyanogenmod.focal.ui;

import android.content.Context;
import android.content.res.Configuration;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.RelativeLayout;

import org.cyanogenmod.focal.Util;

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
        if (ratio <= 0.0) {
            throw new IllegalArgumentException();
        }

        ratio = 1 / ratio;

        if (mAspectRatio != ratio) {
            mAspectRatio = ratio;
            requestLayout();
        }
    }

    public void setPreviewSize(int width, int height) {
        mPreviewWidth = height;
        mPreviewHeight = width;
        requestLayout();
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        // Scale the preview while keeping the aspect ratio
        int fullWidth = Util.getScreenSize(null).x;
        int fullHeight = Util.getScreenSize(null).y;

        if (fullWidth == 0 || mPreviewWidth == 0) {
            setMeasuredDimension(mPreviewWidth, mPreviewHeight);
            return;
        }

        setMeasuredDimension(fullWidth, fullHeight);

        // Ask children to follow the new preview dimension.
        super.onMeasure(MeasureSpec.makeMeasureSpec((int) fullWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec((int) fullHeight, MeasureSpec.EXACTLY));
    }

    /**
     * Called when this view should assign a size and position to all of its children.
     * @param changed
     * @param l
     * @param t
     * @param r
     * @param b
     */
    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        if(changed && getChildCount() > 0) {
            final View child = getChildAt(0);

            // Scale the preview while keeping the aspect ratio
            int fullWidth = Util.getScreenSize(null).x;
            int fullHeight = Util.getScreenSize(null).y;

            if (fullWidth == 0 || mPreviewWidth == 0) {
                setMeasuredDimension(fullWidth, fullHeight);
                return;
            }

            float ratio = Math.min((float) fullHeight / (float) mPreviewHeight,
                    (float) fullWidth / (float) mPreviewWidth);
            float width = mPreviewWidth * ratio;
            float height = mPreviewHeight * ratio;

            // Center the child SurfaceView within the parent.
            if (child != null) {
                Log.v(TAG, "Layout: (" + (int) ((fullWidth - width)) / 2 + ", "
                        + (int) ((fullHeight - height)) / 2 + ", " + (int) ((fullWidth
                        + width)) / 2 + ", " + (int) ((fullHeight + height)) / 2 + ")");
                child.layout((int) ((fullWidth - width)) / 2, (int) ((fullHeight - height))
                        / 2, (int) ((fullWidth + width)) / 2, (int) ((fullHeight + height)) / 2);
            }
        }
    }

    public void setOnSizeChangedListener(OnSizeChangedListener listener) {
        mListener = listener;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        if (mListener != null) mListener.onSizeChanged(w, h);
    }
}
