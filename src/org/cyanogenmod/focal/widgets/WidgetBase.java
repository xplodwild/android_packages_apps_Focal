/*
 * Copyright (C) 2013 The CyanogenMod Project
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

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

import org.cyanogenmod.focal.BitmapFilter;
import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.CameraManager;
import org.cyanogenmod.focal.R;
import org.cyanogenmod.focal.SettingsStorage;
import org.cyanogenmod.focal.Util;
import org.cyanogenmod.focal.ui.CenteredSeekBar;
import org.cyanogenmod.focal.ui.SideBar;
import org.cyanogenmod.focal.ui.WidgetRenderer;

/**
 * Base class for settings widget. Each setting widget
 * will extend this class.
 */
public abstract class WidgetBase {
    public final static String TAG = "WidgetBase";

    protected CameraManager mCamManager;
    private GestureDetector mGestureDetector;
    protected WidgetToggleButton mToggleButton;
    protected WidgetContainer mWidget;
    private int mWidgetMaxWidth;
    private boolean mIsOpen;

    private final static int WIDGET_FADE_DURATION_MS = 200;
    private final static int WIDGET_ANIM_TRANSLATE = 80;

    public WidgetBase(CameraManager cam, Context context, int iconResId) {
        mCamManager = cam;
        mWidget = new WidgetContainer(context);
        mToggleButton = new WidgetToggleButton(context);
        mGestureDetector = new GestureDetector(context, new WidgetFlingGestureListener());
        mIsOpen = false;

        // Setup the container
        mWidget.setColumnCount(0);
        mWidget.setRowCount(1);
        mWidget.setAlpha(0.0f);

        // Setup the toggle button
        mToggleButton.setImageResource(iconResId);

        mWidgetMaxWidth = context.getResources().getInteger(R.integer.widget_default_max_width);

        setHidden(!SettingsStorage.getVisibilitySetting(
                context, this.getClass().getName()));
    }

    /**
     * This method returns whether or not the widget settings are
     * supported by the camera HAL or not.
     * <p/>
     * This method must be overridden by each widget, in order to hide
     * widgets that are not supported by the Camera device.
     *
     * @param params The Camera parameters provided by HAL
     * @return true if the widget is supported by the device
     */
    public abstract boolean isSupported(Camera.Parameters params);

    /**
     * Add a view to the container widget
     *
     * @param v The view to add
     */
    public void addViewToContainer(View v) {
        if (mWidget.getRowCount() * mWidget.getColumnCount() < mWidget.getChildCount() + 1) {
            if (mWidget.getColumnCount() == mWidgetMaxWidth) {
                // Add a new line instead of a column to fit the max width
                mWidget.setRowCount(mWidget.getRowCount() + 1);
            } else {
                mWidget.setColumnCount(mWidget.getColumnCount() + 1);
            }
        }

        mWidget.addView(v);
    }

    /**
     * Force the size of the grid container to the specified
     * row and column count. The views can then be added more precisely
     * to the container using setViewAt()
     *
     * @param row
     * @param column
     */
    public void forceGridSize(int row, int column) {
        mWidget.setColumnCount(column);
        mWidget.setRowCount(row);
    }

    /**
     * Sets the view v at the specific row/column combo slot.
     * Pretty much useful only if you used forceGridSize
     *
     * @param row
     * @param column
     * @param v
     */
    public void setViewAt(int row, int column, int rowSpan, int colSpan, View v) {
        // Do some safety checks first
        if (mWidget.getRowCount() < row + rowSpan) {
            mWidget.setRowCount(row + rowSpan);
        }
        if (mWidget.getColumnCount() < column + colSpan) {
            mWidget.setColumnCount(column + colSpan);
        }

        mWidget.addView(v);

        GridLayout.LayoutParams layoutParams = (GridLayout.LayoutParams) v.getLayoutParams();
        layoutParams.columnSpec = GridLayout.spec(column, colSpan);
        layoutParams.rowSpec = GridLayout.spec(row, rowSpan);
        layoutParams.setGravity(Gravity.FILL);
        layoutParams.width = colSpan * v.getResources().getDimensionPixelSize(
                R.dimen.widget_option_button_size);
        mWidget.setLayoutParams(layoutParams);
    }

    public WidgetToggleButton getToggleButton() {
        return mToggleButton;
    }

    public WidgetContainer getWidget() {
        return mWidget;
    }

    public boolean isOpen() {
        return mIsOpen;
    }

