LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Nemesis

LOCAL_REQUIRED_MODULES := \
	libjni_mosaic2 \
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
	enfuse

include $(BUILD_PACKAGE)

include $(call all-makefiles-under, $(ANDROID_BUILD_TOP)/external/Nemesis)
include $(call all-makefiles-under, jni)
