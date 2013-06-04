package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.ImageView;

public class ShutterButton extends ImageView {
    public final static String TAG = "ShutterButton";
    private boolean mSlideOpen = false;

    /**
     * Interface that notifies the CameraActivity that
     * the shutter button has been slided (and should show the
     * pad ring), or that there is a motionevent handled by this
     * View that should belong to the SwitchRingPad
     */
    public interface ShutterSlideListener {
        public void onSlideOpen();

        public void onSlideClose();

        public void onShutterButtonPressed();

        public boolean onMotionEvent(MotionEvent ev);
    }


    private float mDownX;
    private float mDownY;
    private ShutterSlideListener mListener;

    public ShutterButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public ShutterButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ShutterButton(Context context) {
        super(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            mDownX = event.getRawX();
            mDownY = event.getRawY();
            mListener.onShutterButtonPressed();
        } else if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
            if ((event.getRawX() - mDownX < -getWidth() / 2 || Math.abs(event.getRawY() - mDownY) > getHeight() / 2) && mListener != null) {
                if (!mSlideOpen) {
                    mListener.onSlideOpen();
                    mSlideOpen = true;
                }
            } else {
                if (mSlideOpen) {
                    mListener.onSlideClose();
                    mSlideOpen = false;
                }
            }
        }

        boolean listenerResult = false;
        if (mListener != null && mSlideOpen) {
            listenerResult = mListener.onMotionEvent(event);
        }

        return (super.onTouchEvent(event) || listenerResult);
    }

    public void setSlideListener(ShutterSlideListener listener) {
        mListener = listener;
    }


}
