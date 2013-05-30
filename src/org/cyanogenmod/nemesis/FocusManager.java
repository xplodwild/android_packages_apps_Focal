package org.cyanogenmod.nemesis;

import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.AutoFocusMoveCallback;
import android.os.Handler;
import android.util.Log;

public class FocusManager implements AutoFocusCallback, AutoFocusMoveCallback {
    public final static String TAG = "FocusManager";
    
    // Miliseconds during which we assume the focus is good
    private int mFocusKeepTimeMs = 3000;
    private long mLastFocusTimestamp = 0;

    private Handler mHandler;
    private CameraManager mCamManager;
    private boolean mIsFocusing;

    public FocusManager(CameraManager cam) {
        mHandler = new Handler();
        mCamManager = cam;
        mIsFocusing = false;
        
        mCamManager.setAutoFocusMoveCallback(this);
        
        post();
    }
    
    private void post() {
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                checkFocus();
                post();
            }
        }, 1500);
    }
    

    private void checkFocus() {
        long time = System.currentTimeMillis();
        
        if (time - mLastFocusTimestamp > mFocusKeepTimeMs && !mIsFocusing) {
            refocus();
        }
        else if (time - mLastFocusTimestamp > mFocusKeepTimeMs*2) {
            // Force a refocus after 2 times focus failed
            refocus();
        }
    }

    /**
     * Sets the time during which the focus is considered valid
     * before refocusing
     * @param timeMs Time in miliseconds
     */
    public void setFocusKeepTime(int timeMs) {
        mFocusKeepTimeMs = timeMs;
    }

    public void refocus() {
        if (mCamManager.doAutofocus(this))
            mIsFocusing = true;
    }

    @Override
    public void onAutoFocus(boolean success, Camera cam) {
        mLastFocusTimestamp = System.currentTimeMillis();
        mIsFocusing = false;
    }

    @Override
    public void onAutoFocusMoving(boolean start, Camera cam) {
        if (mIsFocusing && !start) {
            // We were focusing, but we stopped, notify time of last focus
            mLastFocusTimestamp = System.currentTimeMillis();
        }
        
        mIsFocusing = start;
    }
}
