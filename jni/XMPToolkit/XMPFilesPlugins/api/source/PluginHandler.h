// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

/**************************************************************************
* @file Plugin_Handler.h
* @brief Function prototypes for the XMP Plugin.
*
* This contains the prototype for the plug-in based File Handlers. Plug-in need 
* to implement exported function InitializePlugin.
* 
***************************************************************************/

#ifndef __Plugin_Handler_hpp__
#define __Plugin_Handler_hpp__	1
#include "PluginConst.h"

// versioning
#define XMP_PLUGIN_VERSION_1	1	// CS6
#define XMP_PLUGIN_VERSION_2	2	//
#define XMP_PLUGIN_VERSION_3	3	// CS7

#define XMP_PLUGIN_VERSION		XMP_PLUGIN_VERSION_3

namespace XMP_PLUGIN
{

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
	#define DllExport		__attribute__((visibility("default")))
#else
	#define DllExport		__declspec( dllexport )
#endif

struct PluginAPI;
struct HostAPI;

typedef PluginAPI*		PluginAPIRef;
typedef HostAPI*		HostAPIRef;
typedef void*			SessionRef;
typedef void*			XMP_IORef;
typedef int				XMPErrorID;

/** @struct WXMP_Error
 *
 *  This is used to pass error number and error message by the host(the XMPFiles) and the plugin.
 *  All plugin APIs and host APIs return kXMPErr_NoError on success otherwise return error id.
 *  All such APIs also take 'WXMP_Error wError' as the last argument. This structure will be filled
 *  by the error id and message if any error is encountered. The return value and wError->mErrorID
 *  contains the same number.
 */
struct WXMP_Error
{
   XMPErrorID		mErrorID;
   XMP_StringPtr	mErrorMsg;
   WXMP_Error() : mErrorID( kXMPErr_NoError ), mErrorMsg( NULL ) {}
};


/** 
 *  Function pointer to the function TerminatePlugin which will be called
 *  at the time of unloading of the plugin.
 *
 *  @param wError	WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return			kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*TerminatePluginProc)( WXMP_Error * wError );

/** 
 *  Function pointer to the function SetHostAPIProc which will be called to 
 *  set the hostAPI. These host APIs will be used in the plugin to use the Host(the XMPFiles) functionality.
 *
 *  @param hostAPI		An API that is provided by the host (the XMPFiles) that can be used by a plugin implementaion.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*SetHostAPIProc)( HostAPIRef hostAPI, WXMP_Error * wError );

/** 
 *  Function pointer to the function InitializeSession which will be called
 *  at the time of creating instance of the file handler with /param uid for file /param filePath.
 *
 *  @param uid				Unique identifier string (uid) of the file handler whose instance is to be created.
 *  @param filePath			FilePath of the file which is to be opened.
 *  @param format			File format id for the session
 *	@param handlerFlags		Handler flags as defined in the plugin manifest
 *  @param openFlags		Flags that describe the desired access.
 *  @param session			Pointer to a file Handler instance.
 *  @return					kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*InitializeSessionProc)( XMP_StringPtr uid, XMP_StringPtr filePath, XMP_Uns32 format, XMP_Uns32 handlerFlags, XMP_Uns32 openFlags, SessionRef * session, WXMP_Error * wError );

/** 
 *  Function pointer to the function TerminateSession which will be called
 *  at the time of terminating instance of the file handler.
 *
 *  @param session		File Handler instance.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*TerminateSessionProc)( SessionRef session, WXMP_Error * wError );

/** 
 *  Function pointer to the function CheckFileFormat which will be called to
 *  check whether the input file /param filePath is supported by the file handler with /param uid.
 *
 *  @param uid			Unique identifier string (uid) of the file handler.
 *  @param filePath		FilePath of the file which is to be opened.
 *  @para fileRef		File reference to the input file.
 *  @param result		[out] pointer to a boolean which will be true if /param filePath is supported by the file handler.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*CheckSessionFileFormatProc)( XMP_StringPtr uid, XMP_StringPtr filePath, XMP_IORef fileRef, XMP_Bool * result, WXMP_Error * wError );

/** 
 *  Function pointer to the function CheckFolderFormat which will be called to
 *  check whether the input folder name is supported by the file handler with /param uid.
 *
 *  @param uid Unique identifier string (uid) of the file handler.
 *  @param rootPath
 *  @param gpName
 *  @param parentName
 *  @param leafName
 *  @param result		[out] pointer to a boolen which will be true if it is supported by the file handler.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*CheckSessionFolderFormatProc)( XMP_StringPtr uid, XMP_StringPtr rootPath, XMP_StringPtr gpName, XMP_StringPtr parentName, XMP_StringPtr leafName, XMP_Bool * result, WXMP_Error * wError );

/**
 * Function type for GetFileModDate. Returns the most recent file modification date for any file
 * associated with the path that is read to obtain metadata. A static routine in XMPFiles.
 *
 *  @param session		File Handler instance.
 *	@param ok			A pointer to a XMP_Bool for the eventual public API function result.
 *	@param modDate		A pointer to the date to be returned.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 */
typedef XMPErrorID (*GetSessionFileModDateProc) ( SessionRef session, XMP_Bool * ok, XMP_DateTime * modDate, WXMP_Error * wError );

/** 
 *  Function pointer to the function CacheFileData which will be called to
 *  cache xmpData from the file format of the session.
 *
 *  @param session		File Handler instance.
 *  @param fileRef		File reference of the file format which may be needed to read the data.
 *  @param xmpStr		A pointer to a buffer where xmpData will be stored.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*CacheFileDataProc)( SessionRef session, XMP_IORef fileRef, XMP_StringPtr* xmpStr, WXMP_Error * wError );

/** 
 *  Function pointer to the function UpdateFileData which will be called to
 *  update the file format of the session with the /param xmpPacket.
 *
 *  @param session		File Handler instance.
 *  @param fileRef		File reference of the file format which may be needed to read the data.
 *  @param doSafeUpdate If true, file is saved in safe mode that means it write into a temporary file then swap for crash safety.
 *  @param xmpStr		A pointer to a buffer which contain the xmpData.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*UpdateFileProc)( SessionRef session, XMP_IORef fileRef, XMP_Bool doSafeUpdate, XMP_StringPtr xmpStr, WXMP_Error * wError );

/** 
 *  Function pointer to the function WriteTempFile. It write the entire file format into a
 *  temporary file /param fileRef.
 *
 *  @param session		File Handler instance.
 *  @param orgFileRef	File reference to the original source file.
 *  @param fileRef		File reference to a temporary file where file format will be written into.
 *  @param xmpStr		A pointer to a buffer which contain the xmpData.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*WriteTempFileProc)( SessionRef session, XMP_IORef orgFileRef, XMP_IORef fileRef, XMP_StringPtr xmpStr, WXMP_Error * wError );

/** 
 *  Function pointer to the function ImportToXMP. Any non metadata from a file that is supposed 
 *  to be mapped into a XMP namespace should be added to the XMP packet using this function.
 *
 *  @param session		File Handler instance.
 *  @param xmp			XMPMeta reference which will be filled by the function.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*ImportToXMPProc)( SessionRef session, XMPMetaRef xmp, WXMP_Error * wError );

/** 
 *  Function pointer to the function ExportFromXMP. The XMP packet is supposed to be 
 *  written to the file. If the packet contains any data that should be mapped back to
 *  native (non-XMP) metadata values then that should happen here.
 *
 *  @param session		File Handler instance.
 *  @param xmp			XMPMeta reference where the xmpData will be picked from.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*ExportFromXMPProc)( SessionRef session, XMPMetaRef xmp, WXMP_Error * wError );

/** 
 *  Function pointer to the function FillMetadataFiles. This function returns the list of file paths
 *  associated with storing the metadata information.
 *
 *  @param session					File Handler instance.
 *  @param metadataFiles			pointer of std::vector of std::string
 *  @param SetClientStringVector	pointer to client provided function to fill the vector with strings.
 *  @param wError					WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return							kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*FillMetadataFilesProc)( SessionRef session, StringVectorRef metadataFiles,
	SetStringVectorProc SetClientStringVector, WXMP_Error * wError );

/** 
 *  Function pointer to the function FillAssociatedResources. This function returns the list of all file paths
 *  associated to the opened session 
 *
 *  @param session					File Handler instance.
 *  @param resourceList			    pointer of std::vector of std::string
 *  @param SetClientStringVector	pointer to client provided function to fill the vector with strings.
 *  @param wError					WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return							kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*FillAssociatedResourcesProc)( SessionRef session, StringVectorRef resourceList,
	SetStringVectorProc SetClientStringVector, WXMP_Error * wError );

/** 
 *  Function pointer to the function ImportToXMP. Any non metadata from a file that is supposed 
 *  to be mapped into a XMP namespace should be added to the XMP packet using this function.
 *
 *  @param session		File Handler instance.
 *  @param xmpStr		A pointer to a buffer which contain the xmpData.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*ImportToXMPStringProc)( SessionRef session, XMP_StringPtr* xmpStr , WXMP_Error * wError );

/** 
 *  Function pointer to the function ExportFromXMP. The XMP packet is supposed to be 
 *  written to the file. If the packet contains any data that should be mapped back to
 *  native (non-XMP) metadata values then that should happen here.
 *
 *  @param session		File Handler instance.
 *  @param xmpStr		A pointer to a buffer which contain the xmpData.
 *  @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*ExportFromXMPStringProc)( SessionRef session, XMP_StringPtr xmpStr, WXMP_Error * wError );

/** 
 *  Function pointer to the function IsMetadataWritable. This function returns if the metadata can be updated  
 *  for the opened session 
 *
 *  @param session					File Handler instance.
 *  @param result					[out] pointer to a boolen which will be true if the metadata can be updated.
 *  @return							kXMPErr_NoError on success otherwise error id of the failure.
 */
typedef XMPErrorID (*IsMetadataWritableProc)( SessionRef session, XMP_Bool * result, WXMP_Error * wError );


/** @struct PluginAPI
 *  @brief This is a Plugin API structure.
 *
 *  This will consists of init/term of the plugin, init/term of a file handler session,
 *  read from fie and update file.
 */
struct PluginAPI
{
	/** 
	 *  Size of the PluginAPI structure.
	 */
	XMP_Uns32						mSize;
   
