/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright (C) 2013 Guillaume Lesniak
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

package org.cyanogenmod.focal.pano;

/**
 * The Java interface to JNI calls regarding mosaic stitching.
 *
 * A high-level usage is:
 *
 * Mosaic mosaic = new Mosaic();
 * mosaic.setSourceImageDimensions(width, height);
 * mosaic.reset(blendType);
 *
 * while ((pixels = hasNextImage()) != null) {
 *    mosaic.setSourceImage(pixels);
 * }
 *
 * mosaic.createMosaic(highRes);
 * byte[] result = mosaic.getFinalMosaic();
 *
 */
public class Mosaic {
    /**
     * In this mode, the images are stitched together in the same spatial arrangement as acquired
     * i.e. if the user follows a curvy trajectory, the image boundary of the resulting mosaic will
     * be curved in the same manner. This mode is useful if the user wants to capture a mosaic as
     * if "painting" the scene using the smart-phone device and does not want any corrective warps
     * to distort the captured images.
     */
    public static final int BLENDTYPE_FULL = 0;

    /**
     * This mode is the same as BLENDTYPE_FULL except that the resulting mosaic is rotated
     * to balance the first and last images to be approximately at the same vertical offset in the
     * output mosaic. This is useful when acquiring a mosaic by a typical panning-like motion to
     * remove a one-sided curve in the mosaic (typically due to the camera not staying horizontal
     * during the video capture) and convert it to a more symmetrical "smiley-face" like output.
     */
    public static final int BLENDTYPE_PAN = 1;

    /**
     * This mode compensates for typical "smiley-face" like output in longer mosaics and creates
     * a rectangular mosaic with minimal black borders (by unwrapping the mosaic onto an imaginary
     * cylinder). If the user follows a curved trajectory (instead of a perfect panning trajectory),
     * the resulting mosaic here may suffer from some image distortions in trying to map the
     * trajectory to a cylinder.
     */
    public static final int BLENDTYPE_CYLINDERPAN = 2;

    /**
     * This mode is basically BLENDTYPE_CYLINDERPAN plus doing a rectangle cropping before returning
     * the mosaic. The mode is useful for making the resulting mosaic have a rectangle shape.
     */
    public static final int BLENDTYPE_HORIZONTAL =3;

    /**
     * This strip type will use the default thin strips where the strips are
     * spaced according to the image capture rate.
     */
    public static final int STRIPTYPE_THIN = 0;

    /**
     * This strip type will use wider strips for blending. The strip separation
     * is controlled by a threshold on the native side. Since the strips are
     * wider, there is an additional cross-fade blending step to make the seam
     * boundaries smoother. Since this mode uses lesser image frames, it is
     * computationally more efficient than the thin strip mode.
     */
    public static final int STRIPTYPE_WIDE = 1;

    /**
     * Return flags returned by createMosaic() are one of the following.
     */
    public static final int MOSAIC_RET_OK = 1;
    public static final int MOSAIC_RET_ERROR = -1;
    public static final int MOSAIC_RET_CANCELLED = -2;
    public static final int MOSAIC_RET_LOW_TEXTURE = -3;
    public static final int MOSAIC_RET_FEW_INLIERS = 2;


    static {
        System.loadLibrary("jni_mosaic2");
    }

    /**
     * Allocate memory for the image frames at the given resolution.
     *
     * @param width width of the input frames in pixels
     * @param height height of the input frames in pixels
     */
    public native void allocateMosaicMemory(int width, int height);

    /**
     * Free memory allocated by allocateMosaicMemory.
     *
     */
    public native void freeMosaicMemory();

    /**
     * Pass the input image frame to the native layer. Each time the a new
     * source image t is set, the transformation matrix from the first source
     * image to t is computed and returned.
     *
     * @param pixels source image of NV21 format.
     * @return Float array of length 11; first 9 entries correspond to the 3x3
     *         transformation matrix between the first frame and the passed frame;
     *         the 10th entry is the number of the passed frame, where the counting
     *         starts from 1; and the 11th entry is the returning code, whose value
     *         is one of those MOSAIC_RET_* returning flags defined above.
     */
    public native float[] setSourceImage(byte[] pixels);

    /**
     * This is an alternative to the setSourceImage function above. This should
     * be called when the image data is already on the native side in a fixed
     * byte array. In implementation, this array is filled by the GL thread
     * using glReadPixels directly from GPU memory (where it is accessed by
     * an associated SurfaceTexture).
     *
     * @return Float array of length 11; first 9 entries correspond to the 3x3
     *         transformation matrix between the first frame and the passed frame;
     *         the 10th entry is the number of the passed frame, where the counting
     *         starts from 1; and the 11th entry is the returning code, whose value
     *         is one of those MOSAIC_RET_* returning flags defined above.
     */
    public native float[] setSourceImageFromGPU();

    /**
     * Set the type of blending.
     *
     * @param type the blending type defined in the class. {BLENDTYPE_FULL,
     *        BLENDTYPE_PAN, BLENDTYPE_CYLINDERPAN, BLENDTYPE_HORIZONTAL}
     */
    public native void setBlendingType(int type);

    /**
     * Set the type of strips to use for blending.
     * @param type the blending strip type to use {STRIPTYPE_THIN,
     * STRIPTYPE_WIDE}.
     */
    public native void setStripType(int type);

    /**
     * Tell the native layer to create the final mosaic after all the input frame
     * data have been collected.
     * The case of generating high-resolution mosaic may take dozens of seconds to finish.
     *
     * @param value True means generating a high-resolution mosaic -
     *        which is based on the original images set in setSourceImage().
     *        False means generating a low-resolution version -
     *        which is based on 1/4 downscaled images from the original images.
     * @return Returns a status code suggesting if the mosaic building was
     *        successful, in error, or was cancelled by the user.
     */
    public native int createMosaic(boolean value);

    /**
     * Get the data for the created mosaic.
     *
     * @return Returns an integer array which contains the final mosaic in the ARGB_8888 format.
     *         The first MosaicWidth*MosaicHeight values contain the image data, followed by 2
     *         integers corresponding to the values MosaicWidth and MosaicHeight respectively.
     */
    public native int[] getFinalMosaic();

    /**
     * Get the data for the created mosaic.
     *
     * @return Returns a byte array which contains the final mosaic in the NV21 format.
     *         The first MosaicWidth*MosaicHeight*1.5 values contain the image data, followed by
     *         8 bytes which pack the MosaicWidth and MosaicHeight integers into 4 bytes each
     *         respectively.
     */
    public native byte[] getFinalMosaicNV21();

    /**
     * Reset the state of the frame arrays which maintain the captured frame data.
     * Also re-initializes the native mosaic object to make it ready for capturing a new mosaic.
     */
    public native void reset();

    /**
     * Get the progress status of the mosaic computation process.
     * @param hires Boolean flag to select whether to report progress of the
     *              low-res or high-res mosaicer.
     * @param cancelComputation Boolean flag to allow cancelling the
     *              mosaic computation when needed from the GUI end.
     * @return Returns a number from 0-100 where 50 denotes that the mosaic
     *          computation is 50% done.
     */
    public native int reportProgress(boolean hires, boolean cancelComputation);
}
