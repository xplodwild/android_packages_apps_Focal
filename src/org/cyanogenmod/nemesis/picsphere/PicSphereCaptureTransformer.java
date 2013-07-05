package org.cyanogenmod.nemesis.picsphere;

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
    public void onSnapshotProcessed(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {
        mPicSphere.addPicture(info.mUri);
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
