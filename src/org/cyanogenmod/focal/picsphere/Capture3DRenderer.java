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

package org.cyanogenmod.focal.picsphere;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.util.Log;

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Manages the 3D rendering of the sphere capture mode, using gyroscope to
 * orientate the camera and displays the preview at the center.
 *
 * TODO: Fallback for non-GLES2 devices?
 */
public class Capture3DRenderer implements GLSurfaceView.Renderer {
    public final static String TAG = "Capture3DRenderer";

    private final CameraManager mCamManager;

    private List<Snapshot> mSnapshots;
    private List<Snapshot> mDots;
    private ReentrantLock mListBusy;
    private SensorFusion mSensorFusion;
    private Quaternion mCameraQuat;
    private Skybox mSkyBox;
    private float[] mViewMatrix = new float[16];
    private float[] mProjectionMatrix = new float[16];

    private FloatBuffer mVertexBuffer;
    private FloatBuffer m43VertexBuffer;
    private FloatBuffer mTexCoordBuffer;
    private final static float SNAPSHOT_SCALE = 65.5f;
    private final static float RATIO = 4.0f/3.0f;
    private final static float DISTANCE = 135.0f;

    // x, y,
    private final float mVertexData[] =
            {
                    -SNAPSHOT_SCALE,  -SNAPSHOT_SCALE,
                    -SNAPSHOT_SCALE, SNAPSHOT_SCALE,
                    SNAPSHOT_SCALE, SNAPSHOT_SCALE,
                    SNAPSHOT_SCALE, -SNAPSHOT_SCALE
            };

    private final float m43VertexData[] =
            {
                    -SNAPSHOT_SCALE *RATIO,  -SNAPSHOT_SCALE,
                    -SNAPSHOT_SCALE *RATIO, SNAPSHOT_SCALE,
                    SNAPSHOT_SCALE *RATIO, SNAPSHOT_SCALE,
                    SNAPSHOT_SCALE *RATIO, -SNAPSHOT_SCALE
            };

    // u,v
    private final float mTexCoordData[] =
            {
                    0.0f, 0.0f,
                    0.0f, 1.0f,
                    1.0f, 1.0f,
                    1.0f, 0.0f
            };

    private final static int CAMERA = 0;
    private final static int SNAPSHOT = 1;

    private int[] mProgram = new int[2];
    private int[] mVertexShader = new int[2];
    private int[] mFragmentShader = new int[2];
    private int[] mPositionHandler = new int[2];
    private int[] mTexCoordHandler = new int[2];
    private int[] mTextureHandler = new int[2];
    private int[] mAlphaHandler = new int[2];
    private int[] mMVPMatrixHandler = new int[2];

    private SurfaceTexture mCameraSurfaceTex;
    private int mCameraTextureId;
    private Snapshot mCameraBillboard;
    private Snapshot mViewfinderBillboard;
    private Context mContext;
    private Quaternion mTempQuaternion;
    private float[] mMVPMatrix = new float[16];

    private class Skybox {
        private float DIST = SNAPSHOT_SCALE;
        private Snapshot[] mFaces = new Snapshot[6];
        private int FACE_NORTH = 0;
        private int FACE_WEST = 1;
        private int FACE_SOUTH = 2;
        private int FACE_EAST = 3;
        private int FACE_UP = 4;
        private int FACE_DOWN = 5;

        public Skybox() {
            mFaces[FACE_NORTH] = new Snapshot(false);
            mFaces[FACE_NORTH].mModelMatrix = matrixFromEuler(0, 90, 0, 0, 0, DIST);
            mFaces[FACE_NORTH].setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.picsphere_sky_fr));

