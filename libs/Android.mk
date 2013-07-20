LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
	metadata-extractor:metadata-extractor-2.6.4.jar \
	xmpcore:xmpcore.jar

include $(BUILD_MULTI_PREBUILT)
