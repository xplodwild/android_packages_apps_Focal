package org.cyanogenmod.nemesis.widgets;

import android.content.Context;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

public class SceneModeWidget extends SimpleToggleWidget {

    public SceneModeWidget(CameraManager cam, Context context) {
        super(cam, context, "scene-mode", R.drawable.ic_widget_scenemode);

        addValue("auto", R.drawable.ic_widget_scenemode_auto);
        addValue("portrait", R.drawable.ic_widget_scenemode_portrait);
        addValue("landscape", R.drawable.ic_widget_scenemode_landscape);
        addValue("night", R.drawable.ic_widget_scenemode_night);
        addValue("nightportrait", R.drawable.ic_widget_scenemode_nightportrait);
        addValue("beach", R.drawable.ic_widget_scenemode_beach);
        addValue("snow", R.drawable.ic_widget_scenemode_snow);
        addValue("sports", R.drawable.ic_widget_scenemode_sports);
        addValue("party", R.drawable.ic_widget_scenemode_party);
        addValue("barcode", R.drawable.ic_widget_scenemode_barcode);
    }
}
