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
 * Sharpness setting widget (Qualcomm)
 */
public class SharpnessWidget extends WidgetBase {
    private static final String KEY_PARAMETER = "sharpness";
    private static final String KEY_MAX_PARAMETER = "max-sharpness";

    private WidgetOptionButton mMinusButton;
    private WidgetOptionButton mPlusButton;
    private WidgetOptionLabel mValueLabel;


    private class MinusClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            setSharpnessValue(Math.max(getSharpnessValue() - 1, 0));
        }
    }

    private class PlusClickListener implements View.OnClickListener {
        @Override
        public void onClick(View view) {
            setSharpnessValue(Math.min(getSharpnessValue() + 1, getMaxSharpnessValue()));
        }
    }

    public SharpnessWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_sharpness);

        // Add views in the widget
        mMinusButton = new WidgetOptionButton(R.drawable.ic_widget_timer_minus, context); // TODO icon
        mPlusButton = new WidgetOptionButton(R.drawable.ic_widget_timer_plus, context); // TODO icon
        mValueLabel = new WidgetOptionLabel(context);

        mMinusButton.setOnClickListener(new MinusClickListener());
        mPlusButton.setOnClickListener(new PlusClickListener());
        mValueLabel.setText(Integer.toString(getSharpnessValue()));

        addViewToContainer(mMinusButton);
        addViewToContainer(mValueLabel);
        addViewToContainer(mPlusButton);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        if (params.get(KEY_PARAMETER) != null) {
            return true;
        } else {
            return false;
        }
    }

    public int getSharpnessValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_PARAMETER));
    }

    public int getMaxSharpnessValue() {
        return Integer.parseInt(mCamManager.getParameters().get(KEY_MAX_PARAMETER));
    }

    public void setSharpnessValue(int value) {
        mCamManager.setParameterAsync(KEY_PARAMETER, Integer.toString(value));
        mValueLabel.setText(Integer.toString(value));
    }
}
