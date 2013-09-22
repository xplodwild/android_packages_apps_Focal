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
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.SettingsStorage;
import org.cyanogenmod.focal.ui.CenteredSeekBar;

/**
 * Contrast/sharpness/saturation setting widget (Qualcomm)
 */
public class EnhancementsWidget extends WidgetBase {
    private static final String KEY_CONTRAST_PARAMETER = "contrast";
    private static final String KEY_MAX_CONTRAST_PARAMETER = "max-contrast";
    private static final String KEY_SHARPNESS_PARAMETER = "sharpness";
    private static final String KEY_MAX_SHARPNESS_PARAMETER = "max-sharpness";
    private static final String KEY_SATURATION_PARAMETER = "saturation";
    private static final String KEY_MAX_SATURATION_PARAMETER = "max-saturation";

    private static final int ROW_CONTRAST = 0;
    private static final int ROW_SHARPNESS = 1;
    private static final int ROW_SATURATION = 2;
    private static final int ROW_COUNT = 3;

    private WidgetOptionSeekBar[] mSeekBar;
    private WidgetOptionLabel[] mValueLabel;

    private class SeekBarListener implements SeekBar.OnSeekBarChangeListener {
        private String mKey;

        public SeekBarListener(String key) {
            mKey = key;
        }

        @Override
        public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
            setValue(mKey, i);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    }

    public EnhancementsWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_contrast);

        // Add views in the widget
        mSeekBar = new WidgetOptionSeekBar[ROW_COUNT];
        mValueLabel = new WidgetOptionLabel[ROW_COUNT];

        for (int i = 0; i < ROW_COUNT; i++) {
            int max = 0, val = 0;

            switch (i) {
                case ROW_CONTRAST:
                    max = getValue(KEY_MAX_CONTRAST_PARAMETER);
                    val = getValue(KEY_CONTRAST_PARAMETER);
                    break;

                case ROW_SATURATION:
                    max = getValue(KEY_MAX_SATURATION_PARAMETER);
                    val = getValue(KEY_SATURATION_PARAMETER);
                    break;

                case ROW_SHARPNESS:
                    max = getValue(KEY_MAX_SHARPNESS_PARAMETER);
                    val = getValue(KEY_SHARPNESS_PARAMETER);
                    break;
            }

            mSeekBar[i] = new WidgetOptionSeekBar(context);
            mValueLabel[i] = new WidgetOptionLabel(context);
            mSeekBar[i].setMax(max);
            mSeekBar[i].setProgress(val);

            setViewAt(i, 2, 1, 3, mSeekBar[i]);
            setViewAt(i, 1, 1, 1, mValueLabel[i]);

            if (i == ROW_CONTRAST) {
                setViewAt(i, 0, 1, 1, new WidgetOptionImage(
                        R.drawable.ic_widget_contrast, context));
            } else if (i == ROW_SATURATION) {
                setViewAt(i, 0, 1, 1, new WidgetOptionImage(
                        R.drawable.ic_widget_saturation, context));
            } else if (i == ROW_SHARPNESS) {
                setViewAt(i, 0, 1, 1, new WidgetOptionImage(
                        R.drawable.ic_widget_sharpness, context));
            }
        }

        mSeekBar[ROW_CONTRAST].setOnSeekBarChangeListener(
                new SeekBarListener(KEY_CONTRAST_PARAMETER));
        mSeekBar[ROW_SHARPNESS].setOnSeekBarChangeListener(
                new SeekBarListener(KEY_SHARPNESS_PARAMETER));
        mSeekBar[ROW_SATURATION].setOnSeekBarChangeListener(
                new SeekBarListener(KEY_SATURATION_PARAMETER));

        mValueLabel[ROW_CONTRAST].setText(restoreValueFromStorage(KEY_CONTRAST_PARAMETER));
        mValueLabel[ROW_SHARPNESS].setText(restoreValueFromStorage(KEY_SHARPNESS_PARAMETER));
        mValueLabel[ROW_SATURATION].setText(restoreValueFromStorage(KEY_SATURATION_PARAMETER));

        getToggleButton().setHintText(R.string.widget_enhancements);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        if (params.get(KEY_CONTRAST_PARAMETER) != null
                || params.get(KEY_SATURATION_PARAMETER) != null
                || params.get(KEY_SHARPNESS_PARAMETER) != null) {
            return true;
        } else {
            return false;
        }
    }

    public int getValue(String key) {
        Camera.Parameters params = mCamManager.getParameters();
        String value = params.get(key);

        if (value != null) {
            try {
                return Integer.parseInt(value);
            } catch (NumberFormatException e) {
                Log.e(TAG, value + " is not a valid number, returning 0");
                return 0;
            }
        } else {
            return 0;
        }
    }

    public void setValue(String key, int value) {
        String valueStr = Integer.toString(value);
        mCamManager.setParameterAsync(key, valueStr);

        if (key.equals(KEY_CONTRAST_PARAMETER)) {
            mValueLabel[ROW_CONTRAST].setText(valueStr);
            SettingsStorage.storeCameraSetting(getWidget().getContext(),
                    mCamManager.getCurrentFacing(),KEY_CONTRAST_PARAMETER, valueStr);
        } else if (key.equals(KEY_SHARPNESS_PARAMETER)) {
            mValueLabel[ROW_SHARPNESS].setText(valueStr);
            SettingsStorage.storeCameraSetting(getWidget().getContext(),
                    mCamManager.getCurrentFacing(), KEY_SHARPNESS_PARAMETER, valueStr);
        } else if (key.equals(KEY_SATURATION_PARAMETER)) {
            mValueLabel[ROW_SATURATION].setText(valueStr);
            SettingsStorage.storeCameraSetting(getWidget().getContext(),
                    mCamManager.getCurrentFacing(), KEY_SATURATION_PARAMETER, valueStr);
        }
    }
}
