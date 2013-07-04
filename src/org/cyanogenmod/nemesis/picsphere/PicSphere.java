package org.cyanogenmod.nemesis.picsphere;

import android.content.Context;
import android.content.ContextWrapper;
import android.net.Uri;
import android.os.Environment;
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
    private List<Uri> mPictures;
    private Context mContext;
    private File mTempPath;
    private BufferedReader mProcStdOut;
    private BufferedReader mProcStdErr;
    private String mProjectFile;

    protected PicSphere(Context context) {
        mPictures = new ArrayList<Uri>();
        mContext = context;
    }

    /**
     * Adds a picture to the sphere
     * @param pic The URI of the picture
     */
    public void addPicture(Uri pic) {
        mPictures.add(pic);
    }

    /**
     * Renders the sphere
     */
    protected void render() {
        // Prepare a temporary directory
        File appFilesDir = mContext.getFilesDir();
        String tempPathStr = appFilesDir.getAbsolutePath() + "/" + System.currentTimeMillis();
        mTempPath = new File(tempPathStr);
        mTempPath.mkdir();
        mProjectFile = mTempPath + "/project.pto";


        // Process our images
        try {
            doAutopano();
        } catch (IOException ex) {
            Log.e(TAG, "Unable to process: ", ex);
        }
    }

    private void run(String command, String parameters) throws IOException {
        Runtime rt = Runtime.getRuntime();
        String[] commands = {command,parameters};
        Process proc = rt.exec(commands);
        mProcStdOut = new BufferedReader(new
                InputStreamReader(proc.getInputStream()));
        mProcStdErr = new BufferedReader(new
                InputStreamReader(proc.getErrorStream()));
    }

    private void consumeProcLogs() {
        String line;
        try {
            while ((line = mProcStdErr.readLine()) != null) {
                Log.e(TAG, line);
            }

            while ((line = mProcStdOut.readLine()) != null) {
                Log.i(TAG, line);
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
        String filesStr = "";
        for (Uri picture : mPictures) {
            filesStr += " " + picture.getPath();
        }

        // TODO: Load the horizontal view angle from the camera
        run("autopano", "--projection 0,50 -o " + mProjectFile + " " + filesStr);
        consumeProcLogs();

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
        run("ptclean", "-v --output " + mProjectFile + " " + mProjectFile);
        consumeProcLogs();

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
        run("autooptimiser", "-a -l -s -m -o " + mProjectFile + " " + mProjectFile);
        consumeProcLogs();

        return true;
    }

    /**
     * Crop and straighten the panorama to fit a rectangular image
     *
     * @return
     * @throws IOException
     */
    private boolean doPanoModify() throws IOException {
        run("pano_modify", "-o " + mProjectFile + " --center --straighten --canvas=AUTO --crop=AUTO " + mProjectFile);
        consumeProcLogs();

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
        run("nona", "-o " + mTempPath + "/project -m TIFF_m " + mProjectFile);
        consumeProcLogs();

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
        run("enblend", "-o /sdcard/final.tif " + mTempPath + "/*.tif");
        consumeProcLogs();

        return true;
    }
}
