package org.cyanogenmod.nemesis.picsphere;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

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
    private Capture3DRenderer mCapture3DRenderer;

    public PicSphereManager(Context context) {
        mContext = context;
        mPicSpheres = new ArrayList<PicSphere>();
        copyBinaries();
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
     * Returns the 3D renderer context for PicSphere capture mode
     */
    public Capture3DRenderer getRenderer() {
        if (mCapture3DRenderer != null) {
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


    /**
     * Copy the required binaries and libraries to the data folder
     * @return
     */
    private boolean copyBinaries() {
        try {
            String files[] = {
                "autooptimiser", "autopano", "celeste", "enblend", "enfuse", "nona", "pano_modify",
                    "ptclean", "tiffinfo",
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
