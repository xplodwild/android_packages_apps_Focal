package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ScrollView;

public class SideBar extends ScrollView {

    public SideBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }
    
    public SideBar(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
    
    public SideBar(Context context) {
        super(context);
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
        
    }
    
    /**
     * Smoothly close the sidebar
     */
    public void slideClose() {
        
    }
    
    /**
     * Smoothly open the sidebar
     */
    public void slideOpen() {
        
    }

}
