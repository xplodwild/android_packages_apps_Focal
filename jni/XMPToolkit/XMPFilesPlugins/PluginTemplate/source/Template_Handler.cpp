// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFilesPlugins/api/source/PluginBase.h"
#include "XMPFilesPlugins/api/source/PluginRegistry.h"
#include "XMPFilesPlugins/api/source/HostAPIAccess.h"

#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"
#include "source/XMP_LibUtils.hpp"

namespace XMP_PLUGIN
{

// Plugin template file handler class. 
// This handler can be used to read/write XMP from/to text files
// All file handlers should be derived from PluginBase.
class TEMP_MetaHandler : public PluginBase
{
public:
	TEMP_MetaHandler( const std::string& filePath, XMP_Uns32 openFlags, XMP_Uns32 format, XMP_Uns32 handlerFlags )
		: PluginBase( filePath, openFlags, format, handlerFlags ) {}
	~TEMP_MetaHandler() {}

	//Minimum required virtual functions which need to be implemented by the plugin developer.
	virtual void cacheFileData( const IOAdapter& file, std::string& xmpStr );
	virtual void updateFile( const IOAdapter& file, bool doSafeUpdate, const std::string& xmpStr );
	// In case you want to directly support crash-safe updates
	virtual void writeTempFile( const IOAdapter& srcFile, const IOAdapter& tmpFile, const std::string& xmpStr );

	//Static functions which also need to be implemented by the plugin developer.
	
	// This function is called to initialize the file handler. Don't get confused by the file handler instance. 
	// It may be an empty function if nothing needs to be initialized.
	//@return true on success otherwise false.
	static bool initialize();
	
	//This function is called to terminate the file handler.
	//@return true on success otherwise false.
	static bool terminate();

	//The following two functions need to be implemented to check the format. Though file handler 
	//will need only one of these, both of them need to be implemented. The actual check format 
	//function should be implemented properly while the other one will just return false.
	
	//@return true on success otherwise false.
	static bool checkFileFormat( const std::string& filePath, const IOAdapter& file );
	
	// This function is not needed by a normal handler but an "owning" handler needs to implement it.
	static inline bool checkFolderFormat(	const std::string& rootPath, 
											const std::string& gpName, 
											const std::string& parentName, 
											const std::string& leafName ) { return false; }

private:
	void ReconcileXMP( const std::string &xmpStr, std::string *outStr );

