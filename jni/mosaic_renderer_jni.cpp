/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <jni.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "db_utilities_camera.h"
#include "mosaic/ImageUtils.h"
#include "mosaic_renderer/FrameBuffer.h"
#include "mosaic_renderer/WarpRenderer.h"
#include "mosaic_renderer/SurfaceTextureRenderer.h"
#include "mosaic_renderer/YVURenderer.h"

#include "mosaic/Log.h"
#define LOG_TAG "MosaicRenderer"

#include "mosaic_renderer_jni.h"

// Texture handle
GLuint gSurfaceTextureID[1];

bool gWarpImage = true;

// Low-Res input image frame in YUVA format for preview rendering and processing
// and high-res YUVA input image for processing.
unsigned char* gPreviewImage[NR];
// Low-Res & high-res preview image width
int gPreviewImageWidth[NR];
// Low-Res & high-res preview image height
int gPreviewImageHeight[NR];

// Semaphore to protect simultaneous read/writes from gPreviewImage
sem_t gPreviewImage_semaphore;

// Off-screen preview FBO width (large enough to store the entire
// preview mosaic). FBO is frame buffer object.
int gPreviewFBOWidth;
// Off-screen preview FBO height (large enough to store the entire
// preview mosaic).
int gPreviewFBOHeight;

// gK is the transformation to map the canonical {-1,1} vertex coordinate system
// to the {0,gPreviewImageWidth[LR]} input image frame coordinate system before
// applying the given affine transformation trs. gKm is the corresponding
// transformation for going to the {0,gPreviewFBOWidth}.
double gK[9];
double gKinv[9];
double gKm[9];
double gKminv[9];

// Shader to copy input SurfaceTexture into and RGBA FBO. The two shaders
// render to the textures with dimensions corresponding to the low-res and
// high-res image frames.
SurfaceTextureRenderer gSurfTexRenderer[NR];
// Off-screen FBOs to store the low-res and high-res RGBA copied out from
// the SurfaceTexture by the gSurfTexRenderers.
FrameBuffer gBufferInput[NR];

// Shader to convert RGBA textures into YVU textures for processing
YVURenderer gYVURenderer[NR];
// Off-screen FBOs to store the low-res and high-res YVU textures for processing
FrameBuffer gBufferInputYVU[NR];

// Shader to translate the flip-flop FBO - gBuffer[1-current] -> gBuffer[current]
WarpRenderer gWarper1;
// Shader to add warped current frame to the flip-flop FBO - gBuffer[current]
WarpRenderer gWarper2;
// Off-screen FBOs (flip-flop) to store the result of gWarper1 & gWarper2
FrameBuffer gBuffer[2];

// Shader to warp and render the preview FBO to the screen
WarpRenderer gPreview;

// Index of the gBuffer FBO gWarper1 is going to write into
int gCurrentFBOIndex = 0;

// 3x3 Matrices holding the transformation of this frame (gThisH1t) and of
// the last frame (gLastH1t) w.r.t the first frame.
double gThisH1t[9];
double gLastH1t[9];

// Variables to represent the fixed position of the top-left corner of the
// current frame in the previewFBO
double gCenterOffsetX = 0.0f;
double gCenterOffsetY = 0.0f;

// X-Offset of the viewfinder (current frame) w.r.t
// (gCenterOffsetX, gCenterOffsetY). This offset varies with time and is
// used to pan the viewfinder across the UI layout.
double gPanOffset = 0.0f;

// Variables tracking the translation value for the current frame and the
// last frame (both w.r.t the first frame). The difference between these
// values is used to control the panning speed of the viewfinder display
// on the UI screen.
double gThisTx = 0.0f;
double gLastTx = 0.0f;

// These are the scale factors used by the gPreview shader to ensure that
// the image frame is correctly scaled to the full UI layout height while
// maintaining its aspect ratio
double gUILayoutScalingX = 1.0f;
double gUILayoutScalingY = 1.0f;

