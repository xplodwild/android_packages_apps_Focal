LOCAL_PATH:= $(call my-dir)

###
# Build libmosaic2_jni
###
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/feature_stab/db_vlvm \
	$(LOCAL_PATH)/feature_stab/src \
	$(LOCAL_PATH)/feature_stab/src/dbreg \
	$(LOCAL_PATH)/feature_mos/src \
	$(LOCAL_PATH)/feature_mos/src/mosaic

LOCAL_CFLAGS := -O3 -DNDEBUG -fstrict-aliasing

LOCAL_SRC_FILES := \
	feature_mos_jni.cpp \
	mosaic_renderer_jni.cpp \
	feature_mos/src/mosaic/trsMatrix.cpp \
	feature_mos/src/mosaic/AlignFeatures.cpp \
	feature_mos/src/mosaic/Blend.cpp \
	feature_mos/src/mosaic/Delaunay.cpp \
	feature_mos/src/mosaic/ImageUtils.cpp \
	feature_mos/src/mosaic/Mosaic.cpp \
	feature_mos/src/mosaic/Pyramid.cpp \
	feature_mos/src/mosaic_renderer/Renderer.cpp \
	feature_mos/src/mosaic_renderer/WarpRenderer.cpp \
	feature_mos/src/mosaic_renderer/SurfaceTextureRenderer.cpp \
	feature_mos/src/mosaic_renderer/YVURenderer.cpp \
	feature_mos/src/mosaic_renderer/FrameBuffer.cpp \
	feature_stab/db_vlvm/db_feature_detection.cpp \
	feature_stab/db_vlvm/db_feature_matching.cpp \
	feature_stab/db_vlvm/db_framestitching.cpp \
	feature_stab/db_vlvm/db_image_homography.cpp \
	feature_stab/db_vlvm/db_rob_image_homography.cpp \
	feature_stab/db_vlvm/db_utilities.cpp \
	feature_stab/db_vlvm/db_utilities_camera.cpp \
	feature_stab/db_vlvm/db_utilities_indexing.cpp \
	feature_stab/db_vlvm/db_utilities_linalg.cpp \
	feature_stab/db_vlvm/db_utilities_poly.cpp \
	feature_stab/src/dbreg/dbreg.cpp \
	feature_stab/src/dbreg/dbstabsmooth.cpp \
	feature_stab/src/dbreg/vp_motionmodel.c

LOCAL_LDFLAGS := -llog -lGLESv2

LOCAL_SHARED_LIBRARIES := liblog libGLESv2

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libjni_mosaic2

include $(BUILD_SHARED_LIBRARY)

###
# Build libxmphelper_jni
###

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/XMPToolkit \
	$(LOCAL_PATH)/XMPToolkit/public/include \
	$(LOCAL_PATH)/feature_mos/src

LOCAL_CFLAGS := -O3 -DNDEBUG -fstrict-aliasing -fexceptions -DUNIX_ENV -Wno-return-type

LOCAL_SRC_FILES := \
	xmp_helper_jni.cpp

LOCAL_LDFLAGS := -llog

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libxmptoolkit

LOCAL_MODULE := libxmphelper_jni

-include external/Nemesis/gnustl.mk
include $(BUILD_SHARED_LIBRARY)

###
# Build Adobe XMP toolkit
###
include $(LOCAL_PATH)/XMPToolkit/Android.mk
