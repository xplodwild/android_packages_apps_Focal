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

/*
*
 */
#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <db_utilities_camera.h>

#include "mosaic/AlignFeatures.h"
#include "mosaic/Blend.h"
#include "mosaic/Mosaic.h"
#include "mosaic/Log.h"
#define LOG_TAG "FEATURE_MOS_JNI"

#ifdef __cplusplus
extern "C" {
#endif

#include "mosaic_renderer_jni.h"

char buffer[1024];

const int MAX_FRAMES = 100;

static double mTx;

int tWidth[NR];
int tHeight[NR];

ImageType tImage[NR][MAX_FRAMES];// = {{ImageUtils::IMAGE_TYPE_NOIMAGE}}; // YVU24 format image
Mosaic *mosaic[NR] = {NULL,NULL};
ImageType resultYVU = ImageUtils::IMAGE_TYPE_NOIMAGE;
ImageType resultBGR = ImageUtils::IMAGE_TYPE_NOIMAGE;
float gTRS[11]; // 9 elements of the transformation, 1 for frame-number, 1 for alignment error code.
// Variables to keep track of the mosaic computation progress for both LR & HR.
float gProgress[NR];
// Variables to be able to cancel the mosaic computation when the GUI says so.
bool gCancelComputation[NR];

int c;
int width=0, height=0;
int mosaicWidth=0, mosaicHeight=0;

//int blendingType = Blend::BLEND_TYPE_FULL;
//int blendingType = Blend::BLEND_TYPE_CYLPAN;
int blendingType = Blend::BLEND_TYPE_HORZ;
int stripType = Blend::STRIP_TYPE_THIN;
bool high_res = false;
bool quarter_res[NR] = {false,false};
float thresh_still[NR] = {5.0f,0.0f};

/* return current time in milliseconds*/

#ifndef now_ms
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


static int frame_number_HR = 0;
static int frame_number_LR = 0;

int Init(int mID, int nmax)
{
        double  t0, t1, time_c;

        if(mosaic[mID]!=NULL)
        {
                delete mosaic[mID];
                mosaic[mID] = NULL;
        }

        mosaic[mID] = new Mosaic();

        t0 = now_ms();

        // When processing higher than 720x480 video, process low-res at
        // quarter resolution
        if(tWidth[LR]>180)
            quarter_res[LR] = true;


        // Check for initialization and if not, initialize
        if (!mosaic[mID]->isInitialized())
        {
                mosaic[mID]->initialize(blendingType, stripType, tWidth[mID], tHeight[mID],
                        nmax, quarter_res[mID], thresh_still[mID]);
        }

        t1 = now_ms();
        time_c = t1 - t0;
        LOGV("Init[%d]: %g ms [%d frames]",mID,time_c,nmax);
        return 1;
}

void GenerateQuarterResImagePlanar(ImageType im, int input_w, int input_h,
        ImageType &out)
{
    ImageType imp;
    ImageType outp;

    int count = 0;

    for (int j = 0; j < input_h; j += H2L_FACTOR)
    {
        imp = im + j * input_w;
        outp = out + (j / H2L_FACTOR) * (input_w / H2L_FACTOR);

        for (int i = 0; i < input_w; i += H2L_FACTOR)
        {
            *outp++ = *(imp + i);
            count++;
        }
    }

    for (int j = input_h; j < 2 * input_h; j += H2L_FACTOR)
    {
        imp = im + j * input_w;
        outp = out + (j / H2L_FACTOR) * (input_w / H2L_FACTOR);

        for (int i = 0; i < input_w; i += H2L_FACTOR)
        {
            *outp++ = *(imp + i);
            count++;
        }
    }

    for (int j = 2 * input_h; j < 3 * input_h; j += H2L_FACTOR)
    {
        imp = im + j * input_w;
        outp = out + (j / H2L_FACTOR) * (input_w / H2L_FACTOR);

        for (int i = 0; i < input_w; i += H2L_FACTOR)
        {
            *outp++ = *(imp + i);
            count++;
        }
    }
}

int AddFrame(int mID, int k, float* trs1d)
{
    double  t0, t1, time_c;
    double trs[3][3];

    int ret_code = mosaic[mID]->addFrame(tImage[mID][k]);

    mosaic[mID]->getAligner()->getLastTRS(trs);

    if(trs1d!=NULL)
    {

        trs1d[0] = trs[0][0];
        trs1d[1] = trs[0][1];
        trs1d[2] = trs[0][2];
        trs1d[3] = trs[1][0];
        trs1d[4] = trs[1][1];
        trs1d[5] = trs[1][2];
        trs1d[6] = trs[2][0];
        trs1d[7] = trs[2][1];
        trs1d[8] = trs[2][2];
    }

    return ret_code;
}

int Finalize(int mID)
{
    double  t0, t1, time_c;

    t0 = now_ms();
    // Create the mosaic
    int ret = mosaic[mID]->createMosaic(gProgress[mID], gCancelComputation[mID]);
    t1 = now_ms();
    time_c = t1 - t0;
    LOGV("CreateMosaic: %g ms",time_c);

    // Get back the result
    resultYVU = mosaic[mID]->getMosaic(mosaicWidth, mosaicHeight);

    return ret;
}

void YUV420toYVU24(ImageType yvu24, ImageType yuv420sp, int width, int height)
{
    int frameSize = width * height;

    ImageType oyp = yvu24;
    ImageType ovp = yvu24+frameSize;
    ImageType oup = yvu24+frameSize+frameSize;

    for (int j = 0, yp = 0; j < height; j++)
    {
        unsigned char u = 0, v = 0;
        int uvp = frameSize + (j >> 1) * width;
        for (int i = 0; i < width; i++, yp++)
        {
            *oyp++ = yuv420sp[yp];
            //int y = (0xff & (int)yuv420sp[yp]) -16;
            //yvu24p[yp] = (y<0)?0:y;

            if ((i & 1) == 0)
            {
                v = yuv420sp[uvp++];
                u = yuv420sp[uvp++];
            }

            *ovp++ = v;
            *oup++ = u;
        }
    }
}

void YUV420toYVU24_NEW(ImageType yvu24, ImageType yuv420sp, int width,
        int height)
{
    int frameSize = width * height;

    ImageType oyp = yvu24;
    ImageType ovp = yvu24 + frameSize;
    ImageType oup = yvu24 + frameSize + frameSize;

    memcpy(yvu24, yuv420sp, frameSize * sizeof(unsigned char));

    for (int j = 0; j < height; j += 2)
    {
        unsigned char u = 0, v = 0;
        int uvp = frameSize + (j >> 1) * width;
        ovp = yvu24 + frameSize + j * width;
        oup = ovp + frameSize;

        ImageType iuvp = yuv420sp + uvp;

        for (int i = 0; i < width; i += 2)
        {
            v = *iuvp++;
            u = *iuvp++;

            *ovp++ = v;
            *oup++ = u;

            *ovp++ = v;
            *oup++ = u;

        }
        memcpy(ovp, ovp - width, width * sizeof(unsigned char));
        memcpy(oup, oup - width, width * sizeof(unsigned char));
    }
}


JNIEXPORT void JNICALL Java_com_android_camera_Mosaic_allocateMosaicMemory(
        JNIEnv* env, jobject thiz, jint width, jint height)
{
    tWidth[HR] = width;
    tHeight[HR] = height;
    tWidth[LR] = int(width / H2L_FACTOR);
    tHeight[LR] = int(height / H2L_FACTOR);

    for(int i=0; i<MAX_FRAMES; i++)
    {
            tImage[LR][i] = ImageUtils::allocateImage(tWidth[LR], tHeight[LR],
                    ImageUtils::IMAGE_TYPE_NUM_CHANNELS);
            tImage[HR][i] = ImageUtils::allocateImage(tWidth[HR], tHeight[HR],
                    ImageUtils::IMAGE_TYPE_NUM_CHANNELS);
    }

    AllocateTextureMemory(tWidth[HR], tHeight[HR], tWidth[LR], tHeight[LR]);
}

JNIEXPORT void JNICALL Java_com_android_camera_Mosaic_freeMosaicMemory(
        JNIEnv* env, jobject thiz)
{
    for(int i = 0; i < MAX_FRAMES; i++)
    {
        ImageUtils::freeImage(tImage[LR][i]);
        ImageUtils::freeImage(tImage[HR][i]);
    }

    FreeTextureMemory();
}


void decodeYUV444SP(unsigned char* rgb, unsigned char* yuv420sp, int width,
        int height)
{
    int frameSize = width * height;

    for (int j = 0, yp = 0; j < height; j++)
    {
        int vp = frameSize + j * width, u = 0, v = 0;
        int up = vp + frameSize;

        for (int i = 0; i < width; i++, yp++, vp++, up++)
        {
            int y = (0xff & ((int) yuv420sp[yp])) - 16;
            if (y < 0) y = 0;

            v = (0xff & yuv420sp[vp]) - 128;
            u = (0xff & yuv420sp[up]) - 128;

            int y1192 = 1192 * y;
            int r = (y1192 + 1634 * v);
            int g = (y1192 - 833 * v - 400 * u);
            int b = (y1192 + 2066 * u);

            if (r < 0) r = 0; else if (r > 262143) r = 262143;
            if (g < 0) g = 0; else if (g > 262143) g = 262143;
            if (b < 0) b = 0; else if (b > 262143) b = 262143;

            //rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
            int p = j*width*3+i*3;
            rgb[p+0] = (r<<6 & 0xFF0000)>>16;
            rgb[p+1] = (g>>2 & 0xFF00)>>8;
            rgb[p+2] =  b>>10 & 0xFF;
        }
    }
}

static int count = 0;

void ConvertYVUAiToPlanarYVU(unsigned char *planar, unsigned char *in, int width,
        int height)
{
    int planeSize = width * height;
    unsigned char* Yptr = planar;
    unsigned char* Vptr = planar + planeSize;
    unsigned char* Uptr = Vptr + planeSize;

    for (int i = 0; i < planeSize; i++)
    {
        *Yptr++ = *in++;
        *Vptr++ = *in++;
        *Uptr++ = *in++;
        in++;   // Alpha
    }
}

JNIEXPORT jfloatArray JNICALL Java_com_android_camera_Mosaic_setSourceImageFromGPU(
        JNIEnv* env, jobject thiz)
{
    double  t0, t1, time_c;
    t0 = now_ms();
    int ret_code = Mosaic::MOSAIC_RET_OK;

    if(frame_number_HR<MAX_FRAMES && frame_number_LR<MAX_FRAMES)
    {
        double last_tx = mTx;

        sem_wait(&gPreviewImage_semaphore);
        ConvertYVUAiToPlanarYVU(tImage[LR][frame_number_LR], gPreviewImage[LR],
                tWidth[LR], tHeight[LR]);

        sem_post(&gPreviewImage_semaphore);

        ret_code = AddFrame(LR, frame_number_LR, gTRS);

        if(ret_code == Mosaic::MOSAIC_RET_OK || ret_code == Mosaic::MOSAIC_RET_FEW_INLIERS)
        {
            // Copy into HR buffer only if this is a valid frame
            sem_wait(&gPreviewImage_semaphore);
            ConvertYVUAiToPlanarYVU(tImage[HR][frame_number_HR], gPreviewImage[HR],
                    tWidth[HR], tHeight[HR]);
            sem_post(&gPreviewImage_semaphore);

            frame_number_LR++;
            frame_number_HR++;
        }
    }
    else
    {
        gTRS[1] = gTRS[2] = gTRS[3] = gTRS[5] = gTRS[6] = gTRS[7] = 0.0f;
        gTRS[0] = gTRS[4] = gTRS[8] = 1.0f;
    }

    UpdateWarpTransformation(gTRS);

    gTRS[9] = frame_number_HR;
    gTRS[10] = ret_code;

    jfloatArray bytes = env->NewFloatArray(11);
    if(bytes != 0)
    {
        env->SetFloatArrayRegion(bytes, 0, 11, (jfloat*) gTRS);
    }
    return bytes;
}



JNIEXPORT jfloatArray JNICALL Java_com_android_camera_Mosaic_setSourceImage(
        JNIEnv* env, jobject thiz, jbyteArray photo_data)
{
    double  t0, t1, time_c;
    t0 = now_ms();

    int ret_code = Mosaic::MOSAIC_RET_OK;

    if(frame_number_HR<MAX_FRAMES && frame_number_LR<MAX_FRAMES)
    {
        jbyte *pixels = env->GetByteArrayElements(photo_data, 0);

        YUV420toYVU24_NEW(tImage[HR][frame_number_HR], (ImageType)pixels,
                tWidth[HR], tHeight[HR]);

        env->ReleaseByteArrayElements(photo_data, pixels, 0);

        double last_tx = mTx;

        t0 = now_ms();
        GenerateQuarterResImagePlanar(tImage[HR][frame_number_HR], tWidth[HR],
                tHeight[HR], tImage[LR][frame_number_LR]);


        sem_wait(&gPreviewImage_semaphore);
        decodeYUV444SP(gPreviewImage[LR], tImage[LR][frame_number_LR],
                gPreviewImageWidth[LR], gPreviewImageHeight[LR]);
        sem_post(&gPreviewImage_semaphore);

        ret_code = AddFrame(LR, frame_number_LR, gTRS);

        if(ret_code == Mosaic::MOSAIC_RET_OK || ret_code == Mosaic::MOSAIC_RET_FEW_INLIERS)
        {
            frame_number_LR++;
            frame_number_HR++;
        }

    }
    else
    {
        gTRS[1] = gTRS[2] = gTRS[3] = gTRS[5] = gTRS[6] = gTRS[7] = 0.0f;
        gTRS[0] = gTRS[4] = gTRS[8] = 1.0f;
    }

    UpdateWarpTransformation(gTRS);

    gTRS[9] = frame_number_HR;
    gTRS[10] = ret_code;

    jfloatArray bytes = env->NewFloatArray(11);
    if(bytes != 0)
    {
        env->SetFloatArrayRegion(bytes, 0, 11, (jfloat*) gTRS);
    }
    return bytes;
}

JNIEXPORT void JNICALL Java_com_android_camera_Mosaic_setBlendingType(
        JNIEnv* env, jobject thiz, jint type)
{
    blendingType = int(type);
}

JNIEXPORT void JNICALL Java_com_android_camera_Mosaic_setStripType(
        JNIEnv* env, jobject thiz, jint type)
{
    stripType = int(type);
}

JNIEXPORT void JNICALL Java_com_android_camera_Mosaic_reset(
        JNIEnv* env, jobject thiz)
{
    frame_number_HR = 0;
    frame_number_LR = 0;

    gProgress[LR] = 0.0;
    gProgress[HR] = 0.0;

    gCancelComputation[LR] = false;
    gCancelComputation[HR] = false;

    Init(LR,MAX_FRAMES);
}

JNIEXPORT jint JNICALL Java_com_android_camera_Mosaic_reportProgress(
        JNIEnv* env, jobject thiz, jboolean hires, jboolean cancel_computation)
{
    if(bool(hires))
        gCancelComputation[HR] = cancel_computation;
    else
        gCancelComputation[LR] = cancel_computation;

    if(bool(hires))
        return (jint) gProgress[HR];
    else
        return (jint) gProgress[LR];
}

JNIEXPORT jint JNICALL Java_com_android_camera_Mosaic_createMosaic(
        JNIEnv* env, jobject thiz, jboolean value)
{
    high_res = bool(value);

    int ret;

    if(high_res)
    {
        LOGV("createMosaic() - High-Res Mode");
        double  t0, t1, time_c;

        gProgress[HR] = 0.0;
        t0 = now_ms();

        Init(HR, frame_number_HR);

        for(int k = 0; k < frame_number_HR; k++)
        {
            if (gCancelComputation[HR])
                break;
            AddFrame(HR, k, NULL);
            gProgress[HR] += TIME_PERCENT_ALIGN/frame_number_HR;
        }

        if (gCancelComputation[HR])
        {
            ret = Mosaic::MOSAIC_RET_CANCELLED;
        }
        else
        {
            gProgress[HR] = TIME_PERCENT_ALIGN;

            t1 = now_ms();
            time_c = t1 - t0;
            LOGV("AlignAll - %d frames [HR]: %g ms", frame_number_HR, time_c);

            ret = Finalize(HR);

            gProgress[HR] = 100.0;
        }

        high_res = false;
    }
    else
    {
        LOGV("createMosaic() - Low-Res Mode");
        gProgress[LR] = TIME_PERCENT_ALIGN;

        ret = Finalize(LR);

        gProgress[LR] = 100.0;
    }

    return (jint) ret;
}

JNIEXPORT jintArray JNICALL Java_com_android_camera_Mosaic_getFinalMosaic(
        JNIEnv* env, jobject thiz)
{
    int y,x;
    int width = mosaicWidth;
    int height = mosaicHeight;
    int imageSize = width * height;

    // Convert back to RGB24
    resultBGR = ImageUtils::allocateImage(mosaicWidth, mosaicHeight,
            ImageUtils::IMAGE_TYPE_NUM_CHANNELS);
    ImageUtils::yvu2bgr(resultBGR, resultYVU, mosaicWidth, mosaicHeight);

    LOGV("MosBytes: %d, W = %d, H = %d", imageSize, width, height);

    int* image = new int[imageSize];
    int* dims = new int[2];

    for(y=0; y<height; y++)
    {
        for(x=0; x<width; x++)
        {
            image[y*width+x] = (0xFF<<24) | (resultBGR[y*width*3+x*3+2]<<16)|
                    (resultBGR[y*width*3+x*3+1]<<8)| (resultBGR[y*width*3+x*3]);
        }
    }

    dims[0] = width;
    dims[1] = height;

    ImageUtils::freeImage(resultBGR);

    jintArray bytes = env->NewIntArray(imageSize+2);
    if (bytes == 0) {
        LOGE("Error in creating the image.");
        delete[] image;
        return 0;
    }
    env->SetIntArrayRegion(bytes, 0, imageSize, (jint*) image);
    env->SetIntArrayRegion(bytes, imageSize, 2, (jint*) dims);
    delete[] image;
    delete[] dims;
    return bytes;
}

JNIEXPORT jbyteArray JNICALL Java_com_android_camera_Mosaic_getFinalMosaicNV21(
        JNIEnv* env, jobject thiz)
{
    int y,x;
    int width;
    int height;

    width = mosaicWidth;
    height = mosaicHeight;

    int imageSize = 1.5*width * height;

    // Convert YVU to NV21 format in-place
    ImageType V = resultYVU+mosaicWidth*mosaicHeight;
    ImageType U = V+mosaicWidth*mosaicHeight;
    for(int j=0; j<mosaicHeight/2; j++)
    {
        for(int i=0; i<mosaicWidth; i+=2)
        {
            V[j*mosaicWidth+i] = V[(2*j)*mosaicWidth+i];        // V
            V[j*mosaicWidth+i+1] = U[(2*j)*mosaicWidth+i];        // U
        }
    }

    LOGV("MosBytes: %d, W = %d, H = %d", imageSize, width, height);

    unsigned char* dims = new unsigned char[8];

    dims[0] = (unsigned char)(width >> 24);
    dims[1] = (unsigned char)(width >> 16);
    dims[2] = (unsigned char)(width >> 8);
    dims[3] = (unsigned char)width;

    dims[4] = (unsigned char)(height >> 24);
    dims[5] = (unsigned char)(height >> 16);
    dims[6] = (unsigned char)(height >> 8);
    dims[7] = (unsigned char)height;

    jbyteArray bytes = env->NewByteArray(imageSize+8);
    if (bytes == 0) {
        LOGE("Error in creating the image.");
        ImageUtils::freeImage(resultYVU);
        return 0;
    }
    env->SetByteArrayRegion(bytes, 0, imageSize, (jbyte*) resultYVU);
    env->SetByteArrayRegion(bytes, imageSize, 8, (jbyte*) dims);
    delete[] dims;
    ImageUtils::freeImage(resultYVU);
    return bytes;
}

#ifdef __cplusplus
}
#endif
