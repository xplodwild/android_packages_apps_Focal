package org.cyanogenmod.nemesis.ui;

import org.cyanogenmod.nemesis.SnapshotManager;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;

public class ShutterButton extends ImageView implements OnClickListener {

    public ShutterButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }
    
    public ShutterButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }
    
    public ShutterButton(Context context) {
        super(context);
        initialize();
    }
    
    private void initialize() {
        setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        // XXX: Check for HDR, exposure, burst shots, timer, ...
        SnapshotManager.getSingleton().queueSnapshot(true, 0);
    }

}
