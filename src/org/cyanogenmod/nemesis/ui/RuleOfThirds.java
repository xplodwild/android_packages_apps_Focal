package org.cyanogenmod.nemesis.ui;

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
        if (mPaint == null) mPaint = new Paint();

        mPaint.setARGB(255, 255, 255, 255);

        int width = getMeasuredWidth();
        int height = getMeasuredHeight();

        canvas.drawLine(width/3, 0, width/3, height, mPaint);
        canvas.drawLine(width*2/3, 0, width*2/3, height, mPaint);
        canvas.drawLine(0, height/3, width, height/3, mPaint);
        canvas.drawLine(0, height*2/3, width, height*2/3, mPaint);
    }
}
