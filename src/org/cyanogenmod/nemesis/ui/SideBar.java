package org.cyanogenmod.nemesis.ui;

import org.cyanogenmod.nemesis.R;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ScrollView;

public class SideBar extends ScrollView {
    private final static String TAG = "SideBar";
    private final static int SLIDE_ANIMATION_DURATION_MS = 300;

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
    }

    /**
     * Slides the sidebar off the provided distance, and clamp it at either
     * sides of the screen (out/in)
     * @param distance The distance to translate the bar
     */
    public void slide(float distance) {
        float finalY = this.getTranslationY() + distance;

        if (finalY > this.getHeight())
            finalY = this.getHeight();
        else if (finalY < 0)
            finalY = 0;

        this.setTranslationY(finalY);
    }

    /**
     * Clamp the sliding position of the bar. Basically, if the
     * bar is half-visible, it will animate it in its visible position,
     * and vice-versa.
     */
    public void clampSliding() {
        if (this.getTranslationY() > this.getHeight()/2)
            slideClose();
        else
            slideOpen();
    }

    /**
     * Smoothly close the sidebar
     */
    public void slideClose() {
        this.animate().translationY(this.getHeight())
        .setDuration(SLIDE_ANIMATION_DURATION_MS).start();
    }

    /**
     * Smoothly open the sidebar
     */
    public void slideOpen() {
        this.animate().translationY(0)
        .setDuration(SLIDE_ANIMATION_DURATION_MS).start();;
    }

}