// Whether the view that we will render preview FBO onto is in landscape or portrait
// orientation.
bool gIsLandscapeOrientation = true;

// State of the viewfinder. Set to false when the viewfinder hits the UI edge.
bool gPanViewfinder = true;

// Affine transformation in GL 4x4 format (column-major) to warp the
// last frame mosaic into the current frame coordinate system.
GLfloat g_dAffinetransGL[16];
double g_dAffinetrans[16];

// Affine transformation in GL 4x4 format (column-major) to translate the
// preview FBO across the screen (viewfinder panning).
GLfloat g_dAffinetransPanGL[16];
double g_dAffinetransPan[16];

// XY translation in GL 4x4 format (column-major) to center the current
// preview mosaic in the preview FBO
GLfloat g_dTranslationToFBOCenterGL[16];
double g_dTranslationToFBOCenter[16];

// GL 4x4 Identity transformation
GLfloat g_dAffinetransIdentGL[] = {
    1., 0., 0., 0.,
    0., 1., 0., 0.,
    0., 0., 1., 0.,
    0., 0., 0., 1.};

// GL 4x4 Rotation transformation (column-majored): 90 degree
GLfloat g_dAffinetransRotation90GL[] = {
    0., 1., 0., 0.,
    -1., 0., 0., 0.,
    0., 0., 1., 0.,
    0., 0., 0., 1.};

// 3x3 Rotation transformation (row-majored): 90 degree
double gRotation90[] = {
    0., -1., 0.,
    1., 0., 0.,
    0., 0., 1.,};


float g_dIdent3x3[] = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0};

const int GL_TEXTURE_EXTERNAL_OES_ENUM = 0x8D65;

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s", name, v);
}

void checkFramebufferStatus(const char* name) {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == 0) {
      LOGE("Checking completeness of Framebuffer:%s", name);
      checkGlError("checkFramebufferStatus (is the target \"GL_FRAMEBUFFER\"?)");
    } else if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char* msg = "not listed";
        switch (status) {
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: msg = "attachment"; break;
          case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: msg = "dimensions"; break;
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: msg = "missing attachment"; break;
          case GL_FRAMEBUFFER_UNSUPPORTED: msg = "unsupported"; break;
        }
        LOGE("Framebuffer: %s is INCOMPLETE: %s, %x", name, msg, status);
    }
}

// @return false if there was an error
bool checkGLErrorDetail(const char* file, int line, const char* op) {
    GLint error = glGetError();
    const char* err_msg = "NOT_LISTED";
    if (error != 0) {
        switch (error) {
            case GL_INVALID_VALUE: err_msg = "NOT_LISTED_YET"; break;
            case GL_INVALID_OPERATION: err_msg = "INVALID_OPERATION"; break;
            case GL_INVALID_ENUM: err_msg = "INVALID_ENUM"; break;
        }
        LOGE("Error after %s(). glError: %s (0x%x) in line %d of %s", op, err_msg, error, line, file);
        return false;
    }
    return true;
}

void bindSurfaceTexture(GLuint texId)
{
    glBindTexture(GL_TEXTURE_EXTERNAL_OES_ENUM, texId);

    // Can't do mipmapping with camera source
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES_ENUM, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES_ENUM, GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);
    // Clamp to edge is the only option
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES_ENUM, GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES_ENUM, GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);
}

void ClearPreviewImage(int mID)
{
    unsigned char* ptr = gPreviewImage[mID];
    for(int j = 0, i = 0;
            j < gPreviewImageWidth[mID] * gPreviewImageHeight[mID] * 4;
            j += 4)
    {
            ptr[i++] = 0;
            ptr[i++] = 0;
            ptr[i++] = 0;
            ptr[i++] = 255;
    }

}

