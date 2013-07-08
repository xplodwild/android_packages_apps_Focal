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
    }

    public void render() {
        // Prepare a temporary directory
        Log.d(TAG, "Preparing temp dir for PicSphere rendering...");
        File appFilesDir = mContext.getFilesDir();
        mPathPrefix = appFilesDir.getAbsolutePath() + "/";
        String tempPathStr = appFilesDir.getAbsolutePath() + "/" + System.currentTimeMillis();
        mTempPath = new File(tempPathStr);
        mTempPath.mkdir();

        mSnapManager.prepareNamerUri(100,100);
        mOutputUri = mSnapManager.getNamerUri();
        mOutputTitle = mSnapManager.getNamerTitle();

        // Process our images
        try {
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

            mSnapManager.saveImage(mOutputUri, mOutputTitle, 100, 100, 0, jpegData);
        } catch (IOException ex) {
            Log.e(TAG, "Unable to process: ", ex);
        }
    }

    private boolean doEnfuse() throws IOException {
        Log.d(TAG, "Enfuse...");

        String filesStr = "";
        for (Uri picture : mPictures) {
            filesStr += " " + picture.getPath();
        }

        run("enfuse -o "+mTempPath+"/final.jpg --compression=jpeg " + filesStr);

        Log.d(TAG, "Enfuse... done");
        return true;
    }
}
