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

import android.animation.Animator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.widget.ImageView;

/**
 * View designed to show and fade the preview of a snapshot
 * after it was taken.
 */
public class ThumbnailFlinger extends ImageView {
    private final static int FADE_IN_DURATION_MS = 250;
    private final static int FADE_OUT_DURATION_MS = 250;

    public ThumbnailFlinger(Context context) {
        super(context);
    }

    public ThumbnailFlinger(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ThumbnailFlinger(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public void doAnimation() {
        // Setup original values
        this.clearAnimation();

        setScaleX(0.8f);
        setScaleY(0.8f);
        setAlpha(0.0f);
        setTranslationX(0.0f);
        setTranslationY(0.0f);

        // First step of animation: fade in quick
        animate().alpha(1.0f).scaleX(1.0f).scaleY(1.0f).setInterpolator(
                new AccelerateInterpolator()).setDuration(FADE_IN_DURATION_MS)
                .setListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {

            }

            @Override
            public void onAnimationEnd(Animator animator) {
                // Second step: fade out and scale and slide
                animate().alpha(0.0f).scaleY(0.7f).scaleX(0.7f).translationYBy(
                        -getHeight()*1.5f).setStartDelay(100).setDuration(
                        FADE_OUT_DURATION_MS).setListener(
                        new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationStart(Animator animator) {

                    }

                    @Override
                    public void onAnimationEnd(Animator animator) {
                        ((ViewGroup) getParent()).removeView(ThumbnailFlinger.this);
                    }

                    @Override
                    public void onAnimationCancel(Animator animator) {

                    }

                    @Override
                    public void onAnimationRepeat(Animator animator) {

                    }
                }).start();
            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        }).start();
    }
}
