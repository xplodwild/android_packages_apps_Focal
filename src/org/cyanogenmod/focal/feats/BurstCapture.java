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

import android.os.Handler;
import android.util.Log;

import org.cyanogenmod.focal.CameraActivity;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.ui.ShutterButton;

/**
 * Burst capture mode
 */
public class BurstCapture extends CaptureTransformer {
    public final static String TAG = "BurstCapture";

    private int mBurstCount = -1;
    private int mShotsDone;
    private boolean mBurstInProgress = false;
    private Handler mHandler;
    private CameraActivity mActivity;

    public BurstCapture(CameraActivity activity) {
        super(activity.getCamManager(), activity.getSnapManager());
        mHandler = new Handler();
        mActivity = activity;
    }

    /**
     * Set the number of shots to take, or 0 for an infinite shooting
     * (that will need to be stopped using terminateBurstShot)
     *
     * @param count The number of shots
     */
    public void setBurstCount(int count) {
        mBurstCount = count;
    }

    /**
     * Starts the burst shooting
     */
    public void startBurstShot() {
        mShotsDone = 0;
        mBurstInProgress = true;
        mSnapManager.queueSnapshot(true, 0);
    }

    public void terminateBurstShot() {
        mBurstInProgress = false;
    }

    private void tryTakeShot() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mSnapManager.setBypassProcessing(true);
                mSnapManager.queueSnapshot(true, 0);
            }
        });
    }

    @Override
    public void onShutterButtonClicked(ShutterButton button) {
        if (mBurstInProgress) {
            terminateBurstShot();
        } else {
            startBurstShot();
        }
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
        // XXX: Show it in the quick review drawer
        if (!mBurstInProgress) {
            return;
        }

        mShotsDone++;
        Log.v(TAG, "Done " + mShotsDone + " shots");

        if (mShotsDone < mBurstCount || mBurstCount == 0) {
            tryTakeShot();
        } else {
            mBurstInProgress = false;
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
}
