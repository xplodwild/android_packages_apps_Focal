// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H
#include "PluginHandler.h"
#include "ModuleUtils.h"

#if XMP_WinBuild && _MSC_VER >= 1700
	// Visual Studio 2012 or newer supports C++11 (mostly)
	#include <memory>
	#include <functional>
	#define XMP_SHARED_PTR std::shared_ptr
#else
	#define XMP_SHARED_PTR std::tr1::shared_ptr
#endif

namespace XMP_PLUGIN
{

typedef XMP_Uns32 XMPAtom;
typedef XMPAtom FileHandlerType;

typedef XMP_SHARED_PTR<class Module>						ModuleSharedPtr;
typedef XMP_SHARED_PTR<class FileHandler>					FileHandlerSharedPtr;

class FileHandlerInstance;
typedef FileHandlerInstance*								FileHandlerInstancePtr;

typedef std::vector<std::string>							StringVec;
typedef std::vector<ModuleSharedPtr>						ModuleVec;
typedef std::vector<XMP_FileFormat>							XMP_FileFormatVec;

struct FileHandlerPair;

inline void CheckError( WXMP_Error & error )
{
	if( error.mErrorID != kXMPErr_NoError )
	{
		if( error.mErrorID >= kXMPErr_PluginInternal &&
			error.mErrorID <= kXMPErr_PluginLastError )
		{
			throw XMP_Error( kXMPErr_InternalFailure, error.mErrorMsg );
		}
		else
		{
			throw XMP_Error( error.mErrorID, error.mErrorMsg );
		}
	}
}

/** @class PluginManager
 *  @brief Register all file handlers from all the plugins available in Plugin directory.
 *
 *  At the initialization time of XMPFiles, PluginManager loads all the available plugins
 */
class PluginManager
{
public:
	enum HandlerPriority
	{
		kStandardHandler,
		kReplacementHandler,
		kUnknown
	};
	
	/**
	 *  Initialize the plugin manager. It's a singleton class which manages the plugins.
	 *  @param pluginDir The directory where to search for the plugins. 
	 *  @param plugins Comma separated list of the plugins which should be loaded from the plugin directory.
	 *  By default, all plug-ins available in the pluginDir will be loaded.
	 */
	static void initialize( const std::string& pluginDir, const std::string& plugins = std::string() );
	
	/**
	 *  Terminate the plugin manager.
	 */
	static void terminate();

	/** 
	 *  Add file handler corresponding to the given format
	 *  @param format FileFormat supported by the file handler
	 *  @param handler shared pointer to the file handler which is to be added.
	 *  @return Void.
	 */
	static void addFileHandler( XMP_FileFormat format, FileHandlerSharedPtr handler );
	
	/** 
	 *  Returns file handler corresponding to the given format and priority
	 *
	 *  @param format	FileFormat supported by the file handler
	 *  @param priority	The priority of the handler (there could be one standard and 
	 *					one replacing handler, use the enums kStandardHandler or kReplacementHandler)
	 *  @return shared pointer to the file handler. It does not need to be freed.
	 */
	static FileHandlerSharedPtr getFileHandler( XMP_FileFormat format, HandlerPriority priority = kStandardHandler );

	/**
	 * Store mapping between session reference (comes from plugin) and
	 * FileHandlerInstance.
	 * @param session	Session reference from plugin
	 * @param handler	FileHandlerInstance related to the session
	 */
	static void addHandlerInstance( SessionRef session, FileHandlerInstancePtr handler );

	/**
	 * Remove mapping between session reference (comes from plugin) and
	 * FilehandlerInstance
	 * @param session	Session reference from plugin
	 */
	static void removeHandlerInstance( SessionRef session );

	/**
	 * Return FileHandlerInstance that is associated to the session reference
	 * @param session	Session reference from plugin
	 * @return FileHandlerInstance
	 */
	static FileHandlerInstancePtr getHandlerInstance( SessionRef session );

	/**
	 * Return the priority of the handler
	 * @param handler	Instance of file handler
	 * @return			Return kStandardHandler or kReplacementHandler
	 */
	static HandlerPriority getHandlerPriority( FileHandlerInstancePtr handler );

	/**
	 * Return Host API
	 */
	static HostAPIRef getHostAPI( XMP_Uns32 version );

private:
	PluginManager( const std::string& pluginDir, const std::string& plugins );
	~PluginManager();

	/**
	 * Terminate host API
	 */
	void initializeHostAPI();
	void terminateHostAPI();

	/** 
	 *  Load resource file of the given module.
	 *  @param module
	 *  @return Void.
	 */
	void loadResourceFile( ModuleSharedPtr module );
	
	/** 
	 *  Scan mPluginDir for the plugins. It also scans nested folder upto level inMaxNumOfNestedFolder.
	 *  @param inMaxNumOfNestedFolder Nested level where to scan.
	 *  @return Void.
	 */
	void doScan( const XMP_Int32 inMaxNumOfNestedFolder = 1 );
	
	/** 
	 *  Scan recursively the directory /a inPath and insert the found plug-in in ioFoundLibs.
	 *  @param inPath Full path of the directory name to be scanned.
	 *  @param ioFoundLibs Vector of string. Found plug-in will be inserted in this vector.
	 *  @param inLevel The current level
	 *  @param inMaxNumOfNestedFolder Nested level where to scan upto.
	 *  @return Void.
	 */
	void scanRecursive(
			const std::string& inPath, 
			std::vector<std::string>& ioFoundLibs, 
			XMP_Int32 inLevel, 
			XMP_Int32 inMaxNestingLevel );

	/**
	 * Setup passed in HostAPI structure for the host API v1
	 */
	static void SetupHostAPI_V1( HostAPIRef hostAPI );

	/**
	 * Setup passed in HostAPI structure for the host API v2
	 */
	static void SetupHostAPI_V2( HostAPIRef hostAPI );

	/**
	 * Setup passed in HostAPI structure for the host API v3
	 */
	static void SetupHostAPI_V3( HostAPIRef hostAPI );

		/**
	 * Setup passed in HostAPI structure for the host API v4
	 */
	static void SetupHostAPI_V4( HostAPIRef hostAPI );


	typedef std::map<XMP_FileFormat, FileHandlerPair>		PluginHandlerMap;
	typedef std::map<XMP_Uns32, HostAPIRef>					HostAPIMap;
	typedef std::map<SessionRef, FileHandlerInstancePtr>	SessionMap;

	std::string						mPluginDir;
	StringVec						mExtensions;
	StringVec						mPluginsNeeded;
	PluginHandlerMap				mHandlers;
	SessionMap						mSessions;
	HostAPIMap						mHostAPIs;

	static PluginManager*			msPluginManager;
};

} //namespace XMP_PLUGIN
#endif //PLUGINMANAGER_H
