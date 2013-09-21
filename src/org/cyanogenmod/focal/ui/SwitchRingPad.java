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

package org.cyanogenmod.focal.ui;

import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.DecelerateInterpolator;

import fr.xplod.focal.R;
import org.cyanogenmod.focal.Util;

public class SwitchRingPad extends View implements AnimatorUpdateListener {
    public interface RingPadListener {
        public void onButtonActivated(int eventId);
    }

    private float mEdgePadding;
    private int mButtonSize;
    private float mRingRadius;

    public final static int BUTTON_CAMERA = 1;
    public final static int BUTTON_VIDEO = 2;
    public final static int BUTTON_PANO = 3;
    public final static int BUTTON_PICSPHERE = 4;
    public final static int BUTTON_SWITCHCAM = 5;

    private final static int SLOT_RIGHT = 4;
    private final static int SLOT_MIDRIGHT = 3;
    private final static int SLOT_MID = 2;
    private final static int SLOT_MIDLEFT = 1;
    private final static int SLOT_LEFT = 0;
    private final static int SLOT_MAX = 5;

    private final static int RING_ANIMATION_DURATION_MS = 150;

    private PadButton[] mButtons;
    private Paint mPaint;
    private ValueAnimator mAnimator;
    private float mOpenProgress;
    private boolean mIsOpen;
    public float mTargetOrientation;
    public float mCurrentOrientation;
    private RingPadListener mListener;
    private float mHintProgress;
    private ValueAnimator mHintAnimator;

    private class PadButton {
        public Bitmap mNormalBitmap;
        public Bitmap mHoverBitmap;
        public boolean mIsHovering;
        public int mEventId;
        public float mLastDrawnX;
        public float mLastDrawnY;
        public String mHintText;
        public float mHintTextAlpha = 0.0f;
        public boolean mHintAnimationDirection = false;
    }

