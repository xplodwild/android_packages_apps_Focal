LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Focal

# Change this to LOCAL_JNI_SHARED_LIBRARIES to include
# the binaries in the apk
LOCAL_REQUIRED_MODULES := \
	libjni_mosaic2 \
	libxmphelper_jni \
	libexiv2 \
	libglib-2.0 \
	libgmodule-2.0 \
	libgobject-2.0 \
	libgthread-2.0 \
	libjpeg \
	libpano13 \
	libtiffdecoder \
	libvigraimpex \
	libhugin \
	libxml2 \
	libiconv \
	libpng_static \
	autooptimiser \
	autopano \
	celeste \
	nona \
	ptclean \
	enblend \
	enfuse \
	libxmptoolkit

LOCAL_STATIC_JAVA_LIBRARIES := \
	metadata-extractor \
	xmpcore \
        android-support-v4

include $(BUILD_PACKAGE)
include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := metadata-extractor:libs/metadata-extractor-2.6.4.jar xmpcore:libs/xmpcore.jar
include $(BUILD_MULTI_PREBUILT)

include $(call all-makefiles-under, $(ANDROID_BUILD_TOP)/external/Focal)
