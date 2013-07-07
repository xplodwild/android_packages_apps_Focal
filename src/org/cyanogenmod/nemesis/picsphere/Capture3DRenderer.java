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
    private float[] mModelMatrix = new float[16];
    private float[] mMVPMatrix = new float[16];


    /**
     * Stores the information about each snapshot displayed in the sphere
     */
    private class Snapshot {
        private Quaternion mAngle;
        private int mTextureData;
        private Bitmap mBitmapToLoad;

        public Snapshot() {
            mAngle = new Quaternion();
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
                // Displays error
            }

            mTextureData = texture[0];
            mBitmapToLoad = null;
        }

        public void draw(){
            if (mBitmapToLoad != null) {
                loadTexture();
            }

            mModelMatrix = mAngle.getMatrix();
            Matrix.translateM(mModelMatrix, 0, 0.0f, 0.0f, -400);

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

        // Enable depth testing
        //GLES20.glEnable(GLES20.GL_DEPTH_TEST);

        setCameraDirection(0, 0, -5);
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

        float fov = 60;

        // Create a new perspective projection matrix. The height will stay the same
        // while the width will vary as per aspect ratio.
        final float ratio = (float) width / height;
        final float near = 0.1f;
        final float far = 500.0f;
        final float bottom = (float) Math.tan(fov * Math.PI / 360.0f) * near;
        final float top = -bottom;
        final float left = ratio * top;
        final float right = ratio * bottom;

        Matrix.frustumM(mProjectionMatrix, 0, left, right, bottom, top, near, far);
    }

    @Override
    public void onDrawFrame(GL10 glUnused) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        float[] orientation = mSensorFusion.getFusedOrientation();
        setCameraOrientation(orientation[0], orientation[1], orientation[2]);

        mListBusy.lock();
        for (Snapshot snap : mSnapshots) {
            snap.draw();
        }
        mListBusy.unlock();
    }

    public void setCameraDirection(float lookX, float lookY, float lookZ) {
        // Position the eye in front of the origin.
        final float eyeX = 0.0f;
        final float eyeY = 0.0f;
        final float eyeZ = 1f;

        // Set our up vector. This is where our head would be pointing were we holding the camera.
        final float upX = 0.0f;
        final float upY = 1.0f;
        final float upZ = 0.0f;

        // Set the view matrix. This matrix can be said to represent the camera position.
        // NOTE: In OpenGL 1, a ModelView matrix is used, which is a combination of a model and
        // view matrix. In OpenGL 2, we can keep track of these matrices separately if we choose.
        Matrix.setLookAtM(mViewMatrix, 0, eyeX, eyeY, eyeZ, lookX, lookY, lookZ, upX, upY, upZ);
    }

    public void setCameraOrientation(float rX, float rY, float rZ) {
        // Position the eye in front of the origin.
        final float eyeX = 0.0f;
        final float eyeY = 0.0f;
        final float eyeZ = 0.0f;

        // Set our up vector. This is where our head would be pointing were we holding the camera.
        final float upX = 0.0f;
        final float upY = 1.0f;
        final float upZ = 0.0f;

        // Convert to degrees
        rX = (float) (rX * 180.0f/Math.PI);
        rY = (float) (rY * 180.0f/Math.PI);
        rZ = (float) (rZ * 180.0f/Math.PI);

       // Log.e(TAG, "rY = " + rY);

        mCameraQuat.fromEuler(rY, 0.0f, rX);
        mViewMatrix = mCameraQuat.getConjugate().getMatrix();

        /*
        // Set the view matrix. This matrix can be said to represent the camera position.
        // NOTE: In OpenGL 1, a ModelView matrix is used, which is a combination of a model and
        // view matrix. In OpenGL 2, we can keep track of these matrices separately if we choose.
        Matrix.setLookAtM(mViewMatrix, 0, eyeX, eyeY, eyeZ, 0, 0, -1, upX, upY, upZ);


        Matrix.rotateM(mViewMatrix, 0, (float) Math.sqrt(rZ * rZ + rY * rY), rZ, rY, 0);
        //Matrix.rotateM(mViewMatrix, 0, 270.0f, 0, 1.0f, 0);
        //Matrix.rotateM(mViewMatrix, 0, rY, 0, 1.0f, 0);
        //Matrix.rotateM(mViewMatrix, 0, rZ, 0, 0, 1.0f);
        */
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
    public synchronized void addSnapshot(final Bitmap image) {
        Snapshot snap = new Snapshot();
        snap.mAngle = new Quaternion(mCameraQuat);

        snap.setTexture(image);
        Log.e(TAG, "Created snapshot");
        if (image == null) {
            Log.e(TAG, "Bitmap is null");
        } else {
            Log.e(TAG, "Bitmap is not null");
        }

        mListBusy.lock();
        mSnapshots.add(snap);
        mListBusy.unlock();
    }

    /**
     * Clear sphere's snapshots
     */
    public void clearSnapshots() {
        mSnapshots.clear();
    }
}