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
 * The Java interface to JNI calls regarding mosaic preview rendering.
 *
 */
public class MosaicRenderer {
    static {
        System.loadLibrary("jni_mosaic2");
    }

    /**
     * Function to be called in onSurfaceCreated() to initialize
     * the GL context, load and link the shaders and create the
     * program. Returns a texture ID to be used for SurfaceTexture.
     *
     * @return textureID the texture ID of the newly generated texture to
     *          be assigned to the SurfaceTexture object.
     */
    public static native int init();

    /**
     * Pass the drawing surface's width and height to initialize the
     * renderer viewports and FBO dimensions.
     *
     * @param width width of the drawing surface in pixels.
     * @param height height of the drawing surface in pixels.
     * @param isLandscapeOrientation is the orientation of the activity layout in landscape.
     */
    public static native void reset(int width, int height, boolean isLandscapeOrientation);

    /**
     * Changes the orientation of the stitching
     *
     * @param isLandscape
     */
    public static native void setIsLandscape(boolean isLandscape);

    /**
     * Calling this function will render the SurfaceTexture to a new 2D texture
     * using the provided STMatrix.
     *
     * @param stMatrix texture coordinate transform matrix obtained from the
     *        Surface texture
     */
    public static native void preprocess(float[] stMatrix);

    /**
     * This function calls glReadPixels to transfer both the low-res and high-res
     * data from the GPU memory to the CPU memory for further processing by the
     * mosaicing library.
     */
    public static native void transferGPUtoCPU();

    /**
     * Function to be called in onDrawFrame() to update the screen with
     * the new frame data.
     */
    public static native void step();

    /**
     * Call this function when a new low-res frame has been processed by
     * the mosaicing library. This will tell the renderer library to
     * update its texture and warping transformation. Any calls to step()
     * after this call will use the new image frame and transformation data.
     */
    public static native void updateMatrix();

    /**
     * This function allows toggling between showing the input image data
     * (without applying any warp) and the warped image data. For running
     * the renderer as a viewfinder, we set the flag to false. To see the
     * preview mosaic, we set the flag to true.
     *
     * @param flag boolean flag to set the warping to true or false.
     */
    public static native void setWarping(boolean flag);
}
