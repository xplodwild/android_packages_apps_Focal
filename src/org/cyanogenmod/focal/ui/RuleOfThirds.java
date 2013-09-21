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

package org.cyanogenmod.focal.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;

/**
 * Shows Rule of Thirds helper lines on the screen
 */
public class RuleOfThirds extends View {
    private Paint mPaint;

    public RuleOfThirds(Context context) {
        super(context);
    }

    public RuleOfThirds(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public RuleOfThirds(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (mPaint == null) {
            mPaint = new Paint();
        }

        mPaint.setARGB(255, 255, 255, 255);

        int width = getMeasuredWidth();
        int height = getMeasuredHeight();

        canvas.drawLine(width / 3, 0, width / 3, height, mPaint);
        canvas.drawLine(width * 2 / 3, 0, width * 2 / 3, height, mPaint);
        canvas.drawLine(0, height / 3, width, height / 3, mPaint);
        canvas.drawLine(0, height * 2 / 3, width, height * 2 / 3, mPaint);
    }
}
