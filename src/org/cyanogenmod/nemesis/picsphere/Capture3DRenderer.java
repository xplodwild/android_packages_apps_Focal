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
import android.graphics.BitmapFactory;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.os.SystemClock;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Manages the 3D rendering of the sphere capture mode, using gyroscope to orientate the camera
 * and displays the preview at the center.
 *
 * TODO: Fallback for non-GLES2 devices?
 */
public class Capture3DRenderer implements GLSurfaceView.Renderer, SensorEventListener {
    public final static String TAG = "Capture3DRenderer";

    private float[] mModelMatrix = new float[16];
    private float[] mViewMatrix = new float[16];
    private float[] mProjectionMatrix = new float[16];
    private float[] mMVPMatrix = new float[16];

    private final FloatBuffer mRectangleVertices;
    private final FloatBuffer mRectangleTexCoords;

    private int mProgramHandle;
    private int mMVPMatrixHandle;
    private int mPositionHandle;
    private int mTexCoordHandle;
    private int mTextureUniformHandle;

    private final int mBytesPerFloat = 4;
    private final int mStrideBytes = 7 * mBytesPerFloat;
    private final int mPositionOffset = 0;
    private final int mPositionDataSize = 3;
    private final int mTextureCoordinateDataSize = 2;

    private List<Snapshot> mSnapshots;
    private Context mContext;
    private float mGyroscopeX;
    private float mGyroscopeY;
    private float mGyroscopeZ;

    /**
     * Stores the information about each snapshot displayed in the sphere
     */
    private class Snapshot {
        public Vector<Float> mAngle;
        public int mTextureDataHandle;

        public Snapshot() {
            mAngle = new Vector<Float>(3);
            mTextureDataHandle = 0;
        }
    }

    /**
     * Initialize the model data.
     */
    public Capture3DRenderer(Context context) {
        mSnapshots = new ArrayList<Snapshot>();
        mContext = context;

        // Initialize the shape of planes (front-facing plane)
        final float[] rectangleVertexData = {
                -1.0f,  1.0f, 0.0f,
                -1.0f, -1.0f, 0.0f,
                 1.0f,  1.0f, 0.0f,
                -1.0f, -1.0f, 0.0f,
                 1.0f, -1.0f, 0.0f,
                 1.0f,  1.0f, 0.0f,
                };

        final float[] rectangleTexCoords = {
                // Front face
                0.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f
        };

        // Initialize the buffers.
        mRectangleVertices = ByteBuffer.allocateDirect(rectangleVertexData.length * mBytesPerFloat)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();

        mRectangleVertices.put(rectangleVertexData).position(0);

        mRectangleTexCoords = ByteBuffer.allocateDirect(rectangleTexCoords.length * mBytesPerFloat)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mRectangleTexCoords.put(rectangleTexCoords).position(0);
    }

    @Override
    public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
        // Set the background clear color to gray.
        GLES20.glClearColor(0.5f, 0.5f, 0.5f, 0.5f);