	/** 
	 *  Version number of the plugin.
	 */
	XMP_Uns32						mVersion;
	
	// version 1
	TerminatePluginProc				mTerminatePluginProc;
	SetHostAPIProc					mSetHostAPIProc;
	
	InitializeSessionProc			mInitializeSessionProc;
	TerminateSessionProc			mTerminateSessionProc;
	
	CheckSessionFileFormatProc		mCheckFileFormatProc;
	CheckSessionFolderFormatProc	mCheckFolderFormatProc;
	GetSessionFileModDateProc		mGetFileModDateProc;

	CacheFileDataProc				mCacheFileDataProc;
	UpdateFileProc					mUpdateFileProc;
	WriteTempFileProc				mWriteTempFileProc;
	
	ImportToXMPProc					mImportToXMPProc;			// deprecated in version 2 in favour of mImportToXMPStringProc
	ExportFromXMPProc				mExportFromXMPProc;			// deprecated in version 2 in favour of mExportFromXMPStringProc

	// version 2
	FillMetadataFilesProc			mFillMetadataFilesProc;
	ImportToXMPStringProc			mImportToXMPStringProc;
	ExportFromXMPStringProc			mExportFromXMPStringProc;
	FillAssociatedResourcesProc     mFillAssociatedResourcesProc;

