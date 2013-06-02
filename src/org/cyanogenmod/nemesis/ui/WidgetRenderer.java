package org.cyanogenmod.nemesis.ui;

import java.util.ArrayList;
import java.util.List;

import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.widgets.WidgetBase;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;

public class WidgetRenderer extends FrameLayout {
    public final static String TAG = "WidgetRenderer";

    private final static float WIDGETS_MARGIN = 80.0f;

    private List<WidgetBase.WidgetContainer> mOpenWidgets;
    private float mTotalWidth;
    private float mSpacing;
    private int mOrientation;
    private float mWidgetDragStartPoint;
    private boolean mIsHidden;

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
        mTotalWidth = WIDGETS_MARGIN;
        mSpacing = getResources().getDimension(R.dimen.widget_spacing);
    }


    /**
     * Rotate the contents of open widgets
     * @param orientation
     */
    public void notifyOrientationChanged(int orientation) {
        mOrientation = orientation;

        for (int i = 0; i < mOpenWidgets.size(); i++) {
            mOpenWidgets.get(i).notifyOrientationChanged(orientation);
        }
    }

    /**
     * Notifies the renderer a widget has been pressed.
     * @param widget The widget that has been pressed
     */
    public void widgetPressed(WidgetBase.WidgetContainer widget) {
        mWidgetDragStartPoint = widget.getFinalX();
    }

    /**
     * Notifies the renderer a widget has been moved. The renderer
     * will then move the other widgets accordingly if needed.
     * @param widget The widget that has been moved
     */
    public void widgetMoved(WidgetBase.WidgetContainer widget) {
        // Don't move widget if it was just a small tap
        if (Math.abs(widget.getX() - mWidgetDragStartPoint) < 40.0f) return;
        if (mOpenWidgets.size() == 0) return;

        boolean isFirst = (mOpenWidgets.get(0) == widget);

        // Check if we overlap the top of a widget
        for (int i = 0; i < mOpenWidgets.size(); i++) {
            WidgetBase.WidgetContainer tested = mOpenWidgets.get(i);

            if (tested == widget) continue;

            if (widget.getX() < tested.getFinalX()+tested.getWidth()) {
                // Don't try to go before the first if we're already it
                if (isFirst && widget.getX()+widget.getWidth() < tested.getFinalX()-tested.getWidth()/2) break;

                // Move the widget in our list
                mOpenWidgets.remove(widget);
                mOpenWidgets.add(i, widget);

                reorderWidgets(widget);
                break;
            }
        }
    }

    /**
     * Reorder the widgets and clamp their position after a widget
     * has been dropped.
     * @param widget
     */
    public void widgetDropped(WidgetBase.WidgetContainer widget) {
        reorderWidgets(null);
    }

    public void reorderWidgets(WidgetBase.WidgetContainer ignore) {
        mTotalWidth = WIDGETS_MARGIN;

        for (int i = 0; i < mOpenWidgets.size(); i++) {
            WidgetBase.WidgetContainer widget = mOpenWidgets.get(i);

            if (widget != ignore)
                widget.setXSmooth(mTotalWidth);

            mTotalWidth += widget.getWidth() + mSpacing;
        }
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

        Log.e(TAG, "Widget opened");
        mOpenWidgets.add(widget);

        // make sure the widget is properly oriented
        widget.notifyOrientationChanged(mOrientation);

        // position it properly
        widget.forceFinalX(mTotalWidth - WIDGETS_MARGIN);
        widget.setX(mTotalWidth - WIDGETS_MARGIN);
        mTotalWidth += widget.getWidth() + mSpacing;
    }

    /**
     * Notifies the renderer a widget has been closed and its
     * space can be reclaimed
     * @param widget Widget closed
     */
    public void widgetClosed(WidgetBase.WidgetContainer widget) {
        mOpenWidgets.remove(widget);

        // reposition all the widgets
        reorderWidgets(null);
    }


    public void notifySidebarSlideStatus(float distance) {
        float finalY = getTranslationY() + distance;
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) getLayoutParams();

        if (finalY > params.bottomMargin)
            finalY = params.bottomMargin;
        else if (finalY < 0)
            finalY = 0;

        setTranslationY(finalY);
    }

    public void notifySidebarSlideClose() {
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) getLayoutParams();
        animate().translationY(params.bottomMargin).setDuration(SideBar.SLIDE_ANIMATION_DURATION_MS).start();
    }

    public void notifySidebarSlideOpen() {
        animate().translationY(0).setDuration(SideBar.SLIDE_ANIMATION_DURATION_MS).start();
    }

    public void hideWidgets() {
        mIsHidden = true;

        for (int i = 0; i < mOpenWidgets.size(); i++) {
            WidgetBase.WidgetContainer widget = mOpenWidgets.get(i);
            widget.animate().translationYBy(widget.getHeight()).setDuration(300).alpha(0)
            .setInterpolator(new AccelerateInterpolator()).start();
        }
    }

    public void restoreWidgets() {
        mIsHidden = false;

        for (int i = 0; i < mOpenWidgets.size(); i++) {
            WidgetBase.WidgetContainer widget = mOpenWidgets.get(i);
            widget.animate().translationYBy(-widget.getHeight()).setDuration(300).alpha(1)
            .setInterpolator(new DecelerateInterpolator()).start();
        }
    }

    public boolean isHidden() {
        return mIsHidden;
    }

    public int getWidgetsCount() {
        return mOpenWidgets.size();
    }

}
