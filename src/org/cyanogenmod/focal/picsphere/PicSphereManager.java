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

package org.cyanogenmod.focal.picsphere;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.AssetManager;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.view.TextureView;
import android.widget.FrameLayout;

import org.cyanogenmod.focal.CameraActivity;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.SnapshotManager;

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
public class PicSphereManager implements PicSphere.ProgressListener {
    public final static String TAG = "PicSphereManager";
    private List<PicSphere> mPicSpheres;
    private CameraActivity mContext;
    private SnapshotManager mSnapManager;
    private Capture3DRenderer mCapture3DRenderer;
    private PicSphereRenderingService mBoundService;
    private FrameLayout mGLRootView;
    private TextureView mGLSurfaceView;
    private Handler mHandler;
    private boolean mIsBound;

    private ServiceConnection mServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            // This is called when the connection with the service has been
            // established, giving us the service object we can use to
            // interact with the service.  Because we have bound to a explicit
            // service that we know is running in our own process, we can
            // cast its IBinder to a concrete class and directly access it.
            mBoundService = ((PicSphereRenderingService.LocalBinder)service).getService();
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            // Because it is running in our same process, we should never
            // see this happen.
            mBoundService = null;
        }
    };

    public PicSphereManager(CameraActivity context, SnapshotManager snapMan) {
        mContext = context;
        mSnapManager = snapMan;
        mPicSpheres = new ArrayList<PicSphere>();
        mHandler = new Handler();
        mIsBound = false;
        doBindService();
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

    public void clearSpheres() {
        mPicSpheres.clear();
    }

    /**
     * Returns the 3D renderer context for PicSphere capture mode
     */
    public Capture3DRenderer getRenderer() {
        if (mCapture3DRenderer == null) {
            // Initialize the 3D renderer
            mCapture3DRenderer = new Capture3DRenderer(mContext, mContext.getCamManager());

            // We route the camera preview to our texture
            mGLRootView = (FrameLayout) mContext.findViewById(R.id.gl_renderer_container);
        }

        return mCapture3DRenderer;
    }

    /**
     * Returns the progress of the currently rendering picsphere
     * @return The percentage of progress, or -1 if no picsphere is rendering
     */
    public int getRenderingProgress() {
        if (mPicSpheres.size() > 0) {
            return mPicSpheres.get(0).getRenderProgress();
        }

        return -1;
    }

    /**
     * Starts rendering the provided PicSphere in a thread, and handles a service
     * that will keep on processing even once Nemesis is closed
     *
     * @param sphere The PicSphere to render
     */
    public void startRendering(final PicSphere sphere, final int orientation) {
        // Notify toast
        CameraActivity.notify(mContext.getString(R.string.picsphere_toast_background_render), 2500);

        if (mIsBound && mBoundService != null) {
            sphere.addProgressListener(this);
            mBoundService.render(sphere, orientation);
        } else {
            doBindService();
            mHandler.postDelayed(new Runnable() {
                public void run() {
                    startRendering(sphere, orientation);
                }
            }, 500);
        }
    }

    void doBindService() {
        // Establish a connection with the service.  We use an explicit
        // class name because we want a specific service implementation that
        // we know will be running in our own process (and thus won't be
        // supporting component replacement by other applications).
        Log.v(TAG, "Binding PicSphere rendering service");
        mContext.bindService(new Intent(mContext, PicSphereRenderingService.class),
                mServiceConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
    }

    public void tearDown() {
        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.onPause();
            mCapture3DRenderer = null;
        }

        mPicSpheres.clear();

        if (mIsBound) {
            // Detach our existing connection.
            mContext.unbindService(mServiceConnection);
            mIsBound = false;
        }
    }

    public void onPause() {
        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.onPause();
        }

        tearDown();

        mPicSpheres.clear();
    }

    public void onResume() {
        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.onResume();
        }
    }

    public int getSpheresCount() {
        return mPicSpheres.size();
    }

    /**
     * Copy the required binaries and libraries to the data folder
     * @return
     */
    private boolean copyBinaries() {
        boolean result = true;
        try {
            String files[] = {
                    "autooptimiser", "pto_gen", "cpfind", "multiblend", "enfuse", "nona", "pano_modify",
                    "ptclean", "tiffinfo", "align_image_stack", "pto_var",
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
                    result = false;
                }

                os.flush();
                os.close();
                is.close();
            }
        } catch (Exception e) {
            Log.e(TAG, "Error copying libraries and binaries", e);
            result = false;
        }
        return result;
    }


    @Override
    public void onRenderStart(PicSphere sphere) {

    }

    @Override
    public void onStepChange(PicSphere sphere, int newStep) {
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PICSPHERE) {
            mContext.setHelperText(String.format(mContext.getString(
                    R.string.picsphere_rendering_progress), sphere.getRenderProgress()));
        }
    }

    @Override
    public void onRenderDone(PicSphere sphere) {
        mPicSpheres.remove(sphere);

        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PICSPHERE) {
            mContext.setHelperText(mContext.getString(R.string.picsphere_start_hint));
        }

        if (mCapture3DRenderer != null) {
            mCapture3DRenderer.setCamPreviewVisible(true);
        }
    }
}
