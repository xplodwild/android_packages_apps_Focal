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
import android.os.Build;
import android.util.Log;

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;

public class EffectWidget extends SimpleToggleWidget {
    private static final String KEY_EFFECT = "effect";

    public EffectWidget(CameraManager cam, Context context) {
        super(cam, context, KEY_EFFECT, R.drawable.ic_widget_effect);
        inflateFromXml(R.array.widget_effects_values, R.array.widget_effects_icons,
                R.array.widget_effects_hints);
        getToggleButton().setHintText(R.string.widget_effect);
        restoreValueFromStorage(KEY_EFFECT);
    }

    @Override
    public boolean filterDeviceSpecific(String value) {
        if (Build.DEVICE.equals("mako")) {
            if (value.equals("whiteboard") || value.equals("blackboard")) {
                // Hello Google, why do you report whiteboard/blackboard supported (in -values),
                // when they are not? :(
                return false;
            }
        }
        return true;
    }
}
