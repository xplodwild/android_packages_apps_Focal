/**
 * Copyright (C) 2013 The CyanogenMod Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.cyanogenmod.nemesis.R;

/**
 * Little notifier that comes in from the side of the screen
 * to tell the user about something quickly
 */
public class Notifier extends LinearLayout {
    private TextView mTextView;
    private Handler mHandler;

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

    /**
     * Shows the provided {@param text} during [@param durationMs} milliseconds with
     * a nice animation.
     *
     * @param text
     * @param durationMs
     * @note This method must be ran in UI thread!
     */
    public void notify(String text, long durationMs) {
        mTextView = (TextView) findViewById(R.id.notifier_text);
        mTextView.setText(text);

        fadeIn();
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                fadeOut();
            }
        }, durationMs);
    }

    private void fadeIn() {
        animate().setDuration(300).alpha(1.0f).setInterpolator(new AccelerateInterpolator()).start();
    }

    private void fadeOut() {
        animate().setDuration(300).alpha(0.0f).setInterpolator(new DecelerateInterpolator()).start();
    }
}
