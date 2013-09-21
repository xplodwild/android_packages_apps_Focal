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
 * White Balance Widget, manages the wb settings
 */
public class WhiteBalanceWidget extends SimpleToggleWidget {
    private static final String KEY_WHITEBALANCE = "whitebalance";

    public WhiteBalanceWidget(CameraManager cam, Context context) {
        super(cam, context, KEY_WHITEBALANCE, R.drawable.ic_widget_wb_auto);
        inflateFromXml(R.array.widget_whitebalance_values, R.array.widget_whitebalance_icons,
                R.array.widget_whitebalance_hints);
        getToggleButton().setHintText(R.string.widget_whitebalance);
        restoreValueFromStorage(KEY_WHITEBALANCE);
    }
}