	// version 3
	IsMetadataWritableProc			mIsMetadataWritableProc;
};


/** @brief Plugin Entry point (legacy).
 *
 *  This is the legacy entry point for the plugin. It will fill \param pluginAPI inside the plugin.
 * 
 *  @param moduleID		It is the module identifier string which is present in plugin's resource 
 *						file 'MODULE_IDENTIFIER.txt". It should be same as the one which is present in plugin's library.
 *  @param pluginAPI	structure pointer to PluginAPI which will be filled by the plugin.
 *  @param wError		structure pointer to WXMP_Error which will be filled in case of an error.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
DllExport XMPErrorID InitializePlugin( XMP_StringPtr moduleID, PluginAPIRef pluginAPI, WXMP_Error * wError );
typedef XMPErrorID (*InitializePluginProc)( XMP_StringPtr moduleID, PluginAPIRef pluginAPI, WXMP_Error * wError );

/** @brief Plugin Entry point.
 *
 *  This is the entry point for the plugin. It will fill \param pluginAPI inside the plugin.
 * 
 *  @param moduleID		It is the module identifier string which is present in plugin's resource 
 *						file 'MODULE_IDENTIFIER.txt". It should be same as the one which is present in plugin's library.
 *  @param hostAPI		structure pointer to HostAPI which will be stored by the plugin.
 *  @param pluginAPI	structure pointer to PluginAPI which will be filled by the plugin.
 *  @param wError		structure pointer to WXMP_Error which will be filled in case of an error.
 *  @return				kXMPErr_NoError on success otherwise error id of the failure.
 */
DllExport XMPErrorID InitializePlugin2( XMP_StringPtr moduleID, HostAPIRef hostAPI, PluginAPIRef pluginAPI, WXMP_Error * wError );
typedef XMPErrorID (*InitializePlugin2Proc)( XMP_StringPtr moduleID, HostAPIRef hostAPI, PluginAPIRef pluginAPI, WXMP_Error * wError );

#ifdef __cplusplus
}
#endif

} //namespace XMP_PLUGIN
#endif // __Plugin_Handler_hpp__
