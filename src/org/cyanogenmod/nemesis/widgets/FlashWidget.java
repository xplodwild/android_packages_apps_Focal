package org.cyanogenmod.nemesis.widgets;

import org.cyanogenmod.nemesis.R;

import android.content.Context;
import android.hardware.Camera;

/**
 * Flash Widget, manages the flash settings
 */
public class FlashWidget extends WidgetBase {

	public FlashWidget(Context context, int iconResId) {
		super(context, iconResId);
		
		addViewToContainer(new WidgetBase.WidgetOptionButton(iconResId, context));
	}
	
	@Override
	public boolean isSupported(Camera.Parameters params) {
		return ( params.getSupportedFlashModes() != null );
	}

	
}
