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
import android.hardware.Camera;
import android.view.View;

import org.cyanogenmod.focal.CameraManager;
import org.cyanogenmod.focal.SettingsStorage;

import fr.xplod.focal.R;

/**
 * Shutter speed setup widget
 */
public class ShutterSpeedWidget extends WidgetBase {
    // XXX We use shutter-speed here because the wrapper sets the ae-mode
    // according to the value of shutter speed as well as ISO.
    // What is done here (0-12, auto) is sony-specific anyway, so it
    // makes sense to keep it this way
    private static final String KEY_PARAMETER = "sony-shutter-speed";
    private static final String KEY_MAX_PARAMETER = "sony-max-shutter-speed";
    private static final String KEY_MIN_PARAMETER = "sony-min-shutter-speed";
    private static final String KEY_AUTO_VALUE = "auto";

    private WidgetOptionButton mMinusButton;
    private WidgetOptionButton mPlusButton;
    private WidgetOptionButton mAutoButton;
    private WidgetOptionLabel mValueLabel;


    private class MinusClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            if (getShutterSpeedRawValue().equals(KEY_AUTO_VALUE)) {
                setShutterSpeedValue(getMaxShutterSpeedValue());
            } else {
                setShutterSpeedValue(Math.max(getShutterSpeedValue() - 1, getMinShutterSpeedValue()));
            }
        }
    }

    private class PlusClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            if (getShutterSpeedRawValue().equals(KEY_AUTO_VALUE)) {
                setShutterSpeedValue(getMinShutterSpeedValue());
            } else {
                setShutterSpeedValue(Math.min(getShutterSpeedValue() + 1, getMaxShutterSpeedValue()));
            }
        }
    }

    private class AutoClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            setAutoShutterSpeed();
        }
    }

    public ShutterSpeedWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_shutterspeed);

        // Add views in the widget
        mMinusButton = new WidgetOptionButton(R.drawable.ic_widget_timer_minus, context);
        mPlusButton = new WidgetOptionButton(R.drawable.ic_widget_timer_plus, context);
        mAutoButton = new WidgetOptionButton(R.drawable.ic_widget_iso_auto, context);
        mValueLabel = new WidgetOptionLabel(context);

        mMinusButton.setOnClickListener(new MinusClickListener());
        mPlusButton.setOnClickListener(new PlusClickListener());
        mAutoButton.setOnClickListener(new AutoClickListener());

        addViewToContainer(mMinusButton);
        addViewToContainer(mValueLabel);
        addViewToContainer(mPlusButton);
        addViewToContainer(mAutoButton);

        mValueLabel.setText(getShutterSpeedDisplayValue(restoreValueFromStorage(KEY_PARAMETER)));
        if ((restoreValueFromStorage(KEY_PARAMETER) != null) &&
            (restoreValueFromStorage(KEY_PARAMETER).equals(KEY_AUTO_VALUE))) {
            mAutoButton.activeImage(KEY_PARAMETER + ":" + KEY_AUTO_VALUE);
        }

        getToggleButton().setHintText(R.string.widget_shutter_speed);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        return params.get(KEY_PARAMETER) != null;
    }

    public int getShutterSpeedValue() {
        int retVal = 0;
        String val = mCamManager.getParameters().get(KEY_PARAMETER);
        if (val.equals(KEY_AUTO_VALUE) ){
            retVal = getMaxShutterSpeedValue() + 1;
        }
        else {
            retVal = Integer.parseInt(mCamManager.getParameters().get(KEY_PARAMETER));
        }
        return retVal;
    }

    public String getShutterSpeedRawValue() {
        String val = mCamManager.getParameters().get(KEY_PARAMETER);
        return val;
    }

    public int getMinShutterSpeedValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_MIN_PARAMETER));
    }

    public int getMaxShutterSpeedValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_MAX_PARAMETER));
    }

    public void setShutterSpeedValue(int value) {
        String valueStr = Integer.toString(value);
        mCamManager.setParameterAsync(KEY_PARAMETER, valueStr);
        SettingsStorage.storeCameraSetting(mWidget.getContext(), mCamManager.getCurrentFacing(),
                KEY_PARAMETER, valueStr);
        mValueLabel.setText(getShutterSpeedDisplayValue(valueStr));
        mAutoButton.resetImage();
    }

    public void setAutoShutterSpeed() {
        mCamManager.setParameterAsync(KEY_PARAMETER, KEY_AUTO_VALUE);
        SettingsStorage.storeCameraSetting(mWidget.getContext(), mCamManager.getCurrentFacing(),
                KEY_PARAMETER, "auto");
        mValueLabel.setText(getShutterSpeedDisplayValue(KEY_AUTO_VALUE));
        mAutoButton.activeImage(KEY_PARAMETER + ":" + KEY_AUTO_VALUE);
    }


    public String getShutterSpeedDisplayValue(String value) {
        String[] values = mWidget.getContext().getResources().getStringArray(
                R.array.widget_shutter_speed_display_values);
        if (value == null || value.equals(KEY_AUTO_VALUE)) {
            return values[0];
        } else {
            return values[Integer.parseInt(value) + 1];
        }
    }
}
