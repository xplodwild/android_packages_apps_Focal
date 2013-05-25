package org.cyanogenmod.nemesis.widgets;

import org.cyanogenmod.nemesis.R;

import android.content.Context;
import android.hardware.Camera;

/**
 * Flash Widget, manages the flash settings
 */
public class FlashWidget extends WidgetBase {

	public FlashWidget(Context context) {
		super(context, R.drawable.ic_widget_flash_on);
		
		addViewToContainer(new WidgetBase.WidgetOptionButton(R.drawable.ic_widget_flash_on, context));
	}
	
	@Override
	public boolean isSupported(Camera.Parameters params) {
		return ( params.getSupportedFlashModes() != null );
	}

	
}
