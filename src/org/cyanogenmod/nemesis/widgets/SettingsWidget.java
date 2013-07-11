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
import android.content.DialogInterface;
import android.hardware.Camera;
import android.media.CamcorderProfile;
import android.view.View;
import android.view.ViewGroup;
import android.widget.NumberPicker;

import org.cyanogenmod.nemesis.CameraActivity;
import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.SettingsStorage;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;

/**
 * Overflow settings widget
 */
public class SettingsWidget extends WidgetBase {
    private static final String DRAWABLE_KEY_EXPO_RING = "_Nemesis_ExposureRing=true";
    private static final String KEY_SHOW_EXPOSURE_RING = "ShowExposureRing";
    private WidgetOptionButton mResolutionButton;
    private WidgetOptionButton mToggleExposureRing;
    private CameraActivity mContext;
    private List<String> mResolutionsName;
    private List<String> mVideoResolutions;
    private List<Camera.Size> mResolutions;
    private AlertDialog mPickerDialog;
    private NumberPicker mNumberPicker;
    private int mInitialOrientation = -1;
    private int mOrientation;

    private View.OnClickListener mExpoRingClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mContext.setExposureRingVisible(!mContext.isExposureRingVisible());
            if (mContext.isExposureRingVisible()) {
                mToggleExposureRing.setActiveDrawable(DRAWABLE_KEY_EXPO_RING);
            } else {
                mToggleExposureRing.resetImage();
            }

            SettingsStorage.storeAppSetting(mContext, KEY_SHOW_EXPOSURE_RING,
                    mContext.isExposureRingVisible() ? "1" : "0");
        }
    };

    private View.OnClickListener mResolutionClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mInitialOrientation = -1;

            Camera.Size actualSz = mCamManager.getParameters().getPictureSize();
            mNumberPicker = new NumberPicker(mContext);
            String[] names = new String[mResolutionsName.size()];
            mResolutionsName.toArray(names);
            mNumberPicker.setDisplayedValues(names);

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

            for (int i = 0; i < mResolutions.size(); i++) {
                if (mResolutions.get(i).equals(actualSz)) {
                    mNumberPicker.setValue(i);
                    break;
                }
            }

            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setView(mNumberPicker);
            builder.setTitle(null);
            builder.setCancelable(false);
            builder.setPositiveButton(mContext.getString(R.string.OK), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    mInitialOrientation = -1;
                    Camera.Size size = mResolutions.get(mNumberPicker.getValue());

                    if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
                        // Set picture size
                        mCamManager.setPictureSize(size);
                        SettingsStorage.storeCameraSetting(mContext, mCamManager.getCurrentFacing(),
                                "picture-size", ""+size.width+"x"+size.height);
                    } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
                        // Set video size
                        applyVideoResolution(mVideoResolutions.get(mNumberPicker.getValue()));
                        SettingsStorage.storeCameraSetting(mContext, mCamManager.getCurrentFacing(),
                                "video-size", ""+size.width+"x"+size.height);
                    }
                }
            });

            mPickerDialog = builder.create();
            mPickerDialog.show();
            ((ViewGroup)mNumberPicker.getParent().getParent().getParent())
                    .animate().rotation(mOrientation).setDuration(300).start();
        }
    };

    public SettingsWidget(CameraActivity context) {
        super(context.getCamManager(), context, R.drawable.ic_widget_settings);
        mContext = context;
        CameraManager cam = context.getCamManager();

        getToggleButton().setHintText(R.string.widget_settings);

        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
            // Get the available photo size. Unlike AOSP app, we don't
            // store manually each resolution in an XML, but we calculate it directly
            // from the width and height of the picture size.
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

            // Restore picture size if we have any
            String resolution = SettingsStorage.getCameraSetting(context, mCamManager.getCurrentFacing(),
                    "picture-size", ""+mResolutions.get(0).width+"x"+mResolutions.get(0).height);
            mCamManager.setPictureSize(resolution);
        } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            mResolutions = cam.getParameters().getSupportedVideoSizes();

            // We support a fixed set of video resolutions (pretty much like AOSP)
            // because we need to take the CamcorderProfile (media_profiles.xml), which
            // has a fixed set of values...
            mResolutionsName = new ArrayList<String>();
            mVideoResolutions = new ArrayList<String>();
            for (Camera.Size size : mResolutions) {
                if (size.width == 1920 && size.height == 1080) {
                    mResolutionsName.add(mContext.getString(R.string.video_res_1080p));
                    mVideoResolutions.add("1920x1080");
                } else if (size.width == 1280 && size.height == 720) {
                    mResolutionsName.add(mContext.getString(R.string.video_res_720p));
                    mVideoResolutions.add("1280x720");
                } else if (size.width == 720 && size.height == 480) {
                    mResolutionsName.add(mContext.getString(R.string.video_res_480p));
                    mVideoResolutions.add("720x480");
                } else if (size.width == 352 && size.height == 288) {
                    mResolutionsName.add(mContext.getString(R.string.video_res_mms));
                    mVideoResolutions.add("352x288");
                }
            }
        }


        mResolutionButton = new WidgetOptionButton(R.drawable.ic_widget_placeholder, context);
        mResolutionButton.setOnClickListener(mResolutionClickListener);
        addViewToContainer(mResolutionButton);

        mToggleExposureRing = new WidgetOptionButton(R.drawable.ic_widget_placeholder, context);
        mToggleExposureRing.setOnClickListener(mExpoRingClickListener);

        // Restore exposure ring state
        if (SettingsStorage.getAppSetting(mContext, KEY_SHOW_EXPOSURE_RING, "1").equals("1")) {
            mContext.setExposureRingVisible(true);
        } else {
            mContext.setExposureRingVisible(false);
        }

        if (mContext.isExposureRingVisible()) {
            mToggleExposureRing.setActiveDrawable(DRAWABLE_KEY_EXPO_RING);
        }
        addViewToContainer(mToggleExposureRing);
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

    private void applyVideoResolution(String resolution) {
        if (resolution.equals("1920x1080")) {
            mContext.getSnapManager().setVideoProfile(
                    CamcorderProfile.get(CamcorderProfile.QUALITY_1080P));
        } else if (resolution.equals("1280x720")) {
            mContext.getSnapManager().setVideoProfile(
                    CamcorderProfile.get(CamcorderProfile.QUALITY_720P));
        } else if (resolution.equals("720x480")) {
            mContext.getSnapManager().setVideoProfile(
                    CamcorderProfile.get(CamcorderProfile.QUALITY_480P));
        } else if (resolution.equals("352x288")) {
            mContext.getSnapManager().setVideoProfile(
                    CamcorderProfile.get(CamcorderProfile.QUALITY_CIF));
        }
    }

}