    /**
     * Restores the value of this widget from the database to the Camera's preferences
     */
    public String restoreValueFromStorage(String key) {
        Camera.Parameters params = mCamManager.getParameters();
        if (params == null) {
            return "";
        }

        String currentValue = params.get(key);
        if (currentValue != null) {
            String storageValue = SettingsStorage.getCameraSetting(mWidget.getContext(),
                    mCamManager.getCurrentFacing(), key, currentValue);
            mCamManager.setParameterAsync(key, storageValue);

            return storageValue;
        }

        return currentValue;
    }

    public String getCameraValue(String key) {
        Camera.Parameters params = mCamManager.getParameters();
        return params.get(key);
    }

    /**
     * Notifies the WidgetBase that the orientation has changed. WidgetBase doesn't
     * do anything, but specific widgets might need orientation information (such as
     * SettingsWidget to rotate the dialog).
     *
     * @param orientation The screen orientation
     */
    public void notifyOrientationChanged(int orientation) {

    }

    /**
     * Opens the widget, show the fade in animation
     */
    public void open() {
        WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();

        mWidget.setVisibility(View.VISIBLE);
        mWidget.invalidate();

        mIsOpen = true;

        parent.widgetOpened(mWidget);

        mWidget.animate().alpha(1.0f).translationYBy(WIDGET_ANIM_TRANSLATE)
                .setDuration(WIDGET_FADE_DURATION_MS).start();
    }

    public void close() {
        WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
        parent.widgetClosed(mWidget);

        mWidget.animate().alpha(0.0f).translationYBy(-WIDGET_ANIM_TRANSLATE)
                .setDuration(WIDGET_FADE_DURATION_MS).start();

        mIsOpen = false;
    }

    public void setHidden(boolean hide) {
        mToggleButton.setVisibility(hide ? View.GONE : View.VISIBLE);
    }

    public boolean isHidden() {
        return mToggleButton.getVisibility() == View.GONE;
    }

    /**
     * Represents the button that toggles the widget, in the
     * side bar.
     */
    public class WidgetToggleButton extends ImageView {
        private float mDownX;
        private float mDownY;
        private boolean mCancelOpenOnDown;
        private String mHintText;

        public WidgetToggleButton(Context context, AttributeSet attrs,
                                  int defStyle) {
            super(context, attrs, defStyle);
            initialize();
        }

        public WidgetToggleButton(Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize();
        }

        public WidgetToggleButton(Context context) {
            super(context);
            initialize();
        }

        private void initialize() {
            this.setClickable(true);

            int pad = getResources().getDimensionPixelSize(R.dimen.widget_toggle_button_padding);
            this.setPadding(pad, pad, pad, pad);

            this.setOnClickListener(new ToggleClickListener());
            this.setOnLongClickListener(new LongClickListener());
        }

        public void setHintText(String text) {
            mHintText = text;
        }

        public void setHintText(int resId) {
            mHintText = getResources().getString(resId);
        }

        public String getHintText() {
            return mHintText;
        }

        public void toggleBackground(boolean active) {
            if (active) {
                this.setBackgroundColor(getResources().getColor(R.color.widget_toggle_active));
            } else {
                this.setBackgroundColor(0);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            // to dispatch click / long click event, we do it first
            boolean defaultResult;
            if (event.getActionMasked() == MotionEvent.ACTION_UP && mCancelOpenOnDown) {
                toggleBackground(mIsOpen);
                defaultResult = false;
                return true;
            } else {
                defaultResult = super.onTouchEvent(event);
            }

            // Handle the background color on touch
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                this.setBackgroundColor(getResources().getColor(R.color.widget_toggle_pressed));
                mCancelOpenOnDown = false;
                mDownX = event.getX();
                mDownY = event.getY();
            } else if (event.getActionMasked() == MotionEvent.ACTION_UP) {
                // State is not updated yet, but after ACTION_UP is received the button
                // will likely be clicked and its state will change.
                toggleBackground(!mIsOpen);
            } else if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
                Log.e(TAG, "ACTION_MOVE dX=" +  Math.abs(event.getX() - mDownX) +
                        " dY="  + Math.abs(event.getY() - mDownY));
                if (!mIsOpen && (Math.abs(event.getX() - mDownX) > 8
                        || Math.abs(event.getY() - mDownY) > 8)) {
                    toggleBackground(false);
                }
            }

            return true;
        }

        private class LongClickListener implements OnLongClickListener {
            @Override
            public boolean onLongClick(View view) {
                mCancelOpenOnDown = true;
                if (mHintText != null && !mHintText.isEmpty()) {
                    CameraActivity.notify(mHintText, 2000, getX(), Util.getScreenSize(null).y
                            - getHeight() - getBottom());
                }

                toggleBackground(mIsOpen);
                return true;
            }
        }

