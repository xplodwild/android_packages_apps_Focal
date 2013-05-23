package org.cyanogenmod.nemesis.widgets;

import org.cyanogenmod.nemesis.R;

import android.content.Context;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.view.View;
import android.widget.GridLayout;
import android.widget.ImageButton;
import android.widget.ImageView;

/**
 * Base class for settings widget. Each setting widget
 * will extend this class.
 * 
 * Remember that we are working in landscape, so the GridLayout
 * column count is the width of the widget when looking landscape,
 * and become the height when looking portrait.
 * 
 * What we call the "MaxWidgetWidth" is the number of ROWS in landscape,
 * that looks like COLUMNS in portrait.
 * 
 */
public abstract class WidgetBase {
	private WidgetToggleButton mToggleButton;
	private WidgetContainer mWidget;
	private int mWidgetMaxWidth;
	
	public WidgetBase(Context context, int iconResId) {
		mWidget = new WidgetContainer(context);
		mToggleButton = new WidgetToggleButton(context);
		
		// Setup the container
		mWidget.setColumnCount(1);
		mWidget.setRowCount(0);
		
		// Setup the toggle button
		mToggleButton.setImageResource(iconResId);
		
		mWidgetMaxWidth = context.getResources().getInteger(R.integer.widget_default_max_width);
	}
	
	/**
	 * This method returns whether or not the widget settings are
	 * supported by the camera HAL or not.
	 * 
	 * This method must be overridden by each widget, in order to hide
	 * widgets that are not supported by the Camera device.
	 * 
	 * @param params The Camera parameters provided by HAL
	 * @return true if the widget is supported by the device
	 */
	public abstract boolean isSupported(Camera.Parameters params);
	
	/**
	 * Add a view to the container widget
	 * @param v The view to add
	 */
	public void addViewToContainer(View v) {
		if (mWidget.getRowCount() == mWidgetMaxWidth) {
			// Add a new column instead of a line to fit the max width
			mWidget.setColumnCount(mWidget.getColumnCount()+1);
		} else if (mWidget.getRowCount() < mWidgetMaxWidth) {
			mWidget.setRowCount(mWidget.getRowCount()+1);
		}
		
		mWidget.addView(v);
	}
	
	public WidgetToggleButton getToggleButton() {
		return mToggleButton;
	}
	
	public WidgetContainer getWidget() {
		return mWidget;
	}
	
	
	/**
	 * Represents the button that toggles the widget, in the
	 * side bar.
	 */
	public static class WidgetToggleButton extends ImageView {
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
		 * @return Number of columns
		 */
		public int getColSpan();
	}
	
	/**
	 * Represents a standard setting button put inside 
	 * a {@link WidgetContainer}.
	 * Note that you're not forced to use exclusively buttons,
	 * TextViews can also be added (for Timer for instance)
	 */
	public static class WidgetOptionButton extends ImageButton implements WidgetOption {
		public WidgetOptionButton(int resId, Context context, AttributeSet attrs,
				int defStyle) {
			super(context, attrs, defStyle);
			this.setImageResource(resId);
		}
		
		public WidgetOptionButton(int resId, Context context, AttributeSet attrs) {
			super(context, attrs);
			this.setImageResource(resId);
		}
		
		public WidgetOptionButton(int resId, Context context) {
			super(context);
			this.setImageResource(resId);
		}

		@Override
		public int getColSpan() {
			return 1;
		}
	}
	
	/**
	 * Represents the floating window of a widget.
	 */
	public static class WidgetContainer extends GridLayout {
		public WidgetContainer(Context context, AttributeSet attrs,
				int defStyle) {
			super(context, attrs, defStyle);
		}
		
		public WidgetContainer(Context context, AttributeSet attrs) {
			super(context, attrs);
		}
		
		public WidgetContainer(Context context) {
			super(context);
		}
	}
}