void ConvertAffine3x3toGL4x4(double *matGL44, double *mat33)
{
    matGL44[0] = mat33[0];
    matGL44[1] = mat33[3];
    matGL44[2] = 0.0;
    matGL44[3] = mat33[6];

    matGL44[4] = mat33[1];
    matGL44[5] = mat33[4];
    matGL44[6] = 0.0;
    matGL44[7] = mat33[7];

    matGL44[8] = 0;
    matGL44[9] = 0;
    matGL44[10] = 1.0;
    matGL44[11] = 0.0;

    matGL44[12] = mat33[2];
    matGL44[13] = mat33[5];
    matGL44[14] = 0.0;
    matGL44[15] = mat33[8];
}

bool continuePanningFBO(double panOffset) {
    double normalizedScreenLimitLeft = -1.0 + VIEWPORT_BORDER_FACTOR_HORZ * 2.0;
    double normalizedScreenLimitRight = 1.0 - VIEWPORT_BORDER_FACTOR_HORZ * 2.0;
    double normalizedXPositionOnScreenLeft;
    double normalizedXPositionOnScreenRight;

    // Compute the position of the current frame in the screen coordinate system
    if (gIsLandscapeOrientation) {
        normalizedXPositionOnScreenLeft = (2.0 *
            (gCenterOffsetX + panOffset) / gPreviewFBOWidth - 1.0) *
            gUILayoutScalingX;
        normalizedXPositionOnScreenRight = (2.0 *
            ((gCenterOffsetX + panOffset) + gPreviewImageWidth[HR]) /
            gPreviewFBOWidth - 1.0) * gUILayoutScalingX;
    } else {
        normalizedXPositionOnScreenLeft = (2.0 *
            (gCenterOffsetX + panOffset) / gPreviewFBOWidth - 1.0) *
            gUILayoutScalingY;
        normalizedXPositionOnScreenRight = (2.0 *
            ((gCenterOffsetX + panOffset) + gPreviewImageWidth[HR]) /
            gPreviewFBOWidth - 1.0) * gUILayoutScalingY;
    }

    // Stop the viewfinder panning if we hit the maximum border allowed for
    // this UI layout
    if (normalizedXPositionOnScreenRight > normalizedScreenLimitRight ||
            normalizedXPositionOnScreenLeft < normalizedScreenLimitLeft) {
        return false;
    } else {
        return true;
    }
}