        private class ToggleClickListener implements OnClickListener {
            @Override
            public void onClick(View v) {
                SideBar sb = (SideBar) WidgetToggleButton.this.getParent().getParent();
                sb.toggleWidgetVisibility(WidgetBase.this, false);
            }
        }
    }

    /**
     * Interface that all widget options must implement. Give info
     * about the layout of the widget.
     */
    public interface WidgetOption {
        /**
         * Returns the number of columns occupied by the widget.
         * A button typically occupies 1 column, but specific widgets
         * like text widget might occupy two columns in some cases.
         *
         * @return Number of columns
         */
        public int getColSpan();

        /**
         * Notifies the widget that its orientation changed
         *
         * @param orientation The new angle
         */
        public void notifyOrientationChanged(int orientation);
    }

    /**
     * Represents a standard label (text view) put inside
     * a {@link WidgetContainer}.
     */
    public class WidgetOptionLabel extends TextView implements WidgetOption {
        public WidgetOptionLabel(Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, defStyle);
            initialize();
        }

        public WidgetOptionLabel(Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize();
        }

        public WidgetOptionLabel(Context context) {
            super(context);
            initialize();
        }

        private void initialize() {
            setMinimumWidth(getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_size));
            setMinimumHeight(getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_size));
            int padding = getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_padding);
            setPadding(padding, padding, padding, padding);
            setGravity(Gravity.CENTER);

            setTextSize(getResources().getInteger(R.integer.widget_option_label_font_size));
        }

        @Override
        public void onFinishInflate() {
            super.onFinishInflate();
            GridLayout.LayoutParams params = (GridLayout.LayoutParams) this.getLayoutParams();
            params.setGravity(Gravity.CENTER);
        }

        public void setSmall(boolean small) {
            if (small) {
                setTextSize(getResources().getInteger(
                        R.integer.widget_option_label_font_size_small));
            } else {
                setTextSize(getResources().getInteger(R.integer.widget_option_label_font_size));
            }
        }

        @Override
        public int getColSpan() {
            // TODO: Return a different colspan if label larger
            return 1;
        }

        @Override
        public void notifyOrientationChanged(int orientation) {

        }
    }

    /**
     * Represents a non-clickable imageview put inside
     * a {@link WidgetContainer}.
     */
    public class WidgetOptionImage extends ImageView implements WidgetOption {
        public WidgetOptionImage(int resId, Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, defStyle);
            initialize(resId);
        }

        public WidgetOptionImage(int resId, Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize(resId);
        }

        public WidgetOptionImage(int resId, Context context) {
            super(context);
            initialize(resId);
        }

        private void initialize(int resId) {
            setMinimumWidth(getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_size));
            int padding = getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_padding);
            setPadding(padding, padding, padding, padding);
            setImageResource(resId);
        }

        @Override
        public int getColSpan() {
            return 1;
        }

        @Override
        public void notifyOrientationChanged(int orientation) {

        }
    }

    /**
     * Represents a standard setting button put inside
     * a {@link WidgetContainer}.
     * Note that you're not forced to use exclusively buttons,
     * TextViews through WidgetOptionLabel can also be added (for Timer for instance)
     */
    public class WidgetOptionButton extends ImageView implements WidgetOption,
            View.OnLongClickListener {
        private float mTouchOffset = 0.0f;
        private int mOriginalResource;
        private String mHintText;

        public WidgetOptionButton(int resId, Context context, AttributeSet attrs,
                                  int defStyle) {
            super(context, attrs, defStyle);
            initialize(resId);
        }

        public WidgetOptionButton(int resId, Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize(resId);

        }

        public WidgetOptionButton(int resId, Context context) {
            super(context);
            initialize(resId);
        }

        public void resetImage() {
            setImageResource(mOriginalResource);
        }

        public void setHintText(String hint) {
            mHintText = hint;
        }

        public void setHintText(int resId) {
            mHintText = getResources().getString(resId);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            boolean handle = false;

            mGestureDetector.onTouchEvent(event);

            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                handle = true;
                mTouchOffset = mWidget.getY() - event.getRawY();
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetPressed(mWidget);
                this.setBackgroundColor(getResources().getColor(R.color.widget_toggle_pressed));
            } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                mWidget.setY(event.getRawY() + mTouchOffset);
                parent.widgetMoved(mWidget);
                mWidget.forceFinalY(getY());
                handle = true;
            } else if (event.getAction() == MotionEvent.ACTION_UP) {
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetDropped(mWidget);
                this.setBackgroundColor(0);
            }

            return (super.onTouchEvent(event) || handle);
        }

        public void setActiveDrawable(String key) {
            setImageBitmap(BitmapFilter.getSingleton().getGlow(key,
                    getContext().getResources().getColor(R.color.widget_option_active),
                    ((BitmapDrawable) getDrawable()).getBitmap()));
        }

        private void initialize(int resId) {
            this.setImageResource(resId);
            this.setClickable(true);
            int padding = getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_padding);
            this.setPadding(padding, padding, padding, padding);
            mOriginalResource = resId;
            setOnLongClickListener(this);
        }

        @Override
        public int getColSpan() {
            return 1;
        }

        @Override
        public void notifyOrientationChanged(int orientation) {

        }

        @Override
        public boolean onLongClick(View view) {
            if (mHintText != null && mHintText.length() > 0) {
                CameraActivity.notify(mHintText, 2000, Util.dpToPx(getContext(), 8) 
                        + getX() + mWidget.getX(), Util.dpToPx(getContext(), 32)
                        + Util.dpToPx(getContext(), 4) * mHintText.length());
                return true;
            } else {
                return false;
            }
        }
    }

    /**
     * Represents a centered seek bar put inside
     * a {@link WidgetContainer}.
     */
    public class WidgetOptionCenteredSeekBar extends CenteredSeekBar implements WidgetOption {
        public WidgetOptionCenteredSeekBar(Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize();
        }

        public WidgetOptionCenteredSeekBar(Integer min, Integer max, Context context) {
            super(min, max, context);
            initialize();
        }

        private void initialize() {
            setMaxWidth(getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_size) * 3);
            setMaxHeight(getResources().getDimensionPixelSize(R.dimen.widget_option_button_size));

            // Set padding
            int pad = getResources().getDimensionPixelSize(R.dimen.widget_container_padding);
            this.setPadding(pad, pad, pad, pad);
        }

        @Override
        public void onFinishInflate() {
            super.onFinishInflate();
            GridLayout.LayoutParams params = (GridLayout.LayoutParams) this.getLayoutParams();
            params.setGravity(Gravity.CENTER);
        }

        @Override
        public int getColSpan() {
            return 3;
        }

        @Override
        public void notifyOrientationChanged(int orientation) {

        }
    }

    /**
     * Represents a normal seek bar put inside
     * a {@link WidgetContainer}.
     */
    public class WidgetOptionSeekBar extends SeekBar implements WidgetOption {

        public WidgetOptionSeekBar(Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize();
        }

        public WidgetOptionSeekBar(Context context) {
            super(context);
            initialize();
        }

        private void initialize() {
            // Set padding
            int pad = getResources().getDimensionPixelSize(R.dimen.widget_container_padding) * 2;
            this.setPadding(pad, pad, pad, pad);
            this.setMinimumHeight(getResources().getDimensionPixelSize(
                    R.dimen.widget_option_button_size) * 3);
        }

        @Override
        public void onFinishInflate() {
            super.onFinishInflate();
            GridLayout.LayoutParams params = (GridLayout.LayoutParams) this.getLayoutParams();
            params.setMargins(120, 120, 120, 120);
            this.setLayoutParams(params);
        }

        @Override
        public int getColSpan() {
            return 3;
        }

        @Override
        public void notifyOrientationChanged(int orientation) {

        }
    }

    /**
     * Represents the floating window of a widget.
     */
    public class WidgetContainer extends GridLayout {
        private float mTouchOffset = 0.0f;
        private float mTargetY = 0.0f;

        public WidgetContainer(Context context, AttributeSet attrs,
                               int defStyle) {
            super(context, attrs, defStyle);
            initialize();
        }

        public WidgetContainer(Context context, AttributeSet attrs) {
            super(context, attrs);
            initialize();
        }

        public WidgetContainer(Context context) {
            super(context);
            initialize();
        }

        /**
         * Animate the movement on X
         *
         * @param y
         */
        public void setYSmooth(float y) {
            if (mTargetY != y) {
                this.animate().cancel();
                this.animate().y(y).setDuration(100).start();
                mTargetY = y;
            }
        }

        /**
         * Returns the final X position, after the animation is done
         *
         * @return final X position
         */
        public float getFinalY() {
            return mTargetY;
        }

        public void forceFinalY(float y) {
            mTargetY = y;
        }

        public WidgetBase getWidgetBase() {
            return WidgetBase.this;
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            boolean handle = false;
            if (getAlpha() < 1) return false;

            mGestureDetector.onTouchEvent(event);

            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                handle = true;
                mTouchOffset = getY() - event.getRawY();
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetPressed(mWidget);
            } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
                setY(event.getRawY() + mTouchOffset);
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetMoved(mWidget);
                forceFinalY(getY());
                handle = true;
            } else if (event.getAction() == MotionEvent.ACTION_UP) {
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetDropped(mWidget);
            }

            if (event.getAction() == MotionEvent.ACTION_UP) {
                return handle;
            } else {
                return (super.onTouchEvent(event) || handle);
            }
        }

        @Override
        protected void onAttachedToWindow() {
            super.onAttachedToWindow();

            FrameLayout.LayoutParams layoutParam =
                    (FrameLayout.LayoutParams) this.getLayoutParams();
            layoutParam.width = FrameLayout.LayoutParams.WRAP_CONTENT;
            layoutParam.height = FrameLayout.LayoutParams.WRAP_CONTENT;
        }

        private void initialize() {
            this.setBackgroundColor(getResources().getColor(R.color.widget_background));
            this.setColumnOrderPreserved(true);

            // We default the window invisible, so we must respect the status
            // obtained after the "close" animation
            setAlpha(0.0f);
            setTranslationY(-WIDGET_ANIM_TRANSLATE);
            setVisibility(View.VISIBLE);

            // Set padding
            int pad = getResources().getDimensionPixelSize(R.dimen.widget_container_padding);
            this.setPadding(pad, pad, pad, pad);

            this.animate().setListener(new AnimatorListener() {
                @Override
                public void onAnimationCancel(Animator arg0) {

                }

                @Override
                public void onAnimationEnd(Animator arg0) {
                    if (!mIsOpen) {
                        WidgetContainer.this.setVisibility(View.GONE);
                    }
                }

                @Override
                public void onAnimationRepeat(Animator arg0) {
                }

                @Override
                public void onAnimationStart(Animator arg0) {
                    WidgetContainer.this.setVisibility(View.VISIBLE);
                }
            });
        }

        @Override
        protected void onMeasure(int widthSpec, int heightSpec) {
            super.onMeasure(widthSpec, heightSpec);

            if (!isOpen()) {
                setVisibility(View.GONE);
            } else {
                setVisibility(View.VISIBLE);
            }
        }

        public void notifyOrientationChanged(int orientation, boolean fastforward) {
            int buttonsCount = getChildCount();

            for (int i = 0; i < buttonsCount; i++) {
                View child = getChildAt(i);
                WidgetOption option = (WidgetOption) child;

                // Don't rotate seekbars
                if (child instanceof WidgetOptionCenteredSeekBar) {
                    continue;
                }
                if (child instanceof WidgetOptionSeekBar) {
                    continue;
                }

                option.notifyOrientationChanged(orientation);

                if (fastforward) {
                    child.setRotation(orientation);
                } else {
                    child.animate().rotation(orientation).setDuration(200).setInterpolator(
                            new DecelerateInterpolator()).start();
                }
            }

            WidgetBase.this.notifyOrientationChanged(orientation);
        }
    }

    /**
     * Handles the swipe of widgets out of the screen (to the top/left)
     */
    public class WidgetFlingGestureListener extends GestureDetector.SimpleOnGestureListener {
        //private final static String TAG = "GestureListener";

        private static final int SWIPE_MIN_DISTANCE = 10;
        private static final int SWIPE_MAX_OFF_PATH = 250;
        private static final int SWIPE_THRESHOLD_VELOCITY = 200;

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            try {
                if (Math.abs(e1.getY() - e2.getY()) > SWIPE_MAX_OFF_PATH)
                    return false;

                // swipe top to close the widget

                // Somehow, GestureDetector takes the screen orientation into account, even if our
                // activity is locked in landscape.
                float distance = e2.getX() - e1.getX();
                if (distance <= 0.1f) {
                    distance = e2.getY() - e1.getY();
                }

                if (distance > SWIPE_MIN_DISTANCE && Math.abs(velocityX)
                        > SWIPE_THRESHOLD_VELOCITY) {
                    final SideBar sb = (SideBar) mToggleButton.getParent().getParent();
                    final WidgetRenderer wr = (WidgetRenderer) mWidget.getParent();
                    sb.toggleWidgetVisibility(WidgetBase.this, true);
                    wr.widgetClosed(mWidget);
                    mToggleButton.toggleBackground(false);
                }
            } catch (Exception e) {
                // Do nothing here
            }

            return false;
        }
    }
}
