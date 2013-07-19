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
import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.os.Handler;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Vector;
import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Manages the 3D rendering of the sphere capture mode, using gyroscope to orientate the camera
 * and displays the preview at the center.
 *
 * TODO: Fallback for non-GLES2 devices?
 */
public class Capture3DRenderer implements GLSurfaceView.Renderer {
    public final static String TAG = "Capture3DRenderer";

    private List<Snapshot> mSnapshots;
    private ReentrantLock mListBusy;
    private SensorFusion mSensorFusion;
    private Quaternion mCameraQuat;
    private float[] mViewMatrix = new float[16];
    private float[] mProjectionMatrix = new float[16];


    private FloatBuffer mVertexBuffer;
    private FloatBuffer mTexCoordBuffer;
    private final static float mScale = 150f;
    // x, y,
    private final float mVertexData[] =
            {-mScale,  -mScale,
                    -mScale, mScale,
                    mScale, mScale,
                    mScale, -mScale};
    // u,v
    private final float mTexCoordData[] =   {0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f};

    private int mProgram;
    private int mVertexShader;
    private int mFragmentShader;
    private int mPositionHandler;
    private int mTexCoordHandler;
    private int mTextureHandler;
    private int mMVPMatrixHandler;
    private float[] mMVPMatrix = new float[16];


    /**
     * Stores the information about each snapshot displayed in the sphere
     */
    private class Snapshot {
        private float[]mModelMatrix;
        private int mTextureData;
        private Bitmap mBitmapToLoad;

        public Snapshot() {

        }

        public void setTexture(Bitmap tex) {
            mBitmapToLoad = tex;
        }

        private void loadTexture() {
            // Load the snapshot bitmap as a texture to bind to our GLES20 program
            int texture[] = new int[1];

            GLES20.glGenTextures(1, texture, 0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture[0]);

            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, mBitmapToLoad, 0);
            //tex.recycle();

            if(texture[0] == 0){
                Log.e(TAG, "Unable to attribute texture to quad");
            }

            mTextureData = texture[0];
            mBitmapToLoad = null;
        }

        public void draw(){
            if (mBitmapToLoad != null) {
                loadTexture();
            }



            GLES20.glUseProgram(mProgram);
            mVertexBuffer.position(0);
            mTexCoordBuffer.position(0);


            GLES20.glEnableVertexAttribArray(mTexCoordHandler);
            GLES20.glEnableVertexAttribArray(mPositionHandler);

            GLES20.glVertexAttribPointer(mPositionHandler, 2, GLES20.GL_FLOAT, false, 8, mVertexBuffer);
            GLES20.glVertexAttribPointer(mTexCoordHandler, 2, GLES20.GL_FLOAT, false, 8, mTexCoordBuffer);

            // This multiplies the view matrix by the model matrix, and stores the result in the MVP matrix
            // (which currently contains model * view).
            Matrix.multiplyMM(mMVPMatrix, 0, mViewMatrix, 0, mModelMatrix, 0);

            // This multiplies the modelview matrix by the projection matrix, and stores the result in the MVP matrix
            // (which now contains model * view * projection).
            Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mMVPMatrix, 0);

