package org.cyanogenmod.nemesis.widgets;

import android.content.Context;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

public class EffectWidget extends SimpleToggleWidget {

    public EffectWidget(CameraManager cam, Context context) {
        super(cam, context, "effect", R.drawable.ic_widget_effect);

        addValue("none", R.drawable.ic_widget_effect_none);
        addValue("mono", R.drawable.ic_widget_effect_mono);
        addValue("negative", R.drawable.ic_widget_effect_negative);
        addValue("solarize", R.drawable.ic_widget_effect_solarize);
        addValue("sepia", R.drawable.ic_widget_effect_sepia);
        addValue("posterize", R.drawable.ic_widget_effect_posterize);

    }
}
