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

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.hardware.Camera;
import android.media.CamcorderProfile;
import android.view.View;
import android.view.ViewGroup;
import android.widget.NumberPicker;

import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.CameraCapabilities;
import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.SettingsStorage;
import org.cyanogenmod.focal.SnapshotManager;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Overflow settings widget
 */
public class SettingsWidget extends WidgetBase {
    private static final String TAG = "SettingsWidget";

    private static final String DRAWABLE_KEY_EXPO_RING = "_Nemesis_ExposureRing=true";
    private static final String DRAWABLE_KEY_AUTO_ENHANCE = "_Nemesis_AutoEnhance=true";
    private static final String DRAWABLE_KEY_RULE_OF_THIRDS = "_Nemesis_RuleOfThirds=true";
    private static final String KEY_SHOW_EXPOSURE_RING = "ShowExposureRing";
    private static final String KEY_ENABLE_AUTO_ENHANCE = "AutoEnhanceEnabled";
    private static final String KEY_ENABLE_RULE_OF_THIRDS = "RuleOfThirdsEnabled";
    private WidgetOptionButton mResolutionButton;
    private WidgetOptionButton mToggleExposureRing;
    private WidgetOptionButton mToggleAutoEnhancer;
    private WidgetOptionButton mToggleWidgetsButton;
    private WidgetOptionButton mToggleRuleOfThirds;
    private CameraActivity mContext;
    private CameraCapabilities mCapabilities;
    private List<String> mResolutionsName;
    private List<String> mVideoResolutions;
    private List<Camera.Size> mResolutions;
    private AlertDialog mResolutionDialog;
    private AlertDialog mWidgetsDialog;
    private NumberPicker mNumberPicker;
    private int mInitialOrientation = -1;
    private int mOrientation;

    private final Comparator<? super Camera.Size> mResolutionsSorter =
            new Comparator<Camera.Size>() {
        @Override
        public int compare(Camera.Size size, Camera.Size size2) {
            if (size.width * size.height > size2.width * size2.height) {
                return -1;
            } else {
                return 1;
            }
        }
    };

    private View.OnClickListener mExpoRingClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mContext.setExposureRingVisible(!mContext.isExposureRingVisible());
            if (mContext.isExposureRingVisible()) {
                mToggleExposureRing.activeImage(DRAWABLE_KEY_EXPO_RING);
            } else {
                mToggleExposureRing.resetImage();
            }