// This function computes fills the 4x4 matrices g_dAffinetrans,
// and g_dAffinetransPan using the specified 3x3 affine
// transformation between the first captured frame and the current frame.
// The computed g_dAffinetrans is such that it warps the preview mosaic in
// the last frame's coordinate system into the coordinate system of the
// current frame. Thus, applying this transformation will create the current
// frame mosaic but with the current frame missing. This frame will then be
// pasted in by gWarper2 after translating it by g_dTranslationToFBOCenter.
// The computed g_dAffinetransPan is such that it offsets the computed preview
// mosaic horizontally to make the viewfinder pan within the UI layout.
void UpdateWarpTransformation(float *trs)
{
    double H[9], Hp[9], Htemp1[9], Htemp2[9], T[9];

    for(int i = 0; i < 9; i++)
    {
        gThisH1t[i] = trs[i];
    }

    // Alignment is done based on low-res data.
    // To render the preview mosaic, the translation of the high-res mosaic is estimated to
    // H2L_FACTOR x low-res-based tranlation.
    gThisH1t[2] *= H2L_FACTOR;
    gThisH1t[5] *= H2L_FACTOR;

    db_Identity3x3(T);
    T[2] = -gCenterOffsetX;
    T[5] = -gCenterOffsetY;

    // H = ( inv(gThisH1t) * gLastH1t ) * T
    db_Identity3x3(Htemp1);
    db_Identity3x3(Htemp2);
    db_Identity3x3(H);
    db_InvertAffineTransform(Htemp1, gThisH1t);
    db_Multiply3x3_3x3(Htemp2, Htemp1, gLastH1t);
    db_Multiply3x3_3x3(H, Htemp2, T);

    for(int i = 0; i < 9; i++)
    {
        gLastH1t[i] = gThisH1t[i];
    }

    // Move the origin such that the frame is centered in the previewFBO
    // i.e. H = inv(T) * H
    H[2] += gCenterOffsetX;
    H[5] += gCenterOffsetY;

    // Hp = inv(Km) * H * Km
    // Km moves the coordinate system from openGL to image pixels so
    // that the alignment transform H can be applied to them.
    // inv(Km) moves the coordinate system back to openGL normalized
    // coordinates so that the shader can correctly render it.
    db_Identity3x3(Htemp1);
    db_Multiply3x3_3x3(Htemp1, H, gKm);
    db_Multiply3x3_3x3(Hp, gKminv, Htemp1);

    ConvertAffine3x3toGL4x4(g_dAffinetrans, Hp);

    ////////////////////////////////////////////////
    ////// Compute g_dAffinetransPan now...   //////
    ////////////////////////////////////////////////

    gThisTx = trs[2];

    if(gPanViewfinder)
    {
        gPanOffset += (gThisTx - gLastTx) * VIEWFINDER_PAN_FACTOR_HORZ;
    }

    gLastTx = gThisTx;
    gPanViewfinder = continuePanningFBO(gPanOffset);

    db_Identity3x3(H);
    H[2] = gPanOffset;

    // Hp = inv(Km) * H * Km
    db_Identity3x3(Htemp1);
    db_Multiply3x3_3x3(Htemp1, H, gKm);
    db_Multiply3x3_3x3(Hp, gKminv, Htemp1);

    if (gIsLandscapeOrientation) {
        ConvertAffine3x3toGL4x4(g_dAffinetransPan, Hp);
    } else {
        // rotate Hp by 90 degress.
        db_Multiply3x3_3x3(Htemp1, gRotation90, Hp);
        ConvertAffine3x3toGL4x4(g_dAffinetransPan, Htemp1);
    }
}

void AllocateTextureMemory(int widthHR, int heightHR, int widthLR, int heightLR)
{
    gPreviewImageWidth[HR] = widthHR;
    gPreviewImageHeight[HR] = heightHR;

    gPreviewImageWidth[LR] = widthLR;
    gPreviewImageHeight[LR] = heightLR;

    sem_wait(&gPreviewImage_semaphore);
    gPreviewImage[LR] = ImageUtils::allocateImage(gPreviewImageWidth[LR],
            gPreviewImageHeight[LR], 4);
    gPreviewImage[HR] = ImageUtils::allocateImage(gPreviewImageWidth[HR],
            gPreviewImageHeight[HR], 4);
    sem_post(&gPreviewImage_semaphore);

    gPreviewFBOWidth = PREVIEW_FBO_WIDTH_SCALE * gPreviewImageWidth[HR];
    gPreviewFBOHeight = PREVIEW_FBO_HEIGHT_SCALE * gPreviewImageHeight[HR];

    // The origin is such that the current frame will sit with its center
    // at the center of the previewFBO
    gCenterOffsetX = (gPreviewFBOWidth / 2 - gPreviewImageWidth[HR] / 2);
    gCenterOffsetY = (gPreviewFBOHeight / 2 - gPreviewImageHeight[HR] / 2);

    gPanOffset = 0.0f;

    db_Identity3x3(gThisH1t);
    db_Identity3x3(gLastH1t);

    gPanViewfinder = true;

    int w = gPreviewImageWidth[HR];
    int h = gPreviewImageHeight[HR];

    int wm = gPreviewFBOWidth;
    int hm = gPreviewFBOHeight;

    // K is the transformation to map the canonical [-1,1] vertex coordinate
    // system to the [0,w] image coordinate system before applying the given
    // affine transformation trs.
    gKm[0] = wm / 2.0 - 0.5;
    gKm[1] = 0.0;
    gKm[2] = wm / 2.0 - 0.5;
    gKm[3] = 0.0;
    gKm[4] = hm / 2.0 - 0.5;
    gKm[5] = hm / 2.0 - 0.5;
    gKm[6] = 0.0;
    gKm[7] = 0.0;
    gKm[8] = 1.0;

    gK[0] = w / 2.0 - 0.5;
    gK[1] = 0.0;
    gK[2] = w / 2.0 - 0.5;
    gK[3] = 0.0;
    gK[4] = h / 2.0 - 0.5;
    gK[5] = h / 2.0 - 0.5;
    gK[6] = 0.0;
    gK[7] = 0.0;
    gK[8] = 1.0;

    db_Identity3x3(gKinv);
    db_InvertCalibrationMatrix(gKinv, gK);

    db_Identity3x3(gKminv);
    db_InvertCalibrationMatrix(gKminv, gKm);

    //////////////////////////////////////////
    ////// Compute g_Translation now... //////
    //////////////////////////////////////////
    double T[9], Tp[9], Ttemp[9];

    db_Identity3x3(T);
    T[2] = gCenterOffsetX;
    T[5] = gCenterOffsetY;

    // Tp = inv(K) * T * K
    db_Identity3x3(Ttemp);
    db_Multiply3x3_3x3(Ttemp, T, gK);
    db_Multiply3x3_3x3(Tp, gKinv, Ttemp);

    ConvertAffine3x3toGL4x4(g_dTranslationToFBOCenter, Tp);

    UpdateWarpTransformation(g_dIdent3x3);
}

