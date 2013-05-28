package org.cyanogenmod.nemesis.widgets;

import android.content.Context;

import org.cyanogenmod.nemesis.R;

/**
 * White Balance Widget, manages the wb settings
 */
public class WhiteBalanceWidget extends SimpleToggleWidget {

    public WhiteBalanceWidget(Context context) {
        super(context, "whitebalance", R.drawable.ic_widget_wb_auto);

        addValue("auto", R.drawable.ic_widget_wb_auto);
        addValue("cloudy", R.drawable.ic_widget_wb_cloudy);
        addValue("incandescent", R.drawable.ic_widget_wb_incandescent);
        addValue("fluorescent", R.drawable.ic_widget_wb_fluorescent);
        addValue("daylight", R.drawable.ic_widget_wb_daylight);
        addValue("cloudy-daylight", R.drawable.ic_widget_wb_cloudy_daylight);
    }
}
