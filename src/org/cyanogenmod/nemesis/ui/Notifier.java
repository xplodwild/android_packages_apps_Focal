package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.cyanogenmod.nemesis.R;

/**
 * Little notifier that comes in from the side of the screen
 * to tell the user about something quickly
 */
public class Notifier extends LinearLayout {
    private TextView mTextView;
    private Handler mHandler;

    public Notifier(Context context) {
        super(context);
        initialize();
    }

    public Notifier(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public Notifier(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    private void initialize() {
        mHandler = new Handler();
    }

    /**
     * Shows the provided {@param text} during [@param durationMs} milliseconds with
     * a nice animation.
     *
     * @param text
     * @param durationMs
     * @note This method must be ran in UI thread!
     */
    public void notify(String text, long durationMs) {
        mTextView = (TextView) findViewById(R.id.notifier_text);
        mTextView.setText(text);

        fadeIn();
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                fadeOut();
            }
        }, durationMs);
    }

    private void fadeIn() {
        animate().setDuration(300).alpha(1.0f).setInterpolator(new AccelerateInterpolator()).start();
    }

    private void fadeOut() {
        animate().setDuration(300).alpha(0.0f).setInterpolator(new DecelerateInterpolator()).start();
    }
}
