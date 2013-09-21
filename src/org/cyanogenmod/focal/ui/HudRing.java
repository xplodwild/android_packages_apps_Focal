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

    private final static float IDLE_ALPHA = 0.25f;
    private final static int ANIMATION_DURATION = 120;

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
        setAlpha(IDLE_ALPHA);
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        if (motionEvent.getActionMasked() == MotionEvent.ACTION_DOWN) {
            mLastX = motionEvent.getRawX();
            mLastY = motionEvent.getRawY();
           /* mOffsetX = motionEvent.getX() - motionEvent.getRawX();
            mOffsetY = motionEvent.getY() - motionEvent.getRawY();*/
            animatePressDown();
        } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
            setX(getX() + (motionEvent.getRawX() - mLastX));
            setY(getY() + (motionEvent.getRawY() - mLastY));

            mLastX = motionEvent.getRawX();
            mLastY = motionEvent.getRawY();
        } else if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP) {
            animatePressUp();
        }

        return true;
    }

    public void animatePressDown() {
        animate().alpha(1.0f).setDuration(ANIMATION_DURATION).start();
    }

    public void animatePressUp() {
        animate().alpha(IDLE_ALPHA).rotation(0).setDuration(ANIMATION_DURATION).start();
    }

    public void animateWorking(long duration) {
        animate().rotationBy(45.0f).setDuration(duration).setInterpolator(
                new DecelerateInterpolator()).start();
    }
}
