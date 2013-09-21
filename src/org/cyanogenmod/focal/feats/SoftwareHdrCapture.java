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

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.Util;
import org.cyanogenmod.focal.ui.ShutterButton;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Software HDR capture mode
 */
public class SoftwareHdrCapture extends CaptureTransformer {
    public final static String TAG = "SoftwareHdrCapture";
    private final static int SHOTS_COUNT = 3;

    private int mShotsDone;
    private boolean mBurstInProgress = false;
    private Handler mHandler;
    private CameraActivity mActivity;
    private List<Uri> mPictures;
    private List<Uri> mPicturesUri;
    private static SoftwareHdrRenderingService mBoundService;
    private static boolean mIsBound;

    private static ServiceConnection mServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            // This is called when the connection with the service has been
            // established, giving us the service object we can use to
            // interact with the service.  Because we have bound to a explicit
            // service that we know is running in our own process, we can
            // cast its IBinder to a concrete class and directly access it.
            mBoundService = ((SoftwareHdrRenderingService.LocalBinder)service).getService();
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            // Because it is running in our same process, we should never
            // see this happen.
            mBoundService = null;
        }
    };

    public SoftwareHdrCapture(CameraActivity activity) {
        super(activity.getCamManager(), activity.getSnapManager());
        mHandler = new Handler();
        mPictures = new ArrayList<Uri>();
        mPicturesUri = new ArrayList<Uri>();
        mActivity = activity;
        doBindService();
    }

    public static ServiceConnection getServiceConnection() {
        return mServiceConnection;
    }

    public static boolean isServiceBound() {
        return mIsBound;
    }

    void doBindService() {
        // Establish a connection with the service.  We use an explicit
        // class name because we want a specific service implementation that
        // we know will be running in our own process (and thus won't be
        // supporting component replacement by other applications).
        Log.v(TAG, "Binding Software HDR rendering service");
        mActivity.bindService(new Intent(mActivity, SoftwareHdrRenderingService.class),
                mServiceConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
    }

    public int getShotExposure(int shotId) {
        if (shotId == 0) {
            return mCamManager.getParameters().getMinExposureCompensation();
        } else if (shotId == 1) {
            return 0;
        } else if (shotId == 2) {
            return mCamManager.getParameters().getMaxExposureCompensation();
        }

        Log.e(TAG, "Unknown shot exposure ID " + shotId);
        return 0;
    }

    /**
     * Starts the HDR shooting
     */
    public void startBurstShot() {
        mShotsDone = 0;
        mBurstInProgress = true;
        mSnapManager.queueSnapshot(true, getShotExposure(mShotsDone));

        // Open the quick review drawer
        mActivity.getReviewDrawer().openQuickReview();
    }

    private void tryTakeShot() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mSnapManager.setBypassProcessing(true);
                mSnapManager.queueSnapshot(true, getShotExposure(mShotsDone));
            }
        });
    }

    @Override
    public void onShutterButtonClicked(ShutterButton button) {
        startBurstShot();
    }

    @Override
    public void onSnapshotShutter(final SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotProcessing(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {
        if (!mBurstInProgress) {
            return;
        }

        mPictures.add(Uri.fromFile(new File(Util.getRealPathFromURI(mActivity, info.mUri))));
        mPicturesUri.add(info.mUri);

        mShotsDone++;
        Log.v(TAG, "Done " + mShotsDone + " shots");

        if (mShotsDone < SHOTS_COUNT) {
            tryTakeShot();
        } else {
            // Reset exposure
            mCamManager.getParameters().setExposureCompensation(0);

            // Render
            int orientation = (360 - mActivity.getOrientation()) % 360;
            mBoundService.render(mPictures, mPicturesUri, mActivity.getSnapManager(), orientation);

            mShotsDone = 0;
        }
    }

    @Override
    public void onMediaSavingStart() {

    }

    @Override
    public void onMediaSavingDone() {

    }

    @Override
    public void onVideoRecordingStart() {

    }

    @Override
    public void onVideoRecordingStop() {

    }

    public void tearDown() {
        if (mIsBound) {
            // Detach our existing connection.
            mActivity.unbindService(mServiceConnection);
            mIsBound = false;
        }
    }
}
