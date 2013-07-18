LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/public/include \
	$(LOCAL_PATH)/XMPFilesPlugins/api/source

LOCAL_CFLAGS := -O3 -DNDEBUG -fstrict-aliasing -fexceptions -frtti \
	-DUNIX_ENV -DHAVE_MEMMOVE -DkBigEndianHost=0 -DEnableDynamicMediaHandlers=0 \
	-DEnableMiscHandlers=0 -DEnablePluginManager=0 -Wno-address

LOCAL_SRC_FILES := \
	XMPFiles/source/HandlerRegistry.cpp \
	XMPFiles/source/WXMPFiles.cpp \
	XMPFiles/source/XMPFiles.cpp \
	XMPFiles/source/XMPFiles_Impl.cpp \
	XMPFiles/source/FileHandlers/JPEG_Handler.cpp \
	XMPFiles/source/FileHandlers/PSD_Handler.cpp \
	XMPFiles/source/FileHandlers/Scanner_Handler.cpp \
	XMPFiles/source/FileHandlers/TIFF_Handler.cpp \
	XMPFiles/source/FileHandlers/Trivial_Handler.cpp \
	XMPFiles/source/FormatSupport/ID3_Support.cpp \
	XMPFiles/source/FormatSupport/PSIR_FileWriter.cpp \
	XMPFiles/source/FormatSupport/PSIR_MemoryReader.cpp \
	XMPFiles/source/FormatSupport/Reconcile_Impl.cpp \
	XMPFiles/source/FormatSupport/ReconcileIPTC.cpp \
	XMPFiles/source/FormatSupport/ReconcileLegacy.cpp \
	XMPFiles/source/FormatSupport/ReconcileTIFF.cpp \
	XMPFiles/source/FormatSupport/XMPScanner.cpp \
	XMPFiles/source/FormatSupport/TIFF_FileWriter.cpp \
	XMPFiles/source/FormatSupport/TIFF_MemoryReader.cpp \
	XMPFiles/source/FormatSupport/TIFF_Support.cpp \
	XMPFiles/source/FormatSupport/IPTC_Support.cpp \
	XMPFiles/source/FormatSupport/IFF/Chunk.cpp \
	XMPFiles/source/FormatSupport/IFF/ChunkController.cpp \
	XMPFiles/source/FormatSupport/IFF/ChunkPath.cpp \
	XMPFiles/source/FormatSupport/IFF/IChunkBehavior.cpp \
	XMPFiles/source/NativeMetadataSupport/IMetadata.cpp \
	XMPFiles/source/NativeMetadataSupport/IReconcile.cpp \
	XMPFiles/source/NativeMetadataSupport/MetadataSet.cpp \
	XMPCore/source/ExpatAdapter.cpp \
	XMPCore/source/ParseRDF.cpp \
	XMPCore/source/WXMPIterator.cpp \
	XMPCore/source/WXMPMeta.cpp \
	XMPCore/source/WXMPUtils.cpp \
	XMPCore/source/XMPCore_Impl.cpp \
	XMPCore/source/XMPIterator.cpp \
	XMPCore/source/XMPMeta.cpp \
	XMPCore/source/XMPMeta-GetSet.cpp \
	XMPCore/source/XMPMeta-Parse.cpp \
	XMPCore/source/XMPMeta-Serialize.cpp \
	XMPCore/source/XMPUtils.cpp \
	XMPCore/source/XMPUtils-FileInfo.cpp \
	source/Host_IO-POSIX.cpp \
	source/IOUtils.cpp \
	source/PerfUtils.cpp \
	source/UnicodeConversions.cpp \
	source/XIO.cpp \
	source/XML_Node.cpp \
	source/XMPFiles_IO.cpp \
	source/XMP_LibUtils.cpp \
	source/XMP_ProgressTracker.cpp \
	third-party/zuid/interfaces/MD5.cpp \
	third-party/expat/lib/xmlparse.c \
	third-party/expat/lib/xmlrole.c \
	third-party/expat/lib/xmltok.c \
	third-party/expat/lib/xmltok_impl.c \
	third-party/expat/lib/xmltok_ns.c

LOCAL_LDFLAGS := -lz

LOCAL_SHARED_LIBRARIES := libz

LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libxmptoolkit

-include external/Nemesis/gnustl.mk
include $(BUILD_SHARED_LIBRARY)
