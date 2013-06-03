package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

/**
 * Exposure ring HUD that lets user select exposure metering point
 */
public class ExposureHudRing extends HudRing {
    private CameraManager mCamManager;

    public ExposureHudRing(Context context) {
        super(context);
    }

    public ExposureHudRing(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ExposureHudRing(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        setImageResource(R.drawable.hud_exposure_ring);
    }

    public void setManagers(CameraManager camMan) {
        mCamManager = camMan;
    }
    
    /**
     * Centers the exposure ring on the x,y coordinates provided
     * and sets the focus to this position
     * @param x
     * @param y
     */
    public void setPosition(float x, float y) {
        setX(x - getWidth()/2.0f);
        setY(y - getHeight()/2.0f);
        applyExposurePoint();
    }
    
    
    private void applyExposurePoint() {
        float centerPointX = getX() + getWidth()/2.0f;
        float centerPointY = getY() + getHeight()/2.0f;

        centerPointX *= 1000.0f / ((ViewGroup) getParent()).getWidth();
        centerPointY *= 1000.0f / ((ViewGroup) getParent()).getHeight();

        centerPointX = (centerPointX - 500.0f) * 2.0f;
        centerPointY = (centerPointY - 500.0f) * 2.0f;

        mCamManager.setExposurePoint((int) centerPointX, (int) centerPointY);
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        super.onTouch(view, motionEvent);

        if (motionEvent.getActionMasked() == MotionEvent.ACTION_MOVE) {
            applyExposurePoint();
        }

        return true;
    }
}
