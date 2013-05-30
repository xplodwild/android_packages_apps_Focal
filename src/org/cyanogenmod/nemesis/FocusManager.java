package org.cyanogenmod.nemesis;

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
         * @param smallAdjust If this parameter is set to true, the focus is being done because of
         *                    continuous focus mode and thus only make a small adjustment
         */
        public void onFocusStart(boolean smallAdjust);

        /**
         * This method is called when the focus operation ends
         * @param smallAdjust If this parameter is set to true, the focus is being done because of
         *                    continuous focus mode and made only a small adjustment
         * @param success If the focus operation was successful. Note that if smallAdjust is true,
         *                this parameter will always be false.
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
    private boolean mIsContinuousPicture;


    public FocusManager(CameraManager cam) {
        mHandler = new Handler();
        mCamManager = cam;
        mIsFocusing = false;
        
        mCamManager.setAutoFocusMoveCallback(this);
        Camera.Parameters params = mCamManager.getParameters();
        if (params.getSupportedFocusModes().contains("continuous-picture")) {
            params.setFocusMode("continuous-picture");
            mIsContinuousPicture = true;
        }
    }

    public void setListener(FocusListener listener) {
        mListener = listener;
    }

    public void checkFocus() {
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
        if (mCamManager.doAutofocus(this)) {
            mIsFocusing = true;

            if (mListener != null)
                mListener.onFocusStart(false);
        }
    }

    @Override
    public void onAutoFocus(boolean success, Camera cam) {
        mLastFocusTimestamp = System.currentTimeMillis();
        mIsFocusing = false;

        if (mListener != null)
            mListener.onFocusReturns(false, success);
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