void FreeTextureMemory()
{
    sem_wait(&gPreviewImage_semaphore);
    ImageUtils::freeImage(gPreviewImage[LR]);
    ImageUtils::freeImage(gPreviewImage[HR]);
    sem_post(&gPreviewImage_semaphore);
}

extern "C"
{
    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved);
    JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved);
    JNIEXPORT jint JNICALL Java_com_android_camera_MosaicRenderer_init(
            JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_reset(
            JNIEnv * env, jobject obj,  jint width, jint height,
            jboolean isLandscapeOrientation);
    JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_preprocess(
            JNIEnv * env, jobject obj, jfloatArray stMatrix);
    JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_transferGPUtoCPU(
            JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_step(
            JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_updateMatrix(
            JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_setWarping(
            JNIEnv * env, jobject obj, jboolean flag);
};


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    sem_init(&gPreviewImage_semaphore, 0, 1);

    return JNI_VERSION_1_4;
}


JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
    sem_destroy(&gPreviewImage_semaphore);
}
JNIEXPORT jint JNICALL Java_com_android_camera_MosaicRenderer_init(
        JNIEnv * env, jobject obj)
{
    gSurfTexRenderer[LR].InitializeGLProgram();
    gSurfTexRenderer[HR].InitializeGLProgram();
    gYVURenderer[LR].InitializeGLProgram();
    gYVURenderer[HR].InitializeGLProgram();
    gWarper1.InitializeGLProgram();
    gWarper2.InitializeGLProgram();
    gPreview.InitializeGLProgram();
    gBuffer[0].InitializeGLContext();
    gBuffer[1].InitializeGLContext();
    gBufferInput[LR].InitializeGLContext();
    gBufferInput[HR].InitializeGLContext();
    gBufferInputYVU[LR].InitializeGLContext();
    gBufferInputYVU[HR].InitializeGLContext();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, gSurfaceTextureID);
    // bind the surface texture
    bindSurfaceTexture(gSurfaceTextureID[0]);

    return (jint) gSurfaceTextureID[0];
}

