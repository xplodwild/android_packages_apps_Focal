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

import android.hardware.Camera;
import android.view.View;

import org.cyanogenmod.focal.CameraActivity;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.feats.SoftwareHdrCapture;

import java.util.List;

/**
 * Software HDR widget
 * This is distinct from the hardware HDR to avoid putting everything in one same class
 * and have to hack around the XML inflation. This widget only shows up if neither HDR
 * scene-mode is present and there is no AE-Bracket-HDR setting. If there are new
 * methods of having HDR, you'll have to add them in arrays.xml.
 */
public class SoftwareHdrWidget extends WidgetBase implements View.OnClickListener {
    private CameraActivity mContext;
    private WidgetOptionButton mBtnOn;
    private WidgetOptionButton mBtnOff;
    private WidgetOptionButton mPreviousMode;
    private SoftwareHdrCapture mTransformer;
    private final static String DRAWABLE_TAG = "nemesis-soft-hdr";

    public SoftwareHdrWidget(CameraActivity activity) {
        super(activity.getCamManager(), activity, R.drawable.ic_widget_hdr);
        mContext = activity;

        mBtnOn = new WidgetOptionButton(R.drawable.ic_widget_hdr_on, mContext);
        mBtnOff = new WidgetOptionButton(R.drawable.ic_widget_hdr_off, mContext);
        mBtnOn.setOnClickListener(this);
        mBtnOff.setOnClickListener(this);

        addViewToContainer(mBtnOn);
        addViewToContainer(mBtnOff);

        mPreviousMode = mBtnOff;
        mPreviousMode.activeImage(DRAWABLE_TAG + "=off");

        mTransformer = new SoftwareHdrCapture(mContext);

        getToggleButton().setHintText(R.string.widget_softwarehdr);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        // Software HDR only in photo mode!
        if (CameraActivity.getCameraMode() != CameraActivity.CAMERA_MODE_PHOTO) {
            mTransformer.tearDown();
            return false;
        }

        // We hide the software HDR widget if either we have scene-mode=hdr,
        // or any of the HDR setting key defined
        List<String> sceneModes = params.getSupportedSceneModes();
        if (sceneModes != null && sceneModes.contains("hdr")) {
            mTransformer.tearDown();
            return false;
        }

        String[] keys = mContext.getResources().getStringArray(R.array.hardware_hdr_keys);
        for (String key : keys) {
            if (params.get(key) != null) {
                mTransformer.tearDown();
                return false;
            }
        }

        return true;
    }

    @Override
    public void onClick(View view) {
        if (view == mBtnOn) {
            mPreviousMode.resetImage();
            mBtnOn.activeImage(DRAWABLE_TAG + "=on");
            mPreviousMode = mBtnOn;
            mContext.setCaptureTransformer(mTransformer);
        } else if (view == mBtnOff) {
            mPreviousMode.resetImage();
            mBtnOff.activeImage(DRAWABLE_TAG + "=off");
            mPreviousMode = mBtnOff;
            mContext.setCaptureTransformer(null);
        }
    }

    public void render() {

    }
}
