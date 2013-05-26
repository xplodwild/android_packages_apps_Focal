package org.cyanogenmod.nemesis.ui;

import java.util.ArrayList;
import java.util.List;

import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.widgets.WidgetBase;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;

public class WidgetRenderer extends FrameLayout {
    public final static String TAG = "WidgetRenderer";

    private List<WidgetBase.WidgetContainer> mOpenWidgets;
    private float mTotalWidth;
    private float mSpacing;

    public WidgetRenderer(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    public WidgetRenderer(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public WidgetRenderer(Context context) {
        super(context);
        initialize();
    }

    private void initialize() {
        mOpenWidgets = new ArrayList<WidgetBase.WidgetContainer>();
        mTotalWidth = 0.0f;
        mSpacing = getResources().getDimension(R.dimen.widget_spacing);
    }
    

    /**
     * Rotate the contents of open widgets
     * @param orientation
     */
    public void notifyOrientationChanged(int orientation) {
        for (int i = 0; i < mOpenWidgets.size(); i++) {
            mOpenWidgets.get(i).notifyOrientationChanged(orientation);
        }
    }
    
    /**
     * Notifies the renderer a widget has been moved. The renderer
     * will then move the other widgets accordingly if needed.
     * @param widget The widget that has been moved
     */
    public void notifyWidgetMoved(WidgetBase.WidgetContainer widget) {
        
    }

    /**
     * Notifies the renderer a widget has been opened and needs to be
     * positioned.
     * @param widget Widget opened
     */
    public void widgetOpened(final WidgetBase.WidgetContainer widget) {
        if (mOpenWidgets.contains(widget)) {
            Log.e(TAG, "Widget was already rendered!");
            return;
        }

        mOpenWidgets.add(widget);
        widget.setX(getFreePosition());
        mTotalWidth += widget.getWidth() + mSpacing;
    }

    /**
     * Notifies the renderer a widget has been closed and its
     * space can be reclaimed
     * @param widget Widget closed
     */
    public void widgetClosed(WidgetBase.WidgetContainer widget) {
        mOpenWidgets.remove(widget);
        mTotalWidth -= widget.getWidth() + mSpacing;
    }

    /**
     * Computes the X position of the first free space at the bottom/right
     * of the list of open widgets.
     * @return X position for the widget
     */
    public float getFreePosition() {
        return mTotalWidth;
    }

}
