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

import android.util.Log;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.feats.CaptureTransformer;
import org.cyanogenmod.nemesis.ui.ShutterButton;

/**
 * Capture Transformer for PicSphere that will store all shots to feed them to a new PicSphere
 * created by PicSphereManager
 */
public class PicSphereCaptureTransformer extends CaptureTransformer {
    public final static String TAG = "PicSphereCaptureTransformer";
    private PicSphereManager mPicSphereManager;
    private PicSphere mPicSphere;

    public PicSphereCaptureTransformer(PicSphereManager psMan, CameraManager camMan, SnapshotManager snapshotMan) {
        super(camMan, snapshotMan);
        mPicSphereManager = psMan;
    }

    @Override
    public void onShutterButtonClicked(ShutterButton button) {
        if (mPicSphere == null) {
            // Initialize a new sphere
            mPicSphere = mPicSphereManager.createPicSphere();
        }

        mSnapManager.queueSnapshot(true, 0);
    }

    @Override
    public void onShutterButtonLongPressed(ShutterButton button) {
        if (mPicSphere != null) {
            mPicSphereManager.startRendering(mPicSphere);
            mPicSphereManager.getRenderer().clearSnapshots();
            mPicSphere = null;
        }
    }

    @Override
    public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {
        mPicSphereManager.getRenderer().addSnapshot(info.mThumbnail);
    }

    @Override
    public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotProcessing(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {
        if (mPicSphere != null) {
            mPicSphere.addPicture(info.mUri);
        } else {
            Log.e(TAG, "No current PicSphere");
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
