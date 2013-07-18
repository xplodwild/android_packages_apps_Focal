// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "Module.h"
#include "HostAPI.h"

namespace XMP_PLUGIN
{

static bool CheckAPICompatibility_V1 ( const PluginAPIRef pluginAPIs )
{
	// these plugin APIs are mandatory to run an XMP file handler
	if ( pluginAPIs->mTerminatePluginProc
		&& pluginAPIs->mSetHostAPIProc
		&& pluginAPIs->mInitializeSessionProc
		&& pluginAPIs->mTerminateSessionProc
		&& pluginAPIs->mCheckFileFormatProc
		&& pluginAPIs->mCheckFolderFormatProc
		&& pluginAPIs->mGetFileModDateProc
		&& pluginAPIs->mCacheFileDataProc
		&& pluginAPIs->mUpdateFileProc
		&& pluginAPIs->mWriteTempFileProc )
	{
		return true;
	}
	return false;
}

static bool CheckAPICompatibility_V2 ( const PluginAPIRef pluginAPIs )
{
	if ( CheckAPICompatibility_V1 ( pluginAPIs ) )
	{
		if ( pluginAPIs->mFillMetadataFilesProc 
			&& pluginAPIs->mFillAssociatedResourcesProc )
		{
			return true;
		}
	}
	return false;
}

static bool CheckAPICompatibility_V3 ( const PluginAPIRef pluginAPIs )
{
	if ( CheckAPICompatibility_V2 ( pluginAPIs ) )
	{
		if ( pluginAPIs->mIsMetadataWritableProc )
		{
			return true;
		}
	}
	return false;
}

static bool CheckAPICompatibility ( const PluginAPIRef pluginAPIs )
{
	// Note: This is the place where we can reject old plugins.
	//       For example if we consider all functionality of
	//       plugin API version 2 mandatory we can reject
	//       plugin version 1 by returning false in case 1.
	switch ( pluginAPIs->mVersion )
	{
	case 1:
		return CheckAPICompatibility_V1 ( pluginAPIs );
		break;

	case 2:
		return CheckAPICompatibility_V2 ( pluginAPIs );
		break;

	case 3:
		return CheckAPICompatibility_V3 ( pluginAPIs );
		break;

	default:
		// The loaded plugin is newer than the host.
		// Only basic functionality to run the plugin is required.
		return CheckAPICompatibility_V1 ( pluginAPIs );
		break;
	}
}

PluginAPIRef Module::getPluginAPIs() 
{ 
	//
	// return ref. to Plugin API, load module if not yet loaded
	//
	if ( !mPluginAPIs || mLoaded != kModuleLoaded )
	{
		if ( !load() )
		{
			XMP_Throw ( "Plugin API not available.", kXMPErr_Unavailable );
		}
	}
	return mPluginAPIs;
}

bool Module::load()
{
	XMP_AutoLock lock ( &mLoadingLock, kXMP_WriteLock );
	return loadInternal();
}

void Module::unload()
{
	XMP_AutoLock lock (&mLoadingLock, kXMP_WriteLock);
	unloadInternal();
}

void Module::unloadInternal()
{
	WXMP_Error error;

	//
	// terminate plugin
	//
	if( mPluginAPIs != NULL )
	{
		if( mPluginAPIs->mTerminatePluginProc ) 
		{
			mPluginAPIs->mTerminatePluginProc( &error );
		}
		delete mPluginAPIs;
		mPluginAPIs = NULL;
	}

	//
	// unload plugin module
	//
	if( mLoaded != kModuleNotLoaded )
	{
		UnloadModule(mHandle, false);
		mHandle = NULL;
		if( mLoaded == kModuleLoaded )
		{
			//
			// Reset mLoaded to kModuleNotLoaded, if the module was loaded successfully.
			// Otherwise let it remain kModuleErrorOnLoad so that we won't try to load 
			// it again if some other handler ask to do so.
			//
			mLoaded = kModuleNotLoaded;
		}
	}

	CheckError( error );
}

bool Module::loadInternal()
{
	if( mLoaded == kModuleNotLoaded )
	{
		const char * errorMsg = NULL;

		//
		// load module
		//
		mLoaded = kModuleErrorOnLoad;
		mHandle = LoadModule(mPath, false);

		if( mHandle != NULL )
		{
			//
			// get entry point function pointer
			//
			InitializePluginProc InitializePlugin = reinterpret_cast<InitializePluginProc>( 
				GetFunctionPointerFromModuleImpl(mHandle, "InitializePlugin") );	// legacy entry point

			InitializePlugin2Proc InitializePlugin2 = reinterpret_cast<InitializePlugin2Proc>( 
				GetFunctionPointerFromModuleImpl(mHandle, "InitializePlugin2") );

			if( InitializePlugin2 != NULL || InitializePlugin != NULL )
			{
				std::string moduleID;
				GetResourceDataFromModule(mHandle, "MODULE_IDENTIFIER", "txt", moduleID);
				mPluginAPIs = new PluginAPI();
				memset( mPluginAPIs, 0, sizeof(PluginAPI) );
				mPluginAPIs->mSize = sizeof(PluginAPI);
				mPluginAPIs->mVersion = XMP_PLUGIN_VERSION;	// informational: the latest version that the host knows about

				WXMP_Error error;

				//
				// initialize plugin by calling entry point function
				//
				if( InitializePlugin2 != NULL )
				{
					HostAPIRef hostAPI = PluginManager::getHostAPI( XMP_HOST_API_VERSION );
					InitializePlugin2( moduleID.c_str(), hostAPI, mPluginAPIs, &error );

					if ( error.mErrorID == kXMPErr_NoError )
					{
						// check all function pointers are correct based on version numbers
						if( CheckAPICompatibility( mPluginAPIs ) )
						{
							mLoaded = kModuleLoaded;
						}
						else
						{
							errorMsg = "Incompatible plugin API version.";
						}
					}
					else
					{
						errorMsg = "Plugin initialization failed.";
					}
				}
				else if( InitializePlugin != NULL )
				{
					// initialize through legacy plugin entry point
					InitializePlugin( moduleID.c_str(), mPluginAPIs, &error );

					if ( error.mErrorID == kXMPErr_NoError ) {
						// check all function pointers are correct based on version numbers
						bool compatibleAPIs = CheckAPICompatibility(mPluginAPIs);

						if ( compatibleAPIs )
						{
							//
							// set host API at plugin
							//
							HostAPIRef hostAPI = PluginManager::getHostAPI( mPluginAPIs->mVersion );
							
							mPluginAPIs->mSetHostAPIProc( hostAPI, &error );

							if( error.mErrorID == kXMPErr_NoError )
							{
								mLoaded = kModuleLoaded;
							}
							else
							{
								errorMsg = "Plugin API incomplete.";
							}
						}
						else
						{
							errorMsg = "Incompatible plugin API version.";
						}
					}
					else
					{
						errorMsg = "Plugin initialization failed.";
					}
				}
				else
				{
					errorMsg = "Missing plugin entry point in plugin";
				}
			}

			if( mLoaded != kModuleLoaded )
			{
				//
				// plugin wasn't loaded and initialized successfully,
				// so unload the module
				//
				this->unloadInternal();
			}
		}
		else
		{
			errorMsg = "Can't load module";
		}

		if ( mLoaded != kModuleLoaded && errorMsg )
		{
			//
			// error occurred
			//
			throw XMP_Error( kXMPErr_InternalFailure, errorMsg);
		}
	}

	return ( mLoaded == kModuleLoaded ); 

}

} //namespace XMP_PLUGIN
