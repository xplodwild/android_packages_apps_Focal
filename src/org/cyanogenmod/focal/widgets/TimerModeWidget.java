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

package org.cyanogenmod.focal.widgets;

import android.hardware.Camera;
import android.view.View;

import org.cyanogenmod.focal.CameraActivity;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.feats.TimerCapture;

/**
 * Timer mode and voice shutter widget
 */
public class TimerModeWidget extends WidgetBase implements View.OnClickListener {
    private final static String DRAWABLE_TAG = "nemesis-timer-mode";
    private final static int TIMER_MIN_VALUE = 1;
    private final static int TIMER_MAX_VALUE = 60;

    private WidgetOptionButton mBtnToggle;
    private WidgetOptionButton mBtnVoice;
    private WidgetOptionButton mBtnPlus;
    private WidgetOptionButton mBtnMinus;
    private WidgetOptionLabel mLabel;
    private CameraActivity mCameraActivity;
    private TimerCapture mTransformer;
    private boolean mIsEnabled;

    public TimerModeWidget(CameraActivity activity) {
        super(activity.getCamManager(), activity, R.drawable.ic_widget_timer);

        mCameraActivity = activity;
        mIsEnabled = false;

        // Create options
        // XXX: Move that into an XML
        mBtnToggle = new WidgetOptionButton(R.drawable.ic_widget_timer, activity);
        mBtnVoice = new WidgetOptionButton(R.drawable.ic_widget_timer_voice, activity);
        mBtnMinus = new WidgetOptionButton(R.drawable.ic_widget_timer_minus, activity);
        mBtnPlus = new WidgetOptionButton(R.drawable.ic_widget_timer_plus, activity);
        mLabel = new WidgetOptionLabel(activity);

        addViewToContainer(mBtnVoice);
        addViewToContainer(mBtnToggle);
        addViewToContainer(mBtnMinus);
        addViewToContainer(mLabel);
        addViewToContainer(mBtnPlus);

        mBtnToggle.setOnClickListener(this);
        mBtnVoice.setOnClickListener(this);
        mBtnMinus.setOnClickListener(this);
        mBtnPlus.setOnClickListener(this);

        mTransformer = new TimerCapture(activity);
        mLabel.setText(Integer.toString(mTransformer.getTimer()));

        getToggleButton().setHintText(R.string.widget_timermode);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        // Timer mode is supported by everything. If we are in photo mode that is.
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
            return true;
        } else {
            return false;
        }
    }

    private void turnOn() {
        if (!mIsEnabled || mTransformer.getTimer() == TimerCapture.VOICE_TIMER_VALUE) {
            mCameraActivity.setCaptureTransformer(mTransformer);
            mBtnToggle.activeImage(DRAWABLE_TAG + "=on");
            mIsEnabled = true;

            if (mTransformer.getTimer() == TimerCapture.VOICE_TIMER_VALUE) {
                mTransformer.setTimer(5); // some default value
                mBtnVoice.resetImage();
            }
        }
    }

    private void turnOff(boolean nullizeTransformer) {
        if (mIsEnabled) {
            if (nullizeTransformer) {
                mCameraActivity.setCaptureTransformer(null);
            }
            mBtnToggle.resetImage();
            mIsEnabled = false;
        }
    }

    @Override
    public void onClick(View view) {
        if (view == mBtnToggle) {
            // Toggle the transformer
            if (mIsEnabled && mTransformer.getTimer() != TimerCapture.VOICE_TIMER_VALUE) {
                turnOff(true);
            } else {
                turnOn();
            }
        } else if (view == mBtnMinus) {
            turnOn();
            mTransformer.setTimer(clampTimer(mTransformer.getTimer()-1));
            mLabel.setText(Integer.toString(mTransformer.getTimer()));
        } else if (view == mBtnPlus) {
            turnOn();
            mTransformer.setTimer(clampTimer(mTransformer.getTimer()+1));
            mLabel.setText(Integer.toString(mTransformer.getTimer()));
        } else if (view == mBtnVoice) {
            if (mIsEnabled && mTransformer.getTimer() == TimerCapture.VOICE_TIMER_VALUE) {
                mBtnVoice.resetImage();
                turnOff(true);
                return;
            } else if (mIsEnabled && mTransformer.getTimer() != TimerCapture.VOICE_TIMER_VALUE) {
                mBtnToggle.resetImage();
            }

            mTransformer.setTimer(TimerCapture.VOICE_TIMER_VALUE);
            mCameraActivity.setCaptureTransformer(mTransformer);
            mBtnVoice.activeImage(DRAWABLE_TAG + "=voice");
            mIsEnabled = true;
        }
    }

    private int clampTimer(int value) {
        value = Math.max(TIMER_MIN_VALUE, value);
        value = Math.min(TIMER_MAX_VALUE, value);
        return value;
    }
}
