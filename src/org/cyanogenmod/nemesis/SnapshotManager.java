package org.cyanogenmod.nemesis;

import java.util.ArrayList;
import java.util.List;

import android.graphics.Bitmap;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;

/**
 * This class manages taking snapshots from Camera
 */
public class SnapshotManager {
    public final static String TAG = "SnapshotManager";
    
    public interface SnapshotListener {
        /**
         * This callback is called when a snapshot is taken (shutter)
         * @param info A structure containing information about the snapshot taken
         */
        public void onSnapshotShutter(SnapshotInfo info);
        
        /**
         * This callback is called when we have a preview for the snapshot
         * @param info A structure containing information about the snapshot
         */
        public void onSnapshotPreview(SnapshotInfo info);
        
        /**
         * This callback is called when a snapshot has been processed (vignetting, etc)
         * @param info A structure containing information about the snapshot
         */
        public void onSnapshotProcessed(SnapshotInfo info);
        
        /**
         * This callback is called when a snapshot has been saved to the internal memory
         * @param info A structure containing information about the snapshot
         */
        public void onSnapshotSaved(SnapshotInfo info);
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

    private List<SnapshotInfo> mSnapshotsQueue;
    private List<SnapshotListener> mListeners;
    private Handler mHandler;

    private Camera.ShutterCallback mShutterCallback = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {
            Log.e(TAG, "onShutter");
            
            // On shutter confirmed, play the capture sound + a small flashing animation
            SoundManager.getSingleton().play(SoundManager.SOUND_CAPTURE);
            
            final SnapshotInfo snap = mSnapshotsQueue.get(0);
            for (SnapshotListener listener : mListeners) {
                listener.onSnapshotShutter(snap);
            }
            
            // Camera is ready to take another shot, doit
            mSnapshotsQueue.remove(0);
            mHandler.post(mCaptureRunnable);
        }
    };
    
    private Camera.PictureCallback mJpegPictureCallback = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] arg0, Camera arg1) {
            Log.e(TAG, "Jpeg picture callback!");            
        }
    };

    private Runnable mCaptureRunnable = new Runnable() {
        @Override
        public void run() {
            Log.e(TAG, "takeSnapshot: go!");
            CameraManager.getSingleton(null).takeSnapshot(mShutterCallback, null, mJpegPictureCallback);
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
        mSnapshotsQueue = new ArrayList<SnapshotInfo>();
        mListeners = new ArrayList<SnapshotListener>();
        mHandler = new Handler();
    }
    
    public void addListener(SnapshotListener listener) {
        mListeners.add(listener);
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
        Log.e(TAG, "Queued a shot");

        if (mSnapshotsQueue.size() == 1) {
            // We had no other snapshot queued so far, so start things up
            Log.e(TAG, "No snapshot queued, starting runnable");
            mHandler.post(mCaptureRunnable);
        }
    }


}