    public SwitchRingPad(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    public SwitchRingPad(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public SwitchRingPad(Context context) {
        super(context);
        initialize();
    }

    private Bitmap getDrawable(int resId) {
        Bitmap bmp = ((BitmapDrawable) getResources().getDrawable(resId)).getBitmap();
        mButtonSize = bmp.getWidth();
        return bmp;
    }

    public void animateHint() {
        mHintAnimator = new ValueAnimator();
        mHintAnimator.setDuration(1500);
        mHintAnimator.setFloatValues(0, 1);
        mHintAnimator.setStartDelay(500);
        mHintAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                mHintProgress = (Float) arg0.getAnimatedValue();
                invalidate();
            }
        });
        mHintAnimator.start();
    }

    private void initialize() {
        mIsOpen = false;
        mPaint = new Paint();

        animateHint();

        mAnimator = new ValueAnimator();
        mAnimator.setDuration(RING_ANIMATION_DURATION_MS);
        mAnimator.setInterpolator(new DecelerateInterpolator());
        mAnimator.addUpdateListener(this);

        mEdgePadding = getResources().getDimension(R.dimen.ringpad_edge_spacing);
        mRingRadius = getResources().getDimension(R.dimen.ringpad_radius);

        mButtons = new PadButton[SLOT_MAX];

        // Camera pad button
        Resources res = getResources();
        addRingPad(getDrawable(R.drawable.btn_ring_camera_normal),
                getDrawable(R.drawable.btn_ring_camera_hover),
                BUTTON_CAMERA, SLOT_LEFT, res.getString(R.string.mode_photo));

        // Panorama pad button
        addRingPad(getDrawable(R.drawable.btn_ring_pano_normal),
                getDrawable(R.drawable.btn_ring_pano_hover),
                BUTTON_PANO, SLOT_MIDLEFT, res.getString(R.string.mode_panorama));

        // Video pad button
        addRingPad(getDrawable(R.drawable.btn_ring_video_normal),
                getDrawable(R.drawable.btn_ring_video_hover),
                BUTTON_VIDEO, SLOT_MID, res.getString(R.string.mode_video));

        // PictureSphere pad button
        addRingPad(getDrawable(R.drawable.btn_ring_picsphere_normal),
                getDrawable(R.drawable.btn_ring_picsphere_hover),
                BUTTON_PICSPHERE, SLOT_MIDRIGHT, res.getString(R.string.mode_picsphere));

        // Switch Cam pad button
        addRingPad(getDrawable(R.drawable.btn_ring_switchcam_normal),
                getDrawable(R.drawable.btn_ring_switchcam_hover),
                BUTTON_SWITCHCAM, SLOT_RIGHT, res.getString(R.string.mode_switchcam));
    }

    public void setListener(RingPadListener listener) {
        mListener = listener;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (mOpenProgress == 0 && mHintProgress == 1)
            return;

        if (mPaint == null) {
            mPaint = new Paint();
        }

        // Get the size dimensions regardless of orientation
        final Point screenSize = Util.getScreenSize(null);

        final int width = Math.min(screenSize.x, screenSize.y);
        final int height = Math.max(screenSize.x, screenSize.y);

        // Draw the ping animation
        final float buttonOffset = Util.dpToPx(getContext(), 12);
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(4.0f);
        mPaint.setARGB((int) (255.0f - 255.0f * mHintProgress), 255, 255, 255);
        canvas.drawCircle(width / 2, height - mEdgePadding + buttonOffset, mHintProgress
                * mRingRadius, mPaint);
        canvas.drawCircle(width / 2, height - mEdgePadding + buttonOffset, mHintProgress
                * mRingRadius * 0.66f, mPaint);
        canvas.drawCircle(width / 2, height - mEdgePadding + buttonOffset, mHintProgress
                * mRingRadius * 0.33f, mPaint);

        final float ringRadius = mRingRadius * mOpenProgress;

        // Draw the inner circle (dark)
        mPaint.setStyle(Paint.Style.FILL);
        mPaint.setColor(0x88888888);
        canvas.drawCircle(width / 2, height - mEdgePadding, ringRadius, mPaint);

        // Draw the outline stroke
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(4.0f);
        mPaint.setColor(0x88DDDDDD);
        canvas.drawCircle(width / 2, height - mEdgePadding, ringRadius, mPaint);

        // Draw the actual pad buttons
        for (int i = 0; i < SLOT_MAX; i++) {
            mPaint.setAlpha((int) (255.0f * mOpenProgress));

            PadButton button = mButtons[i];
            if (button == null) continue;

            final float radAngle = (float) ((i * (180.0f / 4.0f) + 90.0f) * Math.PI / 180.0f);

            final float y = (float) (height + ringRadius * Math.cos(radAngle) - mButtonSize);
            // We remove the button edge
            final float x = (float) (width / 2 - button.mNormalBitmap.getWidth() / 2
                    - ringRadius * Math.sin(radAngle));

            canvas.save();
            canvas.translate(x + button.mNormalBitmap.getWidth() / 2,
                    y + button.mNormalBitmap.getWidth() / 2);
            canvas.rotate(mCurrentOrientation);

            if (button.mIsHovering) {
                canvas.drawBitmap(button.mHoverBitmap, -button.mNormalBitmap.getWidth() / 2,
                        -button.mNormalBitmap.getWidth() / 2, mPaint);
            } else {
                canvas.drawBitmap(button.mNormalBitmap, -button.mNormalBitmap.getWidth() / 2,
                        -button.mNormalBitmap.getWidth() / 2, mPaint);
            }
            canvas.restore();

            if (mOpenProgress == 1.0f) {
                animateAlpha(button.mIsHovering, button);
            } else {
                animateAlpha(false, button);
            }

            if ((button.mIsHovering || button.mHintTextAlpha > 0.0f) && mOpenProgress == 1.0f) {
                // Draw hint text
                int alpha = (int) (255 * (button.mHintTextAlpha));
                mPaint.setStyle(Paint.Style.FILL);

                mPaint.setTextSize(36);
                mPaint.setTextAlign(Paint.Align.CENTER);
                mPaint.setShadowLayer(12, 0, 0, 0xEE333333 * ((alpha & 0xFF) << 24));
                mPaint.setAlpha(alpha);

                float measureText = mPaint.measureText(button.mHintText);

                canvas.save();
                canvas.translate(x + button.mNormalBitmap.getWidth() / 2 - mPaint.getTextSize() / 2,
                        y - measureText / 2 - Util.dpToPx(getContext(), 4));
                canvas.rotate(mCurrentOrientation);

                canvas.drawText(button.mHintText, 0, 0, mPaint);

                canvas.restore();
            }

            button.mLastDrawnX = x;
            button.mLastDrawnY = y;
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!mIsOpen) return false;

        if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < SLOT_MAX; i++) {
                PadButton button = mButtons[i];
                if (button == null) {
                    continue;
                }

                RectF btnRect = new RectF(button.mLastDrawnX, button.mLastDrawnY,
                        button.mLastDrawnX + button.mNormalBitmap.getWidth(),
                        button.mLastDrawnY + button.mNormalBitmap.getHeight());

                if (btnRect.contains(event.getRawX(), event.getRawY())) {
                    button.mIsHovering = true;
                } else {
                    button.mIsHovering = false;
                }
            }
        } else if (event.getActionMasked() == MotionEvent.ACTION_UP) {
            animateClose();

            for (int i = 0; i < SLOT_MAX; i++) {
                PadButton button = mButtons[i];
                if (button == null) continue;

                if (button.mIsHovering && mListener != null) {
                    mListener.onButtonActivated(button.mEventId);
                }
            }
            return false;
        }

        invalidate();
        return super.onTouchEvent(event);
    }

    private void animateAlpha(boolean in, final PadButton button) {
        if (button.mHintAnimationDirection == in) {
            return;
        }

        button.mHintAnimationDirection = in;
        ValueAnimator anim = new ValueAnimator();
        anim.setDuration(200);
        anim.setFloatValues(in ? 0 : 1, in ? 1 : 0);
        anim.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                button.mHintTextAlpha = (Float) arg0.getAnimatedValue();
                invalidate();
            }
        });
        anim.setInterpolator(new AccelerateDecelerateInterpolator());
        anim.start();
    }

    public void notifyOrientationChanged(float orientation) {
        mTargetOrientation = orientation;
        ValueAnimator anim = new ValueAnimator();
        anim.setDuration(200);
        anim.setFloatValues(mCurrentOrientation, orientation);
        anim.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                mCurrentOrientation = (Float) arg0.getAnimatedValue();
                invalidate();
            }
        });
        anim.setInterpolator(new AccelerateDecelerateInterpolator());
        anim.start();
    }

    public boolean isOpen() {
        return mIsOpen;
    }

    public void animateOpen() {
        if (mIsOpen) {
            return;
        }

        mAnimator.cancel();
        mAnimator.setFloatValues(0, 1);
        mAnimator.start();

        mIsOpen = true;
    }

    public void animateClose() {
        if (!mIsOpen) {
            return;
        }

        mAnimator.cancel();
        mAnimator.setFloatValues(1, 0);
        mAnimator.start();

        mIsOpen = false;
    }

    public void addRingPad(Bitmap iconNormal, Bitmap iconHover,
            int eventId, int slot, String hint) {
        mButtons[slot] = new PadButton();
        mButtons[slot].mNormalBitmap = iconNormal;
        mButtons[slot].mHoverBitmap = iconHover;
        mButtons[slot].mEventId = eventId;
        mButtons[slot].mHintText = hint;
    }

    @Override
    public void onAnimationUpdate(ValueAnimator animator) {
        mOpenProgress = (Float) animator.getAnimatedValue();
        invalidate();
    }
}
