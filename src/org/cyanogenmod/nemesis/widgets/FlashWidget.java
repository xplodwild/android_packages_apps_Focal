package org.cyanogenmod.nemesis.widgets;

import org.cyanogenmod.nemesis.R;

import android.content.Context;

/**
 * Flash Widget, manages the flash settings
 */
public class FlashWidget extends SimpleToggleWidget {
    
    public FlashWidget(Context context) {
        super(context, "flash-mode", R.drawable.ic_widget_flash_on);

        addValue("on", R.drawable.ic_widget_flash_on);
        addValue("off", R.drawable.ic_widget_flash_off);
    }
}
