package org.cyanogenmod.nemesis;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.LinearLayout;

public class CameraActivity extends Activity {    
    public final static String TAG = "CameraActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        
        LinearLayout layout = (LinearLayout) findViewById(R.id.LinearLayout1);
        
        CameraManager man = new CameraManager(this);
        //layout.addView(man.mPreviewBack);
        layout.addView(man.mPreviewFront);
        
        if (man.open(0)) {
            Log.e(TAG, "Camera 0  open");
        } else {
            Log.e(TAG, "Could not open cameras");
        }
        
        CameraManager man2 = new CameraManager(this);
        //layout.addView(man.mPreviewBack);
        layout.addView(man2.mPreviewFront);
        
        if (man.open(1)) {
            Log.e(TAG, "Camera 1 open");
        } else {
            Log.e(TAG, "Could not open cameras");
        }
    }

}