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

package org.cyanogenmod.nemesis.feats;

import android.net.Uri;
import android.os.Handler;
import android.util.Log;

import org.cyanogenmod.nemesis.CameraActivity;
import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.Util;
import org.cyanogenmod.nemesis.ui.ShutterButton;

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

    public SoftwareHdrCapture(CameraActivity activity) {
        super(activity.getCamManager(), activity.getSnapManager());
        mHandler = new Handler();
        mPictures = new ArrayList<Uri>();
        mActivity = activity;
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
        if (!mBurstInProgress) return;

        mPictures.add(Uri.fromFile(new File(Util.getRealPathFromURI(mActivity, info.mUri))));

        mShotsDone++;
        Log.v(TAG, "Done " + mShotsDone + " shots");

        if (mShotsDone < SHOTS_COUNT) {
            tryTakeShot();
        } else {
            // Reset exposure
            mCamManager.getParameters().setExposureCompensation(0);

            // Process
            SoftwareHdrProcessor processor = new SoftwareHdrProcessor(mActivity, mSnapManager);
            processor.setPictures(mPictures);
            processor.render();
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
