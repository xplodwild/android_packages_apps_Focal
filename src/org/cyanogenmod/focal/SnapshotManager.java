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

package org.cyanogenmod.focal;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.CursorIndexOutOfBoundsException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.Camera;
import android.location.Location;
import android.media.CamcorderProfile;
import android.media.ExifInterface;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.util.Log;

import com.drew.imaging.jpeg.JpegMetadataReader;
import com.drew.imaging.jpeg.JpegProcessingException;
import com.drew.metadata.Directory;
import com.drew.metadata.Metadata;
import com.drew.metadata.Tag;

import org.cyanogenmod.focal.feats.AutoPictureEnhancer;
import org.cyanogenmod.focal.feats.PixelBuffer;
import org.cyanogenmod.focal.widgets.SimpleToggleWidget;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import fr.xplod.focal.R;

/**
 * This class manages taking snapshots and videos from Camera
 */
public class SnapshotManager {
    public final static String TAG = "SnapshotManager";
    private boolean mPaused;

    public interface SnapshotListener {
        /**
         * This callback is called when a snapshot is taken (shutter)
         *
         * @param info A structure containing information about the snapshot taken
         */
        public void onSnapshotShutter(SnapshotInfo info);

        /**
         * This callback is called when we have a preview for the snapshot
         *
         * @param info A structure containing information about the snapshot
         */
        public void onSnapshotPreview(SnapshotInfo info);

        /**
         * This callback is called when a snapshot is being processed (auto color enhancement, etc)
         *
         * @param info A structure containing information about the snapshot
         */
        public void onSnapshotProcessing(SnapshotInfo info);

        /**
         * This callback is called when a snapshot has been saved to the internal memory
         *
         * @param info A structure containing information about the snapshot
         */
        public void onSnapshotSaved(SnapshotInfo info);

        /**
         * This callback is called when ImageSaver starts a job of saving an image, or
         * MediaRecorder is storing a video
         * The primary purpose of this method is to show the SavePinger
         */
        public void onMediaSavingStart();

        /**
         * This callback is called when ImageSaver has done its job of saving an image,
         * or MediaRecorder is done storing a video
         * The primary purpose of this method is to hide the SavePinger
         */
        public void onMediaSavingDone();

        /**
         * This callback is called when a video starts recording
         */
        public void onVideoRecordingStart();

        /**
         * This callback is called when a video stops recording
         */
        public void onVideoRecordingStop();
    }

    public class SnapshotInfo {
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

        // Whether or not to bypass image processing (even if user enabled it)
        public boolean mBypassProcessing;
    }

    private Context mContext;
    private CameraManager mCameraManager;
    private FocusManager mFocusManager;
    private boolean mBypassProcessing;

    // Photo-related variables
    private boolean mWaitExposureSettle;
    private int mResetExposure;
    private List<SnapshotInfo> mSnapshotsQueue;
    private int mCurrentShutterQueueIndex;
    private List<SnapshotListener> mListeners;
    private Handler mHandler;
    private ContentResolver mContentResolver;
    private ImageSaver mImageSaver;
    private ImageNamer mImageNamer;
    private PixelBuffer mOffscreenGL;
    private AutoPictureEnhancer mAutoPicEnhancer;
    private boolean mImageIsProcessing;
    private boolean mDoAutoEnhance;

    // Video-related variables
    private long mRecordingStartTime;
    private boolean mIsRecording;
    private VideoNamer mVideoNamer;
    private CamcorderProfile mProfile;

    // The video file that the hardware camera is about to record into
    // (or is recording into.)
    private String mVideoFilename;
    private ParcelFileDescriptor mVideoFileDescriptor;

    // The video file that has already been recorded, and that is being
    // examined by the user.
    private String mCurrentVideoFilename;
    private Uri mCurrentVideoUri;
    private ContentValues mCurrentVideoValues;

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

