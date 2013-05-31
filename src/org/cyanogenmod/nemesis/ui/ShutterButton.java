package org.cyanogenmod.nemesis.ui;

import org.cyanogenmod.nemesis.SnapshotManager;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;

public class ShutterButton extends ImageView {
    private boolean mSlideOpen = false;
    
    /**
     * Interface that notifies the CameraActivity that
     * the shutter button has been slide (and should show the
     * pad ring)
     */
    public interface ShutterSlideListener {
        public void onSlideOpen();
        public void onSlideClose();
    }
    
    
    private float mDownX;
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
        } else if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
            if (event.getRawX() - mDownX < -getWidth()/2 && mListener != null) {
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
        
        return super.onTouchEvent(event);
    }
    
    public void setSlideListener(ShutterSlideListener listener) {
        mListener = listener;
    }

    public static class ClickListener implements OnClickListener {
        private SnapshotManager mSnapshotManager;

        public ClickListener(SnapshotManager manager) {
            mSnapshotManager = manager;
        }

        @Override
        public void onClick(View v) {
            // XXX: Check for HDR, exposure, burst shots, timer, ...
            mSnapshotManager.queueSnapshot(true, 0);
        }
    }

}
