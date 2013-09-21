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

package org.cyanogenmod.focal.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.widget.ImageView;

public class PanoProgressBar extends ImageView {
    @SuppressWarnings("unused")
    private static final String TAG = "PanoProgressBar";
    public static final int DIRECTION_NONE = 0;
    public static final int DIRECTION_LEFT = 1;
    public static final int DIRECTION_RIGHT = 2;
    private float mProgress = 0;
    private float mMaxProgress = 0;
    private float mLeftMostProgress = 0;
    private float mRightMostProgress = 0;
    private float mProgressOffset = 0;
    private float mIndicatorWidth = 0;
    private int mDirection = 0;
    private final Paint mBackgroundPaint = new Paint();
    private final Paint mDoneAreaPaint = new Paint();
    private final Paint mIndicatorPaint = new Paint();
    private float mWidth;
    private float mHeight;
    private RectF mDrawBounds;
    private OnDirectionChangeListener mListener = null;

    public interface OnDirectionChangeListener {
        public void onDirectionChange(int direction);
    }

    public PanoProgressBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        mDoneAreaPaint.setStyle(Paint.Style.FILL);
        mDoneAreaPaint.setAlpha(0xff);

        mBackgroundPaint.setStyle(Paint.Style.FILL);
        mBackgroundPaint.setAlpha(0xff);

        mIndicatorPaint.setStyle(Paint.Style.FILL);
        mIndicatorPaint.setAlpha(0xff);

        mDrawBounds = new RectF();
    }

    public void setOnDirectionChangeListener(OnDirectionChangeListener l) {
        mListener = l;
    }

    private void setDirection(int direction) {
        if (mDirection != direction) {
            mDirection = direction;
            if (mListener != null) {
                mListener.onDirectionChange(mDirection);
            }
            invalidate();
        }
    }

    public int getDirection() {
        return mDirection;
    }

    @Override
    public void setBackgroundColor(int color) {
        mBackgroundPaint.setColor(color);
        invalidate();
    }

    public void setDoneColor(int color) {
        mDoneAreaPaint.setColor(color);
        invalidate();
    }

    public void setIndicatorColor(int color) {
        mIndicatorPaint.setColor(color);
        invalidate();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        mWidth = w;
        mHeight = h;
        mDrawBounds.set(0, 0, mWidth, mHeight);
    }

    public void setMaxProgress(int progress) {
        mMaxProgress = progress;
    }

    public void setIndicatorWidth(float w) {
        mIndicatorWidth = w;
        invalidate();
    }

    public void setRightIncreasing(boolean rightIncreasing) {
        if (rightIncreasing) {
            mLeftMostProgress = 0;
            mRightMostProgress = 0;
            mProgressOffset = 0;
            setDirection(DIRECTION_RIGHT);
        } else {
            mLeftMostProgress = mWidth;
            mRightMostProgress = mWidth;
            mProgressOffset = mWidth;
            setDirection(DIRECTION_LEFT);
        }
        invalidate();
    }

    public void setProgress(int progress) {
        // The panning direction will be decided after user pan more
        // than 10 degrees in one direction.
        if (mDirection == DIRECTION_NONE) {
            if (progress > 10) {
                setRightIncreasing(true);
            } else if (progress < -10) {
                setRightIncreasing(false);
            }
        }
        // mDirection might be modified by setRightIncreasing() above. Need to check again.
        if (mDirection != DIRECTION_NONE) {
            mProgress = progress * mWidth / mMaxProgress + mProgressOffset;
            // Value bounds
            mProgress = Math.min(mWidth, Math.max(0, mProgress));
            if (mDirection == DIRECTION_RIGHT) {
                // The right most progress is adjusted.
                mRightMostProgress = Math.max(mRightMostProgress, mProgress);
            }
            if (mDirection == DIRECTION_LEFT) {
                // The left most progress is adjusted.
                mLeftMostProgress = Math.min(mLeftMostProgress, mProgress);
            }
            invalidate();
        }
    }

    public void reset() {
        mProgress = 0;
        mProgressOffset = 0;
        setDirection(DIRECTION_NONE);
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // the background
        canvas.drawRect(mDrawBounds, mBackgroundPaint);
        if (mDirection != DIRECTION_NONE) {
            // the progress area
            canvas.drawRect(mLeftMostProgress, mDrawBounds.top, mRightMostProgress,
                    mDrawBounds.bottom, mDoneAreaPaint);
            // the indication bar
            float l;
            float r;
            if (mDirection == DIRECTION_RIGHT) {
                l = Math.max(mProgress - mIndicatorWidth, 0f);
                r = mProgress;
            } else {
                l = mProgress;
                r = Math.min(mProgress + mIndicatorWidth, mWidth);
            }
            canvas.drawRect(l, mDrawBounds.top, r, mDrawBounds.bottom, mIndicatorPaint);
        }

        // Draw the mask image on the top for shaping.
        super.onDraw(canvas);
    }
}
