package org.cyanogenmod.nemesis.ui;

import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.Util;

import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.DecelerateInterpolator;

public class SwitchRingPad extends View implements AnimatorUpdateListener {
    private Bitmap mCameraBitmap;
    private Bitmap mVideoBitmap;
    private Bitmap mPanoBitmap;
    private Bitmap mPicSphereBitmap;
    private Bitmap mUnusedBitmap;

    private final static int EDGE_PADDING = 96; // XXX: vvvvvvvvvvvvvvvvvv
    private final static int BUTTON_SIZE = 144; // XXX: DPI INDEPENDANT!!!
    private final static int RING_RADIUS = 400; // XXX: ^^^^^^^^^^^^^^^^^^

    private final static int BUTTON_CAMERA      = 1;
    private final static int BUTTON_VIDEO       = 2;
    private final static int BUTTON_PANO        = 3;
    private final static int BUTTON_PICSPHERE   = 4;
    private final static int BUTTON_UNUSED      = 5;

    private final static int SLOT_RIGHT    = 0;
    private final static int SLOT_MIDRIGHT = 1;
    private final static int SLOT_MID      = 2;
    private final static int SLOT_MIDLEFT  = 3;
    private final static int SLOT_LEFT     = 4;
    private final static int SLOT_MAX = 5;

    private final static int RING_ANIMATION_DURATION_MS = 150;

    private PadButton[] mButtons;
    private Paint mPaint;
    private ValueAnimator mAnimator;
    private float mOpenProgress;
    private boolean mIsOpen;

    private class PadButton {
        public Bitmap mBitmap;
        public int mSlot;
        public int mEventId;
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

    private void initialize() {
        mIsOpen = false;
        mPaint = new Paint();
        
        mAnimator = new ValueAnimator();
        mAnimator.setDuration(RING_ANIMATION_DURATION_MS);
        mAnimator.setInterpolator(new DecelerateInterpolator());
        mAnimator.addUpdateListener(this);
        
        mButtons = new PadButton[SLOT_MAX];

        // Camera pad button
        mCameraBitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.btn_ring_camera_normal)).getBitmap();
        addRingPad(mCameraBitmap, BUTTON_CAMERA, SLOT_LEFT);

        // Panorama pad button
        mPanoBitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.btn_ring_pano_normal)).getBitmap();
        addRingPad(mPanoBitmap, BUTTON_PANO, SLOT_MIDLEFT);

        // Video pad button
        mVideoBitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.btn_ring_video_normal)).getBitmap();
        addRingPad(mVideoBitmap, BUTTON_VIDEO, SLOT_MID);

        // PictureSphere pad button
        mPicSphereBitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.btn_ring_picsphere_normal)).getBitmap();
        addRingPad(mPicSphereBitmap, BUTTON_PICSPHERE, SLOT_MIDRIGHT);

        // Switch Cam pad button
        mUnusedBitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.btn_ring_switchcam_normal)).getBitmap();
        addRingPad(mUnusedBitmap, BUTTON_UNUSED, SLOT_RIGHT);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (mPaint == null) {
            mPaint = new Paint();
        }

        final float ringRadius = (float) RING_RADIUS * mOpenProgress;

        // Get the size dimensions regardless of orientation
        final Point screenSize = Util.getScreenSize(null);

        final int width = Math.min(screenSize.x, screenSize.y);
        final int height = Math.max(screenSize.x, screenSize.y);

        // Draw the inner circle (dark)
        mPaint.setStyle(Paint.Style.FILL);
        mPaint.setColor(0x88888888);
        canvas.drawCircle(height - EDGE_PADDING, width/2, ringRadius, mPaint);

        // Draw the outline stroke
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeWidth(4.0f);
        mPaint.setColor(0x88DDDDDD);
        canvas.drawCircle(height - EDGE_PADDING, width/2, ringRadius, mPaint);

        // Draw the actual pad buttons
        for (int i = 0; i < SLOT_MAX; i++) {
            PadButton button = mButtons[i];
            if (button == null) continue;

            final float radAngle = (float) ((float) (i * (180.0f/4.0f) + 90.0f) * Math.PI / 180.0f);

            final float x = (float) (height - EDGE_PADDING + ringRadius * Math.cos(radAngle) - BUTTON_SIZE);
            final float y = (float) (width/2 - button.mBitmap.getWidth()/2 - ringRadius * Math.sin(radAngle));

            canvas.drawBitmap(button.mBitmap, x, y, mPaint);
        }
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

    public void addRingPad(Bitmap icon, int eventId, int slot) {
        mButtons[slot] = new PadButton();
        mButtons[slot].mBitmap = icon;
        mButtons[slot].mEventId = eventId;
        mButtons[slot].mSlot = slot;
    }

    @Override
    public void onAnimationUpdate(ValueAnimator animator) {
        mOpenProgress = animator.getAnimatedFraction();
        invalidate();
    }
}
