/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2013 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.nemesis;

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.provider.MediaStore.MediaColumns;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;

public class Storage {
    private static final String TAG = "CameraStorage";

    public static final long UNAVAILABLE = -1L;
    public static final long PREPARING = -2L;
    public static final long UNKNOWN_SIZE = -3L;
    public static final long LOW_STORAGE_THRESHOLD= 50000000;

    private String mRoot = Environment.getExternalStorageDirectory().toString();
    private static Storage sStorage;

    // Singleton
    private Storage() {}

    public static Storage getStorage() {
        if (sStorage == null) {
            sStorage = new Storage();
        }
        return sStorage;
    }

    public void setRoot(String root) {
        mRoot = root;
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private static void setImageSize(ContentValues values, int width, int height) {
        values.put(MediaColumns.WIDTH, width);
        values.put(MediaColumns.HEIGHT, height);
    }

    public String writeFile(String title, byte[] data) {
        String path = generateFilepath(title);
        FileOutputStream out = null;
        try {
            out = new FileOutputStream(path);
            out.write(data);
        } catch (Exception e) {
            Log.e(TAG, "Failed to write data", e);
        } finally {
            try {
                out.close();
            } catch (Exception e) {
            }
        }
        return path;
    }

    // Save the image and add it to media store.
    public Uri addImage(ContentResolver resolver, String title,
                        long date, Location location, int orientation, byte[] jpeg,
                        int width, int height) {
        // Save the image.
        String path = writeFile(title, jpeg);
        return addImage(resolver, title, date, location, orientation,
                jpeg.length, path, width, height);
    }

    // Add the image to media store.
    public Uri addImage(ContentResolver resolver, String title,
                        long date, Location location, int orientation, int jpegLength,
                        String path, int width, int height) {
        // Insert into MediaStore.
        ContentValues values = new ContentValues(9);
        values.put(ImageColumns.TITLE, title);
        values.put(ImageColumns.DISPLAY_NAME, title + ".jpg");
        values.put(ImageColumns.DATE_TAKEN, date);
        values.put(ImageColumns.MIME_TYPE, "image/jpeg");
        // Clockwise rotation in degrees. 0, 90, 180, or 270.
        values.put(ImageColumns.ORIENTATION, orientation);
        values.put(ImageColumns.DATA, path);
        values.put(ImageColumns.SIZE, jpegLength);

        setImageSize(values, width, height);

        if (location != null) {
            values.put(ImageColumns.LATITUDE, location.getLatitude());
            values.put(ImageColumns.LONGITUDE, location.getLongitude());
        }

        Uri uri = null;
        try {
            uri = resolver.insert(Images.Media.EXTERNAL_CONTENT_URI, values);
        } catch (Throwable th)  {
            // This can happen when the external volume is already mounted, but
            // MediaScanner has not notify MediaProvider to add that volume.
            // The picture is still safe and MediaScanner will find it and
            // insert it into MediaProvider. The only problem is that the user
            // cannot click the thumbnail to review the picture.
            Log.e(TAG, "Failed to write MediaStore" + th);
        }
        return uri;
    }

    // newImage() and updateImage() together do the same work as
    // addImage. newImage() is the first step, and it inserts the DATE_TAKEN and
    // DATA fields into the database.
    //
    // We also insert hint values for the WIDTH and HEIGHT fields to give
    // correct aspect ratio before the real values are updated in updateImage().
    public Uri newImage(ContentResolver resolver, String title,
                        long date, int width, int height) {
        String path = generateFilepath(title);

        // Insert into MediaStore.
        ContentValues values = new ContentValues(4);
        values.put(ImageColumns.DATE_TAKEN, date);
        values.put(ImageColumns.DATA, path);

        setImageSize(values, width, height);

        Uri uri = null;
        try {
            uri = resolver.insert(Images.Media.EXTERNAL_CONTENT_URI, values);
        } catch (Throwable th)  {
            // This can happen when the external volume is already mounted, but
            // MediaScanner has not notify MediaProvider to add that volume.
            // The picture is still safe and MediaScanner will find it and
            // insert it into MediaProvider. The only problem is that the user
            // cannot click the thumbnail to review the picture.
            Log.e(TAG, "Failed to new image" + th);
        }
        return uri;
    }

    // This is the second step. It completes the partial data added by
    // newImage. All columns other than DATE_TAKEN and DATA are inserted
    // here. This method also save the image data into the file.
    //
    // Returns true if the update is successful.
    public boolean updateImage(ContentResolver resolver, Uri uri,
                               String title, Location location, int orientation, byte[] jpeg,
                               int width, int height) {
        // Save the image.
        String path = generateFilepath(title);
        String tmpPath = path + ".tmp";
        FileOutputStream out = null;
        try {
            // Write to a temporary file and rename it to the final name. This
            // avoids other apps reading incomplete data.
            out = new FileOutputStream(tmpPath);
            out.write(jpeg);
            out.close();
            new File(tmpPath).renameTo(new File(path));
        } catch (Exception e) {
            Log.e(TAG, "Failed to write image", e);
            return false;
        } finally {
            try {
                out.close();
            } catch (Exception e) {
            }
        }

        // Insert into MediaStore.
        ContentValues values = new ContentValues(9);
        values.put(ImageColumns.TITLE, title);
        values.put(ImageColumns.DISPLAY_NAME, title + ".jpg");
        values.put(ImageColumns.MIME_TYPE, "image/jpeg");
        // Clockwise rotation in degrees. 0, 90, 180, or 270.
        values.put(ImageColumns.ORIENTATION, orientation);
        values.put(ImageColumns.SIZE, jpeg.length);

        setImageSize(values, width, height);

        if (location != null) {
            values.put(ImageColumns.LATITUDE, location.getLatitude());
            values.put(ImageColumns.LONGITUDE, location.getLongitude());
        }

        try {
            resolver.update(uri, values, null, null);
        } catch (Throwable th) {
            Log.e(TAG, "Failed to update image (" + th + ") ; uri=" + uri + " values=" + values);
            return false;
        }

        return true;
    }

    public void deleteImage(ContentResolver resolver, Uri uri) {
        try {
            resolver.delete(uri, null, null);
        } catch (Throwable th) {
            Log.e(TAG, "Failed to delete image: " + uri);
        }
    }

    private String generateDCIM() {
        return new File(mRoot, Environment.DIRECTORY_DCIM).toString();
    }

    public String generateDirectory() {
        return generateDCIM() + "/Camera";
    }

    private String generateFilepath(String title) {
        return generateDirectory() + '/' + title + ".jpg";
    }

    public String generateBucketId() {
        return String.valueOf(generateDirectory().toLowerCase().hashCode());
    }

    public int generateBucketIdInt() {
        return generateDirectory().toLowerCase().hashCode();
    }

    public long getAvailableSpace() {
        String state = Environment.getExternalStorageState();
        Log.d(TAG, "External storage state=" + state);
        if (Environment.MEDIA_CHECKING.equals(state)) {
            return PREPARING;
        }
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            return UNAVAILABLE;
        }

        File dir = new File(generateDirectory());
        dir.mkdirs();
        if (!dir.isDirectory() || !dir.canWrite()) {
            return UNAVAILABLE;
        }

        try {
            StatFs stat = new StatFs(generateDirectory());
            return stat.getAvailableBlocks() * (long) stat.getBlockSize();
        } catch (Exception e) {
            Log.i(TAG, "Fail to access external storage", e);
        }
        return UNKNOWN_SIZE;
    }

    /**
     * OSX requires plugged-in USB storage to have path /DCIM/NNNAAAAA to be
     * imported. This is a temporary fix for bug#1655552.
     */
    public void ensureOSXCompatible() {
        File nnnAAAAA = new File(generateDCIM(), "100ANDRO");
        if (!(nnnAAAAA.exists() || nnnAAAAA.mkdirs())) {
            Log.e(TAG, "Failed to create " + nnnAAAAA.getPath());
        }
    }
}