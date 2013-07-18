// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef PLUGINREGISTRY_H
#define PLUGINREGISTRY_H
#include "PluginHandler.h"
#include "HostAPIAccess.h"
#include "PluginBase.h"
#include <map>
#include <string>

namespace XMP_PLUGIN
{

class PluginCreatorBase;
class PluginBase;

/** @class PluginRegistry
 *  @brief It registers file handler in the plugin.
 *  It is singleton class which register the file handlers in one plugin.
 */

class PluginRegistry
{
public:
	
	/** @brief Register file handlers.
	 *  @param pointer to PluginCreator, created with the file handler which needs to be registered.
	 *  @return Void.
	 */
	static void registerHandler( const PluginCreatorBase* inCreator );

	/** @brief Initialize all the registered file handlers.
	 *  @return Void.
	 */
	static bool initialize();
	
	/** @brief Terminate all the registered file handlers.
	 *  @return Void.
	 */
	static bool terminate();

	/** @brief Create instance of the file handler with the given uid.
	 *  @param uid Unique identifier string (uid) of the file handler whose instance is to be created.
	 *  @param openFlags Flags that describe the desired access.
	 *	@param format File format id the class is created for
	 *	@param handlerFlags	According handler flags
	 *  @param filePath FilePath of the file which is to be opened.
	 *  @return Pointer to file Handler instance.
	 */
	static PluginBase* create( const std::string& uid, const std::string& filePath, XMP_Uns32 openFlags, XMP_Uns32 format, XMP_Uns32 handlerFlags );
	
	/** @brief Check whether the input file /a filePath is supported by the file handler with uid /a uid.
	 *  @param uid Unique identifier string (uid) of the file handler.
	 *  @param filePath FilePath of the file which is to be opened.
	 *  @return true if input file is supported by the file handler otherwise false.
	 */
	static bool checkFileFormat( const std::string& uid, const std::string& filePath, const IOAdapter& file );
	static bool checkFolderFormat( const std::string& uid, const std::string& rootPath, const std::string& gpName, const std::string& parentName, const std::string& leafName );
	
private:

	struct StringCompare : std::binary_function< const std::string &, const std::string &, bool >
	{
		bool operator()( const std::string & a, const std::string & b ) const
		{
			return ( a.compare(b) < 0 );
		}
	};
	typedef std::map<std::string, const PluginCreatorBase*,	StringCompare>		RegistryEntryMap;
	
	PluginRegistry(){}
	~PluginRegistry();
	
	RegistryEntryMap mRegistryEntries;
	static PluginRegistry* msRegistry;
};


/** @class PluginCreatorBase
 *  @brief It is a base class which help in registering the file handler.
 *  
 *  The use of this class is only enabling calling of some basic functions on the template class 
 *  using the base class pointer. For actual details see class PluginCreator
 *  
 *  @see PluginCreator
 */

class PluginCreatorBase
{
public:
	PluginCreatorBase() {}
	virtual ~PluginCreatorBase() {}

	virtual PluginBase* create( const std::string& filePath, XMP_Uns32 openFlags, XMP_Uns32 format, XMP_Uns32 handlerFlags ) const = 0;
	
	/** A File handler should provide either checkFileFormat if it is OwningHandler or NormalHandler
	 *  OR it should provide checkFolderFormat if it is FolderHandler. Default implementation returns false
	 *  which mean the handler does not support the file format.
	 */
	virtual bool checkFileFormat( const std::string& filePath, const IOAdapter& file ) const = 0;
	virtual bool checkFolderFormat( const std::string& rootPath, const std::string& gpName, const std::string& parentName, const std::string& leafName ) const = 0;
	
	virtual const std::string & GetUID() const = 0;
	virtual bool initialize() const = 0;
	virtual bool terminate() const = 0;
};

/** @class PluginCreator
 *  @brief This template class is used to register the file handler 'TFileHandler'.
 *  TFileHandler is the actual file handler which needs to be registered.
 *  The file handler need to implement following static functions.
 *  TFileHandler::checkFileFormat(); // Required for checking the format.
 *  TFileHandler::initialize(); // Initialize the file handler.
 *  TFileHandler::terminate(); // Terminate the file handler.
 */

template<typename TFileHandler>
class PluginCreator : public PluginCreatorBase
{
public:
	
	/** @brief CTOR
	 *
	 * @param inUID unique identifier string (uid) of the file handler.
	 * @param format id type of the file format, currently this argument is not being used in the implementation.
	 * @param flags File handler's flag. 
	 */
	PluginCreator( const char* inUID ) 
		: mUID( inUID )
	{}

	/** @deprecated */
	PluginCreator( const char* inUID, XMP_FileFormat , XMP_OptionBits ) 
	: mUID( inUID )
	{}
	
	/** @brief Create instance of file handler TFileHandler.
	 *  @param filePath FilePath of the file which is to be opened.
	 *  @param openFlags Flags that describe the desired access.
	 *	@param format File format id the class is created for
	 *	@param handlerFlags	According handler flags
	 *  @return Pointer to file Handler instance.
	 */
	inline PluginBase* create( const std::string& filePath, XMP_Uns32 openFlags, XMP_Uns32 format, XMP_Uns32 handlerFlags ) const
	{
		TFileHandler* instance = new TFileHandler(filePath, openFlags, format, handlerFlags);
		PluginBase* handler = dynamic_cast<PluginBase*>(instance);

		return handler;
	}

	/** @brief Check whether the input file /a filePath is supported by file handler TFileHandler.
	 *  @param filePath FilePath of the file which is to be opened.
	 *  @return true if input file is supported by TFileHandler otherwise false.
	 */
	inline bool checkFileFormat( const std::string& filePath, const IOAdapter& file ) const
	{
		return TFileHandler::checkFileFormat( filePath, file );
	}

	inline bool checkFolderFormat( const std::string& rootPath, const std::string& gpName, const std::string& parentName, const std::string& leafName ) const
	{
		return TFileHandler::checkFolderFormat( rootPath, gpName, parentName, leafName );
	}

	inline const std::string & GetUID() const
	{ 
		return mUID; 
	}

	/** @brief initialize the file handler.
	 *  @return Void.
	 */
	inline bool initialize() const
	{
		return TFileHandler::initialize();
	}

	/** @brief terminate the file handler.
	 *  @return Void.
	 */
	inline bool terminate() const
	{
		return TFileHandler::terminate();
	}

private:
	std::string			mUID;
};

} //namespace XMP_PLUGIN
#endif // PLUGINREGISTRY_H
