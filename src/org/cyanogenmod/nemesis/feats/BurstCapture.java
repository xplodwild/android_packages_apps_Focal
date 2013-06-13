package org.cyanogenmod.nemesis.feats;

import android.os.Handler;
import android.util.Log;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.ui.ShutterButton;

/**
 * Burst capture mode
 */
public class BurstCapture extends CaptureTransformer {
    public final static String TAG = "BurstCapture";

    private int mBurstCount = -1;
    private int mShotsDone;
    private boolean mBurstInProgress = false;
    private Handler mHandler;

    public BurstCapture(CameraManager camMan, SnapshotManager snapshotMan) {
        super(camMan, snapshotMan);
        mHandler = new Handler();
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
        mBurstInProgress = true;
        Log.e(TAG, "Queued snapshot -- start");
        mSnapManager.queueSnapshot(true, 0);
    }

    public void terminateBurstShot() {
        mBurstInProgress = false;
        Log.e(TAG, "terminate snapshot");
    }

    private void tryTakeShot() {
        // XXX: Find a better way to determine if the camera is ready to take a shot
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mSnapManager.queueSnapshot(true, 0);
            }
        }, 100);
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
        if (!mBurstInProgress) return;

        mShotsDone++;
        Log.e(TAG, "Done " + mShotsDone + " shots");

        if (mShotsDone < mBurstCount || mBurstCount == 0) {
            tryTakeShot();
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
