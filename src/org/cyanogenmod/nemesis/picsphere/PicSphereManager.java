/**
 * Copyright (C) 2013 The CyanogenMod Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.cyanogenmod.nemesis.picsphere;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import org.cyanogenmod.nemesis.SnapshotManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
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
    public final static String TAG = "PicSphereManager";
    private List<PicSphere> mPicSpheres;
    private Context mContext;
    private SnapshotManager mSnapManager;
    private Capture3DRenderer mCapture3DRenderer;

    public PicSphereManager(Context context, SnapshotManager snapMan) {
        mContext = context;
        mSnapManager = snapMan;
        mPicSpheres = new ArrayList<PicSphere>();
        new Thread() {
            public void run() { copyBinaries(); }
        }.start();
    }

    /**
     * Creates an empty PicSphere
     * @return A PicSphere that is empty
     */
    public PicSphere createPicSphere() {
        PicSphere sphere = new PicSphere(mContext, mSnapManager);
        mPicSpheres.add(sphere);
        return sphere;
    }

    /**
     * Returns the 3D renderer context for PicSphere capture mode
     */
    public Capture3DRenderer getRenderer() {
        if (mCapture3DRenderer == null) {
            mCapture3DRenderer = new Capture3DRenderer(mContext);
        }

        return mCapture3DRenderer;
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

    public void tearDown() {
        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.onPause();
            mCapture3DRenderer = null;
        }
    }

    public void onPause() {
        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.onPause();
        }
    }

    public void onResume() {
        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.onResume();
        }
    }

    /**
     * Copy the required binaries and libraries to the data folder
     * @return
     */
    private boolean copyBinaries() {
        try {
            String files[] = {
                "autooptimiser", "autopano", "celeste", "enblend", "enfuse", "nona", "pano_modify",
                    "ptclean", "tiffinfo", "align_image_stack",
                    "libexiv2.so", "libglib-2.0.so", "libgmodule-2.0.so", "libgobject-2.0.so",
                    "libgthread-2.0.so", "libjpeg.so", "libpano13.so", "libtiff.so", "libtiffdecoder.so",
                    "libvigraimpex.so"
            };

            final AssetManager am = mContext.getAssets();

            for (String file : files) {
                InputStream is = am.open("picsphere/" + file);

                // Create the output file
                File dir = mContext.getFilesDir();
                File outFile = new File(dir, file);

                if (outFile.exists())
                    outFile.delete();

                OutputStream os = new FileOutputStream(outFile);

                // Copy the file
                byte[] buffer = new byte[1024];
                int read;
                while ((read = is.read(buffer)) != -1) {
                    os.write(buffer, 0, read);
                }

                if (!outFile.getName().endsWith(".so") && !outFile.setExecutable(true)) {
                    return false;
                }

                os.flush();
                os.close();
                is.close();
            }
        } catch (Exception e) {
            Log.e(TAG, "Error copying libraries and binaries", e);
            return false;
        }
        return true;
    }
}
