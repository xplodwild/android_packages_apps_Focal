package org.cyanogenmod.nemesis.ui;

import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.Util;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class SwitchRingPad extends View {
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

    private PadButton[] mButtons;
    private Paint mPaint;

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
        mPaint = new Paint();
        mPaint.setColor(0xFFFFFFFF);
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

        final Point screenSize = Util.getScreenSize(null);

        final int width = Math.min(screenSize.x, screenSize.y);
        final int height = Math.max(screenSize.x, screenSize.y);

        for (int i = 0; i < SLOT_MAX; i++) {
            PadButton button = mButtons[i];
            if (button == null) continue;
            
            final float radAngle = (float) ((float) (i * (180.0f/4.0f) + 90.0f) * Math.PI / 180.0f);

            final float x = (float) (height - EDGE_PADDING + RING_RADIUS * Math.cos(radAngle) - BUTTON_SIZE);
            final float y = (float) (width/2 - button.mBitmap.getWidth()/2 - RING_RADIUS * Math.sin(radAngle));

            canvas.drawBitmap(button.mBitmap, x, y, mPaint);
        }
    }

    public void addRingPad(Bitmap icon, int eventId, int slot) {
        mButtons[slot] = new PadButton();
        mButtons[slot].mBitmap = icon;
        mButtons[slot].mEventId = eventId;
        mButtons[slot].mSlot = slot;
    }
}
