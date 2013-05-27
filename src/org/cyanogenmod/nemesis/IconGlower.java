package org.cyanogenmod.nemesis;

import java.util.HashMap;
import java.util.Map;

import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.BlurMaskFilter.Blur;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;

/**
 * This class renders a specific icon in "Glow" mode
 * and cache it for future use.
 */
public class IconGlower {
    private static IconGlower mSingleton;

    private Map<String, Bitmap> mCache;

    public static IconGlower getSingleton() {
        if (mSingleton == null)
            mSingleton = new IconGlower();

        return mSingleton;
    }

    private IconGlower() {
        mCache = new HashMap<String, Bitmap>();
    }

    /**
     * Returns a glowed image of the provided icon. If the
     * provided name is already in the cache, the cached image
     * will be returned. Otherwise, the bitmap will be glowed and
     * cached under the provided name
     * @param name The name of the bitmap
     * @param src The bitmap of the icon itself
     * @return Glowed bitmap
     */
    public Bitmap getGlow(String name, Bitmap src) {
        if (mCache.containsKey(name)) {
            return mCache.get(name);
        } else {
            // An added margin to the initial image
            int margin = 0;
            int halfMargin = margin / 2;

            // the glow radius
            int glowRadius = 4;

            // the glow color
            int glowColor = Color.rgb(0, 192, 255);

            // extract the alpha from the source image
            Bitmap alpha = src.extractAlpha();

            // The output bitmap (with the icon + glow)
            Bitmap bmp = Bitmap.createBitmap(src.getWidth() + margin,
                    src.getHeight() + margin, Bitmap.Config.ARGB_8888);

            // The canvas to paint on the image
            Canvas canvas = new Canvas(bmp);

            Paint paint = new Paint();
            paint.setColor(glowColor);

            // outer glow
            canvas.drawBitmap(src, halfMargin, halfMargin, paint);
            paint.setMaskFilter(new BlurMaskFilter(glowRadius, Blur.OUTER));
            canvas.drawBitmap(alpha, halfMargin, halfMargin, paint);

            // original icon
            //canvas.drawBitmap(src, halfMargin, halfMargin, null);

            // cache icon
            mCache.put(name, bmp);

            return bmp;
        }
    }
}
