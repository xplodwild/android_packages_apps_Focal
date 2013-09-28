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

package org.cyanogenmod.focal.picsphere;

import android.content.Context;
import android.database.Cursor;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Log;

import org.cyanogenmod.focal.PopenHelper;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.Util;
import org.cyanogenmod.focal.XMPHelper;

import java.io.File;
import java.io.IOException;
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
    private List<PicSphereImage> mPictures;
    private Context mContext;
    private File mTempPath;
    private String mProjectFile;
    private SnapshotManager mSnapManager;
    private Uri mOutputUri;
    private String mOutputTitle;
    private List<ProgressListener> mProgressListeners;
    private int mRenderProgress = 0;
    private int mOrientation;
    private float mHorizontalAngle;

    public final static int STEP_PTOGEN = 1;
    public final static int STEP_PTOVAR = 2;
    public final static int STEP_CPFIND = 3;
    public final static int STEP_PTCLEAN = 4;
    public final static int STEP_AUTOOPTIMISER = 5;
    public final static int STEP_PANOMODIFY = 6;
    public final static int STEP_NONA = 7;
    public final static int STEP_ENBLEND = 8;
    public final static int STEP_TOTAL = 9;

    private final static int EXIT_SUCCESS = 0;

    private final static String PANO_PROJECT_FILE = "project.pto";
    private final static int PANO_PROJECTION_MODE = 2; // 2=equirectangular, 0=rectilinear
    private final static int PANO_THREADS = 4;
    private final static int PANO_CPFIND_SIEVE1SIZE = 10;
    private final static int PANO_CPFIND_SIEVE2WIDTH = 5;
    private final static int PANO_CPFIND_SIEVE2HEIGHT = 5;
    private final static int PANO_CPFIND_MINMATCHES = 3;
    private final static int PANO_BLEND_COMPRESSION = 90;



    public interface ProgressListener {
        public void onRenderStart(PicSphere sphere);
        public void onStepChange(PicSphere sphere, int newStep);
        public void onRenderDone(PicSphere sphere);
    }

    private class PicSphereImage {
        private Uri mRealPathUri;
        private Uri mContentUri;
        private Vector3 mAngle;

        public PicSphereImage(Uri realPath, Uri contentUri, Vector3 angle) {
            mRealPathUri = realPath;
            mContentUri = contentUri;
            mAngle = angle;
        }

        public Uri getRealPath() {
            return mRealPathUri;
        }

        public Uri getUri() {
            return mContentUri;
        }

        public Vector3 getAngle() {
            return mAngle;
        }
    }


    protected PicSphere(Context context, SnapshotManager snapMan) {
        mPictures = new ArrayList<PicSphereImage>();
        mProgressListeners = new ArrayList<ProgressListener>();
        mContext = context;
        mSnapManager = snapMan;
    }

    public void addProgressListener(ProgressListener listener) {
        mProgressListeners.add(listener);
    }

    /**
     * Adds a picture to the sphere
     * @param pic The URI of the picture
     */
    public void addPicture(Uri pic, Vector3 angle) {
        PicSphereImage image = new PicSphereImage(
                Uri.fromFile(new File(Util.getRealPathFromURI(mContext, pic))),
                pic,
                angle);

        mPictures.add(image);
    }

    /**
     * Removes the last picture of the sphere
     */
    public void removeLastPicture() {
        if (mPictures.size() > 0) {
            mPictures.remove(mPictures.size()-1);
        }
    }

    /**
     * @return The number of pictures in the sphere
     */
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

    /**
     * @return The current rendering progress, or -1 if it's not rendering or done.
     */
    public int getRenderProgress() {
        return mRenderProgress;
    }

    /**
     * Renders the sphere
     */
    protected boolean render(int orientation) {
        mOrientation = (orientation + 360) % 360;

        for (ProgressListener listener : mProgressListeners) {
            listener.onRenderStart(this);
        }

        // Prepare a temporary directory
        File appFilesDir = mContext.getFilesDir();
        mPathPrefix = appFilesDir.getAbsolutePath() + "/";
        String tempPathStr = appFilesDir.getAbsolutePath() + "/" + System.currentTimeMillis();
        mTempPath = new File(tempPathStr);
        if (!mTempPath.mkdir()) {
            Log.e(TAG, "Couldn't create temporary path");
            return false;
        }

        mProjectFile = mTempPath + "/" + PANO_PROJECT_FILE;

        // Wait till all images are saved and accessible
        boolean allSaved = false;
        while (!allSaved) {
            allSaved = true;
            for (PicSphereImage pic : mPictures) {
                File file = new File(pic.getRealPath().getPath());
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

        mRenderProgress = 1;

        // Process our images
        try {
            doPtoGen();
            doPtoVar();
            doCpFind();
            doPtclean();
            doAutoOptimiser();
            doPanoModify();
            doNona();
            doEnblend();
        } catch (Exception ex) {
            Log.e(TAG, "Unable to process: ", ex);
            for (ProgressListener listener : mProgressListeners) {
                listener.onRenderDone(this);
            }
            removeTempFiles();
            return false;
        }

        for (ProgressListener listener : mProgressListeners) {
            listener.onRenderDone(this);
        }

        removeTempFiles();

        mRenderProgress = -1;
        return true;
    }

    /**
     * Remove the temporary files used by the app, and the source pictures of the sphere
     */
    private void removeTempFiles() {
        // Remove source pictures and temporary path
        for (PicSphereImage uri : mPictures) {
            List<String> segments = uri.getUri().getPathSegments();

            if (segments != null && segments.size() > 0) {
                Util.removeFromGallery(mContext.getContentResolver(),
                        Integer.parseInt(segments.get(segments.size()-1)));
            }
        }

        run("rm -r " + mTempPath.getPath());
    }

    /**
     * Run a command on the system shell
     * @param command The command to run
     * @throws IOException
     */
    private boolean run(String command) {
        // HACK: We cd to /storage/emulated/0/DCIM/Camera/ because cpfind tries to
        // find it in the current path instead of the actual path provided in the .pto. I'm
        // leaving this here as a hack until I fix it properly in cpfind source
        String dcimPath = "/sdcard/DCIM/Camera";
        String fullCommand = String.format("PATH=%s; LD_LIBRARY_PATH=%s; cd %s; %s 2>&1",
                mPathPrefix+":/system/bin",
                mPathPrefix+":"+mPathPrefix+"/../lib/:/system/lib",
                dcimPath,
                command);

        Log.v(TAG, "Running: " + fullCommand);

        if (PopenHelper.run(fullCommand) != EXIT_SUCCESS) {
            Log.e(TAG, "Command return code is not EXIT_SUCCESS!");
            return false;
        }

        return true;
    }

    /**
     * Notify the ProgressListeners of a step change
     * @param step The new step
     */
    private void notifyStep(int step) {
        int progressPerStep = 100 / PicSphere.STEP_TOTAL;
        mRenderProgress = step * progressPerStep;

        for (ProgressListener listener : mProgressListeners) {
            listener.onStepChange(this, step);
        }
    }

    /**
     * Most of hugin tools take a pto file as input and output. So the first step is to create this
     * pto file. For this purpose use pto_gen.
     *
     * @return Success status
     * @throws IOException
     */
    private void doPtoGen() throws IOException {
        Log.d(TAG, "PtoGen...");
        notifyStep(STEP_PTOGEN);

        // Build a list of files
        String filesStr = "";
        for (PicSphereImage picture : mPictures) {
            filesStr += " " + picture.getRealPath().getPath();
        }

        if (!run(String.format("pto_gen -f %f -p %d -o %s %s",
                mHorizontalAngle, PANO_PROJECTION_MODE,
                mProjectFile, filesStr))) {
            throw new IOException("PtoGen failed");
        }

        Log.d(TAG, "PtoGen... done");
    }

    /**
     * Since we know the angle of each image thanks to the gyroscope, we're going to set them
     * in our pto file before going on to matching the pictures edges.
     *
     * @return Success status
     * @throws IOException
     */
    private void doPtoVar() throws IOException {
        Log.d(TAG, "PtoVar...");
        notifyStep(STEP_PTOVAR);

        int index = 0;
        boolean referenceYawSet = false;
        float referenceYaw = 0.0f;

        for (PicSphereImage picture : mPictures) {
            // We stored camera angle into x, y and z respectively as degrees
            Vector3 a = picture.getAngle();

            // We now convert them to fit pitch, yaw and roll values as expected by Hugin
            // TODO: Is it orientation independent? Portrait/landscape?

            // Pitch: If you shoot a multi row panorama you will have to tilt the camera up or down.
            // The angle the optical axis of the camera is tilted up (positive value) or down
            // (negative value) is the pitch of the image. The pitch of the zenith image (shot
            // straight upwards) is 90° f.e.
            float pitch = -a.y;

            // Yaw: If shooting a panorama you rotate the camera horizontally (around a vertical
            // axis, TrY). You will treat one image as an anchor image which is considered to have
            // yaw = 0. The angle you rotated your camera relatively to this image is its Yaw value.
            float yaw = a.x + 180.0f;
            if (!referenceYawSet) {
                referenceYaw = yaw;
                referenceYawSet = true;
                yaw = 0.0f;
            } else {
                yaw = (yaw - referenceYaw) + ((yaw - referenceYaw < 0) ? 360 : 0);
            }

            // Roll: Roll is the angle of camera rotation around its optical axis. In normal
            // landscape orientation, the sensor returns a pitch or -180/180 degrees. Hugin expects
            // that angle to be zero, so we clamp it
            float roll = (a.z + 180.0f) % 360.0f;

            Log.e(TAG, "Pitch/Yaw/Roll: " + pitch + " ; " + yaw + " ; " + roll);
            if (!run(String.format("pto_var -o %s --set p%d=%f,y%d=%f,r%d=%f %s",
                    mProjectFile,
                    index,
                    pitch,
                    index,
                    yaw,
                    index,
                    roll,
                    mProjectFile))) {
                throw new IOException("PtoVar failed");
            }

            index++;
        }
    }

    /**
     * We then look for control points and matching pairs in the images, using CpFind. Note that we
     * can enable celeste cleaning in cpfind, but we need the celeste reference database. Also, it's
     * probably a bit slower and not worth it for what we're doing.
     *
     * @return Success status
     * @throws IOException
     */
    private boolean doCpFind() throws IOException {
        Log.d(TAG, "CpFind...");
        notifyStep(STEP_CPFIND);

        if (!run(String.format("cpfind --sieve1size %d --sieve2width %d --sieve2height %d -n %d -v " +
                "--prealigned --minmatches %d -o " +
                "%s %s",
                PANO_CPFIND_SIEVE1SIZE, PANO_CPFIND_SIEVE2WIDTH, PANO_CPFIND_SIEVE2HEIGHT,
                PANO_THREADS, PANO_CPFIND_MINMATCHES, mProjectFile,  mProjectFile))) {
            throw new IOException("CpFind failed");
        }

        Log.d(TAG, "CpFind... done");
        return true;
    }

    /**
     * You could go ahead and optimise this project file straight away, but this can be a bit hit
     * and miss. First it is a good idea to clean up the control points.
     *
     * @return Success status
     * @throws IOException
     */
    private boolean doPtclean() throws IOException {
        Log.d(TAG, "Ptclean...");
        notifyStep(STEP_PTCLEAN);
        if (!run(String.format("ptclean -o %s %s", mProjectFile, mProjectFile))) {
            throw new IOException("PtClean failed");
        }

        Log.d(TAG, "Ptclean... done");
        return true;
    }

    /**
     * Up to now, the project file simply contains an image list and control points, the images are
     * not yet aligned, you can do this by optimising geometric parameters with the autooptimiser
     * tool
     *
     * @return Success status
     * @throws IOException
     */
    private boolean doAutoOptimiser() throws IOException {
        Log.d(TAG, "AutoOptimiser...");
        notifyStep(STEP_AUTOOPTIMISER);
        if (!run(String.format("autooptimiser -v %f -a -s -l -m -o %s %s", mHorizontalAngle,
                mProjectFile, mProjectFile))) {
            throw new IOException("AutoOptimiser failed");
        }

        Log.d(TAG, "AutoOptimiser... done");
        return true;
    }

    /**
     * Crop and straighten the panorama to fit a rectangular image
     *
     * @return Success status
     * @throws IOException
     */
    private boolean doPanoModify() throws IOException {
        Log.d(TAG, "PanoModify...");
        notifyStep(STEP_PANOMODIFY);

        // Google PhotoSphere (both Google Gallery viewer and Google+ viewer, even if the last one
        // is a bit more permissive) must have a 2:1 image to work properly. For now, we left this
        // on AUTO to have a clean final image. They seem to view fine on the Google+ Web viewer,
        // and we'll have to come up with our own Android viewer anyway at some point.
        String canvas = "AUTO";
        String crop = "AUTO";
        if (!run(String.format("pano_modify --crop=%s --canvas=%s -o %s %s",
                crop, canvas, mProjectFile, mProjectFile))) {
            throw new IOException("Pano_Modify failed");
        }

        Log.d(TAG, "PanoModify... done");
        return true;
    }

    /**
     * The hugin tool for remapping and distorting the photos into the final panorama frame is nona,
     * it uses the .pto project file as a set of instructions
     *
     * @return Success status
     * @throws IOException
     */
    private boolean doNona() throws IOException {
        Log.d(TAG, "Nona...");
        notifyStep(STEP_NONA);
        if (!run(String.format("nona -t %d -o %s/project -m TIFF_m %s",
                PANO_THREADS, mTempPath, mProjectFile))) {
            throw new IOException("Nona failed");
        }

        Log.d(TAG, "Nona... done");
        return true;
    }

    /**
     * nona can do rudimentary assembly of the remapped images, but a much better tool for this
     * is enblend, feed it the images, it will pick seam lines and blend the overlapping areas.
     * We use its multithreaded friend, multiblend, which is much faster than enblend for the
     * same final quality.
     *
     * @return Success status
     * @throws IOException
     */
    private void doEnblend() throws IOException {
        Log.d(TAG, "Enblend...");
        notifyStep(STEP_ENBLEND);

        String jpegPath = mTempPath + "/final.jpg";

        // Multiblend can take a lot of RAM when blending a lot of high resolution pictures. We
        // prepare the system for that.
        System.gc();

        // As we are running through a sh shell, we can use * instead of writing the full
        // images list.
        run(String.format("multiblend --wideblend --compression=%d -o %s %s/project*.tif",
                PANO_BLEND_COMPRESSION, jpegPath, mTempPath));

        // Apply PhotoSphere EXIF tags on the final jpeg
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(mTempPath + "/final.jpg", opts);

        if (!doExifTagging(opts.outWidth, opts.outHeight)) {
            throw new IOException("Cannot tag EXIF info");
        }

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
            throw new IOException("Cannot access final file");
        } finally {
            if (f != null) {
                f.close();
            }
        }

        if (opts.outWidth > 0 && opts.outHeight > 0) {
            mSnapManager.prepareNamerUri(opts.outWidth, opts.outHeight);
            mOutputUri = mSnapManager.getNamerUri();
            mOutputTitle = mSnapManager.getNamerTitle();

            Log.i(TAG, "PicSphere size: " + opts.outWidth + "x" + opts.outHeight);
            mSnapManager.saveImage(mOutputUri, mOutputTitle, opts.outWidth, opts.outWidth, 0, jpegData);
        } else {
            Log.e(TAG, "Invalid output image size: " + opts.outWidth + "x" + opts.outHeight);
            throw new IOException("Invalid output image size: " + opts.outWidth + "x" + opts.outHeight);
        }

        Log.d(TAG, "Enblend... done");
    }

    private boolean doExifTagging(int width, int height) throws IOException {
        Log.d(TAG, "XMP metadata tagging...");

        try {
            XMPHelper xmp = new XMPHelper();
            xmp.writeXmpToFile(mTempPath + "/final.jpg", generatePhotoSphereXMP(width,
                    height, mPictures.size()));
        } catch (Exception e) {
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
    public String generatePhotoSphereXMP(int width, int height, int sourceImageCount) {
        return "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n" +
                "<rdf:Description rdf:about=\"\" xmlns:GPano=\"http://ns.google.com/photos/1.0/panorama/\">\n" +
                "    <GPano:UsePanoramaViewer>True</GPano:UsePanoramaViewer>\n" +
                "    <GPano:CaptureSoftware>CyanogenMod Focal</GPano:CaptureSoftware>\n" +
                "    <GPano:StitchingSoftware>CyanogenMod Focal with Hugin</GPano:StitchingSoftware>\n" +
                "    <GPano:ProjectionType>equirectangular</GPano:ProjectionType>\n" +
                "    <GPano:PoseHeadingDegrees>350.0</GPano:PoseHeadingDegrees>\n" +
                "    <GPano:InitialViewHeadingDegrees>90.0</GPano:InitialViewHeadingDegrees>\n" +
                "    <GPano:InitialViewPitchDegrees>45.0</GPano:InitialViewPitchDegrees>\n" +
                "    <GPano:InitialViewRollDegrees>"+mOrientation+"</GPano:InitialViewRollDegrees>\n" +
                "    <GPano:InitialHorizontalFOVDegrees>"+mHorizontalAngle+"</GPano:InitialHorizontalFOVDegrees>\n" +
                "    <GPano:CroppedAreaLeftPixels>0</GPano:CroppedAreaLeftPixels>\n" +
                "    <GPano:CroppedAreaTopPixels>0</GPano:CroppedAreaTopPixels>\n" +
                "    <GPano:CroppedAreaImageWidthPixels>"+(width)+"</GPano:CroppedAreaImageWidthPixels>\n" +
                "    <GPano:CroppedAreaImageHeightPixels>"+(height)+"</GPano:CroppedAreaImageHeightPixels>\n" +
                "    <GPano:FullPanoWidthPixels>"+width+"</GPano:FullPanoWidthPixels>\n" +
                "    <GPano:FullPanoHeightPixels>"+height+"</GPano:FullPanoHeightPixels>\n" +
                //"    <GPano:FirstPhotoDate>2012-11-07T21:03:13.465Z</GPano:FirstPhotoDate>\n" +
                //"    <GPano:LastPhotoDate>2012-11-07T21:04:10.897Z</GPano:LastPhotoDate>\n" +
                "    <GPano:SourcePhotosCount>"+sourceImageCount+"</GPano:SourcePhotosCount>\n" +
                "    <GPano:ExposureLockUsed>False</GPano:ExposureLockUsed>\n" +
                "</rdf:Description></rdf:RDF>";
    }
}
