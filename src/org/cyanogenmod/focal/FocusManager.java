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

package org.cyanogenmod.focal;

import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.AutoFocusMoveCallback;
import android.os.Handler;

public class FocusManager implements AutoFocusCallback, AutoFocusMoveCallback {
    public final static String TAG = "FocusManager";

    public interface FocusListener {
        /**
         * This method is called when the focus operation starts (whether through touch to focus,
         * or via continuous-picture focus mode).
         *
         * @param smallAdjust If this parameter is set to true, the focus is being done because of
         *                    continuous focus mode and thus only make a small adjustment
         */
        public void onFocusStart(boolean smallAdjust);

        /**
         * This method is called when the focus operation ends
         *
         * @param smallAdjust If this parameter is set to true, the focus is being done because of
         *                    continuous focus mode and made only a small adjustment
         * @param success     If the focus operation was successful. Note that if smallAdjust is true,
         *                    this parameter will always be false.
         */
        public void onFocusReturns(boolean smallAdjust, boolean success);
    }

    // Miliseconds during which we assume the focus is good
    private int mFocusKeepTimeMs = 3000;
    private long mLastFocusTimestamp = 0;

    private Handler mHandler;
    private CameraManager mCamManager;
    private FocusListener mListener;
    private boolean mIsFocusing;

    public FocusManager(CameraManager cam) {
        mHandler = new Handler();
        mCamManager = cam;
        mIsFocusing = false;

        mCamManager.setAutoFocusMoveCallback(this);
        Camera.Parameters params = mCamManager.getParameters();
        if (params.getSupportedFocusModes().contains("auto")) {
            params.setFocusMode("auto");
        }

        // Do a first focus after 1 second
        mHandler.postDelayed(new Runnable() {
            public void run() {
                checkFocus();
            }
        }, 1000);
    }

    public void setListener(FocusListener listener) {
        mListener = listener;
    }

    public void checkFocus() {
        long time = System.currentTimeMillis();

        if (time - mLastFocusTimestamp > mFocusKeepTimeMs && !mIsFocusing) {
            refocus();
        } else if (time - mLastFocusTimestamp > mFocusKeepTimeMs * 2) {
            // Force a refocus after 2 times focus failed
            refocus();
        }
    }

    /**
     * Sets the time during which the focus is considered valid
     * before refocusing
     *
     * @param timeMs Time in miliseconds
     */
    public void setFocusKeepTime(int timeMs) {
        mFocusKeepTimeMs = timeMs;
    }

    public void refocus() {
        if (mCamManager.doAutofocus(this)) {
            mIsFocusing = true;

            if (mListener != null) {
                mListener.onFocusStart(false);
            }
        }
    }

    @Override
    public void onAutoFocus(boolean success, Camera cam) {
        mLastFocusTimestamp = System.currentTimeMillis();
        mIsFocusing = false;

        if (mListener != null) {
            mListener.onFocusReturns(false, success);
        }
    }

    @Override
    public void onAutoFocusMoving(boolean start, Camera cam) {
        if (mIsFocusing && !start) {
            // We were focusing, but we stopped, notify time of last focus
            mLastFocusTimestamp = System.currentTimeMillis();
        }

        if (start) {
            if (mListener != null) {
                mListener.onFocusStart(true);
            }
        } else {
            if (mListener != null) {
                mListener.onFocusReturns(true, false);
            }
        }
        mIsFocusing = start;
    }
}