            // Pass in the combined matrix.
            GLES20.glUniformMatrix4fv(mMVPMatrixHandler, 1, false, mMVPMatrix, 0);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureData);

            GLES20.glUniform1i(mTextureHandler, 0);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_FAN, 0, 4);
        }
    }

    /**
     * Initialize the model data.
     */
    public Capture3DRenderer(Context context) {
        mSnapshots = new ArrayList<Snapshot>();
        mListBusy = new ReentrantLock();
        mSensorFusion = new SensorFusion(context);
        mCameraQuat = new Quaternion();
    }

    @Override
    public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
        // Set the background clear color to gray.
        GLES20.glClearColor(0.5f, 0.5f, 0.5f, 0.5f);

        // Initialize plane vertex data and texcoords data
        ByteBuffer bb_data = ByteBuffer.allocateDirect(mVertexData.length * 4);
        bb_data.order(ByteOrder.nativeOrder());
        ByteBuffer bb_texture = ByteBuffer.allocateDirect(mTexCoordData.length * 4);
        bb_texture.order(ByteOrder.nativeOrder());

        mVertexBuffer = bb_data.asFloatBuffer();
        mVertexBuffer.put(mVertexData);
        mVertexBuffer.position(0);
        mTexCoordBuffer = bb_texture.asFloatBuffer();
        mTexCoordBuffer.put(mTexCoordData);
        mTexCoordBuffer.position(0);

        // Simple GLSL vertex/fragment, as GLES2 doesn't have the classical fixed pipeline
        final String vertexShader =
                "uniform mat4 u_MVPMatrix; \n"
                        + "attribute vec4 a_Position;     \n"
                        + "attribute vec2 a_TexCoordinate;\n"
                        + "varying vec2 v_TexCoordinate;  \n"
                        + "void main()                    \n"
                        + "{                              \n"
                        + "   v_TexCoordinate = a_TexCoordinate;\n"
                        + "   gl_Position = u_MVPMatrix * a_Position;   \n"
                        + "}                              \n";

        final String fragmentShader =
                "precision mediump float;       \n"
                        + "uniform sampler2D u_Texture;   \n"
                        + "varying vec2 v_TexCoordinate;  \n"
                        + "void main()                    \n"
                        + "{                              \n"
                        + "   gl_FragColor = texture2D(u_Texture, v_TexCoordinate);\n"
                        + "}                              \n";

        mVertexShader = compileShader(GLES20.GL_VERTEX_SHADER, vertexShader);
        mFragmentShader = compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader);

        // create the program and bind the shader attributes
        mProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(mProgram, mFragmentShader);
        GLES20.glAttachShader(mProgram, mVertexShader);
        GLES20.glLinkProgram(mProgram);

        int[] linkStatus = new int[1];
        GLES20.glGetProgramiv(mProgram, GLES20.GL_LINK_STATUS, linkStatus, 0);

        if (linkStatus[0] == 0) {
            throw new RuntimeException("Error linking shaders");
        }
        mPositionHandler     = GLES20.glGetAttribLocation(mProgram, "a_Position");
        mTexCoordHandler     = GLES20.glGetAttribLocation(mProgram, "a_TexCoordinate");
        mMVPMatrixHandler    = GLES20.glGetUniformLocation(mProgram, "u_MVPMatrix");
        mTextureHandler      = GLES20.glGetUniformLocation(mProgram, "u_Texture");
    }

    @Override
    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        // Set the OpenGL viewport to the same size as the surface.
        GLES20.glViewport(0, 0, width, height);

        // We use here a field of view of 60, which is mostly fine for a camera app representation
        final float fov = 60.0f;

        // Create a new perspective projection matrix. The height will stay the same
        // while the width will vary as per aspect ratio.
        final float ratio = (float) width / height;
        final float near = 0.1f;
        final float far = 500.0f;
        final float bottom = (float) Math.tan(fov * Math.PI / 360.0f) * near;
        final float top = -bottom;
        final float right = ratio * bottom;
        final float left = ratio * top;

        Matrix.frustumM(mProjectionMatrix, 0, left, right, bottom, top, near, far);
    }

    @Override
    public void onDrawFrame(GL10 glUnused) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        // Update camera view matrix
        //mViewMatrix = mSensorFusion.getRotationMatrix();

        float[] orientation = mSensorFusion.getFusedOrientation();
        // Convert angles to degrees
        float rX = (float) (orientation[2] * 180.0f/Math.PI);
        float rY = (float) (orientation[1] * 180.0f/Math.PI);

        // Update quaternion from euler angles out of orientation
        mCameraQuat.fromEuler( rX, 0.0f, rY);
        mViewMatrix = mCameraQuat.getMatrix();


        mListBusy.lock();
        for (Snapshot snap : mSnapshots) {
            snap.draw();
        }
        mListBusy.unlock();
    }

    public void onPause() {
        if (mSensorFusion != null) {
            mSensorFusion.onPauseOrStop();
        }
    }

    public void onResume() {
        if (mSensorFusion != null) {
            mSensorFusion.onResume();
        }
    }

    public void setCameraOrientation(float rX, float rY, float rZ) {
        // Convert angles to degrees
        rX = (float) (rX * 180.0f/Math.PI);
        rY = (float) (rY * 180.0f/Math.PI);

        // Update quaternion from euler angles out of orientation and set it as view matrix
        mCameraQuat.fromEuler(rY, 0.0f, rX);
        mViewMatrix = mCameraQuat.getConjugate().getMatrix();
    }

    /**
     * Helper function to compile a shader.
     *
     * @param shaderType The shader type.
     * @param shaderSource The shader source code.
     * @return An OpenGL handle to the shader.
     */
    public static int compileShader(final int shaderType, final String shaderSource) {
        int shaderHandle = GLES20.glCreateShader(shaderType);

        if (shaderHandle != 0) {
            // Pass in the shader source.
            GLES20.glShaderSource(shaderHandle, shaderSource);

            // Compile the shader.
            GLES20.glCompileShader(shaderHandle);

            // Get the compilation status.
            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(shaderHandle, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

            // If the compilation failed, delete the shader.
            if (compileStatus[0] == 0) {
                Log.e(TAG, "Error compiling shader: " + GLES20.glGetShaderInfoLog(shaderHandle));
                GLES20.glDeleteShader(shaderHandle);
                shaderHandle = 0;
            }
        }

        if (shaderHandle == 0) {
            throw new RuntimeException("Error creating shader.");
        }

        return shaderHandle;
    }

    /**
     * Adds a snapshot to the sphere
     */
    public void addSnapshot(final Bitmap image) {
        Snapshot snap = new Snapshot();
        snap.mModelMatrix = Arrays.copyOf(mViewMatrix, mViewMatrix.length);

        Matrix.invertM(snap.mModelMatrix, 0, snap.mModelMatrix, 0);
        Matrix.translateM(snap.mModelMatrix, 0, 0.0f, 0.0f, -400);

        snap.setTexture(image);

        mListBusy.lock();
        mSnapshots.add(snap);
        mListBusy.unlock();
    }

    /**
     * Clear sphere's snapshots
     */
    public void clearSnapshots() {
        mListBusy.lock();
        mSnapshots.clear();
        mListBusy.unlock();
    }
}