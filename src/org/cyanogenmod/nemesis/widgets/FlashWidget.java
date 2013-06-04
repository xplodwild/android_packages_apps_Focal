package org.cyanogenmod.nemesis.widgets;

import android.content.Context;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

/**
 * Flash Widget, manages the flash settings
 */
public class FlashWidget extends SimpleToggleWidget {

    public FlashWidget(CameraManager cam, Context context) {
        super(cam, context, "flash-mode", R.drawable.ic_widget_flash_on);

        addValue("auto", R.drawable.ic_widget_flash_auto);
        addValue("on", R.drawable.ic_widget_flash_on);
        addValue("off", R.drawable.ic_widget_flash_off);
    }
}
