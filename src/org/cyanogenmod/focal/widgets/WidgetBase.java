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

package org.cyanogenmod.focal.widgets;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.BitmapDrawable;
import android.hardware.Camera;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewPropertyAnimator;
import android.view.animation.DecelerateInterpolator;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.GridLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

import org.cyanogenmod.focal.BitmapFilter;
import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.SettingsStorage;
import org.cyanogenmod.focal.Util;
import org.cyanogenmod.focal.ui.CenteredSeekBar;
import org.cyanogenmod.focal.ui.WidgetRenderer;

/**
 * Base class for settings widget. Each setting widget
 * will extend this class.
 */
public abstract class WidgetBase {
    public final static String TAG = "WidgetBase";

    protected CameraManager mCamManager;
    protected WidgetToggleButton mToggleButton;
    protected WidgetToggleButton mShortcutButton;
    protected WidgetContainer mWidget;
    private int mWidgetMaxWidth;
    private boolean mIsOpen;

    private final static int WIDGET_FADE_DURATION_MS = 200;
    private final static float WIDGET_FADE_SCALE = 0.75f;



    private class ToggleClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            if (isOpen()) {
                WidgetBase.this.close();
            } else {
                WidgetBase.this.open();
            }
        }
    }

    public WidgetBase(CameraManager cam, Context context, int iconResId) {
        mCamManager = cam;
        mWidget = new WidgetContainer(context);
        mToggleButton = new WidgetToggleButton(context);
        mShortcutButton = new WidgetToggleButton(context);
        mShortcutButton.setShortcut(true);
        mIsOpen = false;

        // Setup the toggle button
        mToggleButton.setCompoundDrawablesWithIntrinsicBounds(0, iconResId, 0, 0);
        mShortcutButton.setCompoundDrawablesWithIntrinsicBounds(0, iconResId, 0, 0);
        mWidget.setIconId(iconResId);

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
        if (mWidget.getGrid().getRowCount() * mWidget.getGrid().getColumnCount() < mWidget.getGrid().getChildCount() + 1) {
            if (mWidget.getGrid().getColumnCount() == mWidgetMaxWidth) {
                // Add a new line instead of a column to fit the max width
                mWidget.getGrid().setRowCount(mWidget.getGrid().getRowCount() + 1);
            } else {
                mWidget.getGrid().setColumnCount(mWidget.getGrid().getColumnCount() + 1);
            }
        }

        mWidget.getGrid().addView(v);
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
        mWidget.getGrid().setColumnCount(column);
        mWidget.getGrid().setRowCount(row);
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
        if (mWidget.getGrid().getRowCount() < row + rowSpan) {
            mWidget.getGrid().setRowCount(row + rowSpan);
        }
        if (mWidget.getGrid().getColumnCount() < column + colSpan) {
            mWidget.getGrid().setColumnCount(column + colSpan);
        }

        mWidget.getGrid().addView(v);

        GridLayout.LayoutParams layoutParams = (GridLayout.LayoutParams) v.getLayoutParams();
        layoutParams.columnSpec = GridLayout.spec(column, colSpan);
        layoutParams.rowSpec = GridLayout.spec(row, rowSpan);
        layoutParams.setGravity(Gravity.FILL);
        layoutParams.width = colSpan * v.getMinimumWidth();
        v.setLayoutParams(layoutParams);
    }

    public WidgetToggleButton getToggleButton() {
        return mToggleButton;
    }

    public WidgetToggleButton getShortcutButton() {
        return mShortcutButton;
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

    /**
     * @param key The name of the parameter to get
     * @return The parameter value, or null if it doesn't exist
     */
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

        if (parent == null) return;

        mWidget.setVisibility(View.VISIBLE);
        mWidget.invalidate();

        mIsOpen = true;

        parent.widgetOpened(mWidget);

        // Set initial widget state
        mWidget.setAlpha(0.0f);
        mWidget.setScaleX(WIDGET_FADE_SCALE);
        mWidget.setScaleY(WIDGET_FADE_SCALE);
        mWidget.setX(0);

        mWidget.setY(Util.dpToPx(mWidget.getContext(), 8));

        mWidget.animate().alpha(1.0f).scaleX(1.0f).scaleY(1.0f)
                .setDuration(WIDGET_FADE_DURATION_MS).start();

        mShortcutButton.toggleBackground(true);
        mToggleButton.toggleBackground(true);
    }

    public void close() {
        WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
        if (parent != null) {
            parent.widgetClosed(mWidget);
        }

        mWidget.animate().alpha(0.0f).scaleX(WIDGET_FADE_SCALE).scaleY(WIDGET_FADE_SCALE)
                .setDuration(WIDGET_FADE_DURATION_MS).start();

        mShortcutButton.toggleBackground(false);
        mToggleButton.toggleBackground(false);
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
    public class WidgetToggleButton extends Button {
        private float mDownX;
        private float mDownY;
        private boolean mCancelOpenOnDown;
        private String mHintText;
        private boolean mIsShortcut = false;

        public WidgetToggleButton(Context context, AttributeSet attrs,
                                  int defStyle) {
            super(context, attrs, android.R.style.Widget_DeviceDefault_Button_Borderless_Small);
            initialize();
        }

        public WidgetToggleButton(Context context, AttributeSet attrs) {
            super(context, attrs, android.R.style.Widget_DeviceDefault_Button_Borderless_Small);
            initialize();
        }

        public WidgetToggleButton(Context context) {
            super(context, null, android.R.style.Widget_DeviceDefault_Button_Borderless_Small);
            initialize();
        }

        private void initialize() {
            Resources res = getResources();
            if (res == null) return;

            this.setClickable(true);

            int pad = res.getDimensionPixelSize(R.dimen.widget_toggle_button_padding);
            this.setPadding(pad, pad, pad, pad);

            this.setOnClickListener(new ToggleClickListener());
            this.setOnLongClickListener(new LongClickListener());

            setTextSize(11);
            setGravity(Gravity.CENTER);
            setLines(2);

            int pxSize = (int) Util.dpToPx(getContext(), 82);
            setMaxWidth(pxSize);
            setMinWidth(pxSize);
        }

        public void setHintText(String text) {
            mHintText = text;
            if (!mIsShortcut) {
                setText(text);
            }
        }

        public void setHintText(int resId) {
            mHintText = getResources().getString(resId);
            if (!mIsShortcut) {
                setText(mHintText);
            }
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
            if (event.getActionMasked() == MotionEvent.ACTION_UP && mCancelOpenOnDown) {
                toggleBackground(mIsOpen);
                return true;
            } else {
                super.onTouchEvent(event);
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
                if (!mIsOpen && (Math.abs(event.getX() - mDownX) > 8
                        || Math.abs(event.getY() - mDownY) > 8)) {
                    toggleBackground(false);
                }
            }

            return true;
        }

        public void setShortcut(boolean b) {
            mIsShortcut = b;
            if (b) {
                setText("");
                int pxSize = (int) Util.dpToPx(getContext(), 72);
                setMaxWidth(pxSize);
                setMinWidth(pxSize);
                setMaxHeight(pxSize / 2);
            }
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
            Resources res = getResources();
            if (res == null) return;

            setMinimumWidth(res.getDimensionPixelSize(
                    R.dimen.widget_option_button_size) / 2);
            setMaxWidth(getMinimumWidth());

            int padding = res.getDimensionPixelSize(
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
    public class WidgetOptionButton extends Button implements WidgetOption,
            View.OnLongClickListener {
        private float mTouchOffset = 0.0f;
        private BitmapDrawable mOriginalDrawable;
        private String mHintText;


        public WidgetOptionButton(int resId, Context context, AttributeSet attrs,
                                  int defStyle) {
            super(context, attrs, android.R.style.Widget_DeviceDefault_Button_Borderless_Small);
            initialize(resId);
        }

        public WidgetOptionButton(int resId, Context context, AttributeSet attrs) {
            super(context, attrs, android.R.style.Widget_DeviceDefault_Button_Borderless_Small);
            initialize(resId);

        }

        public WidgetOptionButton(int resId, Context context) {
            super(context, null, android.R.style.Widget_DeviceDefault_Button_Borderless_Small);
            initialize(resId);
        }

        public void resetImage() {
            this.setCompoundDrawablesRelativeWithIntrinsicBounds(null, mOriginalDrawable, null, null);
        }

        public void setHintText(String hint) {
            mHintText = hint;
            this.setText(mHintText);
        }

        public void setHintText(int resId) {
            Resources res = getResources();
            if (res == null) return;

            mHintText = res.getString(resId);
            this.setText(mHintText);
        }

        public void activeImage(String key) {
            Resources res = getResources();
            if (res == null) return;

            this.setCompoundDrawablesRelativeWithIntrinsicBounds(null,
                    new BitmapDrawable(res,
                            BitmapFilter.getSingleton().getGlow(key,
                                    res.getColor(R.color.widget_option_active),
                                    mOriginalDrawable.getBitmap())), null, null);
        }

        private void initialize(int resId) {
            Resources res = getResources();
            if (res == null) return;

            mOriginalDrawable = (BitmapDrawable) res.getDrawable(resId);
            this.setCompoundDrawablesRelativeWithIntrinsicBounds(null, mOriginalDrawable, null, null);
            this.setGravity(Gravity.CENTER);
            this.setLines(2);

            int pxHeight = (int) Util.dpToPx(getContext(), 56);
            int pxWidth = (int) Util.dpToPx(getContext(), 64);
            this.setMinimumWidth(pxWidth);
            this.setMinimumHeight(pxHeight);
            this.setMaxWidth(pxWidth);
            this.setMaxHeight(pxHeight);
            this.setSelected(true);
            this.setFadingEdgeLength((int) Util.dpToPx(getContext(), 6));
            this.setHorizontalFadingEdgeEnabled(true);

            this.setTextSize(10);
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
            Resources res = getResources();
            if (res == null) return;

            setMaxWidth(res.getDimensionPixelSize(
                    R.dimen.widget_option_button_size) * 3);
            setMaxHeight(res.getDimensionPixelSize(R.dimen.widget_option_button_size));

            // Set padding
            int pad = res.getDimensionPixelSize(R.dimen.widget_container_padding);
            this.setPadding(pad, pad, pad, pad);
        }

        @Override
        public void onFinishInflate() {
            super.onFinishInflate();
            GridLayout.LayoutParams params = (GridLayout.LayoutParams) this.getLayoutParams();

            if (params != null) {
                params.setGravity(Gravity.CENTER);
            }
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
            Resources res = getResources();
            if (res == null) return;

            // Set padding
            int pad = res.getDimensionPixelSize(R.dimen.widget_container_padding) * 2;
            this.setPadding(pad, pad, pad, pad);
            this.setMinimumHeight(res.getDimensionPixelSize(
                    R.dimen.widget_option_button_size) * 3);
        }

        @Override
        public void onFinishInflate() {
            super.onFinishInflate();
            GridLayout.LayoutParams params = (GridLayout.LayoutParams) this.getLayoutParams();

            if (params != null) {
                params.setMargins(120, 120, 120, 120);
                this.setLayoutParams(params);
            }
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
    public class WidgetContainer extends FrameLayout {
        private float mTouchOffsetY = 0.0f;
        private float mTouchOffsetX = 0.0f;
        private float mTouchDownX = 0.0f;
        private float mTouchDownY = 0.0f;
        private float mTargetY = 0.0f;
        private ViewGroup mRootView;
        private GridLayout mGridView;
        private ImageView mWidgetIcon;
        private Button mPinButton;
        private ImageButton mResetButton;
        private ViewPropertyAnimator mRootAnimator;

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
                mRootAnimator.cancel();
                mRootAnimator.y(y).setDuration(100).start();
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
            if (getAlpha() == 0.0f) return false;

            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                handle = true;
                mTouchOffsetY = getY() - event.getRawY();
                mTouchOffsetX = getX() - event.getRawX();
                mTouchDownX = event.getRawX();
                mTouchDownY = event.getRawY();
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetPressed(mWidget);
            } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
                float driftX = Math.abs(event.getRawX() - mTouchDownX);
                float driftY = Math.abs(event.getRawY() - mTouchDownY);

                if (driftX < driftY) {
                    setX(0);
                    setY(event.getRawY() + mTouchOffsetY);
                } else {
                    setY(mTargetY);
                    setX(event.getRawX() + mTouchOffsetX);
                }

                setAlpha(1 - driftX / getWidth());
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetMoved(mWidget);
                forceFinalY(getY());
                handle = true;
            } else if (event.getAction() == MotionEvent.ACTION_UP) {
                WidgetRenderer parent = (WidgetRenderer) mWidget.getParent();
                parent.widgetDropped(mWidget);

                if (event.getRawX() - mTouchDownX > getWidth() / 2) {
                    animate().x(getWidth()).alpha(0).start();
                    close();
                    mToggleButton.toggleBackground(false);
                } else {
                    animate().x(0).alpha(1).start();
                }
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
            if (layoutParam != null) {
                layoutParam.width = FrameLayout.LayoutParams.WRAP_CONTENT;
                layoutParam.height = FrameLayout.LayoutParams.WRAP_CONTENT;
            }
        }

        private void initialize() {
            Context context = getContext();
            if (context == null) {
                Log.e(TAG, "Null context when initializing widget");
                return;
            }
            Resources res = getResources();
            if (res == null) {
                Log.e(TAG, "Null resources when initializing widget");
                return;
            }

            // Inflate our widget layout
            LayoutInflater inflater =
                    (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            mRootView = (ViewGroup) inflater.inflate(R.layout.widget_container, this);

            if (mRootView == null) {
                Log.e(TAG, "Error inflating the widget layout XML");
                return;
            }

            // Grab all the views
            mGridView = (GridLayout) mRootView.findViewById(R.id.widget_options_container);
            mWidgetIcon = (ImageView) mRootView.findViewById(R.id.widget_icon);
            mPinButton = (Button) mRootView.findViewById(R.id.btn_pin_widget);
            //mResetButton = (ImageButton) mRootView.findViewById(R.id.btn_reset_widget);

            mGridView.setColumnOrderPreserved(true);
            mGridView.setRowCount(1);
            mGridView.setColumnCount(0);

            mGridView.setOnTouchListener(new OnTouchListener() {
                @Override
                public boolean onTouch(View view, MotionEvent motionEvent) {
                    // We consume all events of the scrollview to avoid dragging the
                    // widget by scrolling
                    mGridView.onTouchEvent(motionEvent);
                    return true;
                }
            });

            mPinButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    Resources res = getResources();
                    if (res == null) return;

                    // Toggle shortcut (pinned) status
                    if (SettingsStorage.getShortcutSetting(getContext(),
                            WidgetBase.this.getClass().getCanonicalName())) {
                        // It was a shortcut, remove it
                        SettingsStorage.storeShortcutSetting(getContext(),
                                WidgetBase.this.getClass().getCanonicalName(), false);
                        mPinButton.setBackground(res.getDrawable(R.drawable.btn_pin_widget_inactive));
                        mShortcutButton.setVisibility(View.GONE);
                        mToggleButton.setVisibility(View.VISIBLE);
                    } else {
                        // Add it
                        SettingsStorage.storeShortcutSetting(getContext(),
                                WidgetBase.this.getClass().getCanonicalName(), true);
                        mPinButton.setBackground(res.getDrawable(R.drawable.ic_pin_widget_active));
                        mShortcutButton.setVisibility(View.VISIBLE);
                        mToggleButton.setVisibility(View.GONE);
                    }
                }
            });

            // We default the window invisible, so we must respect the status
            // obtained after the "close" animation
            setAlpha(0.0f);
            setScaleX(WIDGET_FADE_SCALE);
            setScaleY(WIDGET_FADE_SCALE);
            setVisibility(View.VISIBLE);

            // Set padding
            int pad = getResources().getDimensionPixelSize(R.dimen.widget_container_padding);
            this.setPadding(pad, pad, pad, pad);

            mRootAnimator = mRootView.animate();

            if (mRootAnimator != null) {
                mRootAnimator.setListener(new AnimatorListener() {
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
            int buttonsCount = mGridView.getChildCount();

            for (int i = 0; i < buttonsCount; i++) {
                View child = mGridView.getChildAt(i);
                WidgetOption option = (WidgetOption) child;

                // Don't rotate seekbars
                if (child instanceof WidgetOptionCenteredSeekBar) {
                    continue;
                }
                if (child instanceof WidgetOptionSeekBar) {
                    continue;
                }

                if (option != null) {
                    option.notifyOrientationChanged(orientation);
                }

                if (fastforward) {
                    child.setRotation(orientation);
                } else {
                    child.animate().rotation(orientation).setDuration(200).setInterpolator(
                            new DecelerateInterpolator()).start();
                }
            }

            WidgetBase.this.notifyOrientationChanged(orientation);
        }

        public GridLayout getGrid() {
            return mGridView;
        }

        public void setIconId(int iconResId) {
            mWidgetIcon.setImageResource(iconResId);
        }
    }
}
