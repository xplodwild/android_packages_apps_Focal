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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.hardware.Camera;
import android.view.View;
import android.view.ViewGroup;
import android.widget.NumberPicker;

import org.cyanogenmod.nemesis.CameraActivity;
import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;

/**
 * Overflow settings widget
 */
public class SettingsWidget extends WidgetBase implements View.OnClickListener {
    private WidgetOptionButton mResolutionButton;
    private Context mContext;
    private List<String> mResolutionsName;
    private List<Camera.Size> mResolutions;
    private AlertDialog mPickerDialog;
    private NumberPicker mNumberPicker;
    private int mInitialOrientation = -1;
    private int mOrientation;

    public SettingsWidget(CameraManager cam, Context context) {
        super(cam, context, R.drawable.ic_widget_placeholder);
        mContext = context;


        // Get the available photo/video size. Unlike AOSP app, we don't
        // store manually each resolution in an XML, but we calculate it directly
        // from the width and height of the picture size.
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
            mResolutions = cam.getParameters().getSupportedPictureSizes();
            mResolutionsName = new ArrayList<String>();

            DecimalFormat df = new DecimalFormat();
            df.setMaximumFractionDigits(1);
            df.setMinimumFractionDigits(0);
            df.setDecimalSeparatorAlwaysShown(false);

            for (Camera.Size size : mResolutions) {
                float megapixels = size.width * size.height / 1000000.0f;
                mResolutionsName.add(df.format(megapixels) + "MP (" + size.width + "x" + size.height + ")");
            }
        } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {

        }

        mResolutionButton = new WidgetOptionButton(R.drawable.ic_widget_placeholder, context);

        mResolutionButton.setOnClickListener(this);

        addViewToContainer(mResolutionButton);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        int mode = CameraActivity.getCameraMode();

        return (mode == CameraActivity.CAMERA_MODE_PHOTO || mode == CameraActivity.CAMERA_MODE_VIDEO);
    }

    @Override
    public void notifyOrientationChanged(int orientation) {
        if (mInitialOrientation == -1) mInitialOrientation = orientation;
        if (mOrientation == orientation) return;

        mOrientation = orientation;
        if (mPickerDialog != null) {
            float finalRot = orientation;
            if (mInitialOrientation <= 0)
                finalRot -= mInitialOrientation;
            else
                finalRot += mInitialOrientation;

            ((ViewGroup) mNumberPicker.getParent().getParent().getParent().getParent())
                    .animate().rotation(orientation - mInitialOrientation).setDuration(300).start();

        }
    }

    @Override
    public void onClick(View view) {
        if (view == mResolutionButton) {
            mInitialOrientation = -1;

            mNumberPicker = new NumberPicker(mContext);
            String[] names = new String[mResolutionsName.size()];
            mResolutionsName.toArray(names);
            mNumberPicker.setDisplayedValues(names);
            mNumberPicker.setValue(0);
            mNumberPicker.setMinValue(0);
            mNumberPicker.setMaxValue(names.length - 1);
            mNumberPicker.setWrapSelectorWheel(false);
            mNumberPicker.setDescendantFocusability(NumberPicker.FOCUS_BLOCK_DESCENDANTS);
            mNumberPicker.setFormatter(new NumberPicker.Formatter() {
                @Override
                public String format(int i) {
                    return mResolutionsName.get(i);
                }
            });

            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setView(mNumberPicker);
            builder.setTitle(null);
            builder.setCancelable(false);
            builder.setPositiveButton(mContext.getString(R.string.OK), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    mInitialOrientation = -1;

                    // Set picture size
                    mCamManager.setPictureSize(mResolutions.get(mNumberPicker.getValue()));
                }
            });


            mPickerDialog = builder.create();
            mPickerDialog.show();
            ((ViewGroup) mNumberPicker.getParent().getParent().getParent())
                    .animate().rotation(mOrientation).setDuration(300).start();
        }
    }
}
