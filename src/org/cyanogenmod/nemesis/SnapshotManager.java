package org.cyanogenmod.nemesis;

import android.graphics.Bitmap;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Handler;

import java.util.Queue;
import java.util.concurrent.SynchronousQueue;

/**
 * This class manages taking snapshots from Camera
 */
public class SnapshotManager {
    public interface SnapshotListener {
        /**
         * This callback is called when a snapshot is fully taken
         * @param info A structure containing information about the snapshot taken
         */
        public void onSnapshotTaken(SnapshotInfo info);
    }

    protected class SnapshotInfo {
        // Whether or not the snapshot has to be saved to internal memory
        public boolean mSave;

        // The URI of the saved shot, if it has been saved, and if the save is done (mSave = true).
        // This field will likely be only different than null when onSnapshotSaved is called.
        public Uri mUri;

        // The exposure compensation value set for the shot, IF a specific
        // value was needed
        public int mExposureCompensation;

        // A bitmap containing a thumbnail of the image
        public Bitmap mThumbnail;
    }


    private static SnapshotManager mSingleton;

    private Queue<SnapshotInfo> mSnapshotsQueue;
    private Handler mHandler;

    private Camera.ShutterCallback mShutterCallback = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {

        }
    };

    private Runnable mCaptureRunnable = new Runnable() {
        @Override
        public void run() {
            //CameraManager.getSingleton(null).takeSnapshot();
        }
    };


    /**
     * @return The single instance of the class
     */
    public static SnapshotManager getSingleton() {
        if (mSingleton == null)
            mSingleton = new SnapshotManager();

        return mSingleton;
    }

    private SnapshotManager() {
        mSnapshotsQueue = new SynchronousQueue<SnapshotInfo>();
        mHandler = new Handler();
    }

    /**
     * Queues a snapshot that will be taken as soon as possible
     * @param save Whether or not to save the snapshot in gallery
     *             (for example, software HDR doesn't need all the shots to
     *             be saved)
     * @param exposureCompensation If the shot has to be taken at a different
     *                             exposure value, otherwise set it to 0
     */
    public void queueSnapshot(boolean save, int exposureCompensation) {
        SnapshotInfo info = new SnapshotInfo();
        info.mSave = save;
        info.mExposureCompensation = exposureCompensation;

        mSnapshotsQueue.add(info);

        if (mSnapshotsQueue.size() == 1) {
            // We had no other snapshot queued so far, so start things up
            mHandler.post(mCaptureRunnable);
        }
    }


}
