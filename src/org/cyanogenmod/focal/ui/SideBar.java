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

package org.cyanogenmod.focal.ui;

import android.content.Context;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.HorizontalScrollView;
import android.widget.ScrollView;

import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.CameraCapabilities;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.widgets.WidgetBase;

public class SideBar extends ScrollView {
    public final static String TAG = "SideBar";
    public final static int SLIDE_ANIMATION_DURATION_MS = 300;
    private final static float BAR_MARGIN = 0;
    private CameraCapabilities mCapabilities;
    private ViewGroup mToggleContainer;
    private boolean mIsOpen;


    public SideBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    public SideBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public SideBar(Context context) {
        super(context);
        initialize();
    }

    /**
     * Setup the bar
     */
    private void initialize() {
        this.setBackgroundColor(getResources().getColor(R.color.widget_background));
        mIsOpen = true;
    }


    /**
     * Check the capabilities of the device, and populate the sidebar
     * with the toggle buttons.
     */
    public void checkCapabilities(CameraActivity activity, ViewGroup widgetsContainer) {
        mToggleContainer = (ViewGroup) this.getChildAt(0);
        ViewGroup shortcutsContainer = (ViewGroup) activity.findViewById(R.id.shortcuts_container);

        if (mCapabilities != null) {
            mToggleContainer.removeAllViews();
            shortcutsContainer.removeAllViews();
            mCapabilities = null;
        }

        mCapabilities = new CameraCapabilities(activity);
        Camera.Parameters params = activity.getCamManager().getParameters();

        if (params != null) {
            mCapabilities.populateSidebar(params, mToggleContainer,
                    shortcutsContainer, widgetsContainer);
        } else {
            Log.e(TAG, "Parameters were null when capabilities were checked");
        }
    }

    /**
     * Notify the bar of the rotation
     *
     * @param target Target orientation
     */
    public void notifyOrientationChanged(float target) {
        mToggleContainer = (ViewGroup) this.getChildAt(0);
        int buttonsCount = mToggleContainer.getChildCount();

        for (int i = 0; i < buttonsCount; i++) {
            View child = mToggleContainer.getChildAt(i);

            child.animate().rotation(target)
                    .setDuration(200).setInterpolator(new DecelerateInterpolator()).start();
        }
    }

    /**
     * Slides the sidebar off the provided distance, and clamp it at either
     * sides of the screen (out/in)
     *
     * @param distance The distance to translate the bar
     */
    public void slide(float distance) {
        float finalX = this.getTranslationX() + distance;

        if (finalX > 0)
            finalX = 0;
        else if (finalX < -getWidth())
            finalX = -getWidth();

        this.setTranslationX(finalX);
    }

    /**
     * Clamp the sliding position of the bar. Basically, if the
     * bar is half-visible, it will animate it in its visible position,
     * and vice-versa.
     */
    public void clampSliding() {
        if (this.getTranslationX() < 0) {
            slideClose();
        } else {
            slideOpen();
        }
    }

    /**
     * @return Whether or not the sidebar is open
     */
    public boolean isOpen() {
        return mIsOpen;
    }

    /**
     * Smoothly close the sidebar
     */
    public void slideClose() {
        this.animate().translationX(-this.getWidth())
                .setDuration(SLIDE_ANIMATION_DURATION_MS).start();

        mIsOpen = false;
    }

    /**
     * Smoothly open the sidebar
     */
    public void slideOpen() {
        this.animate().translationX(0)
                .setDuration(SLIDE_ANIMATION_DURATION_MS).start();

        mIsOpen = true;
    }
}