            mFaces[FACE_SOUTH] = new Snapshot(false);
            mFaces[FACE_SOUTH].mModelMatrix = matrixFromEuler(0, 90, 0, 0, 0, -DIST);
            mFaces[FACE_SOUTH].setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.picsphere_sky_bk));

            mFaces[FACE_WEST] = new Snapshot(false);
            mFaces[FACE_WEST].mModelMatrix = matrixFromEuler(0, 90, 90, 0, 0, DIST);
            mFaces[FACE_WEST].setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.picsphere_sky_lt));

            mFaces[FACE_EAST] = new Snapshot(false);
            mFaces[FACE_EAST].mModelMatrix = matrixFromEuler(0, 90, 270, 0, 0, DIST);
            mFaces[FACE_EAST].setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.picsphere_sky_rt));

            mFaces[FACE_UP] = new Snapshot(false);
            mFaces[FACE_UP].mModelMatrix = matrixFromEuler(90, 0, 270, 0, 0, DIST);
            mFaces[FACE_UP].setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.picsphere_sky_up));

            mFaces[FACE_DOWN] = new Snapshot(false);
            mFaces[FACE_DOWN].mModelMatrix = matrixFromEuler(-90, 0, 90, 0, 0, DIST);
            mFaces[FACE_DOWN].setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.picsphere_sky_dn));
        }

        public void draw() {
            for (int i = 0; i < mFaces.length; i++) {
                if (mFaces[i] != null) {
                    mFaces[i].draw();
                }
            }
        }
    }

    /**
     * Stores the information about each snapshot displayed in the sphere
     */
    private class Snapshot {
        private float[]mModelMatrix;
        private int mTextureData;
        private Bitmap mBitmapToLoad;
        private boolean mIsFourToThree;
        private int mMode;
        private boolean mIsVisible = true;
        private float mAlpha = 1.0f;
        private float mAutoAlphaX;
        private float mAutoAlphaY;

        public Snapshot() {
            mIsFourToThree = true;
            mMode = SNAPSHOT;
        }

        public Snapshot(boolean isFourToThree) {
            mIsFourToThree = isFourToThree;
            mMode = SNAPSHOT;
        }

        public void setVisible(boolean visible) {
            mIsVisible = visible;
        }

        /**
         * Sets whether to use the CAMERA shaders or the SNAPSHOT shaders
         * @param mode CAMERA or SNAPSHOT
         */
        public void setMode(int mode) {
            mMode = mode;
        }

        public void setTexture(Bitmap tex) {
            mBitmapToLoad = tex;
        }

        public void setTextureId(int id) {
            mTextureData = id;
        }

        public void setAlpha(float alpha) {
            mAlpha = alpha;
        }

        public void setAutoAlphaAngle(float x, float y) {
            mAutoAlphaX = x;
            mAutoAlphaY = y;
        }

        public float getAutoAlphaX() {
            return mAutoAlphaX;
        }

        public float getAutoAlphaY() {
            return mAutoAlphaY;
        }

        private void loadTexture() {
            // Load the snapshot bitmap as a texture to bind to our GLES20 program
            int texture[] = new int[1];

            GLES20.glGenTextures(1, texture, 0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture[0]);

            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
                    GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
                    GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
                    GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
                    GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, mBitmapToLoad, 0);
            //tex.recycle();

            if(texture[0] == 0){
                Log.e(TAG, "Unable to attribute texture to quad");
            }

            mTextureData = texture[0];
            mBitmapToLoad = null;
        }

        public void draw() {
            if (!mIsVisible) return;

            if (mBitmapToLoad != null) {
                loadTexture();
            }

            GLES20.glUseProgram(mProgram[mMode]);
            if (mIsFourToThree) {
                m43VertexBuffer.position(0);
            } else {
                mVertexBuffer.position(0);
            }
            mTexCoordBuffer.position(0);

            GLES20.glEnableVertexAttribArray(mTexCoordHandler[mMode]);
            GLES20.glEnableVertexAttribArray(mPositionHandler[mMode]);

            if (mIsFourToThree) {
                GLES20.glVertexAttribPointer(mPositionHandler[mMode],
                        2, GLES20.GL_FLOAT, false, 8, m43VertexBuffer);
            } else {
                GLES20.glVertexAttribPointer(mPositionHandler[mMode],
                        2, GLES20.GL_FLOAT, false, 8, mVertexBuffer);
            }
            GLES20.glVertexAttribPointer(mTexCoordHandler[mMode], 2,
                    GLES20.GL_FLOAT, false, 8, mTexCoordBuffer);

            // This multiplies the view matrix by the model matrix, and stores the
            // result in the MVP matrix (which currently contains model * view).
            Matrix.multiplyMM(mMVPMatrix, 0, mViewMatrix, 0, mModelMatrix, 0);

            // This multiplies the modelview matrix by the projection matrix, and stores
            // the result in the MVP matrix (which now contains model * view * projection).
            Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mMVPMatrix, 0);

            // Pass in the combined matrix.
            GLES20.glUniformMatrix4fv(mMVPMatrixHandler[mMode], 1, false, mMVPMatrix, 0);

            GLES20.glUniform1f(mAlphaHandler[mMode], mAlpha);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureData);

            GLES20.glUniform1i(mTextureHandler[mMode], 0);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_FAN, 0, 4);
        }
    }

    /**
     * Initialize the model data.
     */
    public Capture3DRenderer(Context context, CameraManager cameraManager) {
        mSnapshots = new ArrayList<Snapshot>();
        mDots = new ArrayList<Snapshot>();
        mCamManager = cameraManager;
        mListBusy = new ReentrantLock();
        mSensorFusion = new SensorFusion(context);
        mCameraQuat = new Quaternion();
        mContext = context;
        mTempQuaternion = new Quaternion();

        // Position the dots every 40°
        for (int x = 0; x < 360; x += 360/12) {
            for (int y = 0; y < 360; y += 360/12) {
                createDot(x, y);
            }
        }

    }

    private void createDot(float rx, float ry) {
        Snapshot dot = new Snapshot(false);
        dot.setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                R.drawable.ic_picsphere_marker));
        dot.mModelMatrix = matrixFromEuler(rx, 0, ry, 0, 0, 100);
        Matrix.scaleM(dot.mModelMatrix, 0, 0.1f, 0.1f, 0.1f);
        dot.setAutoAlphaAngle(rx, ry);
        mDots.add(dot);
    }

    private float[] matrixFromEuler(float rx, float ry, float rz, float tx, float ty, float tz) {
        Quaternion quat = new Quaternion();
        quat.fromEuler(rx,ry,rz);
        float[] matrix = quat.getMatrix();

        Matrix.translateM(matrix, 0, tx, ty, tz);

        return matrix;
    }


    @Override
    public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
        // Set the background clear color to gray.
        GLES20.glClearColor(0.5f, 0.5f, 0.5f, 0.5f);

        // Initialize plane vertex data and texcoords data
        ByteBuffer bb_data = ByteBuffer.allocateDirect(mVertexData.length * 4);
        bb_data.order(ByteOrder.nativeOrder());
        ByteBuffer bb_43data = ByteBuffer.allocateDirect(m43VertexData.length * 4);
        bb_43data.order(ByteOrder.nativeOrder());
        ByteBuffer bb_texture = ByteBuffer.allocateDirect(mTexCoordData.length * 4);
        bb_texture.order(ByteOrder.nativeOrder());

        mVertexBuffer = bb_data.asFloatBuffer();
        mVertexBuffer.put(mVertexData);
        mVertexBuffer.position(0);
        m43VertexBuffer = bb_43data.asFloatBuffer();
        m43VertexBuffer.put(m43VertexData);
        m43VertexBuffer.position(0);
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
                        + "uniform float f_Alpha;\n"
                        + "void main()                    \n"
                        + "{                              \n"
                        + "   gl_FragColor = texture2D(u_Texture, v_TexCoordinate);\n"
                        + "   gl_FragColor.a = gl_FragColor.a * f_Alpha;"
                        + "}                              \n";

        // As the camera preview is stored in the OES external slot, we need a different shader
        final String camPreviewShader = "#extension GL_OES_EGL_image_external : require\n"
                + "precision mediump float;       \n"
                + "uniform samplerExternalOES u_Texture;   \n"
                + "varying vec2 v_TexCoordinate;  \n"
                + "uniform float f_Alpha;\n"
                + "void main()                    \n"
                + "{                              \n"
                + "   gl_FragColor = texture2D(u_Texture, v_TexCoordinate);\n"
                + "   gl_FragColor.a = gl_FragColor.a * f_Alpha;"
                + "}                              \n";


        mVertexShader[CAMERA] = compileShader(GLES20.GL_VERTEX_SHADER, vertexShader);
        mFragmentShader[CAMERA] = compileShader(GLES20.GL_FRAGMENT_SHADER, camPreviewShader);

        mVertexShader[SNAPSHOT] = compileShader(GLES20.GL_VERTEX_SHADER, vertexShader);
        mFragmentShader[SNAPSHOT] = compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader);

        // create the program and bind the shader attributes
        for (int i = 0; i < 2; i++) {
            mProgram[i] = GLES20.glCreateProgram();
            GLES20.glAttachShader(mProgram[i], mFragmentShader[i]);
            GLES20.glAttachShader(mProgram[i], mVertexShader[i]);
            GLES20.glLinkProgram(mProgram[i]);

            int[] linkStatus = new int[1];
            GLES20.glGetProgramiv(mProgram[i], GLES20.GL_LINK_STATUS, linkStatus, 0);

            if (linkStatus[0] == 0) {
                throw new RuntimeException("Error linking shaders");
            }
            mPositionHandler[i]     = GLES20.glGetAttribLocation(mProgram[i], "a_Position");
            mTexCoordHandler[i]     = GLES20.glGetAttribLocation(mProgram[i], "a_TexCoordinate");
            mMVPMatrixHandler[i]    = GLES20.glGetUniformLocation(mProgram[i], "u_MVPMatrix");
            mTextureHandler[i]      = GLES20.glGetUniformLocation(mProgram[i], "u_Texture");
            mAlphaHandler[i]      = GLES20.glGetUniformLocation(mProgram[i], "f_Alpha");
        }

        mSkyBox = new Skybox();

        initCameraBillboard();
    }

    private void initCameraBillboard() {
        int texture[] = new int[1];

        GLES20.glGenTextures(1, texture, 0);
        mCameraTextureId = texture[0];

        if (mCameraTextureId == 0) {
            throw new RuntimeException("CAMERA TEXTURE ID == 0");
        }

        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mCameraTextureId);
        // Can't do mipmapping with camera source
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
        // Clamp to edge is the only option
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T,
                GLES20.GL_CLAMP_TO_EDGE);

        mCameraSurfaceTex = new SurfaceTexture(mCameraTextureId);
        mCameraSurfaceTex.setDefaultBufferSize(640, 480);

        mCameraBillboard = new Snapshot();
        mCameraBillboard.setTextureId(mCameraTextureId);
        mCameraBillboard.setMode(CAMERA);
        mCamManager.setRenderToTexture(mCameraSurfaceTex);

        // Setup viewfinder billboard
        mViewfinderBillboard = new Snapshot(false);
        mViewfinderBillboard.setTexture(BitmapFactory.decodeResource(mContext.getResources(),
                R.drawable.ic_picsphere_viewfinder));
    }

    @Override
    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        // Set the OpenGL viewport to the same size as the surface.
        GLES20.glViewport(0, 0, width, height);

        // We use here a field of view of 40, which is mostly fine for a camera app representation
        final float hfov = 90f;

        // Create a new perspective projection matrix. The height will stay the same
        // while the width will vary as per aspect ratio.
        final float ratio = 640.0f / 480.0f;
        final float near = 0.1f;
        final float far = 1500.0f;
        final float left = (float) Math.tan(hfov * Math.PI / 360.0f) * near;
        final float right = -left;
        final float bottom = ratio * right / 1.0f;
        final float top = ratio * left / 1.0f;

        Matrix.frustumM(mProjectionMatrix, 0, left, right, bottom, top, near, far);
    }

    @Override
    public void onDrawFrame(GL10 glUnused) {
        mCameraSurfaceTex.updateTexImage();

        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        GLES20.glEnable(GLES20.GL_BLEND);

        // Update camera view matrix
        float[] orientation = mSensorFusion.getFusedOrientation();

        // Convert angles to degrees
        float rX = (float) (orientation[1] * 180.0f/Math.PI);
        float rY = (float) (orientation[0] * 180.0f/Math.PI);
        float rZ = (float) (orientation[2] * 180.0f/Math.PI);

        // Update quaternion from euler angles out of orientation
        mCameraQuat.fromEuler( rX, 180.0f-rZ, rY);
        mCameraQuat = mCameraQuat.getConjugate();
        mCameraQuat.normalise();
        mViewMatrix = mCameraQuat.getMatrix();

        // Update camera billboard
        mCameraBillboard.mModelMatrix = mCameraQuat.getMatrix();

        Matrix.invertM(mCameraBillboard.mModelMatrix, 0, mCameraBillboard.mModelMatrix, 0);
        Matrix.translateM(mCameraBillboard.mModelMatrix, 0, 0.0f, 0.0f, -DISTANCE);
        Matrix.rotateM(mCameraBillboard.mModelMatrix, 0, -90, 0, 0, 1);

        mViewfinderBillboard.mModelMatrix = Arrays.copyOf(mCameraBillboard.mModelMatrix,
                mCameraBillboard.mModelMatrix.length);
        Matrix.scaleM(mViewfinderBillboard.mModelMatrix, 0, 0.25f, 0.25f, 0.25f);

        // Draw all teh things
        // First the skybox, then the marker dots, then the snapshots
        mSkyBox.draw();

        mCameraBillboard.draw();

        mListBusy.lock();
        for (Snapshot snap : mSnapshots) {
            snap.draw();
        }
        mListBusy.unlock();

        for (Snapshot dot : mDots) {
            // Set alpha based on camera distance to the point
            float dX = dot.getAutoAlphaX() - (rX + 180.0f);
            float dY = dot.getAutoAlphaY() - rY;
            dX = (dX + 180.0f) % 360.0f - 180.0f;
            dot.setAlpha(1.0f - Math.abs(dX)/180.0f * 8.0f);

            dot.draw();
        }

        mViewfinderBillboard.draw();
    }

    public void setCamPreviewVisible(boolean visible) {
        mCameraBillboard.setVisible(visible);
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

    public Vector3 getAngleAsVector() {
        float[] orientation = mSensorFusion.getFusedOrientation();

        // Convert angles to degrees
        float rX = (float) (orientation[0] * 180.0f/Math.PI);
        float rY = (float) (orientation[1] * 180.0f/Math.PI);
        float rZ = (float) (orientation[2] * 180.0f/Math.PI);

        return new Vector3(rX, rY, rZ);
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
        Matrix.translateM(snap.mModelMatrix, 0, 0.0f, 0.0f, -DISTANCE);
        Matrix.rotateM(snap.mModelMatrix, 0, -90, 0, 0, 1);

        snap.setTexture(image);

        mListBusy.lock();
        mSnapshots.add(snap);
        mListBusy.unlock();
    }

    /**
     * Removes the last taken snapshot
     */
    public void removeLastPicture() {
        mListBusy.lock();
        if (mSnapshots.size() > 0) {
            mSnapshots.remove(mSnapshots.size()-1);
        }
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
