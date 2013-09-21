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
import android.os.Handler;
import android.util.AttributeSet;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.LinearLayout;
import android.widget.TextView;

import fr.xplod.focal.R;
import org.cyanogenmod.focal.Util;

/**
 * Little notifier that comes in from the side of the screen
 * to tell the user about something quickly
 */
public class Notifier extends LinearLayout {
    private TextView mTextView;
    private Handler mHandler;
    private int mOrientation;
    private Runnable mFadeOutRunnable = new Runnable() {
        @Override
        public void run() {
            fadeOut();
        }
    };
    private float mTargetX;
    private float mTargetY;

    public Notifier(Context context) {
        super(context);
        initialize();
    }

    public Notifier(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public Notifier(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    private void initialize() {
        mHandler = new Handler();
    }

    public void notifyOrientationChanged(int orientation) {
        mOrientation = orientation;
        if (mTextView == null) return;

        if (orientation % 180 != 0) {
            // We translate to fit orientation properly within the screen
            animate().rotation(orientation).setDuration(200).x(mTargetX + mTextView
                    .getMeasuredHeight() / 2).y(mTargetY - mTextView.getMeasuredWidth() / 2)
                    .setInterpolator(new DecelerateInterpolator()).start();
        } else {
            animate().rotation(orientation).setDuration(200).x(mTargetX)
                    .y(mTargetY).setInterpolator(new DecelerateInterpolator()).start();
        }
    }

    /**
     * Shows the provided {@param text} during {@param durationMs} milliseconds with
     * a nice animation.
     *
     * @param text
     * @param durationMs
     * @note This method must be ran in UI thread!
     */
    public void notify(final String text, final long durationMs) {
        mHandler.post(new Runnable() {
            public void run() {
                mHandler.removeCallbacks(mFadeOutRunnable);
                mTextView = (TextView) findViewById(R.id.notifier_text);
                mTextView.setText(text);
                setAlpha(0.0f);
                setX(Util.getScreenSize(null).x*1.0f/3.0f);
                setY(Util.getScreenSize(null).y*2.0f/3.0f);
                mTargetX = getX();
                mTargetY = getY();
                notifyOrientationChanged(mOrientation);

                fadeIn();
                mHandler.postDelayed(mFadeOutRunnable, durationMs);
            }
        });
    }

    /**
     * Shows the provided {@param text} during {@param durationMs} milliseconds at the
     * provided {@param x} and {@param y} position with a nice animation.
     *
     * @param text The text to display
     * @param durationMs How long should we display it
     * @param x The X position
     * @param y The Y position
     */
    public void notify(final String text, final long durationMs, final float x, final float y) {
        mHandler.post(new Runnable() {
            public void run() {
                mHandler.removeCallbacks(mFadeOutRunnable);
                mTextView = (TextView) findViewById(R.id.notifier_text);
                mTextView.setText(text);
                setAlpha(0.0f);
                setX(x);
                setY(y);

                mTargetX = getX();
                mTargetY = getY();

                notifyOrientationChanged(mOrientation);

                fadeIn();
                mHandler.postDelayed(mFadeOutRunnable, durationMs);
            }
        });
    }

    private void fadeIn() {
        animate().setDuration(300).alpha(1.0f).setInterpolator(
                new AccelerateInterpolator()).start();
    }

    private void fadeOut() {
        animate().setDuration(300).alpha(0.0f).setInterpolator(
                new DecelerateInterpolator()).start();
    }
}