// width: the width of the view
// height: the height of the view
// isLandscape: whether the device is in landscape or portrait. Android
//     Compatibility Definition Document specifies that the long side of the
//     camera aligns with the long side of the screen.
void calculateUILayoutScaling(int width, int height, bool isLandscape) {
    if (isLandscape) {
        //  __________        ________
        // |          |  =>  |________|
        // |__________|  =>    (View)
        // (Preview FBO)
        //
        // Scale the preview FBO's height to the height of view and
        // maintain the aspect ratio of the current frame on the screen.
        gUILayoutScalingY = PREVIEW_FBO_HEIGHT_SCALE;

        // Note that OpenGL scales a texture to view's width and height automatically.
        // The "width / height" inverts the scaling, so as to maintain the aspect ratio
        // of the current frame.
        gUILayoutScalingX = ((float) gPreviewFBOWidth / gPreviewFBOHeight)
                / ((float) width / height) * PREVIEW_FBO_HEIGHT_SCALE;
    } else {
        //                   ___
        //  __________      |   |     ______
        // |          |  => |   | => |______|
        // |__________|  => |   | =>  (View)
        // (Preview FBO)    |   |
        //                  |___|
        //
        // Scale the preview FBO's height to the width of view and
        // maintain the aspect ratio of the current frame on the screen.
        // In preview, Java_com_android_camera_MosaicRenderer_step rotates the
        // preview FBO by 90 degrees. In capture, UpdateWarpTransformation
        // rotates the preview FBO.
        gUILayoutScalingY = PREVIEW_FBO_WIDTH_SCALE;

        // Note that OpenGL scales a texture to view's width and height automatically.
        // The "height / width" inverts the scaling, so as to maintain the aspect ratio
        // of the current frame.
        gUILayoutScalingX = ((float) gPreviewFBOHeight / gPreviewFBOWidth)
                / ((float) width / height) * PREVIEW_FBO_WIDTH_SCALE;
    }
}

JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_reset(
        JNIEnv * env, jobject obj,  jint width, jint height, jboolean isLandscapeOrientation)
{
    gIsLandscapeOrientation = isLandscapeOrientation;
    calculateUILayoutScaling(width, height, gIsLandscapeOrientation);

    gBuffer[0].Init(gPreviewFBOWidth, gPreviewFBOHeight, GL_RGBA);
    gBuffer[1].Init(gPreviewFBOWidth, gPreviewFBOHeight, GL_RGBA);

    gBufferInput[LR].Init(gPreviewImageWidth[LR],
            gPreviewImageHeight[LR], GL_RGBA);

    gBufferInput[HR].Init(gPreviewImageWidth[HR],
            gPreviewImageHeight[HR], GL_RGBA);

    gBufferInputYVU[LR].Init(gPreviewImageWidth[LR],
            gPreviewImageHeight[LR], GL_RGBA);

    gBufferInputYVU[HR].Init(gPreviewImageWidth[HR],
            gPreviewImageHeight[HR], GL_RGBA);

    // bind the surface texture
    bindSurfaceTexture(gSurfaceTextureID[0]);

    // To speed up, there is no need to clear the destination buffers
    // (offscreen/screen buffers) of gSurfTexRenderer, gYVURenderer
    // and gPreview because we always fill the whole destination buffers
    // when we draw something to those offscreen/screen buffers.
    gSurfTexRenderer[LR].SetupGraphics(&gBufferInput[LR]);
    gSurfTexRenderer[LR].SetViewportMatrix(1, 1, 1, 1);
    gSurfTexRenderer[LR].SetScalingMatrix(1.0f, -1.0f);
    gSurfTexRenderer[LR].SetInputTextureName(gSurfaceTextureID[0]);
    gSurfTexRenderer[LR].SetInputTextureType(GL_TEXTURE_EXTERNAL_OES_ENUM);

    gSurfTexRenderer[HR].SetupGraphics(&gBufferInput[HR]);
    gSurfTexRenderer[HR].SetViewportMatrix(1, 1, 1, 1);
    gSurfTexRenderer[HR].SetScalingMatrix(1.0f, -1.0f);
    gSurfTexRenderer[HR].SetInputTextureName(gSurfaceTextureID[0]);
    gSurfTexRenderer[HR].SetInputTextureType(GL_TEXTURE_EXTERNAL_OES_ENUM);

    gYVURenderer[LR].SetupGraphics(&gBufferInputYVU[LR]);
    gYVURenderer[LR].SetInputTextureName(gBufferInput[LR].GetTextureName());
    gYVURenderer[LR].SetInputTextureType(GL_TEXTURE_2D);

    gYVURenderer[HR].SetupGraphics(&gBufferInputYVU[HR]);
    gYVURenderer[HR].SetInputTextureName(gBufferInput[HR].GetTextureName());
    gYVURenderer[HR].SetInputTextureType(GL_TEXTURE_2D);

    // gBuffer[1-gCurrentFBOIndex] --> gWarper1 --> gBuffer[gCurrentFBOIndex]
    gWarper1.SetupGraphics(&gBuffer[gCurrentFBOIndex]);

    // Clear the destination buffer of gWarper1.
    gWarper1.Clear(0.0, 0.0, 0.0, 1.0);
    gWarper1.SetViewportMatrix(1, 1, 1, 1);
    gWarper1.SetScalingMatrix(1.0f, 1.0f);
    gWarper1.SetInputTextureName(gBuffer[1 - gCurrentFBOIndex].GetTextureName());
    gWarper1.SetInputTextureType(GL_TEXTURE_2D);

    // gBufferInput[HR] --> gWarper2 --> gBuffer[gCurrentFBOIndex]
    gWarper2.SetupGraphics(&gBuffer[gCurrentFBOIndex]);

    // gWarp2's destination buffer is the same to gWarp1's. No need to clear it
    // again.
    gWarper2.SetViewportMatrix(gPreviewImageWidth[HR],
            gPreviewImageHeight[HR], gBuffer[gCurrentFBOIndex].GetWidth(),
            gBuffer[gCurrentFBOIndex].GetHeight());
    gWarper2.SetScalingMatrix(1.0f, 1.0f);
    gWarper2.SetInputTextureName(gBufferInput[HR].GetTextureName());
    gWarper2.SetInputTextureType(GL_TEXTURE_2D);

    // gBuffer[gCurrentFBOIndex] --> gPreview --> Screen
    gPreview.SetupGraphics(width, height);
    gPreview.SetViewportMatrix(1, 1, 1, 1);

    // Scale the previewFBO so that the viewfinder window fills the layout height
    // while maintaining the image aspect ratio
    gPreview.SetScalingMatrix(gUILayoutScalingX, -1.0f * gUILayoutScalingY);
    gPreview.SetInputTextureName(gBuffer[gCurrentFBOIndex].GetTextureName());
    gPreview.SetInputTextureType(GL_TEXTURE_2D);
}

JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_preprocess(
        JNIEnv * env, jobject obj, jfloatArray stMatrix)
{
    jfloat *stmat = env->GetFloatArrayElements(stMatrix, 0);

    gSurfTexRenderer[LR].SetSTMatrix((float*) stmat);
    gSurfTexRenderer[HR].SetSTMatrix((float*) stmat);

    env->ReleaseFloatArrayElements(stMatrix, stmat, 0);

    gSurfTexRenderer[LR].DrawTexture(g_dAffinetransIdentGL);
    gSurfTexRenderer[HR].DrawTexture(g_dAffinetransIdentGL);
}

#ifndef now_ms
#include <time.h>
static double
now_ms(void)
{
    //struct timespec res;
    struct timeval res;
    //clock_gettime(CLOCK_REALTIME, &res);
    gettimeofday(&res, NULL);
    return 1000.0*res.tv_sec + (double)res.tv_usec/1e3;
}
#endif



JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_transferGPUtoCPU(
        JNIEnv * env, jobject obj)
{
    double t0, t1, time_c;

    gYVURenderer[LR].DrawTexture();
    gYVURenderer[HR].DrawTexture();

    sem_wait(&gPreviewImage_semaphore);
    // Bind to the input LR FBO and read the Low-Res data from there...
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferInputYVU[LR].GetFrameBufferName());
    t0 = now_ms();
    glReadPixels(0,
                 0,
                 gBufferInput[LR].GetWidth(),
                 gBufferInput[LR].GetHeight(),
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 gPreviewImage[LR]);

    checkGlError("glReadPixels LR (MosaicRenderer.transferGPUtoCPU())");

    // Bind to the input HR FBO and read the high-res data from there...
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferInputYVU[HR].GetFrameBufferName());
    t0 = now_ms();
    glReadPixels(0,
                 0,
                 gBufferInput[HR].GetWidth(),
                 gBufferInput[HR].GetHeight(),
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 gPreviewImage[HR]);

    checkGlError("glReadPixels HR (MosaicRenderer.transferGPUtoCPU())");

    sem_post(&gPreviewImage_semaphore);
}

JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_step(
        JNIEnv * env, jobject obj)
{
    if(!gWarpImage) // ViewFinder
    {
        gWarper2.SetupGraphics(&gBuffer[gCurrentFBOIndex]);
        gPreview.SetInputTextureName(gBuffer[gCurrentFBOIndex].GetTextureName());

        gWarper2.DrawTexture(g_dTranslationToFBOCenterGL);

        if (gIsLandscapeOrientation) {
            gPreview.DrawTexture(g_dAffinetransIdentGL);
        } else {
            gPreview.DrawTexture(g_dAffinetransRotation90GL);
        }
    }
    else
    {
        gWarper1.SetupGraphics(&gBuffer[gCurrentFBOIndex]);
        // Clear the destination so that we can paint on it afresh
        gWarper1.Clear(0.0, 0.0, 0.0, 1.0);
        gWarper1.SetInputTextureName(
                gBuffer[1 - gCurrentFBOIndex].GetTextureName());
        gWarper2.SetupGraphics(&gBuffer[gCurrentFBOIndex]);
        gPreview.SetInputTextureName(gBuffer[gCurrentFBOIndex].GetTextureName());

        gWarper1.DrawTexture(g_dAffinetransGL);
        gWarper2.DrawTexture(g_dTranslationToFBOCenterGL);
        gPreview.DrawTexture(g_dAffinetransPanGL);

        gCurrentFBOIndex = 1 - gCurrentFBOIndex;
    }
}

JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_setWarping(
        JNIEnv * env, jobject obj, jboolean flag)
{
    // TODO: Review this logic
    if(gWarpImage != (bool) flag) //switching from viewfinder to capture or vice-versa
    {
        // Clear gBuffer[0]
        gWarper1.SetupGraphics(&gBuffer[0]);
        gWarper1.Clear(0.0, 0.0, 0.0, 1.0);
        // Clear gBuffer[1]
        gWarper1.SetupGraphics(&gBuffer[1]);
        gWarper1.Clear(0.0, 0.0, 0.0, 1.0);
        // Clear the screen to black.
        gPreview.Clear(0.0, 0.0, 0.0, 1.0);

        gLastTx = 0.0f;
        gPanOffset = 0.0f;
        gPanViewfinder = true;

        db_Identity3x3(gThisH1t);
        db_Identity3x3(gLastH1t);
        // Make sure g_dAffinetransGL and g_dAffinetransPanGL are updated.
        // Otherwise, the first frame after setting the flag to true will be
        // incorrectly drawn.
        if ((bool) flag) {
            UpdateWarpTransformation(g_dIdent3x3);
        }
    }

    gWarpImage = (bool)flag;
}

JNIEXPORT void JNICALL Java_com_android_camera_MosaicRenderer_updateMatrix(
        JNIEnv * env, jobject obj)
{
    for(int i=0; i<16; i++)
    {
        g_dAffinetransGL[i] = g_dAffinetrans[i];
        g_dAffinetransPanGL[i] = g_dAffinetransPan[i];
        g_dTranslationToFBOCenterGL[i] = g_dTranslationToFBOCenter[i];
    }
}
