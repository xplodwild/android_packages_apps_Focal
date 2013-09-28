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

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;

/**
 * Exposure ring HUD that lets user select exposure metering point
 */
public class ExposureHudRing extends HudRing {
    private CameraManager mCamManager;
    private long mTimeLastSet = 0;
    private final static long SET_INTERVAL = 100;

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
     *
     * @param x
     * @param y
     */
    public void setPosition(float x, float y) {
        setX(x - getWidth() / 2.0f);
        setY(y - getHeight() / 2.0f);
        applyExposurePoint();
    }


    private void applyExposurePoint() {
        ViewGroup parent = (ViewGroup) getParent();
        if (parent == null) return;

        // We swap X/Y as we have a landscape preview in portrait mode
        float centerPointX = getY() + getHeight() / 2.0f;
        float centerPointY = parent.getWidth() - (getX() + getWidth() / 2.0f);

        centerPointX *= 1000.0f / parent.getHeight();
        centerPointY *= 1000.0f / parent.getWidth();

        centerPointX = (centerPointX - 500.0f) * 2.0f;
        centerPointY = (centerPointY - 500.0f) * 2.0f;

        mTimeLastSet = System.currentTimeMillis();
        mCamManager.setExposurePoint((int) centerPointX, (int) centerPointY);
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        super.onTouch(view, motionEvent);

        if (motionEvent.getActionMasked() == MotionEvent.ACTION_MOVE) {
            if (System.currentTimeMillis() - mTimeLastSet > SET_INTERVAL) {
                applyExposurePoint();
            }
        } else if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP) {
            applyExposurePoint();
        }

        return true;
    }
}
