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

package org.cyanogenmod.focal.feats;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import org.cyanogenmod.focal.SnapshotManager;

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

    public File getTempPath() {
        return mTempPath;
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

    public boolean render(final int orientation) {
        mOutputLogger.start();

        // Prepare a temporary directory
        Log.d(TAG, "Preparing temp dir for Software HDR rendering...");
        File appFilesDir = mContext.getFilesDir();
        mPathPrefix = appFilesDir.getAbsolutePath() + "/";
        String tempPathStr = appFilesDir.getAbsolutePath() + "/" + System.currentTimeMillis();
        mTempPath = new File(tempPathStr);
        mTempPath.mkdir();


        // Process our images
        try {
            if (!doAlignImageStack()) {
                return false;
            }
            if (!doEnfuse()) {
                return false;
            }

            // Save it to gallery
            // XXX: This needs opening the output byte array...
            // Isn't there any way to update gallery data without having to reload/save
            // the JPEG file? Because we have it already, we could just move it.
            byte[] jpegData;
            RandomAccessFile f = new RandomAccessFile(mTempPath+"/final.jpg", "r");
            try {
                // Get and check length
                long longlength = f.length();
                int length = (int) longlength;
                if (length != longlength) {
                    throw new IOException("File size >= 2 GB");
                }
                // Read file and return data
                jpegData = new byte[length];
                f.readFully(jpegData);
            } finally {
                f.close();
            }

            mSnapManager.prepareNamerUri(100,100);
            mOutputUri = mSnapManager.getNamerUri();
            mOutputTitle = mSnapManager.getNamerTitle();
            mSnapManager.saveImage(mOutputUri, mOutputTitle, 100, 100, orientation, jpegData);
        } catch (IOException ex) {
            Log.e(TAG, "Unable to process: ", ex);
            return false;
        }

        return true;
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
            if (new File(picture.getPath()).exists()) {
                filesStr += " " + picture.getPath();
            }
        }

        run("align_image_stack -v -v -v -C -g 4 -a "+mTempPath+"/project " + filesStr);
        consumeProcLogs();

        Log.d(TAG, "Align Image Stack... done");
        return true;
    }

    private boolean doEnfuse() throws IOException {
        Log.d(TAG, "Enfuse...");

        // Build the list of output files. The convention set up by
        // AlignImageStack is projectXXXX.tif, so we basically build that
        // list out of the number of shots we fed to align_image_stack
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
