// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "PluginManager.h"
#include "FileHandler.h"
#include <algorithm>
#include "XMPAtoms.h"
#include "XMPFiles/source/HandlerRegistry.h"
#include "FileHandlerInstance.h"
#include "HostAPI.h"

using namespace Common;
using namespace std;

// =================================================================================================

namespace XMP_PLUGIN
{

const char* kResourceName_UIDs  = "XMPPLUGINUIDS";
const char* kLibraryExtensions[] = { "xpi" };

struct FileHandlerPair
{
	FileHandlerSharedPtr	mStandardHandler;
	FileHandlerSharedPtr	mReplacementHandler;
};

// =================================================================================================

static XMP_FileFormat GetXMPFileFormatFromFilePath( XMP_StringPtr filePath )
{
	XMP_StringPtr pathName = filePath + strlen(filePath);
	for ( ; pathName > filePath; --pathName ) {
		if ( *pathName == '.' ) break;
	}
	
	XMP_StringPtr fileExt = pathName + 1;
	return HandlerRegistry::getInstance().getFileFormat ( fileExt );
}

// =================================================================================================

static XMPFileHandler* Plugin_MetaHandlerCTor ( FileHandlerSharedPtr handler, XMPFiles* parent )
{
	SessionRef object;
	WXMP_Error error;

	if( (handler == 0) || (! handler->load()) ) 
	{
		XMP_Throw ( "Plugin not loaded", kXMPErr_InternalFailure );
	}

	handler->getModule()->getPluginAPIs()->mInitializeSessionProc ( handler->getUID().c_str(), parent->GetFilePath().c_str(), (XMP_Uns32)parent->format, (XMP_Uns32)handler->getHandlerFlags(), (XMP_Uns32)parent->openFlags, &object, &error );
	CheckError ( error );

	FileHandlerInstance* instance = new FileHandlerInstance ( object, handler, parent );
	return instance;
}

static XMPFileHandler* Plugin_MetaHandlerCTor_Standard( XMPFiles * parent )
{
	FileHandlerSharedPtr handler = PluginManager::getFileHandler( parent->format, PluginManager::kStandardHandler );

	return Plugin_MetaHandlerCTor( handler, parent );
}

static XMPFileHandler* Plugin_MetaHandlerCTor_Replacement( XMPFiles * parent )
{
	FileHandlerSharedPtr handler = PluginManager::getFileHandler( parent->format, PluginManager::kReplacementHandler );

	return Plugin_MetaHandlerCTor( handler, parent );
}

// =================================================================================================

static bool Plugin_CheckFileFormat ( FileHandlerSharedPtr handler, XMP_StringPtr filePath, XMP_IO * fileRef, XMPFiles * parent )
{
	if ( handler != 0 ) {

		// call into plugin if owning handler or if manifest has no CheckFormat entry
		if ( fileRef == 0 || handler->getCheckFormatSize() == 0) {

			XMP_Bool ok;
			WXMP_Error error;
			CheckSessionFileFormatProc checkProc = handler->getModule()->getPluginAPIs()->mCheckFileFormatProc;
			checkProc ( handler->getUID().c_str(), filePath, fileRef, &ok, &error );
			CheckError ( error );
			return ConvertXMP_BoolToBool( ok );

		} else {

			// all CheckFormat manifest entries must match
			for ( XMP_Uns32 i=0; i < handler->getCheckFormatSize(); i++ ) {

				CheckFormat checkFormat = handler->getCheckFormat ( i );

				if ( checkFormat.empty() ) return false;

				XMP_Uns8 buffer[1024];

				if ( checkFormat.mLength > 1024 ) {
					//Ideally check format string should not be that long. 
					//The check is here to handle only malicious data.
					checkFormat.mLength = 1024;
				}

				fileRef->Seek ( checkFormat.mOffset, kXMP_SeekFromStart );
				XMP_Uns32 len = fileRef->Read ( buffer, checkFormat.mLength );

				if ( len != checkFormat.mLength ) {

					// Not enough byte read from the file.
					return false;

				} else {

					// Check if byteSeq is hexadecimal byte sequence, e.g 0x03045100
					
					bool isHex = ( (checkFormat.mLength > 0 ) &&
								   (checkFormat.mByteSeq.size() == (2 + 2*checkFormat.mLength) &&
								   (checkFormat.mByteSeq[0] == '0') &&
								   (checkFormat.mByteSeq[1] == 'x') )
								   );

					if ( ! isHex ) {

						if ( memcmp ( buffer, checkFormat.mByteSeq.c_str(), checkFormat.mLength ) != 0 ) return false;

					} else {

						for ( XMP_Uns32 current = 0; current < checkFormat.mLength; current++ ) {

							char oneByteBuffer[3];
							oneByteBuffer[0] = checkFormat.mByteSeq [ 2 + 2*current ];
							oneByteBuffer[1] = checkFormat.mByteSeq [ 2 + 2*current + 1 ];
							oneByteBuffer[2] = '\0';

							XMP_Uns8 oneByte = (XMP_Uns8) strtoul ( oneByteBuffer, 0, 16 );
							if ( oneByte != buffer[current] ) return false;

						}

					}

				}

			}
			
			return true;	// The checkFormat string comparison passed.

		}

	}

	return false;	// Should never get here.
}	// Plugin_CheckFileFormat

static bool Plugin_CheckFileFormat_Standard( XMP_FileFormat format, XMP_StringPtr filePath, XMP_IO* fileRef, XMPFiles* parent )
{
	FileHandlerSharedPtr handler = PluginManager::getFileHandler( format, PluginManager::kStandardHandler );

	return Plugin_CheckFileFormat( handler, filePath, fileRef, parent );
}

static bool Plugin_CheckFileFormat_Replacement( XMP_FileFormat format, XMP_StringPtr filePath, XMP_IO* fileRef, XMPFiles* parent )
{
	FileHandlerSharedPtr handler = PluginManager::getFileHandler( format, PluginManager::kReplacementHandler );

	return Plugin_CheckFileFormat( handler, filePath, fileRef, parent );
}

// =================================================================================================

static bool Plugin_CheckFolderFormat( FileHandlerSharedPtr handler,
									  const std::string & rootPath,
									  const std::string & gpName,
									  const std::string & parentName,
									  const std::string & leafName,
									  XMPFiles * parent )
{
	XMP_Bool result = false;

	if ( handler != 0 ) 
	{
		WXMP_Error error;
		CheckSessionFolderFormatProc checkProc = handler->getModule()->getPluginAPIs()->mCheckFolderFormatProc;
		checkProc ( handler->getUID().c_str(), rootPath.c_str(), gpName.c_str(), parentName.c_str(), leafName.c_str(), &result, &error );
		CheckError( error );
	}

	return ConvertXMP_BoolToBool( result );

}

static bool Plugin_CheckFolderFormat_Standard( XMP_FileFormat format,
	const std::string & rootPath,
	const std::string & gpName,
	const std::string & parentName,
	const std::string & leafName,
	XMPFiles * parent )
{
	FileHandlerSharedPtr handler = PluginManager::getFileHandler( format, PluginManager::kStandardHandler );

	return Plugin_CheckFolderFormat( handler, rootPath, gpName, parentName, leafName, parent );
}

static bool Plugin_CheckFolderFormat_Replacement( XMP_FileFormat format,
	const std::string & rootPath,
	const std::string & gpName,
	const std::string & parentName,
	const std::string & leafName,
	XMPFiles * parent )
{
	FileHandlerSharedPtr handler = PluginManager::getFileHandler( format, PluginManager::kReplacementHandler );

	return Plugin_CheckFolderFormat( handler, rootPath, gpName, parentName, leafName, parent );
}

// =================================================================================================

PluginManager* PluginManager::msPluginManager = 0;

PluginManager::PluginManager( const std::string& pluginDir, const std::string& plugins )
: mPluginDir ( pluginDir )
{

	const std::size_t count = sizeof(kLibraryExtensions) / sizeof(kLibraryExtensions[0]);

	for ( std::size_t i = 0; i<count; ++i ) {
		mExtensions.push_back ( std::string ( kLibraryExtensions[i] ) );
	}

	size_t pos = std::string::npos;

	#if XMP_WinBuild
		// convert to Win kDirChar
		while ( (pos = mPluginDir.find ('/')) != string::npos ) {
			mPluginDir.replace (pos, 1, "\\");
		}
	#else
		while ( (pos = mPluginDir.find ('\\')) != string::npos ) {
			mPluginDir.replace (pos, 1, "/");
		}
	#endif

	if ( ! mPluginDir.empty() && Host_IO::Exists( mPluginDir.c_str() ) ) {

		XMP_StringPtr strPtr = plugins.c_str();
		size_t pos = 0;
		size_t length = 0;

		for ( ; ; ++strPtr, ++length ) {

			if ( (*strPtr == ',') || (*strPtr == '\0') ) {

				if ( length != 0 ) {

					//Remove white spaces from front
					while ( plugins[pos] == ' ' ) {
						++pos;
						--length;
					}

					std::string pluginName;
					pluginName.assign ( plugins, pos, length );
					
					//Remove extension from the plugin name
					size_t found = pluginName.find ( '.' );
					if ( found != string::npos ) pluginName.erase ( found );

					//Remove white spaces from the back
					found = pluginName.find ( ' ' );
					if ( found != string::npos ) pluginName.erase ( found );
					
					MakeLowerCase ( &pluginName );
					mPluginsNeeded.push_back ( pluginName );

					//Reset for next plugin
					pos = pos + length + 1;
					length = 0;

				}

				if ( *strPtr == '\0' ) break;

			}

		}

	}

}	// PluginManager::PluginManager

// =================================================================================================

PluginManager::~PluginManager()
{
	mPluginDir.clear();
	mExtensions.clear();
	mPluginsNeeded.clear();
	mHandlers.clear();
	mSessions.clear();

	terminateHostAPI();
}

// =================================================================================================

static bool registerHandler( XMP_FileFormat format, FileHandlerSharedPtr handler )
{
	bool ret = false;

	HandlerRegistry& hdlrReg				= HandlerRegistry::getInstance();
	FileHandlerType type					= handler->getHandlerType();
	CheckFileFormatProc chkFileFormat		= NULL;
	CheckFolderFormatProc chkFolderFormat	= NULL;
	XMPFileHandlerCTor hdlCtor				= NULL;

	if ( handler->getHandlerFlags() & kXMPFiles_NeedsPreloading )
	{
		try
		{
			handler->load();
		}
		catch ( ... )
		{
			return false;
		}
	}

	if( handler->getOverwriteHandler() )
	{
		//
		// ctor, checkformat function pointers for replacement handler
		//
		hdlCtor			= Plugin_MetaHandlerCTor_Replacement;
		chkFileFormat	= Plugin_CheckFileFormat_Replacement;
		chkFolderFormat	= Plugin_CheckFolderFormat_Replacement;
	}
	else
	{
		//
		// ctor, checkformat function pointers for standard handler
		//
		hdlCtor			= Plugin_MetaHandlerCTor_Standard;
		chkFileFormat	= Plugin_CheckFileFormat_Standard;
		chkFolderFormat	= Plugin_CheckFolderFormat_Standard;
	}

	//
	// register handler according to its type
	//
	switch( handler->getHandlerType() ) 
	{
		case NormalHandler_K:
			ret = hdlrReg.registerNormalHandler( format, handler->getHandlerFlags(), chkFileFormat, 
												 hdlCtor, handler->getOverwriteHandler() );
			break;

		case OwningHandler_K:
			ret = hdlrReg.registerOwningHandler( format, handler->getHandlerFlags(), chkFileFormat, 
												 hdlCtor, handler->getOverwriteHandler() );
			break;

		case FolderHandler_K:
			ret = hdlrReg.registerFolderHandler( format, handler->getHandlerFlags(), chkFolderFormat, 
												 hdlCtor, handler->getOverwriteHandler() );
			break;

		default:
			break;
	}

	return ret;
}

void PluginManager::initialize( const std::string& pluginDir, const std::string& plugins )
{
	try 
	{
		HandlerRegistry & hdlrReg = HandlerRegistry::getInstance();
		if( msPluginManager == 0 ) msPluginManager = new PluginManager( pluginDir, plugins );
		msPluginManager->initializeHostAPI();

		msPluginManager->doScan( 2 );

		//
		// Register all the found plugin based file handler
		//
		for( PluginHandlerMap::iterator it = msPluginManager->mHandlers.begin(); it != msPluginManager->mHandlers.end(); ++it ) 
		{
			XMP_FileFormat format = it->first;
			FileHandlerPair handlers = it->second;

			if( handlers.mStandardHandler != NULL )
			{
				registerHandler( format, handlers.mStandardHandler );
			}

			if( handlers.mReplacementHandler != NULL )
			{
				registerHandler( format, handlers.mReplacementHandler );
			}
		}
	} 
	catch( ... ) 
	{
		// Absorb exceptions. This is the plugin-architecture entry point.
	}

}	// PluginManager::initialize

// =================================================================================================

void PluginManager::terminate()
{
	delete msPluginManager;
	msPluginManager = 0;
	ResourceParser::terminate();
}

// =================================================================================================

void PluginManager::addFileHandler( XMP_FileFormat format, FileHandlerSharedPtr handler )
{
	if ( msPluginManager != 0 ) 
	{
		PluginHandlerMap & handlerMap = msPluginManager->mHandlers;

		//
		// Create placeholder in map for format
		//
		if ( handlerMap.find(format) == handlerMap.end() ) 
		{
			FileHandlerPair pair;
			handlerMap.insert( handlerMap.end(), std::pair<XMP_FileFormat, FileHandlerPair>( format, pair) );
		}

		//
		//
		// if there is already a standard handler or a replacement handler for the file format
		// then use the one with the highest version. If both versions are the same the first one wins.
		//
		FileHandlerSharedPtr& existingHandler =
			handler->getOverwriteHandler() ? handlerMap[format].mReplacementHandler : handlerMap[format].mStandardHandler;

		if( ! existingHandler )
		{
			existingHandler = handler;
		}
		else
		{
			if( existingHandler->getUID() == handler->getUID() )
			{
				if( existingHandler->getVersion() < handler->getVersion() )
				{
					existingHandler = handler;	// replace older handler
				}
			}
			else
			{
				// TODO: notify client that two plugin handlers try to handle the same file format
				// -> need access to the global notification handler
			}
		}
	}
}

// =================================================================================================

FileHandlerSharedPtr PluginManager::getFileHandler( XMP_FileFormat format, HandlerPriority priority /*= kStandardHandler*/ )
{
	if ( msPluginManager != 0 ) 
	{
		PluginHandlerMap::iterator it = msPluginManager->mHandlers.find( format );
	
		if( it != msPluginManager->mHandlers.end() ) 
		{
			if( priority == kStandardHandler )
			{
				return it->second.mStandardHandler;
			}
			else if( priority == kReplacementHandler )
			{
				return it->second.mReplacementHandler;
			}
		}
	}
	
	return FileHandlerSharedPtr();
}

// =================================================================================================

static XMP_ReadWriteLock sSessionMapPluginManagerRWLock;

void PluginManager::addHandlerInstance( SessionRef session, FileHandlerInstancePtr handler )
{
	if ( msPluginManager != 0 ) {
		XMP_AutoLock lock(&sSessionMapPluginManagerRWLock, kXMP_WriteLock);
		SessionMap & sessionMap = msPluginManager->mSessions;
		if ( sessionMap.find(session) == sessionMap.end() ) {
			sessionMap[session] = handler;	
		}
	}
}

// =================================================================================================

void PluginManager::removeHandlerInstance( SessionRef session )
{
	if ( msPluginManager != 0 ) {
		XMP_AutoLock lock(&sSessionMapPluginManagerRWLock, kXMP_WriteLock);
		SessionMap & sessionMap = msPluginManager->mSessions;
		sessionMap.erase ( session );
	}
}

// =================================================================================================

FileHandlerInstancePtr PluginManager::getHandlerInstance( SessionRef session )
{
	FileHandlerInstancePtr ret = 0;
	if ( msPluginManager != 0 ) {
		XMP_AutoLock lock(&sSessionMapPluginManagerRWLock, kXMP_ReadLock);
		ret = msPluginManager->mSessions[session];
	}
	return ret;
}

// =================================================================================================

PluginManager::HandlerPriority PluginManager::getHandlerPriority( FileHandlerInstancePtr handler )
{
	if( handler != NULL )
	{
		for( PluginHandlerMap::iterator it=msPluginManager->mHandlers.begin();
			 it != msPluginManager->mHandlers.end(); it++ )
		{
			if( it->second.mStandardHandler == handler->GetHandlerInfo() )		return kStandardHandler;
			if( it->second.mReplacementHandler == handler->GetHandlerInfo() )	return kReplacementHandler;
		}
	}

	return kUnknown;
}

// =================================================================================================

static bool CheckPluginArchitecture ( XMLParserAdapter * xmlParser ) {

	#if XMP_MacBuild
		bool okArchitecture = true;		// Missing Architecture attribute means load on Mac.
	#else
		bool okArchitecture = false;	// Missing Architecture attribute means do not load elsewhere.
	#endif
	
	#if XMP_64
		const char * nativeArchitecture = "x64";
	#else
		const char * nativeArchitecture = "x86";
	#endif

	size_t i, limit;
	XML_Node & xmlTree = xmlParser->tree;
	XML_NodePtr rootElem = 0;

	// Find the outermost XML element and see if it is PluginResource.
	for ( i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
		if ( xmlTree.content[i]->kind == kElemNode ) {
			rootElem = xmlTree.content[i];
			break;
		}
	}
	
	if ( (rootElem == 0) || (rootElem->name != "PluginResource") ) return okArchitecture;
	
	// Look for the Architecture attribute and see if it matches.

	XML_NodePtr archAttr = 0;
	for ( i = 0, limit = rootElem->attrs.size(); i < limit; ++i ) {
		if ( rootElem->attrs[i]->name == "Architecture" ) {
			archAttr = rootElem->attrs[i];
			break;
		}
	}
	
	if ( archAttr != 0 ) okArchitecture = (archAttr->value == nativeArchitecture);
	
	return okArchitecture;
	
}	// CheckPluginArchitecture

// =================================================================================================

void PluginManager::loadResourceFile( ModuleSharedPtr module )
{
	
	OS_ModuleRef moduleRef = LoadModule ( module->getPath(), true );

	if ( moduleRef != 0 ) {

		XMLParserAdapter* parser = 0;
	
		try {

			std::string buffer;
			if ( GetResourceDataFromModule ( moduleRef, kResourceName_UIDs, "txt", buffer ) ) {

				ResourceParser::initialize(); // Initialize XMPAtoms before processing resource file.

				parser = XMP_NewExpatAdapter ( ExpatAdapter::kUseGlobalNamespaces );
				parser->ParseBuffer ( (XMP_Uns8*)buffer.c_str(), buffer.size(), true );

				if ( CheckPluginArchitecture ( parser ) ) {
					ResourceParser resource ( module );
					resource.parseElementList ( &parser->tree, true );
				}

				delete parser;

			}

		} catch ( ... ) {

			if ( parser != 0 ) delete parser;
			// Otherwise ignore errors.

		}
		
		UnloadModule ( moduleRef, true );

	}

}	// PluginManager::loadResourceFile

// =================================================================================================

void PluginManager::scanRecursive( const std::string & tempPath, std::vector<std::string>& ioFoundLibs, XMP_Int32 inLevel, XMP_Int32 inMaxNestingLevel )
{
	++inLevel;
	Host_IO::AutoFolder aFolder;
	if ( Host_IO::GetFileMode( tempPath.c_str() ) != Host_IO::kFMode_IsFolder ) return;

	aFolder.folder = Host_IO::OpenFolder( tempPath.c_str() );
	std::string childPath, childName;

	while ( Host_IO::GetNextChild ( aFolder.folder, &childName ) ) {	

		// Make sure the children of CONTENTS are legit.
		childPath = tempPath;
		childPath += kDirChar;
		childPath += childName;
		Host_IO::FileMode clientMode = Host_IO::GetFileMode ( childPath.c_str() );
		
		bool okFolder = (clientMode == Host_IO::kFMode_IsFolder);
		#if XMP_MacBuild
			if ( okFolder ) okFolder = ( ! IsValidLibrary ( childPath ) );
		#endif
		
		// only step into non-packages (neither bundle nor framework) on Mac
		if ( okFolder ) {

			if ( inLevel < inMaxNestingLevel ) {
				scanRecursive ( childPath + kDirChar, ioFoundLibs, inLevel, inMaxNestingLevel );
			}

		} else {

			if ( childName[0] == '~' ) continue; // ignore plug-ins like "~PDFL.xpi"

			std::string fileExt;
			XMP_StringPtr extPos = childName.c_str() + childName.size();
			for ( ; (extPos != childName.c_str()) && (*extPos != '.'); --extPos ) {}
			if ( *extPos == '.' ) {
				fileExt.assign ( extPos+1 );
				MakeLowerCase ( &fileExt );
			}

			StringVec::const_iterator iterFound =
				std::find_if ( mExtensions.begin(), mExtensions.end(), 
							   std::bind2nd ( std::equal_to<std::string>(), fileExt ) );

			if ( iterFound != mExtensions.end() ) {

				//Check if the found plugin is present in the user's demanding plugin list.
				childName.erase ( extPos - childName.c_str() );
				MakeLowerCase ( &childName );
				
				StringVec::const_iterator pluginNeeded =
					std::find_if ( mPluginsNeeded.begin(), mPluginsNeeded.end(),
								   std::bind2nd ( std::equal_to<std::string>(), childName ) );

				if ( (pluginNeeded != mPluginsNeeded.end()) || mPluginsNeeded.empty() ) {
					ioFoundLibs.push_back ( childPath );
				}

			}

		}

	}

	aFolder.Close();

}	// PluginManager::scanRecursive

// =================================================================================================

void PluginManager::doScan( const XMP_Int32 inMaxNumOfNestedFolder )
{
	XMP_Assert(inMaxNumOfNestedFolder > 0);
	if ( inMaxNumOfNestedFolder < 1 ) return; // noop, wrong parameter
		
	// scan directory
	std::vector<std::string> foundLibs;
	XMP_Int32 iteration = 0;
	scanRecursive ( mPluginDir, foundLibs, iteration, inMaxNumOfNestedFolder );

	// add found modules
	std::vector<std::string>::const_iterator iter = foundLibs.begin();
	std::vector<std::string>::const_iterator iterEnd = foundLibs.end();
	for ( ; iter != iterEnd; ++iter ) {
		std::string path ( *iter );
		ModuleSharedPtr module ( new Module ( path ) );
		loadResourceFile ( module );
	}

}	// PluginManager::doScan

// =================================================================================================

HostAPIRef PluginManager::getHostAPI( XMP_Uns32 version )
{
	HostAPIRef hostAPI = NULL;

	if( msPluginManager == NULL )	return NULL;
	if(  version < 1 )				return NULL;

	HostAPIMap::iterator iter = msPluginManager->mHostAPIs.find( version );

	if( iter != msPluginManager->mHostAPIs.end() )
	{
		hostAPI = iter->second;
	}

	return hostAPI;
}

// =================================================================================================

void PluginManager::terminateHostAPI()
{
	for( HostAPIMap::iterator it = msPluginManager->mHostAPIs.begin(); it != msPluginManager->mHostAPIs.end(); ++it )
	{
		XMP_Uns32 version = it->first;
		HostAPIRef hostAPI = it->second;

		switch( version )
		{
		case 1:
		case 2:
		case 3:
		case 4:
			{
				delete hostAPI->mFileIOAPI;
				delete hostAPI->mStrAPI;
				delete hostAPI->mAbortAPI;
				delete hostAPI->mStandardHandlerAPI;
				delete hostAPI;
			}
			break;

		default:
			{
				delete hostAPI;
			}
		}
	}
}

void PluginManager::initializeHostAPI()
{
	HostAPIRef hostAPI = NULL;


	for ( int i = 0; i < XMP_HOST_API_VERSION_4; i++ )
	{
		hostAPI				= new HostAPI();
		hostAPI->mSize		= sizeof( HostAPI );
		hostAPI->mVersion	= i + 1;

		switch( hostAPI->mVersion )
		{
		case 1:
			SetupHostAPI_V1( hostAPI );
			break;

		case 2:
			SetupHostAPI_V2( hostAPI );
			break;

		case 3:
			SetupHostAPI_V3( hostAPI );
			break;

		default:
		case 4:
			SetupHostAPI_V4( hostAPI );
			break;
		}

		if( hostAPI != NULL )
		{
			msPluginManager->mHostAPIs[ hostAPI->mVersion ] = hostAPI;
		}
	}
}

}	// namespace XMP_PLUGIN
