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

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.SettingsStorage;
import org.cyanogenmod.focal.ui.CenteredSeekBar;

/**
 * Exposure compensation setup widget
 */
public class ExposureCompensationWidget extends WidgetBase implements
        CenteredSeekBar.OnCenteredSeekBarChangeListener {
    private static final String KEY_PARAMETER = "exposure-compensation";
    private static final String KEY_MAX_PARAMETER = "max-exposure-compensation";
    private static final String KEY_MIN_PARAMETER = "min-exposure-compensation";

    private WidgetOptionCenteredSeekBar mSeekBar;
    private WidgetOptionLabel mValueLabel;

    public ExposureCompensationWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_exposure);

        // Add views in the widget
        mSeekBar = new WidgetOptionCenteredSeekBar(getMinExposureValue(),
                getMaxExposureValue(), context);
        mSeekBar.setNotifyWhileDragging(true);
        mValueLabel = new WidgetOptionLabel(context);

        addViewToContainer(mSeekBar);
        addViewToContainer(mValueLabel);

        mValueLabel.setText(restoreValueFromStorage(KEY_PARAMETER));
        mSeekBar.setSelectedMinValue(Integer.parseInt(restoreValueFromStorage(KEY_PARAMETER)));
        mSeekBar.setOnCenteredSeekBarChangeListener(this);

        getToggleButton().setHintText(R.string.widget_exposure_compensation);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        return params != null && params.get(KEY_PARAMETER) != null;
    }

    public int getExposureValue() {
        try {
            return Integer.parseInt(mCamManager.getParameters().get(KEY_PARAMETER));
        } catch (Exception e) {
            return 0;
        }
    }

    public int getMinExposureValue() {
        try {
            return Integer.parseInt(mCamManager.getParameters().get(KEY_MIN_PARAMETER));
        } catch (Exception e) {
            return 0;
        }
    }

    public int getMaxExposureValue() {
        try {
            return Integer.parseInt(mCamManager.getParameters().get(KEY_MAX_PARAMETER));
        } catch (Exception e) {
            return 0;
        }
    }

    public void setExposureValue(int value) {
        if (getExposureValue() == value) return;

        String valueStr = Integer.toString(value);

        mCamManager.setParameterAsync(KEY_PARAMETER, valueStr);
        SettingsStorage.storeCameraSetting(mWidget.getContext(), mCamManager.getCurrentFacing(),
                KEY_PARAMETER, valueStr);
        mValueLabel.setText(valueStr);
    }

    @Override
    public void OnCenteredSeekBarValueChanged(CenteredSeekBar bar, Integer minValue) {
        setExposureValue(minValue);
    }
}