            SettingsStorage.storeAppSetting(mContext, KEY_SHOW_EXPOSURE_RING,
                    mContext.isExposureRingVisible() ? "1" : "0");
        }
    };

    private View.OnClickListener mRuleOfThirdsClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            View rot = mContext.findViewById(R.id.rule_of_thirds);

            if (rot.getVisibility() == View.GONE) {
                mToggleRuleOfThirds.activeImage(DRAWABLE_KEY_RULE_OF_THIRDS);
                rot.setVisibility(View.VISIBLE);
            } else {
                mToggleRuleOfThirds.resetImage();
                rot.setVisibility(View.GONE);
            }

            SettingsStorage.storeAppSetting(mContext, KEY_ENABLE_RULE_OF_THIRDS,
                    rot.getVisibility() == View.GONE ? "0" : "1");
        }
    };

    private View.OnClickListener mAutoEnhanceClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            SnapshotManager snapMan = mContext.getSnapManager();
            snapMan.setAutoEnhance(!snapMan.getAutoEnhance());

            if (snapMan.getAutoEnhance()) {
                mToggleAutoEnhancer.activeImage(DRAWABLE_KEY_AUTO_ENHANCE);
            } else {
                mToggleAutoEnhancer.resetImage();
            }

            SettingsStorage.storeAppSetting(mContext, KEY_ENABLE_AUTO_ENHANCE,
                    snapMan.getAutoEnhance() ? "1" : "0");
        }
    };

    private View.OnClickListener mResolutionClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mInitialOrientation = -1;

            mNumberPicker = new NumberPicker(mContext);
            String[] names = new String[mResolutionsName.size()];
            mResolutionsName.toArray(names);
            if (names.length > 0) {
                mNumberPicker.setDisplayedValues(names);

                mNumberPicker.setMinValue(0);
                mNumberPicker.setMaxValue(names.length - 1);
            } else {
                mNumberPicker.setMinValue(0);
                mNumberPicker.setMaxValue(0);
            }
            mNumberPicker.setWrapSelectorWheel(false);
            mNumberPicker.setDescendantFocusability(NumberPicker.FOCUS_BLOCK_DESCENDANTS);
            mNumberPicker.setFormatter(new NumberPicker.Formatter() {
                @Override
                public String format(int i) {
                    return mResolutionsName.get(i);
                }
            });

            if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO){
                // TODO set correct menu selection also for video
                String actualSz = getActualProfileResolution();
                if (mVideoResolutions != null) {
                    for (int i = 0; i < mVideoResolutions.size(); i++) {
                        if (mVideoResolutions.get(i).equals(actualSz)) {
                            mNumberPicker.setValue(i);
                            break;
                        }
                    }
                }
            } else {
                Camera.Size actualSz = mCamManager.getParameters().getPictureSize();
                if (mResolutions != null) {
                    for (int i = 0; i < mResolutions.size(); i++) {
                        if (mResolutions.get(i).equals(actualSz)) {
                            mNumberPicker.setValue(i);
                            break;
                        }
                    }
                }
            }

            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setView(mNumberPicker);
            builder.setTitle(null);
            builder.setCancelable(false);
            builder.setPositiveButton(mContext.getString(R.string.ok),
                    new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    mInitialOrientation = -1;

                    if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO
                            || CameraActivity.getCameraMode()
                            == CameraActivity.CAMERA_MODE_PICSPHERE) {
                        // Set picture size
                        Camera.Size size = mResolutions.get(mNumberPicker.getValue());
                        // TODO: only one method
                        mCamManager.setPictureSize(""+size.width+"x"+size.height);

                        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
                            SettingsStorage.storeCameraSetting(mContext, mCamManager
                                    .getCurrentFacing(), "picture-size",
                                    ""+size.width+"x"+size.height);
                        } else {
                            SettingsStorage.storeCameraSetting(mContext,
                                    mCamManager.getCurrentFacing(), "picsphere-picture-size",
                                    ""+size.width+"x"+size.height);
                        }
                    } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
                        // Set video size
                        String size = mVideoResolutions.get(mNumberPicker.getValue());
                        applyVideoResolution(size);
                        
                        String[] splat = size.split("x");
                        int width = Integer.parseInt(splat[0]);
                        int height = Integer.parseInt(splat[1]);

                        SettingsStorage.storeCameraSetting(mContext, mCamManager.getCurrentFacing(),
                                "video-size", ""+width+"x"+height);
                    }
                }
            });

            mResolutionDialog = builder.create();
            mResolutionDialog.show();
            ((ViewGroup)mNumberPicker.getParent().getParent().getParent())
                    .animate().rotation(mOrientation).setDuration(300).start();
        }
    };

    public SettingsWidget(CameraActivity context, CameraCapabilities capabilities) {
        super(context.getCamManager(), context, R.drawable.ic_widget_settings);
        mContext = context;
        mCapabilities = capabilities;

        CameraManager cam = context.getCamManager();

        getToggleButton().setHintText(R.string.widget_settings);

        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO
                || CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PICSPHERE) {
            // Get the available photo size. Unlike AOSP app, we don't
            // store manually each resolution in an XML, but we calculate it directly
            // from the width and height of the picture size.
            mResolutions = cam.getParameters().getSupportedPictureSizes();
            mResolutionsName = new ArrayList<String>();

            DecimalFormat df = new DecimalFormat();
            df.setMaximumFractionDigits(1);
            df.setMinimumFractionDigits(0);
            df.setDecimalSeparatorAlwaysShown(false);

            Collections.sort(mResolutions, mResolutionsSorter);

            for (Camera.Size size : mResolutions) {
                float megapixels = size.width * size.height / 1000000.0f;
                mResolutionsName.add(df.format(megapixels) +
                        "MP (" + size.width + "x" + size.height + ")");
            }

            // Restore picture size if we have any
            String resolution = "";
            if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PHOTO) {
                resolution = SettingsStorage.getCameraSetting(context,
                        mCamManager.getCurrentFacing(), "picture-size", ""+mResolutions
                        .get(0).width+"x"+mResolutions.get(0).height);
            } else {
                resolution = SettingsStorage.getCameraSetting(context,
                        mCamManager.getCurrentFacing(), "picsphere-picture-size", "640x480");
            }
            mCamManager.setPictureSize(resolution);
        } else if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            mResolutions = cam.getParameters().getSupportedVideoSizes();
            mResolutionsName = new ArrayList<String>();
            mVideoResolutions = new ArrayList<String>();

            if (mResolutions != null) {
                // We support a fixed set of video resolutions (pretty much like AOSP)
                // because we need to take the CamcorderProfile (media_profiles.xml), which
                // has a fixed set of values...
                for (Camera.Size size : mResolutions) {
                    if (size.width == 1920 && size.height == 1080
                            || size.width == 1920 && size.height == 1088) {
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
            } else if (mResolutions == null || mResolutions.size() == 0) {
                // We detected no compatible video resolution! We add default ones.
                mResolutionsName.add(mContext.getString(R.string.video_res_1080p));
                mVideoResolutions.add("1920x1080");
                mResolutionsName.add(mContext.getString(R.string.video_res_720p));
                mVideoResolutions.add("1280x720");
                mResolutionsName.add(mContext.getString(R.string.video_res_480p));
                mVideoResolutions.add("720x480");
            }

            // TODO: Restore video size if we have any
            String resolution = SettingsStorage.getCameraSetting(context,
                        mCamManager.getCurrentFacing(), "video-size", mVideoResolutions.get(0));
            applyVideoResolution(resolution);
        }

        mResolutionButton = new WidgetOptionButton(
                R.drawable.ic_widget_settings_resolution, context);
        mResolutionButton.setOnClickListener(mResolutionClickListener);
        mResolutionButton.setHintText(mContext.getString(R.string.widget_settings_picture_size));
        addViewToContainer(mResolutionButton);

        if (mCamManager.isExposureAreaSupported()
                && CameraActivity.getCameraMode() != CameraActivity.CAMERA_MODE_PICSPHERE) {
            mToggleExposureRing = new WidgetOptionButton(
                    R.drawable.ic_widget_settings_exposurering, context);
            mToggleExposureRing.setHintText(mContext.getString(
                    R.string.widget_settings_exposure_ring));
            mToggleExposureRing.setOnClickListener(mExpoRingClickListener);

            // Restore exposure ring state
            if (SettingsStorage.getAppSetting(mContext, KEY_SHOW_EXPOSURE_RING, "0").equals("1")) {
                mContext.setExposureRingVisible(true);
                mToggleExposureRing.activeImage(DRAWABLE_KEY_EXPO_RING);
            } else {
                mContext.setExposureRingVisible(false);
            }
            addViewToContainer(mToggleExposureRing);
        }

        // Toggle auto enhancer
        if (CameraActivity.getCameraMode() != CameraActivity.CAMERA_MODE_PICSPHERE) {
            mToggleAutoEnhancer = new WidgetOptionButton(R.drawable.ic_enhancing, context);
            mToggleAutoEnhancer.setOnClickListener(mAutoEnhanceClickListener);
            mToggleAutoEnhancer.setHintText(mContext.getString(
                    R.string.widget_settings_autoenhance));

            // Restore auto enhancer state
            if (SettingsStorage.getAppSetting(mContext, KEY_ENABLE_AUTO_ENHANCE, "0").equals("1")) {
                mContext.getSnapManager().setAutoEnhance(true);
                mToggleAutoEnhancer.activeImage(DRAWABLE_KEY_AUTO_ENHANCE);
            } else {
                if (mContext.getSnapManager() != null) {
                    mContext.getSnapManager().setAutoEnhance(false);
                }
            }

            addViewToContainer(mToggleAutoEnhancer);

            // Toggle rule of thirds
            mToggleRuleOfThirds = new WidgetOptionButton(
                    R.drawable.ic_widget_settings_rulethirds, context);
            mToggleRuleOfThirds.setOnClickListener(mRuleOfThirdsClickListener);
            mToggleRuleOfThirds.setHintText(mContext.getString(
                    R.string.widget_settings_ruleofthirds));

            // Restore rule of thirds visibility state
            if (SettingsStorage.getAppSetting(mContext,
                    KEY_ENABLE_RULE_OF_THIRDS, "0").equals("1")) {
                mContext.findViewById(R.id.rule_of_thirds).setVisibility(View.VISIBLE);
                mToggleRuleOfThirds.activeImage(KEY_ENABLE_RULE_OF_THIRDS);
            } else {
                mContext.findViewById(R.id.rule_of_thirds).setVisibility(View.GONE);
                mToggleRuleOfThirds.resetImage();
            }

            addViewToContainer(mToggleRuleOfThirds);
        }

        // Choose widgets to appear
        mToggleWidgetsButton = new WidgetOptionButton(
                R.drawable.ic_widget_settings_widgets, context);
        mToggleWidgetsButton.setHintText(mContext.getString(
                R.string.widget_settings_choose_widgets_button));
        mToggleWidgetsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openWidgetsToggleDialog();
            }
        });
        addViewToContainer(mToggleWidgetsButton);
    }

    @Override
    public boolean isSupported(Camera.Parameters params) {
        int mode = CameraActivity.getCameraMode();

        return (mode == CameraActivity.CAMERA_MODE_PHOTO || mode == CameraActivity.CAMERA_MODE_VIDEO
                || mode == CameraActivity.CAMERA_MODE_PICSPHERE);
    }

    @Override
    public void notifyOrientationChanged(int orientation) {
        if (mInitialOrientation == -1) mInitialOrientation = orientation;
        if (mOrientation == orientation) return;

        mOrientation = orientation;
        if (mResolutionDialog != null) {
            ((ViewGroup) mNumberPicker.getParent().getParent().getParent().getParent())
                    .animate().rotation(orientation - mInitialOrientation).setDuration(300).start();
        }

        if (mWidgetsDialog != null) {
            ((ViewGroup) mWidgetsDialog.getListView().getParent()
                    .getParent().getParent().getParent()).animate().rotation(orientation
                    - mInitialOrientation).setDuration(300).start();
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

    private String getActualProfileResolution() {
        CamcorderProfile actualProfile = mContext.getSnapManager().getVideoProfile();
        return "" + actualProfile.videoFrameWidth + "x" + actualProfile.videoFrameHeight;
    }

    private void openWidgetsToggleDialog() {
        final List<WidgetBase> widgets = mCapabilities.getWidgets();
        List<String> widgetsName = new ArrayList<String>();

        // Construct a list of widgets
        for (WidgetBase widget : widgets) {
            if (widget.getClass().getName().contains("SettingsWidget")) continue;
            widgetsName.add(widget.getToggleButton().getHintText());
        }

        CharSequence[] items = widgetsName.toArray(new CharSequence[widgetsName.size()]);

        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mContext.getString(R.string.widget_settings_choose_widgets));
        builder.setMultiChoiceItems(items, null,
                new DialogInterface.OnMultiChoiceClickListener() {
                    // indexSelected contains the index of item (of which checkbox checked)
                    @Override
                    public void onClick(DialogInterface dialog, int indexSelected,
                                        boolean isChecked) {
                        widgets.get(indexSelected).setHidden(!isChecked);
                    }
                })
                // Set the action buttons
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        // Store the widget visibility status in SharedPreferences, using the
                        // widget class name as key
                        for (WidgetBase widget : widgets) {
                            if (widget.getClass().getName().contains("SettingsWidget")) {
                                continue;
                            }

                            SettingsStorage.storeVisibilitySetting(mContext,
                                    widget.getClass().getName(), !widget.isHidden());
                        }
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        // Restore visibility status from storage
                        for (WidgetBase widget : widgets) {
                            if (widget.getClass().getName().contains("SettingsWidget")) {
                                continue;
                            }

                            widget.setHidden(!SettingsStorage.getVisibilitySetting(
                                    mContext, widget.getClass().getName()));
                        }
                    }
                });


        mWidgetsDialog = builder.create();
        mWidgetsDialog.show();
        for (int i = 0; i < widgets.size(); i++) {
            mWidgetsDialog.getListView().setItemChecked(i, !widgets.get(i).isHidden());
        }
        ((ViewGroup)mWidgetsDialog.getListView().getParent().getParent().getParent())
                .animate().rotation(mOrientation).setDuration(300).start();
    }
}
