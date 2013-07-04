package org.cyanogenmod.nemesis.picsphere;

import android.content.Context;

import java.util.ArrayList;
import java.util.List;

/**
 * This class manages the interaction and processing of Picture Spheres ("PicSphere"). PicSphere
 * is basically an open-source clone of Google's PhotoSphere, using Hugin open-source panorama
 * stitching libraries.
 *
 * Can you feel the awesomeness?
 */
public class PicSphereManager {
    private List<PicSphere> mPicSpheres;
    private Context mContext;

    public PicSphereManager(Context context) {
        mContext = context;
        mPicSpheres = new ArrayList<PicSphere>();
    }

    /**
     * Creates an empty PicSphere
     * @return A PicSphere that is empty
     */
    public PicSphere createPicSphere() {
        PicSphere sphere = new PicSphere(mContext);
        mPicSpheres.add(sphere);
        return sphere;
    }

    /**
     * Starts rendering the provided PicSphere in a thread, and handles a service
     * that will keep on processing even once Nemesis is closed
     *
     * @param sphere The PicSphere to render
     */
    public void startRendering(final PicSphere sphere) {
        // TODO: Handle app exit better

        new Thread() {
            public void run() {
                sphere.render();
            }
        }.start();
    }
}
