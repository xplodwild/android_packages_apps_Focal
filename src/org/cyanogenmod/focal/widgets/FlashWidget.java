/*
 * Copyright (C) 2013 Guillaume Lesniak
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

package org.cyanogenmod.focal.widgets;

import android.content.Context;

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;

/**
 * Flash Widget, manages the flash settings
 */
public class FlashWidget extends SimpleToggleWidget {
    private static final String KEY_REDEYE_REDUCTION = "redeye-reduction";
    private static final String KEY_FLASH_MODE = "flash-mode";

    public FlashWidget(CameraManager cam, Context context) {
        super(cam, context, KEY_FLASH_MODE, R.drawable.ic_widget_flash_on);
        inflateFromXml(R.array.widget_flash_values, R.array.widget_flash_icons,
                R.array.widget_flash_hints);
        getToggleButton().setHintText(R.string.widget_flash);
        restoreValueFromStorage(KEY_FLASH_MODE);
    }

    /**
     * When the flash is enabled, try to enable red-eye reduction (qualcomm)
     *
     * @param value The value set to the key
     */
    @Override
    public void onValueSet(String value) {
        if (value.equals("on") || value.equals("auto")) {
            if (mCamManager.getParameters().get(KEY_REDEYE_REDUCTION) != null) {
                mCamManager.setParameterAsync(KEY_REDEYE_REDUCTION, "enable");
            }
        } else {
            if (mCamManager.getParameters().get(KEY_REDEYE_REDUCTION) != null) {
                mCamManager.setParameterAsync(KEY_REDEYE_REDUCTION, "disable");
            }
        }
    }
}
