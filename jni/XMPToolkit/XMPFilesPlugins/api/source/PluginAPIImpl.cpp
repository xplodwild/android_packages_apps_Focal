// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "HostAPIAccess.h"
#include "PluginBase.h"
#include "PluginHandler.h"
#include "PluginRegistry.h"

namespace XMP_PLUGIN
{

#define XMP_MIN(a, b) (((a) < (b)) ? (a) : (b))

///////////////////////////////////////////////////////////////////////////////
//
//		Exception handler
//

static void HandleException( WXMP_Error* wError ) 
{
	try
	{
		throw;
	}
	catch(  XMP_Error& xmpErr )
	{
		wError->mErrorMsg = xmpErr.GetErrMsg();
		wError->mErrorID = xmpErr.GetID();
	}
	catch( std::exception& stdErr )
	{
		wError->mErrorMsg = stdErr.what();
	}
	catch( ... )
	{
		wError->mErrorMsg = "Caught unknown exception";
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//		Plugin API
//

static XMPErrorID Static_TerminatePlugin( WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginTerminate;

	try 
	{
		PluginRegistry::terminate();
		wError->mErrorID = kXMPErr_NoError;
	}
	catch( ... )
	{
		HandleException( wError );
	}
	
	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_SetHostAPI( HostAPIRef hostAPI, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_SetHostAPI;

	if( SetHostAPI( hostAPI ) )
	{
		wError->mErrorID = kXMPErr_NoError;
	}
	
	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_InitializeSession( XMP_StringPtr uid, XMP_StringPtr filePath, XMP_Uns32 format, XMP_Uns32 handlerFlags, XMP_Uns32 openFlags, SessionRef * session, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginSessionInit;

	try
	{
		*session = PluginRegistry::create(uid, filePath, openFlags, format, handlerFlags );

		if( *session != NULL )
		{
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}
	
	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_TerminateSession( SessionRef session, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginSessionTerm;
	PluginBase* thiz = (PluginBase*) session;
	
	try 
	{
		delete thiz;
		wError->mErrorID = kXMPErr_NoError;
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_CheckFileFormat( XMP_StringPtr uid, XMP_StringPtr filePath, XMP_IORef fileRef, XMP_Bool * value, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginCheckFileFormat;

	try 
	{
		IOAdapter file( fileRef );
		*value = PluginRegistry::checkFileFormat( uid, filePath, file );
		wError->mErrorID = kXMPErr_NoError;
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_CheckFolderFormat( XMP_StringPtr uid, XMP_StringPtr rootPath, XMP_StringPtr gpName, XMP_StringPtr parentName, XMP_StringPtr leafName, XMP_Bool * value, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginCheckFolderFormat;
	
	try
	{
		*value = PluginRegistry::checkFolderFormat( uid, rootPath, gpName, parentName, leafName );
		wError->mErrorID = kXMPErr_NoError;
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_GetFileModDate ( SessionRef session, XMP_Bool * ok, XMP_DateTime * modDate, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginGetFileModDate;
	PluginBase* thiz = (PluginBase*) session;
	if ( (thiz == 0) || (ok == 0) || (modDate == 0) ) return kXMPErr_PluginGetFileModDate;

	try 	
	{
		*ok = thiz->getFileModDate ( modDate );
		wError->mErrorID = kXMPErr_NoError;
	} 
	catch ( ... ) 
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID Static_CacheFileData( SessionRef session, XMP_IORef fileRef, XMP_StringPtr* xmpStr, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginCacheFileData;
	
	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->cacheFileData( fileRef, xmpStr );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_UpdateFile( SessionRef session, XMP_IORef fileRef, XMP_Bool doSafeUpdate, XMP_StringPtr xmpStr, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginUpdateFile;
	
	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->updateFile( fileRef, ConvertXMP_BoolToBool( doSafeUpdate ), xmpStr );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_WriteTempFile( SessionRef session, XMP_IORef srcFileRef, XMP_IORef fileRef, XMP_StringPtr xmpStr, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginWriteTempFile;
	
	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->writeTempFile( srcFileRef, fileRef, xmpStr );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_ImportToXMP( SessionRef session, XMPMetaRef xmp, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;


	wError->mErrorID = kXMPErr_NoError;

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_ExportFromXMP( SessionRef session, XMPMetaRef xmp, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;


	wError->mErrorID = kXMPErr_NoError;

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_FillMetadataFiles( SessionRef session, StringVectorRef metadataFiles,
	SetStringVectorProc SetStringVector, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginFillMetadataFiles;

	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->FillMetadataFiles(metadataFiles, SetStringVector);
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}


static XMPErrorID Static_FillAssociatedResources( SessionRef session, StringVectorRef resourceList,
	SetStringVectorProc SetStringVector, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginFillAssociatedResources;

	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->FillAssociatedResources(resourceList, SetStringVector);
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID Static_ImportToXMPString( SessionRef session, XMP_StringPtr* xmpStr, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginImportToXMP;
	
	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->importToXMP( xmpStr );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

// ============================================================================

static XMPErrorID Static_ExportFromXMPString( SessionRef session, XMP_StringPtr xmpStr, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginExportFromXMP;

	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			thiz->exportFromXMP( xmpStr );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}


static XMPErrorID Static_IsMetadataWritable( SessionRef session, XMP_Bool * result, WXMP_Error * wError )
{
	if( wError == NULL || result == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginIsMetadataWritable;

	PluginBase* thiz = (PluginBase*) session;
	try
	{
		if(thiz)
		{
			*result = false;
			*result = ConvertBoolToXMP_Bool( thiz->IsMetadataWritable( ) );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}
// ============================================================================

XMPErrorID InitializePlugin( XMP_StringPtr moduleID, PluginAPIRef pluginAPI, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_PluginInitialized;
	
	if( !pluginAPI || moduleID == NULL )
	{
		wError->mErrorID = kXMPErr_BadParam;
		wError->mErrorMsg = "pluginAPI or moduleID is NULL";
		return wError->mErrorID;
	}

	try
	{
		// Test if module identifier is same as found in the resource file MODULE_IDENTIFIER.txt
		bool identical = (0 == memcmp(GetModuleIdentifier(), moduleID, XMP_MIN(strlen(GetModuleIdentifier()), strlen(moduleID))));
		if ( !identical )
		{
			wError->mErrorMsg = "Module identifier doesn't match";
			return wError->mErrorID;
		}
	
		RegisterFileHandlers(); // Register all file handlers

		bool initialized = PluginRegistry::initialize(); // Initialize all the registered file handlers

		if( initialized )
		{
			XMP_Uns32 size = sizeof(*pluginAPI);
			
			// The pluginAPI struct from the (older) host might be smaller than expected
			// so make sure that we don't write over the end of the struct.
			pluginAPI->mVersion = XMP_PLUGIN_VERSION;
			
			pluginAPI->mTerminatePluginProc = Static_TerminatePlugin;
			pluginAPI->mSetHostAPIProc      = Static_SetHostAPI;

			pluginAPI->mInitializeSessionProc = Static_InitializeSession;
			pluginAPI->mTerminateSessionProc = Static_TerminateSession;

			pluginAPI->mCheckFileFormatProc = Static_CheckFileFormat;
			pluginAPI->mCheckFolderFormatProc = Static_CheckFolderFormat;
			pluginAPI->mGetFileModDateProc = Static_GetFileModDate;
			pluginAPI->mCacheFileDataProc = Static_CacheFileData;
			pluginAPI->mUpdateFileProc = Static_UpdateFile;
			pluginAPI->mWriteTempFileProc = Static_WriteTempFile;

			pluginAPI->mImportToXMPProc = Static_ImportToXMP;
			pluginAPI->mExportFromXMPProc = Static_ExportFromXMP;
			
			// version 2
			if( pluginAPI->mSize > offsetof( PluginAPI, mFillMetadataFilesProc ) )
			{
				pluginAPI->mFillMetadataFilesProc = Static_FillMetadataFiles;
				pluginAPI->mImportToXMPStringProc = Static_ImportToXMPString;
				pluginAPI->mExportFromXMPStringProc = Static_ExportFromXMPString;
				pluginAPI->mFillAssociatedResourcesProc = Static_FillAssociatedResources;
			}
			
			// version 3
			if( pluginAPI->mSize > offsetof( PluginAPI, mIsMetadataWritableProc ) )
			{
				pluginAPI->mIsMetadataWritableProc = Static_IsMetadataWritable;
			}
			
			// Compatibility hack for CS6 (plugin version 1):
			// set mVersion to 1 if pluginAPI is for version 1
			// because in CS6 plugin version is used to determine the hostAPI version.
			if (pluginAPI->mSize <= offsetof( PluginAPI, mFillMetadataFilesProc ))
			{
				pluginAPI->mVersion = 1;
			}

			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}
	
	return wError->mErrorID;
}
	
// ============================================================================

XMPErrorID InitializePlugin2( XMP_StringPtr moduleID, HostAPIRef hostAPI, PluginAPIRef pluginAPI, WXMP_Error * wError )
{
	if( wError == NULL )
	{
		return kXMPErr_BadParam;	
	}
	
	if( hostAPI == NULL )
	{
		wError->mErrorID = kXMPErr_BadParam;
		wError->mErrorMsg = "hostAPI is NULL";
		return wError->mErrorID;
	}
	
	wError->mErrorID = kXMPErr_PluginInitialized;
	
	try
	{
		if( SetHostAPI(hostAPI) )
		{
			if( SetupPlugin() )
			{
				if( InitializePlugin( moduleID, pluginAPI, wError ) )
				{
					wError->mErrorID = kXMPErr_NoError;
				}
			}
			else
			{
				wError->mErrorMsg = "SetupPlugin failed";
			}
		}
		else
		{
			wError->mErrorMsg = "SetHostAPI failed";			
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

} //namespace XMP_PLUGIN
