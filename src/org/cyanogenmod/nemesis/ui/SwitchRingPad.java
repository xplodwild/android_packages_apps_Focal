package org.cyanogenmod.nemesis.ui;

import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.DecelerateInterpolator;

import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.Util;

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

    private final static int SLOT_RIGHT = 0;
    private final static int SLOT_MIDRIGHT = 1;
    private final static int SLOT_MID = 2;
    private final static int SLOT_MIDLEFT = 3;
    private final static int SLOT_LEFT = 4;
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
        addRingPad(getDrawable(R.drawable.btn_ring_camera_normal),
                getDrawable(R.drawable.btn_ring_camera_hover),
                BUTTON_CAMERA, SLOT_LEFT);

        // Panorama pad button
        addRingPad(getDrawable(R.drawable.btn_ring_pano_normal),
                getDrawable(R.drawable.btn_ring_pano_hover),
                BUTTON_PANO, SLOT_MIDLEFT);

        // Video pad button
        addRingPad(getDrawable(R.drawable.btn_ring_video_normal),
                getDrawable(R.drawable.btn_ring_video_hover),
                BUTTON_VIDEO, SLOT_MID);

        // PictureSphere pad button
        addRingPad(getDrawable(R.drawable.btn_ring_picsphere_normal),
                getDrawable(R.drawable.btn_ring_picsphere_hover),
                BUTTON_PICSPHERE, SLOT_MIDRIGHT);

        // Switch Cam pad button
        addRingPad(getDrawable(R.drawable.btn_ring_switchcam_normal),
                getDrawable(R.drawable.btn_ring_switchcam_hover),
                BUTTON_SWITCHCAM, SLOT_RIGHT);
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


        final float buttonOffset = Util.dpToPx(getContext(), 12);
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(4.0f);
        mPaint.setARGB((int) (255.0f - 255.0f * mHintProgress), 255, 255, 255);
        canvas.drawCircle(height - mEdgePadding + buttonOffset, width / 2, mHintProgress * mRingRadius, mPaint);
        canvas.drawCircle(height - mEdgePadding + buttonOffset, width / 2, mHintProgress * mRingRadius * 0.66f, mPaint);
        canvas.drawCircle(height - mEdgePadding + buttonOffset, width / 2, mHintProgress * mRingRadius * 0.33f, mPaint);

        final float ringRadius = (float) mRingRadius * mOpenProgress;

        // Draw the inner circle (dark)
        mPaint.setStyle(Paint.Style.FILL);
        mPaint.setColor(0x88888888);
        canvas.drawCircle(height - mEdgePadding, width / 2, ringRadius, mPaint);

        // Draw the outline stroke
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(4.0f);
        mPaint.setColor(0x88DDDDDD);
        canvas.drawCircle(height - mEdgePadding, width / 2, ringRadius, mPaint);

        mPaint.setAlpha((int) (255.0f * mOpenProgress));
        // Draw the actual pad buttons
        for (int i = 0; i < SLOT_MAX; i++) {
            PadButton button = mButtons[i];
            if (button == null) continue;

            final float radAngle = (float) ((float) (i * (180.0f / 4.0f) + 90.0f) * Math.PI / 180.0f);

            final float x = (float) (height + ringRadius * Math.cos(radAngle) - mButtonSize);
            // We remove the button edge
            final float y = (float) (width / 2 - button.mNormalBitmap.getWidth() / 2 - ringRadius * Math.sin(radAngle));

            canvas.save();
            canvas.translate(x + button.mNormalBitmap.getWidth() / 2, y + button.mNormalBitmap.getWidth() / 2);
            canvas.rotate(mCurrentOrientation);

            if (button.mIsHovering)
                canvas.drawBitmap(button.mHoverBitmap, -button.mNormalBitmap.getWidth() / 2, -button.mNormalBitmap.getWidth() / 2, mPaint);
            else
                canvas.drawBitmap(button.mNormalBitmap, -button.mNormalBitmap.getWidth() / 2, -button.mNormalBitmap.getWidth() / 2, mPaint);

            canvas.restore();

            button.mLastDrawnX = x;
            button.mLastDrawnY = y;
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < SLOT_MAX; i++) {
                PadButton button = mButtons[i];
                if (button == null) continue;

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

    public void notifyOrientationChanged(float orientation) {
        mTargetOrientation = orientation;
        ValueAnimator anim = new ValueAnimator();
        anim.setDuration(200);
        anim.setFloatValues(mCurrentOrientation, orientation);
        anim.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                mCurrentOrientation = (Float) arg0.getAnimatedValue();
            }
        });
        anim.setInterpolator(new AccelerateDecelerateInterpolator());
        anim.start();
    }

    public boolean isOpen() {
        return mIsOpen;
    }

    public void animateOpen() {
        if (mIsOpen) return;

        mAnimator.cancel();
        mAnimator.setFloatValues(0, 1);
        mAnimator.start();

        mIsOpen = true;
    }

    public void animateClose() {
        if (!mIsOpen) return;

        mAnimator.cancel();
        mAnimator.setFloatValues(1, 0);
        mAnimator.start();

        mIsOpen = false;
    }

    public void addRingPad(Bitmap iconNormal, Bitmap iconHover, int eventId, int slot) {
        mButtons[slot] = new PadButton();
        mButtons[slot].mNormalBitmap = iconNormal;
        mButtons[slot].mHoverBitmap = iconHover;
        mButtons[slot].mEventId = eventId;
    }

    @Override
    public void onAnimationUpdate(ValueAnimator animator) {
        mOpenProgress = (Float) animator.getAnimatedValue();
        invalidate();
    }
}
