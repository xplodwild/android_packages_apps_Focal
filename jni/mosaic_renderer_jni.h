#pragma once
#include <semaphore.h>

// The Preview FBO dimensions are determined from the high-res
// frame dimensions (gPreviewImageWidth, gPreviewImageHeight)
// using the scale factors below.
const int PREVIEW_FBO_WIDTH_SCALE = 2;
const int PREVIEW_FBO_HEIGHT_SCALE = 2;

// The factor below determines the (horizontal) speed at which the viewfinder
// will pan across the UI during capture. A value of 0.0 will keep the viewfinder
// static in the center of the screen and 1.0f will make it pan at the
// same speed as the device.
const float VIEWFINDER_PAN_FACTOR_HORZ = 0.0f;

// What fraction of the screen viewport width has been allocated to show the
// arrows on the direction of motion side.
const float VIEWPORT_BORDER_FACTOR_HORZ = 0.1f;

const int LR = 0; // Low-resolution mode
const int HR = 1; // High-resolution mode
const int NR = 2; // Number of resolution modes

const int H2L_FACTOR = 4; // Can be 2

extern "C" void AllocateTextureMemory(int widthHR, int heightHR,
        int widthLR, int heightLR);
extern "C" void FreeTextureMemory();
extern "C" void UpdateWarpTransformation(float *trs);

extern unsigned char* gPreviewImage[NR];
extern int gPreviewImageWidth[NR];
extern int gPreviewImageHeight[NR];

extern sem_t gPreviewImage_semaphore;