	static int init;
	static const std::string XPACKET_HEADER;
	static const std::string XMPMETA_HEADER;
};

int TEMP_MetaHandler::init = 0;
const std::string TEMP_MetaHandler::XPACKET_HEADER = "<?xpacket";
const std::string TEMP_MetaHandler::XMPMETA_HEADER = "<x:xmpmeta";

//============================================================================
// registerHandler TEMP_MetaHandler
//============================================================================

// Return module identifier string. This string should be same as the string present in 
// the resource file otherwise plugin won't be loaded.
// This function needs to be implemented by the plugin developer.
const char* GetModuleIdentifier()
{
	static const char* kModuleIdentifier = "com.adobe.xmp.plugins.template";
	return kModuleIdentifier;
}

// This function will be called during initialization of the plugin.
// Additional host API suite can be requested using RequestAPISuite().
// The initialization will be aborted if false is returned.
bool SetupPlugin()
{
	/*
	 ExampleSuite* exampleSuite =
	 	reinterpret_cast<ExampleSuite*>( RequestAPISuite( "exampleSuite", 1 ) );
	 
	 return (exampleSuite != NULL);
	 */
	
	return true;
}

// All the file handlers present in this module will be registered. There may be many file handlers
// inside one module. Only those file handlers which are registered here will be visible to XMPFiles.
// This function need to be implemented by the plugin developer.
//@param sXMPTemplate uid of the file handler, 
void RegisterFileHandlers()
{
	//uid of the file handler. It's different from the module identifier.
	const char* sXMPTemplate = "com.adobe.xmp.plugins.template.handler"; 
	PluginRegistry::registerHandler( new PluginCreator<TEMP_MetaHandler>(sXMPTemplate) );
}

//============================================================================
// TEMP_MetaHandler specific functions
//============================================================================

bool TEMP_MetaHandler::initialize()
{
	init++;

	if(init != 1) return true;

	{
		SXMPMeta::Initialize();
	
		// Do init work here
	}

	return true;
}


bool TEMP_MetaHandler::terminate()
{
	init--;

	if(init != 0) return true;

	// Do termination work here

	SXMPMeta::Terminate();
	
	return true;
}


bool TEMP_MetaHandler::checkFileFormat( const std::string& filePath, const IOAdapter& file )
{
	try 
	{
		const XMP_Uns8 MIN_CHECK_LENGTH = 9;

		if ( file.Length() < MIN_CHECK_LENGTH ) return false;

		char buffer[MIN_CHECK_LENGTH];
		XMP_Int64 offset = 0;
		file.Seek ( offset, kXMP_SeekFromStart );
		file.Read ( buffer, MIN_CHECK_LENGTH, true );

		return ( strncmp(buffer, XPACKET_HEADER.c_str(), MIN_CHECK_LENGTH) == 0 || 
				strncmp(buffer, XMPMETA_HEADER.c_str(), MIN_CHECK_LENGTH) == 0 );
	} 
	catch ( ... ) 
	{
		return false;
	}

	return false;
}


void TEMP_MetaHandler::cacheFileData( const IOAdapter& file, std::string& xmpStr )
{
	xmpStr.erase();
		
	// Extract the contents.

	XMP_Int64 xmpLength = file.Length();
	if ( xmpLength > 1024*1024*1024 ) 
	{	// Sanity check and to not overflow std::string.
		XMP_Throw ( "XMP file is too large", kXMPErr_PluginCacheFileData );
	}

	if ( xmpLength > 0 ) 
	{
		xmpStr.assign ( (size_t)xmpLength, ' ' );
		XMP_Int64 offset = 0;
		file.Seek ( offset, kXMP_SeekFromStart );
		file.Read ( (void*)xmpStr.c_str(), (XMP_Uns32)xmpLength, true );
	}
}


void TEMP_MetaHandler::updateFile( const IOAdapter& file, bool doSafeUpdate, const std::string& xmpStr )
{
	// safe update option only relevant for handlers which "own" the file I/O
	// for other handlers the method writeTempFile is called in case the client wants a safe update
	
	// Example how to use XMPCore to edit the XMP data before export
	// Please note that if you do not want to explicitely edit the XMP at this point
	// you can write it directly to the file as it is already correctly serialized
	// as indicated by the plugin manifest serialize options
	std::string outStr;
	ReconcileXMP(xmpStr, &outStr);
	
	// overwrite existing file
	XMP_Int64 offset = 0;
	file.Seek ( offset, kXMP_SeekFromStart );
	file.Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));
	file.Truncate(static_cast<XMP_Uns32>(outStr.length()));
}


void TEMP_MetaHandler::writeTempFile( const IOAdapter& srcFile, const IOAdapter& tmpFile, const std::string& xmpStr )
{
	// The source file is in this case irrelevant

	// Please note that if you are creating a plugin for Adobe applications like Premiere there is a bug
	// in the implementation of the caller of this function which passes in the "xmpStr" parameter
	// the path to the media file instead of the serialized XMP packet!
	// If the plugin is for your own application using the XMP SDK, the parameter value is correct

	// example how to use XMPCore to edit the XMP data before export
	std::string outStr;
	ReconcileXMP(xmpStr, &outStr);

	tmpFile.Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));
}


void TEMP_MetaHandler::ReconcileXMP( const std::string &xmpStr, std::string *outStr ) 
{
	// This is an example how to use XMPCore to edit the XMP data
	SXMPMeta xmp;
	xmp.ParseFromBuffer( xmpStr.c_str(), xmpStr.length() );

	xmp.SetProperty( kXMP_NS_XMP, "CreatorTool", "My plugin" );

	xmp.SerializeToBuffer( outStr, kXMP_UseCompactFormat | kXMP_OmitPacketWrapper );
}

} //namespace XMP_PLUGIN
