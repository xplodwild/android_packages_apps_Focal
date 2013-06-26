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

    private static final int ROW_CONTRAST   = 0;
    private static final int ROW_SHARPNESS  = 1;
    private static final int ROW_SATURATION = 2;
    private static final int ROW_COUNT      = 3;

    private WidgetOptionButton[] mMinusButton;
    private WidgetOptionButton[] mPlusButton;
    private WidgetOptionLabel[]  mValueLabel;


    private class MinusClickListener implements View.OnClickListener {
        private String mKey;

        public MinusClickListener(String key) {
            mKey = key;
        }

        @Override
        public void onClick(View view) {
            setValue(mKey, Math.max(getValue(mKey) - 1, 0));
        }
    }

    private class PlusClickListener implements View.OnClickListener {
        private String mKey;
        private String mMaxKey;

        public PlusClickListener(String key, String maxKey) {
            mKey = key;
            mMaxKey = maxKey;
        }

        @Override
        public void onClick(View view) {
            setValue(mKey, Math.min(getValue(mKey) + 1, getValue(mMaxKey)));
        }
    }

    public EnhancementsWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_contrast);

        // Add views in the widget
        //forceGridSize(3, 3);

        mMinusButton = new WidgetOptionButton[ROW_COUNT];
        mPlusButton = new WidgetOptionButton[ROW_COUNT];
        mValueLabel = new WidgetOptionLabel[ROW_COUNT];

        for (int i = 0; i < ROW_COUNT; i++) {
            mMinusButton[i] = new WidgetOptionButton(R.drawable.ic_widget_timer_minus, context); // TODO icon
            mPlusButton[i] = new WidgetOptionButton(R.drawable.ic_widget_timer_plus, context); // TODO icon
            mValueLabel[i] = new WidgetOptionLabel(context);

            setViewAt(0, i, mPlusButton[i]);
            setViewAt(1, i, mValueLabel[i]);
            setViewAt(2, i, mMinusButton[i]);

            if (i == ROW_CONTRAST) {
                setViewAt(3, i, new WidgetOptionImage(R.drawable.ic_widget_contrast, context));
            } else if (i == ROW_SATURATION) {
                setViewAt(3, i, new WidgetOptionImage(R.drawable.ic_widget_saturation, context));
            } else if (i == ROW_SHARPNESS) {
                setViewAt(3, i, new WidgetOptionImage(R.drawable.ic_widget_sharpness, context));
            }

        }

        mMinusButton[ROW_CONTRAST].setOnClickListener(new MinusClickListener(KEY_CONTRAST_PARAMETER));
        mMinusButton[ROW_SHARPNESS].setOnClickListener(new MinusClickListener(KEY_SHARPNESS_PARAMETER));
        mMinusButton[ROW_SATURATION].setOnClickListener(new MinusClickListener(KEY_SATURATION_PARAMETER));

        mPlusButton[ROW_CONTRAST].setOnClickListener(new PlusClickListener(KEY_CONTRAST_PARAMETER,
                KEY_MAX_CONTRAST_PARAMETER));
        mPlusButton[ROW_SHARPNESS].setOnClickListener(new PlusClickListener(KEY_SHARPNESS_PARAMETER,
                KEY_MAX_SHARPNESS_PARAMETER));
        mPlusButton[ROW_SATURATION].setOnClickListener(new PlusClickListener(KEY_SATURATION_PARAMETER,
                KEY_MAX_SATURATION_PARAMETER));

        mValueLabel[ROW_CONTRAST].setText(Integer.toString(getValue(KEY_CONTRAST_PARAMETER)));
        mValueLabel[ROW_SHARPNESS].setText(Integer.toString(getValue(KEY_SHARPNESS_PARAMETER)));
        mValueLabel[ROW_SATURATION].setText(Integer.toString(getValue(KEY_SATURATION_PARAMETER)));
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
        return Integer.parseInt(mCamManager.getParameters().get(key));
    }

    public void setValue(String key, int value) {
        mCamManager.setParameterAsync(key, Integer.toString(value));

        if (key.equals(KEY_CONTRAST_PARAMETER)) {
            mValueLabel[ROW_CONTRAST].setText(Integer.toString(value));
        } else if (key.equals(KEY_SHARPNESS_PARAMETER)) {
            mValueLabel[ROW_SHARPNESS].setText(Integer.toString(value));
        } else if (key.equals(KEY_SATURATION_PARAMETER)) {
            mValueLabel[ROW_SATURATION].setText(Integer.toString(value));
        }
    }
}
