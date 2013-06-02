package org.cyanogenmod.nemesis;

import java.util.ArrayList;
import java.util.List;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.location.Location;
import android.media.CamcorderProfile;
import android.net.Uri;
import android.os.Environment;
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

    private List<SnapshotInfo> mSnapshotsQueue;
    private int mCurrentShutterQueueIndex;
    private List<SnapshotListener> mListeners;
    private Handler mHandler;
    private ContentResolver mContentResolver;
    private ImageSaver mImageSaver;
    private ImageNamer mImageNamer;
    private CameraManager mCameraManager;
    private FocusManager mFocusManager;
    private Context mContext;
    private boolean mIsRecording;

    private Camera.ShutterCallback mShutterCallback = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {
            Log.v(TAG, "onShutter");

            Camera.Size s = mCameraManager.getParameters().getPictureSize();
            mImageNamer.prepareUri(mContentResolver, System.currentTimeMillis(), s.width, s.height, 0);

            // On shutter confirmed, play a small flashing animation
            final SnapshotInfo snap = mSnapshotsQueue.get(mCurrentShutterQueueIndex);

            for (SnapshotListener listener : mListeners) {
                listener.onSnapshotShutter(snap);
            }

            // Camera is ready to take another shot, doit
            if (mSnapshotsQueue.size() > mCurrentShutterQueueIndex+1) {
                mCurrentShutterQueueIndex++;
                mHandler.post(mCaptureRunnable);
            }
        }
    };

    private Camera.PictureCallback mJpegPictureCallback = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] jpegData, Camera camera) {
            Log.v(TAG, "onPicture: Got JPEG data");
            mCameraManager.restartPreviewIfNeeded();

            final SnapshotInfo snap = mSnapshotsQueue.get(0);

            // Store the jpeg on internal memory if needed
            if (snap.mSave) {
                // Calculate the width and the height of the jpeg.
                Camera.Size s = mCameraManager.getParameters().getPictureSize();
                int orientation = Exif.getOrientation(jpegData) - mCameraManager.getOrientation();
                int width, height;
                width = s.width;
                height = s.height;

                Uri uri = mImageNamer.getUri();
                String title = mImageNamer.getTitle();
                mImageSaver.addImage(jpegData, uri, title, null,
                        width, height, orientation);
            }

            for (SnapshotListener listener : mListeners) {
                listener.onSnapshotSaved(snap);
            }

            // We're done with our shot here!
            mCurrentShutterQueueIndex--;
            mSnapshotsQueue.remove(0);
        }
    };

    private Runnable mCaptureRunnable = new Runnable() {
        @Override
        public void run() {
            mCameraManager.takeSnapshot(mShutterCallback, null, mJpegPictureCallback);
        }
    };

    public SnapshotManager(CameraManager man, FocusManager focusMan, Context ctx) {
        mContext = ctx;
        mCameraManager = man;
        mFocusManager = focusMan;
        mSnapshotsQueue = new ArrayList<SnapshotInfo>();
        mListeners = new ArrayList<SnapshotListener>();
        mHandler = new Handler();
        mImageSaver = new ImageSaver();
        mImageNamer = new ImageNamer();
        mContentResolver = ctx.getContentResolver();
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
        if (mSnapshotsQueue.size() == 2) return; // No more than 2 shots at a time

        SnapshotInfo info = new SnapshotInfo();
        info.mSave = save;
        info.mExposureCompensation = exposureCompensation;
        info.mThumbnail = mCameraManager.getLastPreviewFrame();

        mSnapshotsQueue.add(info);

        if (mSnapshotsQueue.size() == 1) {
            // We had no other snapshot queued so far, so start things up
            Log.v(TAG, "No snapshot queued, starting runnable");

            mCurrentShutterQueueIndex = 0;
            new Thread(mCaptureRunnable).start();
        }
    }
    
    /**
     * Starts recording a video with the current settings
     */
    public void startVideo() {
        Log.e(TAG, "startVideo");
        mCameraManager.prepareVideoRecording(Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_PICTURES) + "/test.mp4",
                CamcorderProfile.get(CamcorderProfile.QUALITY_HIGH));
        
        mCameraManager.startVideoRecording();
        mIsRecording = true;
    }
    
    /**
     * Stops the current recording video, if any
     */
    public void stopVideo() {
        Log.e(TAG, "stopVideo");
        if (mIsRecording) {
            mCameraManager.stopVideoRecording();
            mIsRecording = false;
        }
    }
    
    /**
     * Returns whether or not a video is recording
     */
    public boolean isRecording() {
        return mIsRecording;
    }

    // Each SaveRequest remembers the data needed to save an image.
    private static class SaveRequest {
        byte[] data;
        Uri uri;
        String title;
        Location loc;
        int width, height;
        int orientation;
    }

    // We use a queue to store the SaveRequests that have not been completed
    // yet. The main thread puts the request into the queue. The saver thread
    // gets it from the queue, does the work, and removes it from the queue.
    //
    // The main thread needs to wait for the saver thread to finish all the work
    // in the queue, when the activity's onPause() is called, we need to finish
    // all the work, so other programs (like Gallery) can see all the images.
    //
    // If the queue becomes too long, adding a new request will block the main
    // thread until the queue length drops below the threshold (QUEUE_LIMIT).
    // If we don't do this, we may face several problems: (1) We may OOM
    // because we are holding all the jpeg data in memory. (2) We may ANR
    // when we need to wait for saver thread finishing all the work (in
    // onPause() or gotoGallery()) because the time to finishing a long queue
    // of work may be too long.
    private class ImageSaver extends Thread {
        private static final int QUEUE_LIMIT = 3;

        private ArrayList<SaveRequest> mQueue;
        private boolean mStop;

        // Runs in main thread
        public ImageSaver() {
            mQueue = new ArrayList<SaveRequest>();
            start();
        }

        // Runs in main thread
        public void addImage(final byte[] data, Uri uri, String title,
                             Location loc, int width, int height, int orientation) {
            SaveRequest r = new SaveRequest();
            r.data = data;
            r.uri = uri;
            r.title = title;
            r.loc = (loc == null) ? null : new Location(loc);  // make a copy
            r.width = width;
            r.height = height;
            r.orientation = orientation;
            synchronized (this) {
                while (mQueue.size() >= QUEUE_LIMIT) {
                    try {
                        wait();
                    } catch (InterruptedException ex) {
                        // ignore.
                    }
                }
                mQueue.add(r);
                notifyAll();  // Tell saver thread there is new work to do.
            }
        }

        // Runs in saver thread
        @Override
        public void run() {
            while (true) {
                SaveRequest r;
                synchronized (this) {
                    if (mQueue.isEmpty()) {
                        notifyAll();  // notify main thread in waitDone

                        // Note that we can only stop after we saved all images
                        // in the queue.
                        if (mStop) break;

                        try {
                            wait();
                        } catch (InterruptedException ex) {
                            // ignore.
                        }
                        continue;
                    }
                    r = mQueue.get(0);
                }
                storeImage(r.data, r.uri, r.title, r.loc, r.width, r.height,
                        r.orientation);
                synchronized (this) {
                    mQueue.remove(0);
                    notifyAll();  // the main thread may wait in addImage
                }
            }
        }

        // Runs in main thread
        public void waitDone() {
            synchronized (this) {
                while (!mQueue.isEmpty()) {
                    try {
                        wait();
                    } catch (InterruptedException ex) {
                        // ignore.
                    }
                }
            }
        }

        // Runs in main thread
        public void finish() {
            waitDone();
            synchronized (this) {
                mStop = true;
                notifyAll();
            }
            try {
                join();
            } catch (InterruptedException ex) {
                // ignore.
            }
        }

        // Runs in saver thread
        private void storeImage(final byte[] data, Uri uri, String title,
                                Location loc, int width, int height, int orientation) {
            boolean ok = Storage.getStorage().updateImage(mContentResolver, uri, title, loc,
                    orientation, data, width, height);
            if (ok) {
                Util.broadcastNewPicture(mContext, uri);
            }
        }
    }

    private static class ImageNamer extends Thread {
        private boolean mRequestPending;
        private ContentResolver mResolver;
        private long mDateTaken;
        private int mWidth, mHeight;
        private boolean mStop;
        private Uri mUri;
        private String mTitle;

        // Runs in main thread
        public ImageNamer() {
            start();
        }

        // Runs in main thread
        public synchronized void prepareUri(ContentResolver resolver,
                                            long dateTaken, int width, int height, int rotation) {
            if (rotation % 180 != 0) {
                int tmp = width;
                width = height;
                height = tmp;
            }
            mRequestPending = true;
            mResolver = resolver;
            mDateTaken = dateTaken;
            mWidth = width;
            mHeight = height;
            notifyAll();
        }

        // Runs in main thread
        public synchronized Uri getUri() {
            // wait until the request is done.
            while (mRequestPending) {
                try {
                    wait();
                } catch (InterruptedException ex) {
                    // ignore.
                }
            }

            // return the uri generated
            Uri uri = mUri;
            mUri = null;
            return uri;
        }

        // Runs in main thread, should be called after getUri().
        public synchronized String getTitle() {
            return mTitle;
        }

        // Runs in namer thread
        @Override
        public synchronized void run() {
            while (true) {
                if (mStop) break;
                if (!mRequestPending) {
                    try {
                        wait();
                    } catch (InterruptedException ex) {
                        // ignore.
                    }
                    continue;
                }
                cleanOldUri();
                generateUri();
                mRequestPending = false;
                notifyAll();
            }
            cleanOldUri();
        }

        // Runs in main thread
        public synchronized void finish() {
            mStop = true;
            notifyAll();
        }

        // Runs in namer thread
        private void generateUri() {
            mTitle = Util.createJpegName(mDateTaken);
            mUri = Storage.getStorage().newImage(mResolver, mTitle, mDateTaken, mWidth, mHeight);
        }

        // Runs in namer thread
        private void cleanOldUri() {
            if (mUri == null) return;
            Storage.getStorage().deleteImage(mResolver, mUri);
            mUri = null;
        }
    }
}
