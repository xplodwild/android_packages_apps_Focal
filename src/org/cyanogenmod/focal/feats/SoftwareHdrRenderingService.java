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

package org.cyanogenmod.focal.feats;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import fr.xplod.focal.R;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.Util;
import org.cyanogenmod.focal.picsphere.PicSphere;

import java.io.File;
import java.util.List;

/**
 * Service that handles HDR rendering outside
 * the app context (as the rendering thread would get killed)
 */
public class SoftwareHdrRenderingService extends Service {
    private NotificationManager mNM;

    // Unique Identification Number for the Notification.
    // We use it on Notification start, and to cancel it.
    private int NOTIFICATION = 4242;

    private boolean mHasFailed = false;

    /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        SoftwareHdrRenderingService getService() {
            return SoftwareHdrRenderingService.this;
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

    public void render(final List<Uri> pictures, final List<Uri> picturesUri,
                       final SnapshotManager snapMan, final int orientation) {
        // Display a notification
        mNM.notify(NOTIFICATION, buildProgressNotification());
        mHasFailed = false;

        new Thread() {
            public void run() {
                SoftwareHdrProcessor processor =
                        new SoftwareHdrProcessor(SoftwareHdrRenderingService.this, snapMan);
                processor.setPictures(pictures);

                if (processor.render(orientation)) {
                    mNM.cancel(NOTIFICATION);
                    removeTempFiles(picturesUri, processor.getTempPath());
                } else {
                    mHasFailed = true;
                    mNM.notify(NOTIFICATION, buildFailureNotification(getString(
                            R.string.software_hdr_failed), getString(
                            R.string.software_hdr_failed_details)));
                }

                SoftwareHdrRenderingService.this.stopSelf();
            }
        }.start();
    }

    private void removeTempFiles(List<Uri> pictures, File tempPath) {
        // Remove source pictures and temporary path
        for (Uri uri : pictures) {
            List<String> segments = uri.getPathSegments();
            Util.removeFromGallery(getContentResolver(),
                    Integer.parseInt(segments.get(segments.size() - 1)));
        }

        tempPath.delete();
    }

    private Notification buildProgressNotification() {
        Notification.Builder mBuilder =
                new Notification.Builder(this)
                        .setSmallIcon(R.drawable.ic_launcher)
                        .setContentTitle(getString(R.string.software_hdr_notif_title))
                        .setContentText(getString(R.string.please_wait))
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
