/*
 * Copyright (C) 2013 Guillaume Lesniak
 * Copyright (C) 2012 Alex Curran
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

package org.cyanogenmod.focal.ui.showcase;

import android.app.Activity;
import android.view.View;

public class ShowcaseViewBuilder {
    private final ShowcaseView showcaseView;

    public ShowcaseViewBuilder(Activity activity) {
        this.showcaseView = new ShowcaseView(activity);
    }

    public ShowcaseViewBuilder(ShowcaseView showcaseView) {
        this.showcaseView = showcaseView;
    }

    public ShowcaseViewBuilder(Activity activity, int showcaseLayoutViewId) {
        this.showcaseView = (ShowcaseView) activity.getLayoutInflater()
                .inflate(showcaseLayoutViewId, null);
    }

    public ShowcaseViewBuilder setShowcaseView(View view) {
        showcaseView.setShowcaseView(view);
        return this;
    }

    public ShowcaseViewBuilder setShowcasePosition(float x, float y) {
        showcaseView.setShowcasePosition(x, y);
        return this;
    }

    public ShowcaseViewBuilder setShowcaseItem(int itemType,
            int actionItemId, Activity activity) {
        showcaseView.setShowcaseItem(itemType, actionItemId, activity);
        return this;
    }

    public ShowcaseViewBuilder setShowcaseIndicatorScale(float scale) {
        showcaseView.setShowcaseIndicatorScale(scale);
        return this;
    }

    public ShowcaseViewBuilder overrideButtonClick(View.OnClickListener listener) {
        showcaseView.overrideButtonClick(listener);
        return this;
    }

    public ShowcaseViewBuilder animateGesture(float offsetStartX, float offsetStartY,
            float offsetEndX, float offsetEndY) {
        showcaseView.animateGesture(offsetStartX, offsetStartY, offsetEndX, offsetEndY);
        return this;
    }

    public ShowcaseViewBuilder setTextColors(int titleTextColor, int detailTextColor) {
        showcaseView.setTextColors(titleTextColor, detailTextColor);
        return this;
    }

    public ShowcaseViewBuilder setText(String titleText, String subText) {
        showcaseView.setText(titleText, subText);
        return this;
    }

    public ShowcaseViewBuilder setText(int titleText, int subText) {
        showcaseView.setText(titleText, subText);
        return this;
    }

    public ShowcaseViewBuilder pointTo(View view) {
        showcaseView.pointTo(view);
        return this;
    }

    public ShowcaseViewBuilder pointTo(float x, float y) {
        showcaseView.pointTo(x, y);
        return this;
    }

    public ShowcaseView build() {
        return showcaseView;
    }
}