        // Enable depth testing
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);

        // The below glEnable() call is a holdover from OpenGL ES 1, and is not needed in OpenGL ES 2.
        // Enable texture mapping
        // GLES20.glEnable(GLES20.GL_TEXTURE_2D);

        // Make the camera look at some random direction while we wait for gyro
        setCameraDirection(0.0f, 0.0f, -5.0f);

        // Simple GLSL vertex/fragment, as GLES2 doesn't have the classical fixed pipeline
        final String vertexShader =
                "uniform mat4 u_MVPMatrix;      \n"
                        + "attribute vec4 a_Position;     \n"
                        + "attribute vec2 a_TexCoordinate;\n"
                        + "varying vec2 v_TexCoordinate;  \n"
                        + "void main()                    \n"
                        + "{                              \n"
                        + "   v_TexCoordinate = a_TexCoordinate;\n"
                        + "   gl_Position = u_MVPMatrix   \n"
                        + "               * a_Position;   \n"
                        + "}                              \n";

        final String fragmentShader =
                "precision mediump float;       \n"
                        + "uniform sampler2D u_Texture;   \n"
                        + "varying vec2 v_TexCoordinate;  \n"
                        + "void main()                    \n"
                        + "{                              \n"
                        + "   gl_FragColor = texture2D(u_Texture, v_TexCoordinate);\n"
                        + "}                              \n";

        // Load in the vertex shader.
        int vertexShaderHandle = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);

        if (vertexShaderHandle != 0) {
            // Pass in the shader source.
            GLES20.glShaderSource(vertexShaderHandle, vertexShader);

            // Compile the shader.
            GLES20.glCompileShader(vertexShaderHandle);

            // Get the compilation status.
            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(vertexShaderHandle, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

            // If the compilation failed, delete the shader.
            if (compileStatus[0] == 0) {
                GLES20.glDeleteShader(vertexShaderHandle);
                vertexShaderHandle = 0;
            }
        }

        if (vertexShaderHandle == 0) {
            throw new RuntimeException("Error creating vertex shader.");
        }

        // Load in the fragment shader shader.
        int fragmentShaderHandle = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);

        if (fragmentShaderHandle != 0) {
            // Pass in the shader source.
            GLES20.glShaderSource(fragmentShaderHandle, fragmentShader);

            // Compile the shader.
            GLES20.glCompileShader(fragmentShaderHandle);

            // Get the compilation status.
            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(fragmentShaderHandle, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

            // If the compilation failed, delete the shader.
            if (compileStatus[0] == 0) {
                GLES20.glDeleteShader(fragmentShaderHandle);
                fragmentShaderHandle = 0;
            }
        }

        if (fragmentShaderHandle == 0) {
            throw new RuntimeException("Error creating fragment shader.");
        }

        // Create a program object and store the handle to it.
        int programHandle = GLES20.glCreateProgram();

        if (programHandle != 0) {
            // Bind the vertex shader to the program.
            GLES20.glAttachShader(programHandle, vertexShaderHandle);

            // Bind the fragment shader to the program.
            GLES20.glAttachShader(programHandle, fragmentShaderHandle);

            // Bind attributes
            GLES20.glBindAttribLocation(programHandle, 0, "a_Position");
            GLES20.glBindAttribLocation(programHandle, 1, "a_TexCoordinate");

            // Link the two shaders together into a program.
            GLES20.glLinkProgram(programHandle);

            // Get the link status.
            final int[] linkStatus = new int[1];
            GLES20.glGetProgramiv(programHandle, GLES20.GL_LINK_STATUS, linkStatus, 0);

            // If the link failed, delete the program.
            if (linkStatus[0] == 0) {
                GLES20.glDeleteProgram(programHandle);
                programHandle = 0;
            }
        }

        if (programHandle == 0) {
            throw new RuntimeException("Error creating program.");
        }

        mProgramHandle = programHandle;

        // Set program handles. These will later be used to pass in values to the program.
        mMVPMatrixHandle = GLES20.glGetUniformLocation(programHandle, "u_MVPMatrix");
        mPositionHandle = GLES20.glGetAttribLocation(programHandle, "a_Position");
        mTexCoordHandle = GLES20.glGetAttribLocation(programHandle, "a_TexCoordinate");
        mTextureUniformHandle = GLES20.glGetUniformLocation(mProgramHandle, "u_Texture");

        // Tell OpenGL to use this program when rendering.
        GLES20.glUseProgram(programHandle);
    }

    @Override
    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        // Set the OpenGL viewport to the same size as the surface.
        GLES20.glViewport(0, 0, width, height);

        // Create a new perspective projection matrix. The height will stay the same
        // while the width will vary as per aspect ratio.
        final float ratio = (float) width / height;
        final float left = -ratio;
        final float right = ratio;
        final float bottom = -1.0f;
        final float top = 1.0f;
        final float near = 1.0f;
        final float far = 10.0f;

        Matrix.frustumM(mProjectionMatrix, 0, left, right, bottom, top, near, far);
    }

    @Override
    public void onDrawFrame(GL10 glUnused) {
        GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

        for (Snapshot snap : mSnapshots) {
            // Set the texture of this snapshot
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, snap.mTextureDataHandle);
            GLES20.glUniform1i(mTextureUniformHandle, 0);

            // Draw the rectangle facing straight on.
            Matrix.setIdentityM(mModelMatrix, 0);
            Matrix.rotateM(mModelMatrix, 0, snap.mAngle.get(0), 1.0f, 0.0f, 0.0f);
            Matrix.rotateM(mModelMatrix, 0, snap.mAngle.get(1), 0.0f, 1.0f, 0.0f);
            Matrix.rotateM(mModelMatrix, 0, snap.mAngle.get(2), 0.0f, 0.0f, 1.0f);
            Matrix.translateM(mModelMatrix, 0, 0.0f, 0.0f, 5.0f);
            drawTriangle(mRectangleVertices);
        }
    }

    /**
     * Draws a triangle from the given vertex data.
     *
     * @param aTriangleBuffer The buffer containing the vertex data.
     */
    private void drawTriangle(final FloatBuffer aTriangleBuffer) {
        // Pass in the position information
        aTriangleBuffer.position(mPositionOffset);
        GLES20.glVertexAttribPointer(mPositionHandle, mPositionDataSize, GLES20.GL_FLOAT, false,
                mStrideBytes, aTriangleBuffer);

        GLES20.glEnableVertexAttribArray(mPositionHandle);

        // Pass in the texture coordinate information
        mRectangleTexCoords.position(0);
        GLES20.glVertexAttribPointer(mTexCoordHandle, mTextureCoordinateDataSize, GLES20.GL_FLOAT, false,
                0, mRectangleTexCoords);

        GLES20.glEnableVertexAttribArray(mTexCoordHandle);

        // This multiplies the view matrix by the model matrix, and stores the result in the MVP matrix
        // (which currently contains model * view).
        Matrix.multiplyMM(mMVPMatrix, 0, mViewMatrix, 0, mModelMatrix, 0);

        // This multiplies the modelview matrix by the projection matrix, and stores the result in the MVP matrix
        // (which now contains model * view * projection).
        Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mMVPMatrix, 0);

        GLES20.glUniformMatrix4fv(mMVPMatrixHandle, 1, false, mMVPMatrix, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 6);
    }

    public void setCameraDirection(final float x, final float y, final float z) {
        // Position the camera at the center of our sphere
        final float eyeX = 0.0f;
        final float eyeY = 0.0f;
        final float eyeZ = 0.0f;

        // Set our up vector. This is where our head would be pointing were we holding the camera.
        final float upX = 0.0f;
        final float upY = 1.0f;
        final float upZ = 0.0f;

        // Set the view matrix. This matrix can be said to represent the camera position.
        // NOTE: In OpenGL 1, a ModelView matrix is used, which is a combination of a model and
        // view matrix. In OpenGL 2, we can keep track of these matrices separately if we choose.
        Matrix.setLookAtM(mViewMatrix, 0, eyeX, eyeY, eyeZ, x, y, z, upX, upY, upZ);
    }

    /**
     * Adds a snapshot to the sphere
     */
    public void addSnapshot(Bitmap image) {
        Snapshot snap = new Snapshot();
        snap.mAngle.set(0, mGyroscopeX);
        snap.mAngle.set(1, mGyroscopeY);
        snap.mAngle.set(2, mGyroscopeZ);

        // Load the snapshot bitmap as a texture to bind to our GLES20 program
        final int[] textureHandle = new int[1];

        GLES20.glGenTextures(1, textureHandle, 0);

        if (textureHandle[0] != 0) {
            // Bind to the texture in OpenGL
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureHandle[0]);

            // Set filtering
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);

            // Load the bitmap into the bound texture.
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, image, 0);

            // Recycle the bitmap, since its data has been loaded into OpenGL.
            // TODO: Do we still need it?
            // bitmap.recycle();
        }

        if (textureHandle[0] == 0) {
            throw new RuntimeException("Error loading texture.");
        }

        snap.mTextureDataHandle = textureHandle[0];

        mSnapshots.add(snap);
    }

    /**
     * Clear sphere's snapshots
     */
    public void clearSnapshots() {
        mSnapshots.clear();
    }

    /*
	 * (non-Javadoc)
	 *
	 * @see android.hardware.SensorEventListener#onAccuracyChanged(android.hardware.Sensor, int)
	 */
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        if (sensor.getType() == Sensor.TYPE_GYROSCOPE) {
            String accuracyStr;
            if (SensorManager.SENSOR_STATUS_ACCURACY_HIGH == accuracy) {
                accuracyStr = "SENSOR_STATUS_ACCURACY_HIGH";
            } else if (SensorManager.SENSOR_STATUS_ACCURACY_LOW == accuracy) {
                accuracyStr = "SENSOR_STATUS_ACCURACY_LOW";
            } else if (SensorManager.SENSOR_STATUS_ACCURACY_MEDIUM == accuracy) {
                accuracyStr = "SENSOR_STATUS_ACCURACY_MEDIUM";
            } else {
                accuracyStr = "SENSOR_STATUS_UNRELIABLE";
            }

            // TODO: Remove me
            Log.d(TAG, "Sensor's values (" + mGyroscopeX + "," + mGyroscopeY + "," + mGyroscopeZ + ") and accuracy : "
                    + accuracyStr);
        }
    }

    /*
     * (non-Javadoc)
     *
     * @see android.hardware.SensorEventListener#onSensorChanged(android.hardware.SensorEvent)
     */
    @Override
    public void onSensorChanged(SensorEvent event) {
        // update only when your are in the right case:
        if (event.sensor.getType() == Sensor.TYPE_GYROSCOPE) {
            // the angular speed around each axis
            mGyroscopeX = event.values[0];
            mGyroscopeY = event.values[1];
            mGyroscopeZ = event.values[2];

            setCameraDirection(mGyroscopeX, mGyroscopeY, mGyroscopeZ);
        }
    }
}