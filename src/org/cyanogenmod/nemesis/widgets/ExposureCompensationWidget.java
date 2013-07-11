/**
 * Copyright (C) 2013 The CyanogenMod Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.cyanogenmod.nemesis.widgets;

import android.content.Context;
import android.hardware.Camera;
import android.view.View;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.SettingsStorage;

/**
 * Exposure compensation setup widget
 */
public class ExposureCompensationWidget extends WidgetBase {
    private static final String KEY_PARAMETER = "exposure-compensation";
    private static final String KEY_MAX_PARAMETER = "max-exposure-compensation";
    private static final String KEY_MIN_PARAMETER = "min-exposure-compensation";

    private WidgetOptionButton mMinusButton;
    private WidgetOptionButton mPlusButton;
    private WidgetOptionLabel mValueLabel;


    private class MinusClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            setExposureValue(Math.max(getExposureValue() - 1, getMinExposureValue()));
        }
    }

    private class PlusClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            setExposureValue(Math.min(getExposureValue() + 1, getMaxExposureValue()));
        }
    }

    public ExposureCompensationWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_exposure);

        // Add views in the widget
        mMinusButton = new WidgetOptionButton(R.drawable.ic_widget_timer_minus, context);
        mPlusButton = new WidgetOptionButton(R.drawable.ic_widget_timer_plus, context);
        mValueLabel = new WidgetOptionLabel(context);

        mMinusButton.setOnClickListener(new MinusClickListener());
        mPlusButton.setOnClickListener(new PlusClickListener());

        addViewToContainer(mMinusButton);
        addViewToContainer(mValueLabel);
        addViewToContainer(mPlusButton);

        mValueLabel.setText(restoreValueFromStorage(KEY_PARAMETER));

        getToggleButton().setHintText(R.string.widget_exposure_compensation);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        return params.get(KEY_PARAMETER) != null;
    }

    public int getExposureValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_PARAMETER));
    }

    public int getMinExposureValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_MIN_PARAMETER));
    }

    public int getMaxExposureValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_MAX_PARAMETER));
    }

    public void setExposureValue(int value) {
        String valueStr = Integer.toString(value);

        mCamManager.setParameterAsync(KEY_PARAMETER, valueStr);
        SettingsStorage.storeCameraSetting(mWidget.getContext(), mCamManager.getCurrentFacing(),
                KEY_PARAMETER, valueStr);
        mValueLabel.setText(valueStr);
    }
}
