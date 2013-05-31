package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.DecelerateInterpolator;
import android.widget.ImageView;

/**
 * Represents a ring that can be dragged on the screen
 */
public class HudRing extends ImageView implements View.OnTouchListener {
    private float mLastX;
    private float mLastY;

    public HudRing(Context context) {
        super(context);
    }

    public HudRing(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public HudRing(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        setOnTouchListener(this);
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        if (motionEvent.getActionMasked() == MotionEvent.ACTION_DOWN) {
            mLastX = motionEvent.getRawX();
            mLastY = motionEvent.getRawY();
           /* mOffsetX = motionEvent.getX() - motionEvent.getRawX();
            mOffsetY = motionEvent.getY() - motionEvent.getRawY();*/
            animatePressDown();
        }
        else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
            setX(getX() + (motionEvent.getRawX() - mLastX));
            setY(getY() + (motionEvent.getRawY() - mLastY));

            mLastX = motionEvent.getRawX();
            mLastY = motionEvent.getRawY();
        }
        else if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP) {
            animatePressUp();
        }

        return true;
    }

    public void animatePressDown() {
        animate().alpha(1.0f).setDuration(80).start();
    }

    public void animatePressUp() {
        animate().alpha(0.75f).rotation(0).setDuration(80).start();
    }

    public void animateWorking(long duration) {

        animate().rotationBy(45.0f).setDuration(duration).setInterpolator(new DecelerateInterpolator()).start();
    }
}
