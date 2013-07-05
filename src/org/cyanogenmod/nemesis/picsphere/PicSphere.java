package org.cyanogenmod.nemesis.picsphere;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
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

    protected PicSphere(Context context) {
        mPictures = new ArrayList<Uri>();
        mContext = context;
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
        Process proc = rt.exec(mPathPrefix + command);
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
        run("ptclean --output " + mProjectFile + " " + mProjectFile);
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
            files += " " + mTempPath + "/" + String.format("project%04d.tif", i);
        }

        run("enblend -o /sdcard/final.tif " + files);
        consumeProcLogs();

        Log.d(TAG, "Enblend... done");
        return true;
    }
}
