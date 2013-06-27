package org.cyanogenmod.nemesis.feats;

import android.os.Handler;
import android.util.Log;

import org.cyanogenmod.nemesis.CameraActivity;
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

        // Open the quick review drawer
        mActivity.getReviewDrawer().openQuickReview();
    }

    public void terminateBurstShot() {
        mBurstInProgress = false;
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
        Log.v(TAG, "Done " + mShotsDone + " shots");

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
