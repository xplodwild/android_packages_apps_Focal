package org.cyanogenmod.nemesis.feats;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import org.cyanogenmod.nemesis.SnapshotManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.util.List;

/**
 * Manages the processing of multiple shots into one HDR shot
 */
public class SoftwareHdrProcessor {
    public final static String TAG = "SoftwareHdr";
    private String mPathPrefix;
    private File mTempPath;
    private List<Uri> mPictures;
    private SnapshotManager mSnapManager;
    private Uri mOutputUri;
    private String mOutputTitle;
    private Context mContext;
    private BufferedReader mProcStdOut;
    private BufferedReader mProcStdErr;

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

    public SoftwareHdrProcessor(Context context, SnapshotManager snapMan) {
        mSnapManager = snapMan;
        mContext = context;
    }

    public void setPictures(List<Uri> pictures) {
        mPictures = pictures;
    }

    private void run(String command) throws IOException {
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

    public void render() {
        mOutputLogger.start();

        // Prepare a temporary directory
        Log.d(TAG, "Preparing temp dir for PicSphere rendering...");
        File appFilesDir = mContext.getFilesDir();
        mPathPrefix = appFilesDir.getAbsolutePath() + "/";
        String tempPathStr = appFilesDir.getAbsolutePath() + "/" + System.currentTimeMillis();
        mTempPath = new File(tempPathStr);
        mTempPath.mkdir();

        mSnapManager.prepareNamerUri(100,100);


        // Process our images
        try {
            doAlignImageStack();
            doEnfuse();

            // Save it to gallery
            // XXX: This needs opening the output byte array... Isn't there any way to update
            // gallery data without having to reload/save the JPEG file? Because we have it already,
            // we could just move it.
            byte[] jpegData;
            RandomAccessFile f = new RandomAccessFile(mTempPath+"/final.jpg", "r");
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

            mOutputUri = mSnapManager.getNamerUri();
            mOutputTitle = mSnapManager.getNamerTitle();
            mSnapManager.saveImage(mOutputUri, mOutputTitle, 100, 100, 0, jpegData);
        } catch (IOException ex) {
            Log.e(TAG, "Unable to process: ", ex);
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

    private boolean doAlignImageStack() throws IOException {
        Log.d(TAG, "Align Image Stack...");

        String filesStr = "";
        for (Uri picture : mPictures) {
            filesStr += " " + picture.getPath();
        }

        run("align_image_stack -v -v -v -a "+mTempPath+"/project " + filesStr);
        consumeProcLogs();

        Log.d(TAG, "Align Image Stack... done");
        return true;
    }

    private boolean doEnfuse() throws IOException {
        Log.d(TAG, "Enfuse...");

        // Build the list of output files. The convention set up by AlignImageStack is projectXXXX.tif,
        // so we basically build that list out of the number of shots we fed to align_image_stack
        String files = "";
        for (int i = 0; i < mPictures.size(); i++) {
            // Check if file exists, otherwise enfuse will fail
            String filePath = mTempPath + "/" + String.format("project%04d.tif", i);
            if (new File(filePath).exists()) {
                files += " " + filePath;
            }
        }
        run("enfuse -o "+mTempPath+"/final.jpg --compression=jpeg " + files);
        consumeProcLogs();

        Log.d(TAG, "Enfuse... done");
        return true;
    }
}
