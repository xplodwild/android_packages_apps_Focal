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

import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import fr.xplod.focal.R;

/**
 * Service that handles PicSphere rendering outside
 * the app context (as the rendering thread would get killed)
 */
public class PicSphereRenderingService extends Service implements PicSphere.ProgressListener {
    private NotificationManager mNM;

    // Unique Identification Number for the Notification.
    // We use it on Notification start, and to cancel it.
    private int NOTIFICATION = 1337;

    private boolean mHasFailed = false;
    private int mRenderQueue = 0;

    /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        PicSphereRenderingService getService() {
            return PicSphereRenderingService.this;
        }
    }

    @Override
    public void onCreate() {
        mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i("LocalService", "Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        // Cancel the persistent notification.
        mNM.cancel(NOTIFICATION);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();

    public void render(final PicSphere sphere, final int orientation) {
        sphere.addProgressListener(this);
        mRenderQueue++;
        new Thread() {
            public void run() {
                if (!sphere.render(orientation)) {
                    mHasFailed = true;
                    mNM.notify(NOTIFICATION,
                            buildFailureNotification(getString(R.string.picsphere_failed),
                                    getString(R.string.picsphere_failed_details)));
                }
                mRenderQueue--;

                if (mRenderQueue == 0) {
                    // We have no more PicSpheres to render, so we can stop the service. Otherwise,
                    // we leave it on for other spheres.
                    PicSphereRenderingService.this.stopSelf();
                }
            }
        }.start();
    }

    @Override
    public void onRenderStart(PicSphere sphere) {
        // Display a notification
        mNM.notify(NOTIFICATION, buildProgressNotification(0,
                getString(R.string.picsphere_step_preparing)));
        mHasFailed = false;
    }

    @Override
    public void onStepChange(PicSphere sphere, int newStep) {
        int progressPerStep = 100 / PicSphere.STEP_TOTAL;
        int progress = newStep * progressPerStep;
        String text = "";

        switch (newStep) {
            case PicSphere.STEP_PTOGEN:
                text = getString(R.string.picsphere_step_ptogen);
                break;

            case PicSphere.STEP_PTOVAR:
                text = getString(R.string.picsphere_step_ptovar);
                break;

            case PicSphere.STEP_CPFIND:
                text = getString(R.string.picsphere_step_cpfind);
                break;

            case PicSphere.STEP_PTCLEAN:
                text = getString(R.string.picsphere_step_ptclean);
                break;

            case PicSphere.STEP_AUTOOPTIMISER:
                text = getString(R.string.picsphere_step_autooptimiser);
                break;

            case PicSphere.STEP_PANOMODIFY:
                text = getString(R.string.picsphere_step_panomodify);
                break;

            case PicSphere.STEP_NONA:
                text = getString(R.string.picsphere_step_nona);
                break;

            case PicSphere.STEP_ENBLEND:
                text = getString(R.string.picsphere_step_enblend);
                break;
        }

        mNM.notify(NOTIFICATION, buildProgressNotification(progress, text));
    }

    @Override
    public void onRenderDone(PicSphere sphere) {
        if (!mHasFailed) {
            mNM.cancel(NOTIFICATION);
        }
    }

    private Notification buildProgressNotification(int percentage, String text) {
        Notification.Builder mBuilder =
                new Notification.Builder(this)
                        .setSmallIcon(R.drawable.ic_launcher)
                        .setContentTitle(getString(R.string.picsphere_notif_title))
                        .setContentText(percentage+"% ("+text+")")
                        .setOngoing(true);

        return mBuilder.build();
    }

    private Notification buildFailureNotification(String title, String text) {
        Notification.Builder mBuilder =
                new Notification.Builder(this)
                        .setSmallIcon(R.drawable.ic_launcher)
                        .setContentTitle(title)
                        .setContentText(text)
                        .setOngoing(false);

        return mBuilder.build();
    }
}
