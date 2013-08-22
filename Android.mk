# Copyright (C) 2013 The CyanogenMod Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Focal

# Change this to LOCAL_JNI_SHARED_LIBRARIES to include
# the binaries in the apk
LOCAL_REQUIRED_MODULES := \
	libjni_mosaic2 \
	libxmphelper_jni \
	libpopen_helper_jni \
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