            // If we used Samsung HDR, reset exposure
            if (mContext.getResources().getBoolean(R.bool.config_useSamsungHDR) &&
                SimpleToggleWidget.isWidgetEnabled(mContext, mCameraManager, "scene-mode", "hdr")) {
                mCameraManager.setParameterAsync("exposure-compensation",
                        Integer.toString(mResetExposure));
            }
        }
    };

    private Camera.PictureCallback mJpegPictureCallback = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] jpegData, Camera camera) {
            Log.v(TAG, "onPicture: Got JPEG data");
            mCameraManager.restartPreviewIfNeeded();

            // Calculate the width and the height of the jpeg.
            final Camera.Size s = mCameraManager.getParameters().getPictureSize();
            int orientation = CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_PANO ? 0 :
                    mCameraManager.getOrientation();
            final int width = s.width,
                    height = s.height;

            if (mSnapshotsQueue.size() == 0) {
                Log.e(TAG, "DERP! Why is snapshotqueue empty? Two JPEG callbacks!?");
                return;
            }

            final SnapshotInfo snap = mSnapshotsQueue.get(0);

            // If we have a Samsung HDR, convert from YUV422 to JPEG first
            if (mContext.getResources().getBoolean(R.bool.config_useSamsungHDR) &&
                SimpleToggleWidget.isWidgetEnabled(mContext, mCameraManager, "scene-mode", "hdr")) {
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                Bitmap bm = Util.decodeYUV422P(jpegData, s.width, s.height);
                // TODO: Replace 90 with real JPEG compression level when we'll have that setting
                bm.compress(Bitmap.CompressFormat.JPEG, 90, baos);
                jpegData = baos.toByteArray();
            }

            // Store the jpeg on internal memory if needed
            if (snap.mSave) {
                final Uri uri = mImageNamer.getUri();
                final String title = mImageNamer.getTitle();
                snap.mUri = uri;

                // If the orientation is somehow negative, avoid the Gallery crashing dumbly
                // (see com/android/gallery3d/ui/PhotoView.java line 758 (setTileViewPosition))
                while (orientation < 0) {
                    orientation += 360;
                }
                orientation = orientation % 360;

                final int correctedOrientation = orientation;
                final byte[] finalData = jpegData;

                if (!snap.mBypassProcessing && mDoAutoEnhance) {
                    new Thread() {
                        public void run() {
                            mImageIsProcessing = true;
                            for (SnapshotListener listener : mListeners) {
                                listener.onSnapshotProcessing(snap);
                            }

                            // Read EXIF
                            List<Tag> tagsList = new ArrayList<Tag>();
                            BufferedInputStream is = new BufferedInputStream(
                                    new ByteArrayInputStream(finalData));
                            try {
                                Metadata metadata = JpegMetadataReader.readMetadata(is, false);

                                for (Directory directory : metadata.getDirectories()) {
                                    for (Tag tag : directory.getTags()) {
                                        tagsList.add(tag);
                                    }
                                }
                            } catch (JpegProcessingException e) {
                                Log.e(TAG, "Error processing input JPEG", e);
                            }

                            // XXX: PixelBuffer has to be created every time because the GL context
                            // can only be used from its original thread. It's not very intense, but
                            // ideally we would be re-using the same thread every time.
                            try {
                                mOffscreenGL = new PixelBuffer(mContext, s.width, s.height);
                                mAutoPicEnhancer = new AutoPictureEnhancer(mContext);
                                mOffscreenGL.setRenderer(mAutoPicEnhancer);
                                mAutoPicEnhancer.setTexture(BitmapFactory.decodeByteArray(finalData,
                                        0, finalData.length));

                                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                                mOffscreenGL.getBitmap().compress(Bitmap.CompressFormat.JPEG, 90, baos);

                                if (mImageSaver != null) {
                                    mImageSaver.addImage(baos.toByteArray(), uri, title, null,
                                            width, height, correctedOrientation, tagsList, snap);
                                } else {
                                    Log.e(TAG, "ImageSaver was null: couldn't save image!");
                                }

                                if (mPaused && mImageSaver != null) {
                                    // We were paused, stop the saver now
                                    mImageSaver.finish();
                                    mImageSaver = null;
                                }

                                mImageIsProcessing = false;
                            }
                            catch (Exception e) {
                                // The rendering failed, the device might not be compatible for
                                // whatever reason. We just save the original file.
                                if (mImageSaver != null) {
                                    mImageSaver.addImage(finalData, uri, title, null,
                                            width, height, correctedOrientation, snap);
                                }

                                CameraActivity.notify("Auto-enhance failed: Original shot saved", 2000);
                            }
                            catch (OutOfMemoryError e) {
                                // The rendering failed, the device might not be compatible for
                                // whatever reason. We just save the original file.
                                if (mImageSaver != null) {
                                    mImageSaver.addImage(finalData, uri, title, null,
                                            width, height, correctedOrientation, snap);
                                }

                                CameraActivity.notify("Error: Out of memory. Original shot saved", 2000);
                            }
                        }
                    }.start();
                } else {
                    // Just save it as is
                    mImageSaver.addImage(finalData, uri, title, null,
                            width, height, correctedOrientation, snap);
                }
            }

            // Camera is ready to take another shot, doit
            if (mSnapshotsQueue.size() > mCurrentShutterQueueIndex + 1) {
                mCurrentShutterQueueIndex++;
                mHandler.post(mCaptureRunnable);
            }

            // We're done with our shot here!
            mCurrentShutterQueueIndex--;
            mSnapshotsQueue.remove(0);
        }
    };

    private Runnable mCaptureRunnable = new Runnable() {
        @Override
        public void run() {
            if (mWaitExposureSettle) {
                try {
                    Thread.sleep(200);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            mCameraManager.takeSnapshot(mShutterCallback, null, mJpegPictureCallback);
        }
    };

    private Runnable mPreviewCaptureRunnable = new Runnable() {
        @Override
        public void run() {
            Camera.Size s = mCameraManager.getParameters().getPreviewSize();
            mImageNamer.prepareUri(mContentResolver, System.currentTimeMillis(),
                    s.width, s.height, 0);

            SnapshotInfo info = new SnapshotInfo();
            info.mSave = true;
            info.mExposureCompensation = 0;
            info.mThumbnail = mCameraManager.getLastPreviewFrame();

            for (SnapshotListener listener : mListeners) {
                listener.onSnapshotShutter(info);
            }

            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            info.mThumbnail.compress(Bitmap.CompressFormat.JPEG, 80, baos);
            byte[] jpegData = baos.toByteArray();

            // Calculate the width and the height of the jpeg.
            int orientation = Exif.getOrientation(jpegData) - mCameraManager.getOrientation();
            int width, height;
            width = s.width;
            height = s.height;

            Uri uri = mImageNamer.getUri();
            String title = mImageNamer.getTitle();
            info.mUri = uri;

            mImageSaver.addImage(jpegData, uri, title, null,
                    width, height, orientation);

            for (SnapshotListener listener : mListeners) {
                listener.onSnapshotSaved(info);
            }
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
        mVideoNamer = new VideoNamer();
        mContentResolver = ctx.getContentResolver();
        mProfile = CamcorderProfile.get(CamcorderProfile.QUALITY_HIGH);
        mPaused = false;
        mImageIsProcessing = false;
    }

    public void addListener(SnapshotListener listener) {
        if (!mListeners.contains(listener)) {
            mListeners.add(listener);
        }
    }

    public void removeListener(SnapshotListener listener) {
        mListeners.remove(listener);
    }

    /**
     * Sets whether or not to bypass image processing (for burst shot or hdr for instance)
     * This value is reset after each snapshot queued!
     * @param bypass
     */
    public void setBypassProcessing(boolean bypass) {
        mBypassProcessing = bypass;
    }

    public void setAutoEnhance(boolean enhance) {
        mDoAutoEnhance = enhance;
    }

    public boolean getAutoEnhance() {
        return mDoAutoEnhance;
    }

    public void prepareNamerUri(int width, int height) {
        if (mImageNamer == null) {
            // ImageNamer can be dead if the user exitted the app.
            // We restart it temporarily.
            mImageNamer = new ImageNamer();
        }
        mImageNamer.prepareUri(mContentResolver,
                System.currentTimeMillis(), width, height, 0);
    }

    public Uri getNamerUri() {
        if (mImageNamer == null) {
            // ImageNamer can be dead if the user exitted the app.
            // We restart it temporarily.
            mImageNamer = new ImageNamer();
        }
        return mImageNamer.getUri();
    }

    public String getNamerTitle() {
        if (mImageNamer == null) {
            // ImageNamer can be dead if the user exitted the app.
            // We restart it temporarily.
            mImageNamer = new ImageNamer();
        }
        return mImageNamer.getTitle();
    }

    public void saveImage(Uri uri, String title, int width, int height,
                          int orientation, byte[] jpegData) {
        if (mImageSaver == null) {
            // ImageSaver can be dead if the user exitted the app.
            // We restart it temporarily.
            mImageSaver = new ImageSaver();
        }
        mImageSaver.addImage(jpegData, uri, title, null,
                width, height, orientation);
        mImageSaver.waitDone();
        mImageSaver.finish();
    }

    /**
     * Queues a snapshot that will be taken as soon as possible
     *
     * @param save                 Whether or not to save the snapshot in gallery
     *                             (for example, software HDR doesn't need all the shots to
     *                             be saved)
     * @param exposureCompensation If the shot has to be taken at a different
     *                             exposure value, otherwise set it to 0
     */
    public void queueSnapshot(boolean save, int exposureCompensation) {
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO
                && mContext.getResources().getBoolean(R.bool.config_usePreviewForVideoSnapshot)) {
            // We use the preview data for the video snapshot, instead of queueing a normal snapshot
            new Thread(mPreviewCaptureRunnable).start();
            return;
        }

        if (mSnapshotsQueue.size() == 2) return; // No more than 2 shots at a time

        // If we use Samsung HDR, we must set exposure level, as it corresponds to the HDR bracket
        if (mContext.getResources().getBoolean(R.bool.config_useSamsungHDR) &&
            SimpleToggleWidget.isWidgetEnabled(mContext, mCameraManager, "scene-mode", "hdr")) {
            exposureCompensation = mCameraManager.getParameters().getMaxExposureCompensation();
            mResetExposure = mCameraManager.getParameters().getExposureCompensation();
        }

        SnapshotInfo info = new SnapshotInfo();
        info.mSave = save;
        info.mExposureCompensation = exposureCompensation;
        info.mThumbnail = mCameraManager.getLastPreviewFrame();
        info.mBypassProcessing = mBypassProcessing;

        Camera.Parameters params = mCameraManager.getParameters();
        if (params != null && params.getExposureCompensation() != exposureCompensation) {
            mCameraManager.setParameterAsync("exposure-compensation",
                    Integer.toString(exposureCompensation));
            mWaitExposureSettle = true;
        }
        mSnapshotsQueue.add(info);

        // Reset bypass in any case
        mBypassProcessing = false;

        if (mSnapshotsQueue.size() == 1) {
            // We had no other snapshot queued so far, so start things up
            Log.v(TAG, "No snapshot queued, starting runnable");

            mCurrentShutterQueueIndex = 0;
            new Thread(mCaptureRunnable).start();
        }
    }

    public ImageNamer getImageNamer() {
        return mImageNamer;
    }

    public ImageSaver getImageSaver() {
        return mImageSaver;
    }

    public CamcorderProfile getVideoProfile(){
        return mProfile;
    }
    
    public void setVideoProfile(final CamcorderProfile profile) {
        mProfile = profile;

        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            // TODO: setVideoSize is handling preview changing
            mCameraManager.setVideoSize(profile.videoFrameWidth, profile.videoFrameHeight);
        }
    }

    /**
     * Starts recording a video with the current settings
     */
    public void startVideo() {
        Log.v(TAG, "startVideo");

        // Setup output file
        generateVideoFilename(mProfile.fileFormat);
        mCameraManager.prepareVideoRecording(mVideoFilename, mProfile);

        mCameraManager.startVideoRecording();
        mIsRecording = true;
        mRecordingStartTime = SystemClock.uptimeMillis();

        for (SnapshotListener listener : mListeners) {
            listener.onVideoRecordingStart();
        }
    }

    /**
     * Stops the current recording video, if any
     */
    public void stopVideo() {
        Log.v(TAG, "stopVideo");
        if (mIsRecording) {
            // Stop the video in a thread to not stall the UI thread
            new Thread() {
                public void run() {
                    mCameraManager.stopVideoRecording();

                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            for (SnapshotListener listener : mListeners) {
                                listener.onVideoRecordingStop();
                                listener.onMediaSavingDone();
                            }
                        }
                    });
                }
            }.start();

            mCurrentVideoFilename = mVideoFilename;

            mIsRecording = false;

            for (SnapshotListener listener : mListeners) {
                listener.onVideoRecordingStop();
                listener.onMediaSavingStart();
            }

            addVideoToMediaStore();
        }
    }

    /**
     * Returns whether or not a video is recording
     */
    public boolean isRecording() {
        return mIsRecording;
    }

    /**
     * Add the last recorded video to the MediaStore
     *
     * @return True if operation succeeded
     */
    private boolean addVideoToMediaStore() {
        boolean fail = false;
        if (mVideoFileDescriptor == null) {
            mCurrentVideoValues.put(MediaStore.Video.Media.SIZE,
                    new File(mCurrentVideoFilename).length());
            long duration = SystemClock.uptimeMillis() - mRecordingStartTime;
            if (duration > 0) {
                mCurrentVideoValues.put(MediaStore.Video.Media.DURATION, duration);
            } else {
                Log.w(TAG, "Video duration <= 0 : " + duration);
            }
            try {
                mCurrentVideoUri = mVideoNamer.getUri();

                // Rename the video file to the final name. This avoids other
                // apps reading incomplete data.  We need to do it after the
                // above mVideoNamer.getUri() call, so we are certain that the
                // previous insert to MediaProvider is completed.
                String finalName = mCurrentVideoValues.getAsString(
                        MediaStore.Video.Media.DATA);
                if (new File(mCurrentVideoFilename).renameTo(new File(finalName))) {
                    mCurrentVideoFilename = finalName;
                }

                mContentResolver.update(mCurrentVideoUri, mCurrentVideoValues
                        , null, null);
                mContext.sendBroadcast(new Intent(Util.ACTION_NEW_VIDEO,
                        mCurrentVideoUri));
            } catch (Exception e) {
                // We failed to insert into the database. This can happen if
                // the SD card is unmounted.
                Log.e(TAG, "failed to add video to media store", e);
                mCurrentVideoUri = null;
                mCurrentVideoFilename = null;
                fail = true;
            } finally {
                Log.v(TAG, "Current video URI: " + mCurrentVideoUri);
            }
        }
        mCurrentVideoValues = null;
        return fail;
    }

    /**
     * Generates a filename for the next video to record
     *
     * @param outputFileFormat The file format of the video
     */
    private void generateVideoFilename(int outputFileFormat) {
        long dateTaken = System.currentTimeMillis();
        String title = Util.createVideoName(dateTaken);
        // Used when emailing.
        String filename = title + convertOutputFormatToFileExt(outputFileFormat);
        String mime = convertOutputFormatToMimeType(outputFileFormat);
        String path = Storage.getStorage().generateDirectory() + '/' + filename;
        String tmpPath = path + ".tmp";
        mCurrentVideoValues = new ContentValues(7);
        mCurrentVideoValues.put(MediaStore.Video.Media.TITLE, title);
        mCurrentVideoValues.put(MediaStore.Video.Media.DISPLAY_NAME, filename);
        mCurrentVideoValues.put(MediaStore.Video.Media.DATE_TAKEN, dateTaken);
        mCurrentVideoValues.put(MediaStore.Video.Media.MIME_TYPE, mime);
        mCurrentVideoValues.put(MediaStore.Video.Media.DATA, path);
        mCurrentVideoValues.put(MediaStore.Video.Media.RESOLUTION,
                Integer.toString(mProfile.videoFrameWidth) + "x" +
                        Integer.toString(mProfile.videoFrameHeight));
        Location loc = null; // TODO: mLocationManager.getCurrentLocation();
        if (loc != null) {
            mCurrentVideoValues.put(MediaStore.Video.Media.LATITUDE, loc.getLatitude());
            mCurrentVideoValues.put(MediaStore.Video.Media.LONGITUDE, loc.getLongitude());
        }
        mVideoNamer.prepareUri(mContentResolver, mCurrentVideoValues);
        mVideoFilename = tmpPath;
        Log.v(TAG, "New video filename: " + mVideoFilename);
    }

    private String convertOutputFormatToMimeType(int outputFileFormat) {
        if (outputFileFormat == MediaRecorder.OutputFormat.MPEG_4) {
            return "video/mp4";
        }
        return "video/3gpp";
    }

    private String convertOutputFormatToFileExt(int outputFileFormat) {
        if (outputFileFormat == MediaRecorder.OutputFormat.MPEG_4) {
            return ".mp4";
        }
        return ".3gp";
    }

    public void onPause() {
        mPaused = true;

        if (!mImageIsProcessing && mImageSaver != null) {
            // We wait until the last processing image was saved
            mImageSaver.finish();
        }
        mImageNamer.finish();
        mVideoNamer.finish();
        mImageSaver = null;
        mImageNamer = null;
        mVideoNamer = null;
    }

    public void onResume() {
        mPaused = false;

        // Restore threads if needed
        if (mImageSaver == null) {
            mImageSaver = new ImageSaver();
        }

        if (mImageNamer == null) {
            mImageNamer = new ImageNamer();
        }

        if (mVideoNamer == null) {
            mVideoNamer = new VideoNamer();
        }
    }

    // Each SaveRequest remembers the data needed to save an image.
    private static class SaveRequest {
        byte[] data;
        Uri uri;
        String title;
        Location loc;
        int width, height;
        int orientation;
        List<Tag> exifTags;
        SnapshotInfo snap;
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
                             Location loc, int width, int height, int orientation, SnapshotInfo snap) {
            addImage(data, uri, title, loc, width, height, orientation, null, snap);
        }

        // Runs in main thread
        public void addImage(final byte[] data, Uri uri, String title,
                             Location loc, int width, int height, int orientation) {
           addImage(data, uri, title, loc, width, height, orientation, null, null);
        }

        // Runs in main thread
        public void addImage(final byte[] data, Uri uri, String title,
                             Location loc, int width, int height, int orientation,
                             List<Tag> exifTags, SnapshotInfo snap) {
            SaveRequest r = new SaveRequest();
            r.data = data;
            r.uri = uri;
            r.title = title;
            r.loc = (loc == null) ? null : new Location(loc);  // make a copy
            r.width = width;
            r.height = height;
            r.orientation = orientation;
            r.exifTags = exifTags;
            r.snap = snap;
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
                    for (SnapshotListener listener : mListeners) {
                        listener.onMediaSavingStart();
                    }
                }
                storeImage(r.data, r.uri, r.title, r.loc, r.width, r.height,
                        r.orientation, r.exifTags, r.snap);
                synchronized (this) {
                    mQueue.remove(0);
                    for (SnapshotListener listener : mListeners) {
                        listener.onMediaSavingDone();
                    }
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
                                Location loc, int width, int height, int orientation,
                                List<Tag> exifTags, SnapshotInfo snap) {
            boolean ok = Storage.getStorage().updateImage(mContentResolver, uri, title, loc,
                    orientation, data, width, height);

            if (ok) {
                if (exifTags != null && exifTags.size() > 0) {
                    // Write exif tags to final picture
                    try {
                        ExifInterface exifIf = new ExifInterface(Util.getRealPathFromURI(mContext, uri));

                        for (Tag tag : exifTags) {
                            // move along
                            String[] hack = tag.toString().split("\\]");
                            hack = hack[1].split("-");
                            exifIf.setAttribute(tag.getTagName(), hack[1].trim());
                        }

                        exifIf.saveAttributes();
                    } catch (IOException e) {
                        Log.e(TAG, "Couldn't write exif", e);
                    } catch (CursorIndexOutOfBoundsException e) {
                        Log.e(TAG, "Couldn't find original picture", e);
                    }
                }

                Util.broadcastNewPicture(mContext, uri);
            }

            if (snap != null) {
                for (SnapshotListener listener : mListeners) {
                    listener.onSnapshotSaved(snap);
                }
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
                    // Do nothing here
                }
            }

            // Return the uri generated
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


    private static class VideoNamer extends Thread {
        private boolean mRequestPending;
        private ContentResolver mResolver;
        private ContentValues mValues;
        private boolean mStop;
        private Uri mUri;

        // Runs in main thread
        public VideoNamer() {
            start();
        }

        // Runs in main thread
        public synchronized void prepareUri(
                ContentResolver resolver, ContentValues values) {
            mRequestPending = true;
            mResolver = resolver;
            mValues = new ContentValues(values);
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
            Uri uri = mUri;
            mUri = null;
            return uri;
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
                        // Do nothing here
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
            Uri videoTable = Uri.parse("content://media/external/video/media");
            mUri = mResolver.insert(videoTable, mValues);
        }

        // Runs in namer thread
        private void cleanOldUri() {
            if (mUri == null) return;
            mResolver.delete(mUri, null, null);
            mUri = null;
        }
    }
}
