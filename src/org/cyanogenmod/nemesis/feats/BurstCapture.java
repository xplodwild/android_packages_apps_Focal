package org.cyanogenmod.nemesis.feats;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.SnapshotManager;

/**
 * Burst capture mode
 */
public class BurstCapture extends CaptureTransformer {
    private int mBurstCount = -1;
    private int mShotsDone;

    public BurstCapture(CameraManager camMan, SnapshotManager snapshotMan) {
        super(camMan, snapshotMan);
    }

    /**
     * Set the number of shots to take, or 0 for an infinite shooting
     * (that will need to be stopped using terminateBurstShot)
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
        mSnapManager.queueSnapshot(true, 0);
    }

    @Override
    public void onShutterButtonClicked() {
        startBurstShot();
    }

    @Override
    public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {
        mShotsDone++;

        if (mShotsDone < mBurstCount || mBurstCount == 0) {
            mSnapManager.queueSnapshot(true, 0);
        }
    }

    @Override
    public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotProcessed(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {
        // XXX: Show it in the quick review drawer
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
