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
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;

import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.Util;
import org.cyanogenmod.nemesis.XMPHelper;

import java.io.BufferedReader;
import java.io.File;
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
    private List<Uri> mPicturesUri;
    private Context mContext;
    private File mTempPath;
    private BufferedReader mProcStdOut;
    private BufferedReader mProcStdErr;
    private String mProjectFile;
    private SnapshotManager mSnapManager;
    private Uri mOutputUri;
    private String mOutputTitle;
    private List<ProgressListener> mProgressListeners;
    private int mRenderProgress = 0;
    public final static int STEP_AUTOPANO = 1;
    public final static int STEP_PTCLEAN = 2;
    public final static int STEP_AUTOOPTIMISER = 3;
    public final static int STEP_PANOMODIFY = 4;
    public final static int STEP_NONA = 5;
    public final static int STEP_ENBLEND = 6;
    public final static int STEP_TOTAL = 6;

    private float mHorizontalAngle;

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

    public interface ProgressListener {
        public void onRenderStart(PicSphere sphere);
        public void onStepChange(PicSphere sphere, int newStep);
        public void onRenderDone(PicSphere sphere);
    }

    protected PicSphere(Context context, SnapshotManager snapMan) {
        mPictures = new ArrayList<Uri>();
        mPicturesUri = new ArrayList<Uri>();
        mProgressListeners = new ArrayList<ProgressListener>();
        mContext = context;
        mSnapManager = snapMan;
        mOutputLogger.start();
    }

    public void addProgressListener(ProgressListener listener) {
        mProgressListeners.add(listener);
    }

    /**
     * Adds a picture to the sphere
     * @param pic The URI of the picture
     */
    public void addPicture(Uri pic) {
        mPictures.add(Uri.fromFile(new File(Util.getRealPathFromURI(mContext, pic))));
        mPicturesUri.add(pic);
    }

    /**
     * Removes the last picture of the sphere
     */
    public void removeLastPicture() {
        if (mPictures.size() > 0) {
            mPictures.remove(mPictures.size()-1);
        }
    }

    public int getPicturesCount() {
        return mPictures.size();
    }

    /**
     * Sets the horizontal angle of the camera. AOSP don't check values are real values, and
     * OEMs don't seem to care much about returning a real value (sony reports 360°...), so we
     * pass 45° by default.
     *
     * @param angle The horizontal angle, in degrees
     */
    public void setHorizontalAngle(float angle) {
        mHorizontalAngle = angle;
    }

    public int getRenderProgress() {
        return mRenderProgress;
    }

    /**
     * Renders the sphere
     */
    protected boolean render() {
        for (ProgressListener listener : mProgressListeners) {
            listener.onRenderStart(this);
        }

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

        // Wait till all images are saved and accessible
        boolean allSaved = false;
        while (!allSaved) {
            allSaved = true;
            for (Uri pic : mPictures) {
                File file = new File(pic.getPath());
                if (!file.exists() || !file.canRead()) {
                    allSaved = false;

                    // Wait a bit until we retry
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    break;
                }
            }
        }

        mRenderProgress = STEP_AUTOPANO * 100 / STEP_TOTAL;

        // Process our images
        try {
            if (!doAutopano()) return false;
            if (!doPtclean()) return false;
            if (!doAutoOptimiser()) return false;
            if (!doPanoModify()) return false;
            if (!doNona()) return false;
            if (!doEnblend()) return false;
        } catch (IOException ex) {
            Log.e(TAG, "Unable to process: ", ex);
            for (ProgressListener listener : mProgressListeners) {
                listener.onRenderDone(this);
            }
            return false;
        }

        for (ProgressListener listener : mProgressListeners) {
            listener.onRenderDone(this);
        }

        // Remove source pictures and temporary path
        for (Uri uri : mPicturesUri) {
            List<String> segments = uri.getPathSegments();
            Util.removeFromGallery(mContext.getContentResolver(), Integer.parseInt(segments.get(segments.size()-1)));
        }

        mTempPath.delete();

        mRenderProgress = -1;
        return true;
    }

    private void run(String command) throws IOException {
        Log.v(TAG, "Running: " + command);
        Runtime rt = Runtime.getRuntime();
        Process proc = rt.exec(command, new String[]{"PATH="+mPathPrefix+":/system/bin",
                "LD_LIBRARY_PATH="+mPathPrefix+":/system/lib"});
        mProcStdOut = new BufferedReader(new
                InputStreamReader(proc.getInputStream()));
        mProcStdErr = new BufferedReader(new
                InputStreamReader(proc.getErrorStream()));

        try {
            proc.waitFor();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void run(String[] commandWithArgs) throws IOException {
        Runtime rt = Runtime.getRuntime();
        Process proc = rt.exec(commandWithArgs, new String[]{"PATH="+mPathPrefix+":/system/bin",
                "LD_LIBRARY_PATH="+mPathPrefix+":/system/lib"});
        mProcStdOut = new BufferedReader(new
                InputStreamReader(proc.getInputStream()));
        mProcStdErr = new BufferedReader(new
                InputStreamReader(proc.getErrorStream()));

        try {
            proc.waitFor();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void notifyStep(int step) {
        int progressPerStep = 100 / PicSphere.STEP_TOTAL;
        mRenderProgress = step * progressPerStep;

        for (ProgressListener listener : mProgressListeners) {
            listener.onStepChange(this, step);
        }
    }

    private void consumeProcLogs() {
        String line;
        try {
            if (mProcStdOut != null && mProcStdOut.ready()) {
                while ((line = mProcStdOut.readLine()) != null) {
                    Log.i(TAG, line);
                }
            }

            if (mProcStdErr != null && mProcStdErr.ready()) {
                while ((line = mProcStdErr.readLine()) != null) {
                    Log.e(TAG, line);
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
        notifyStep(STEP_AUTOPANO);
        String filesStr = "";
        for (Uri picture : mPictures) {
            filesStr += " " + picture.getPath();
        }

        run("autopano --align --bottom-is-left --ransac on --refine-by-mean " +
                "--projection 2,"+mHorizontalAngle+" " + mProjectFile + " " + filesStr);
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
     *
     * @return
     * @throws IOException
     */
    private boolean doPtclean() throws IOException {
        Log.d(TAG, "Ptclean...");
        notifyStep(STEP_PTCLEAN);
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
        notifyStep(STEP_AUTOOPTIMISER);
        run("autooptimiser -v " + mHorizontalAngle + " -p -o " + mProjectFile + " " + mProjectFile);
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
        notifyStep(STEP_PANOMODIFY);
        run("pano_modify -o " + mProjectFile + " --center --canvas=AUTO " + mProjectFile);
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
        notifyStep(STEP_NONA);
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
        notifyStep(STEP_ENBLEND);

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
        doExifTagging();


        // Save it to gallery
        // XXX: This needs opening the output byte array... Isn't there any way to update
        // gallery data without having to reload/save the JPEG file? Because we have it already,
        // we could just move it.
        byte[] jpegData;
        RandomAccessFile f = null;
        try {
            f = new RandomAccessFile(jpegPath, "r");

            // Get and check length
            long longlength = f.length();
            int length = (int) longlength;
            if (length != longlength)
                throw new IOException("File size >= 2 GB");
            // Read file and return data
            jpegData = new byte[length];
            f.readFully(jpegData);
        } catch (Exception e) {
            Log.e(TAG, "Couldn't access final file, did rendering fail?");
            return false;
        } finally {
            if (f != null) {
                f.close();
            }
        }

        mSnapManager.saveImage(mOutputUri, mOutputTitle, 100, 100, 90, jpegData);

        Log.d(TAG, "Enblend... done");
        return true;
    }

    private boolean doExifTagging() throws IOException {
        Log.d(TAG, "XMP metadata tagging...");

        try {
            BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inJustDecodeBounds = true;
            BitmapFactory.decodeFile(mTempPath + "/final.jpg", opts);

            XMPHelper xmp = new XMPHelper();
            xmp.writeXmpToFile(mTempPath + "/final.jpg", generatePhotoSphereXMP(opts.outWidth,
                    opts.outHeight, mPictures.size()));
        }
        catch (Exception e) {
            Log.e(TAG, "Couldn't access final file, did rendering fail?");
            return false;
        }

        Log.d(TAG, "XMP metadata tagging... done");
        return true;
    }

    /**
     * Generate PhotoSphere XMP data file to apply on panorama images
     * https://developers.google.com/photo-sphere/metadata/
     *
     * @param width The width of the panorama
     * @param height The height of the panorama
     * @return RDF XML XMP metadata
     */
    public static String generatePhotoSphereXMP(int width, int height, int sourceImageCount) {
        return "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n" +
                "    <rdf:Description rdf:about=\"\"\n" +
                "        xmlns:GPano=\"http://ns.google.com/photos/1.0/panorama/\"\n" +
                "      GPano:UsePanoramaViewer=\"True\"\n" +
                "      GPano:ProjectionType=\"equirectangular\"\n" +
                //"      GPano:PoseHeadingDegrees=\"122.0\"\n" +
                "      GPano:CroppedAreaLeftPixels=\"10\"\n" +
                "      GPano:FullPanoWidthPixels=\""+(width+10)+"\"\n" +
                //"      GPano:FirstPhotoDate=\"2012-11-22T23:15:59.773Z\"\n" +
                "      GPano:CroppedAreaImageHeightPixels=\""+height+"\"\n" +
                "      GPano:FullPanoHeightPixels=\""+(height+10)+"\"\n" +
                "      GPano:SourcePhotosCount=\""+sourceImageCount+"\"\n" +
                "      GPano:CroppedAreaImageWidthPixels=\""+width+"\"\n" +
                //"      GPano:LastPhotoDate=\"2012-11-22T23:16:41.177Z\"\n" +
                "      GPano:CroppedAreaTopPixels=\"10\" />\n" +
                "  </rdf:RDF>";
    }
}
