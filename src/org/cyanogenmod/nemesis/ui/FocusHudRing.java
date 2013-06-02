package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.FocusManager;
import org.cyanogenmod.nemesis.R;

/**
 * Focus ring HUD that lets user select focus point (tap to focus)
 */
public class FocusHudRing extends HudRing {
    private CameraManager mCamManager;
    private FocusManager mFocusManager;

    public FocusHudRing(Context context) {
        super(context);
    }

    public FocusHudRing(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public FocusHudRing(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        setFocusImage(true);
    }

    public void setFocusImage(boolean success) {
        if (success)
            setImageResource(R.drawable.hud_focus_ring_success);
        else
            setImageResource(R.drawable.hud_focus_ring_fail);
    }

    public void setManagers(CameraManager camMan, FocusManager focusMan) {
        mCamManager = camMan;
        mFocusManager = focusMan;
    }
    
    /**
     * Centers the focus ring on the x,y coordinates provided
     * and sets the focus to this position
     * @param x
     * @param y
     */
    public void setPosition(float x, float y) {
        setX(x - getWidth()/2.0f);
        setY(y - getHeight()/2.0f);
        applyFocusPoint();
    }
    
    
    private void applyFocusPoint() {
        float centerPointX = getX() + getWidth()/2.0f;
        float centerPointY = getY() + getHeight()/2.0f;

        centerPointX *= 1000.0f / ((ViewGroup) getParent()).getWidth();
        centerPointY *= 1000.0f / ((ViewGroup) getParent()).getHeight();

        centerPointX = (centerPointX - 500.0f) * 2.0f;
        centerPointY = (centerPointY - 500.0f) * 2.0f;

        mCamManager.setFocusPoint((int) centerPointX, (int) centerPointY);
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        super.onTouch(view, motionEvent);

        if (motionEvent.getActionMasked() == MotionEvent.ACTION_MOVE) {
            applyFocusPoint();
        }
        else if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP) {
            mFocusManager.refocus();
        }

        return true;
    }
}
