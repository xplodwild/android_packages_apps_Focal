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

package org.cyanogenmod.focal;

import android.hardware.Camera;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import org.cyanogenmod.focal.widgets.*;

import java.util.ArrayList;
import java.util.List;

/**
 * This class holds all the possible widgets of the
 * sidebar. It checks for support prior to adding them
 * effectively in the sidebar.
 */
public class CameraCapabilities {
    private List<WidgetBase> mWidgets;

    /**
     * Default constructor, initializes all the widgets. They will
     * then be sorted by populateSidebar.
     *
     * @param context The CameraActivity context
     */
    public CameraCapabilities(CameraActivity context) {
        mWidgets = new ArrayList<WidgetBase>();
        CameraManager cam = context.getCamManager();

        // Populate the list of widgets.
        // Basically, if we add a new widget, we just put it here.
        // They will populate the sidebar in the same order as here.
        mWidgets.add(new FlashWidget(cam, context));
        mWidgets.add(new WhiteBalanceWidget(cam, context));
        mWidgets.add(new SceneModeWidget(cam, context));
        mWidgets.add(new HdrWidget(cam, context));
        mWidgets.add(new SoftwareHdrWidget(context));
        mWidgets.add(new VideoHdrWidget(cam, context));
        mWidgets.add(new EffectWidget(cam, context));
        mWidgets.add(new ExposureCompensationWidget(cam, context));
        mWidgets.add(new EnhancementsWidget(cam, context));
        mWidgets.add(new AutoExposureWidget(cam, context));
        mWidgets.add(new IsoWidget(cam, context));
        mWidgets.add(new ShutterSpeedWidget(cam, context));
        mWidgets.add(new BurstModeWidget(context));
        mWidgets.add(new TimerModeWidget(context));
        mWidgets.add(new VideoFrWidget(cam, context));
        mWidgets.add(new SettingsWidget(context, this));
    }

    /**
     * @return The list of currently enabled/capable widgets
     */
    public List<WidgetBase> getWidgets() {
        return mWidgets;
    }

    /**
     * Populates the sidebar (through sideBarContainer) with the widgets actually
     * compatible with the device.
     *
     * @param params           The Camera parameters returned from the HAL for compatibility check
     * @param sideBarContainer The side bar layout that will contain all the toggle buttons
     * @param homeContainer    The viewgroup containing home shortcuts
     * @param widgetsContainer The container of the final rendered widgets
     */
    public void populateSidebar(Camera.Parameters params, ViewGroup sideBarContainer,
                                ViewGroup homeContainer, ViewGroup widgetsContainer) {
        List<WidgetBase> unsupported = new ArrayList<WidgetBase>();

        for (int i = 0; i < mWidgets.size(); i++) {
            final WidgetBase widget = mWidgets.get(i);

            // Add the widget to the sidebar if it is supported by the device.
            // The compatibility is determined by widgets themselves.
            if (widget.isSupported(params)) {
                widgetsContainer.addView(widget.getWidget());
                homeContainer.addView(widget.getShortcutButton());
                sideBarContainer.addView(widget.getToggleButton());

                // If the widget is pinned, show it on the main screen, otherwise in the bar
                if (SettingsStorage.getShortcutSetting(widget.getWidget().getContext(),
                        widget.getClass().getCanonicalName())) {
                    widget.getShortcutButton().setVisibility(View.VISIBLE);
                    widget.getToggleButton().setVisibility(View.GONE);
                } else {
                    widget.getShortcutButton().setVisibility(View.GONE);
                    widget.getToggleButton().setVisibility(View.VISIBLE);
                }
            } else {
                unsupported.add(widget);
            }
        }

        for (int i = 0; i < unsupported.size(); i++) {
            mWidgets.remove(unsupported.get(i));
        }
    }
}
