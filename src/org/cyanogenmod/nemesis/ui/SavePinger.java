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

package org.cyanogenmod.nemesis.ui;

import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.view.View;

import org.cyanogenmod.nemesis.R;

public class SavePinger extends View {
    public final static String TAG = "SavePinger";

    private ValueAnimator mFadeAnimator;
    private ValueAnimator mConstantAnimator;
    private float mFadeProgress;
    private Paint mPaint;
    private long mRingTime[] = new long[CIRCLES_COUNT];
    private long mLastTime;
    private Bitmap mSaveIcon;
    private int mOrientation;

    private final static int CIRCLES_COUNT = 3;
    private float mRingRadius;

    public SavePinger(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    public SavePinger(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public SavePinger(Context context) {
        super(context);
        initialize();
    }

    private void initialize() {
        mPaint = new Paint();

        // start hidden
        mFadeProgress = 0.0f;

        mSaveIcon = ((BitmapDrawable) getResources().getDrawable(R.drawable.ic_save)).getBitmap();

        mFadeAnimator = new ValueAnimator();
        mFadeAnimator.setDuration(1500);
        mFadeAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                mFadeProgress = (Float) arg0.getAnimatedValue();
                invalidate();
            }
        });

        mConstantAnimator = new ValueAnimator();
        mConstantAnimator.setDuration(1000);
        mConstantAnimator.setRepeatMode(ValueAnimator.INFINITE);
        mConstantAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                invalidate();
            }
        });
        mConstantAnimator.setFloatValues(0, 1);
        mConstantAnimator.start();

        mLastTime = System.currentTimeMillis();

        for (int i = 0; i < CIRCLES_COUNT; i++) {
            mRingTime[i] = i * -500;
        }
    }

    public void startSaving() {
        mFadeAnimator.setFloatValues(0, 1);
        mFadeAnimator.start();
    }

    public void stopSaving() {
        mFadeAnimator.setFloatValues(1, 0);
        mFadeAnimator.start();
    }

    public void notifyOrientationChanged(int angle) {
        mOrientation = angle;
    }

    @Override
    public void onDraw(Canvas canvas) {
        mRingRadius = getWidth() * 0.5f;
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(4.0f);

        long systemTime = System.currentTimeMillis();
        long deltaMs = systemTime - mLastTime;

        for (int i = 0; i < CIRCLES_COUNT; i++) {
            mRingTime[i] += deltaMs * 0.2f;

            if (mRingTime[i] < 0) continue;

            float circleValue = mRingTime[i] / 255.0f;
            float ringProgress = circleValue * mRingRadius;

            if (circleValue > 1) circleValue = 1;

            mPaint.setARGB((int) ((255.0f - 255.0f * circleValue) * mFadeProgress * 0.5f),
                    255, 255, 255);
            canvas.drawCircle(getWidth() / 2, getHeight() / 2, ringProgress, mPaint);

            if (circleValue == 1) {
                mRingTime[i] = 0;
            }
        }

        int alpha = (int) (((Math.cos((double) systemTime / 200.0) + 1.0f) / 2.0f) * 255.0f);
        canvas.save();
        canvas.translate(getWidth() / 2, getHeight() / 2);
        canvas.rotate(mOrientation);
        mPaint.setARGB((int) (alpha * mFadeProgress), 255, 255, 255);
        canvas.drawBitmap(mSaveIcon, -mSaveIcon.getWidth() / 2, -mSaveIcon.getHeight() / 2, mPaint);

        canvas.restore();

        mLastTime = systemTime;

        invalidate();
    }
}
