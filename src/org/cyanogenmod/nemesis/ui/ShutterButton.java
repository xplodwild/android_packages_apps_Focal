package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;

import org.cyanogenmod.nemesis.SnapshotManager;

public class ShutterButton extends ImageView {

    public ShutterButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }
    
    public ShutterButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
    
    public ShutterButton(Context context) {
        super(context);
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
