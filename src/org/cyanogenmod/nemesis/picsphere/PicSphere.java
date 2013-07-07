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

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;
import android.util.Log;

import org.cyanogenmod.nemesis.SnapshotManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.List;

/**
 * This class handles the data needed for a PicSphere to happen, as well as the current
 * status of the processing.
 *
 * You shouldn't instantiate the awesomeness directly, but rather go through PicSphereManager.
 */
public class PicSphere {
    public final static String TAG = "PicSphere";
    private String mPathPrefix;
    private List<Uri> mPictures;
    private Context mContext;
    private File mTempPath;
    private BufferedReader mProcStdOut;
    private BufferedReader mProcStdErr;
    private String mProjectFile;
    private SnapshotManager mSnapManager;
    private Uri mOutputUri;
    private String mOutputTitle;

    private Thread mOutputLogger = new Thread() {
        public void run() {
            while (true) {
                try {
                    Thread.sleep(10);
                    consumeProcLogs();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    };

    protected PicSphere(Context context, SnapshotManager snapMan) {
        mPictures = new ArrayList<Uri>();
        mContext = context;
        mSnapManager = snapMan;
        mOutputLogger.start();
    }

    private String getRealPathFromURI(Uri contentURI) {
        Cursor cursor = mContext.getContentResolver().query(contentURI, null, null, null, null);
        if (cursor == null) { // Source is Dropbox or other similar local file path
            return contentURI.getPath();
        } else {
            cursor.moveToFirst();
            int idx = cursor.getColumnIndex(MediaStore.Images.ImageColumns.DATA);
            return cursor.getString(idx);
        }
    }

    /**
     * Adds a picture to the sphere
     * @param pic The URI of the picture
     */
    public void addPicture(Uri pic) {
        mPictures.add(Uri.fromFile(new File(getRealPathFromURI(pic))));
    }

    /**
     * Renders the sphere
     */
    protected void render() {
        // Prepare a temporary directory
        Log.d(TAG, "Preparing temp dir for PicSphere rendering...");
        File appFilesDir = mContext.getFilesDir();
        mPathPrefix = appFilesDir.getAbsolutePath() + "/";
        String tempPathStr = appFilesDir.getAbsolutePath() + "/" + System.currentTimeMillis();
        mTempPath = new File(tempPathStr);
        mTempPath.mkdir();
        mProjectFile = mTempPath + "/project.pto";

        mSnapManager.prepareNamerUri(100,100);
        mOutputUri = mSnapManager.getNamerUri();
        mOutputTitle = mSnapManager.getNamerTitle();

        // Process our images
        try {
            doAutopano();
            doPtclean();
            doAutoOptimiser();
            doPanoModify();
            doNona();
            doEnblend();
        } catch (IOException ex) {
            Log.e(TAG, "Unable to process: ", ex);
        }
    }

    private void run(String command) throws IOException {
        Runtime rt = Runtime.getRuntime();
        Process proc = rt.exec(command, new String[]{"PATH="+mPathPrefix+":/system/bin",
                "LD_LIBRARY_PATH="+mPathPrefix+":/system/lib"});
        mProcStdOut = new BufferedReader(new
                InputStreamReader(proc.getInputStream()));
        mProcStdErr = new BufferedReader(new
                InputStreamReader(proc.getErrorStream()));
    }

    private void consumeProcLogs() {
        String line;
        try {
            if (mProcStdErr != null) {
                while ((line = mProcStdErr.readLine()) != null) {
                    Log.e(TAG, line);
                }
            }

            if (mProcStdOut != null) {
                while ((line = mProcStdOut.readLine()) != null) {
                    Log.i(TAG, line);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error while consuming proc logs", e);
        }
    }

    /**
     * This will create a .pto project with control points (if any) linking the three photos,
     * note that the projection format (f0, rectilinear) of the input photos and approximate
     * horizontal angle of view have to be specified.
     *
     * @return
     * @throws IOException
     */
    private boolean doAutopano() throws IOException {
        Log.d(TAG, "Autopano...");
        String filesStr = "";
        for (Uri picture : mPictures) {
            filesStr += " " + picture.getPath();
        }

        // TODO: Load the horizontal view angle from the camera
        run("autopano --projection 0,50 " + mProjectFile + " " + filesStr);
        consumeProcLogs();

        Log.d(TAG, "Autopano... done");
        return true;
    }

    /**
     * You could go ahead and optimise this project file straight away, but this can be a bit hit
     * and miss. First it is a good idea to clean up the control points. There are currently two
     * useful tools for cleaning control points: celeste removes points from areas of sky and
     * ptclean removes points with large error distances
     *
     * TODO: Add celeste for outdoor cleanup
     *
     * @return
     * @throws IOException
     */
    private boolean doPtclean() throws IOException {
        Log.d(TAG, "Ptclean...");
        run("ptclean -o " + mProjectFile + " " + mProjectFile);
        consumeProcLogs();

        Log.d(TAG, "Ptclean... done");
        return true;
    }

    /**
     * Up to now, the project file simply contains an image list and control points, the images are
     * not yet aligned, you can do this by optimising geometric parameters with the autooptimiser
     * tool
     *
     * @return
     * @throws IOException
     */
    private boolean doAutoOptimiser() throws IOException {
        Log.d(TAG, "AutoOptimiser...");
        run("autooptimiser -a -l -s -m -o " + mProjectFile + " " + mProjectFile);
        consumeProcLogs();

        Log.d(TAG, "AutoOptimiser... done");
        return true;
    }

    /**
     * Crop and straighten the panorama to fit a rectangular image
     *
     * @return
     * @throws IOException
     */
    private boolean doPanoModify() throws IOException {
        Log.d(TAG, "PanoModify...");
        run("pano_modify -o " + mProjectFile + " --center --straighten --canvas=AUTO --crop=AUTO " + mProjectFile);
        consumeProcLogs();

        Log.d(TAG, "PanoModify... done");
        return true;
    }

    /**
     * The hugin tool for remapping and distorting the photos into the final panorama frame is nona,
     * it uses the .pto project file as a set of instructions
     *
     * @return
     * @throws IOException
     */
    private boolean doNona() throws IOException {
        Log.d(TAG, "Nona...");
        run("nona -o " + mTempPath + "/project -m TIFF_m " + mProjectFile);
        consumeProcLogs();

        Log.d(TAG, "Nona... done");
        return true;
    }

    /**
     * nona can do rudimentary assembly of the remapped images, but a much better tool for this
     * is enblend, feed it the images, it will pick seam lines and blend the overlapping areas
     *
     * @return
     * @throws IOException
     */
    private boolean doEnblend() throws IOException {
        Log.d(TAG, "Enblend...");

        // Build the list of output files. The convention set up by Nona is projectXXXX.tif,
        // so we basically build that list out of the number of shots we fed to autopano
        String files = "";
        for (int i = 0; i < mPictures.size(); i++) {
            // Check if file exists, otherwise enblend will fail
            String filePath = mTempPath + "/" + String.format("project%04d.tif", i);
            if (new File(filePath).exists()) {
                files += " " + filePath;
            }
        }

        String jpegPath = mTempPath + "/final.jpg";
        run("enblend --compression=jpeg -o " + jpegPath + " " + files);
        consumeProcLogs();

        // Apply PhotoSphere EXIF tags on the final jpeg
        doExiv2();

        // Save it to gallery
        // XXX: This needs opening the output byte array... Isn't there any way to update
        // gallery data without having to reload/save the JPEG file? Because we have it already,
        // we could just move it.
        byte[] jpegData;
        RandomAccessFile f = new RandomAccessFile(jpegPath, "r");
        try {
            // Get and check length
            long longlength = f.length();
            int length = (int) longlength;
            if (length != longlength)
                throw new IOException("File size >= 2 GB");
            // Read file and return data
            jpegData = new byte[length];
            f.readFully(jpegData);
        } finally {
            f.close();
        }

        mSnapManager.saveImage(mOutputUri, mOutputTitle, 100, 100, 0, jpegData);

        Log.d(TAG, "Enblend... done");
        return true;
    }

    private boolean doExiv2() throws IOException {
        Log.d(TAG, "Exiv2 tagging...");

        String[] xmp = generatePhotoSphereXMP(2000, 2000, mPictures.size());
        for (String line : xmp) {
            run("exiv2 -m\"" + line + "\" " + mTempPath+"final.jpg");
        }
        consumeProcLogs();

        Log.d(TAG, "Exiv2... done");
        return true;
    }

    /**
     * Generate PhotoSphere XMP data file for use with exiv2 tool
     * https://developers.google.com/photo-sphere/metadata/
     *
     * @param width The width of the panorama
     * @param height The height of the panorama
     * @return Exiv2 config file string
     */
    public static String[] generatePhotoSphereXMP(int width, int height, int sourceImageCount) {
        return new String[]{
                "set Xmp.GPano.UsePanoramaViewer             XmpText     True",
                "set Xmp.GPano.CaptureSoftware               XmpText     CyanogenMod-Nemesis",
                "set Xmp.GPano.StitchingSoftware             XmpText     CyanogenMod-Nemesis+Hugin",
                "set Xmp.GPano.SourcePhotosCount             XmpText     "+sourceImageCount,
                "set Xmp.GPano.ProjectionType                XmpText     equirectangular",
                "set Xmp.GPano.CroppedAreaImageWidthPixels   XmpText     "+width,
                "set Xmp.GPano.CroppedAreaImageHeightPixels  XmpText     "+height,
                "set Xmp.GPano.FullPanoWidthPixels           XmpText     "+width,
                "set Xmp.GPano.FullPanoHeightPixels          XmpText     "+height,
                "set Xmp.GPano.CroppedAreaLeftPixels         XmpText     0",
                "set Xmp.GPano.CroppedAreaTopPixels          XmpText     0"};
    }
}
