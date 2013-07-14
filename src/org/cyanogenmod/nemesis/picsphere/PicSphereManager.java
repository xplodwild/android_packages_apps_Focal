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

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.TaskStackBuilder;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.util.Log;

import org.cyanogenmod.nemesis.CameraActivity;
import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.SnapshotManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

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
    private Map<PicSphere, Integer> mPicSpheresNotif;
    int mNextNotificationId;
    private Context mContext;
    private SnapshotManager mSnapManager;
    private Capture3DRenderer mCapture3DRenderer;

    public PicSphereManager(Context context, SnapshotManager snapMan) {
        mContext = context;
        mSnapManager = snapMan;
        mPicSpheres = new ArrayList<PicSphere>();
        mPicSpheresNotif = new HashMap<PicSphere, Integer>();
        mNextNotificationId = 0;
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
                sphere.setProgressListener(PicSphereManager.this);
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

    @Override
    public void onRenderStart(PicSphere sphere) {
        // Notify toast
        CameraActivity.notify("Rendering your PicSphere in the background...", 2500);

        // Display a notification
        NotificationManager mNotificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.notify(mNextNotificationId, buildProgressNotification(0, "Preparing..."));

        // Store notification ID to reuse it later
        mPicSpheresNotif.put(sphere, mNextNotificationId);
        mNextNotificationId++;
    }

    @Override
    public void onStepChange(PicSphere sphere, int newStep) {
        int progressPerStep = 100 / PicSphere.STEP_TOTAL;
        int progress = newStep * progressPerStep;
        String text = "";

        switch (newStep) {
            case PicSphere.STEP_AUTOPANO:
                text = mContext.getString(R.string.picsphere_step_autopano);
                break;

            case PicSphere.STEP_PTCLEAN:
                text = mContext.getString(R.string.picsphere_step_ptclean);
                break;

            case PicSphere.STEP_AUTOOPTIMISER:
                text = mContext.getString(R.string.picsphere_step_autooptimiser);
                break;

            case PicSphere.STEP_PANOMODIFY:
                text = mContext.getString(R.string.picsphere_step_panomodify);
                break;

            case PicSphere.STEP_NONA:
                text = mContext.getString(R.string.picsphere_step_nona);
                break;

            case PicSphere.STEP_ENBLEND:
                text = mContext.getString(R.string.picsphere_step_enblend);
                break;
        }

        NotificationManager mNotificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.notify(mPicSpheresNotif.get(sphere),
                buildProgressNotification(progress, text));
    }

    @Override
    public void onRenderDone(PicSphere sphere) {
        NotificationManager mNotificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.cancel(mPicSpheresNotif.get(sphere));
    }

    private Notification buildProgressNotification(int percentage, String text) {
        Notification.Builder mBuilder =
                new Notification.Builder(mContext)
                        .setSmallIcon(R.drawable.ic_launcher)
                        .setContentTitle(mContext.getString(R.string.picsphere_notif_title))
                        .setContentText(percentage+"% ("+text+")")
                        .setOngoing(true);

        return mBuilder.build();
    }
}
