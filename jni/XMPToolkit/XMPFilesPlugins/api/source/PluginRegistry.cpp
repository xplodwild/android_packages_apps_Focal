// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "PluginRegistry.h"
#include "PluginBase.h"

namespace XMP_PLUGIN
{

// static class member initialization
PluginRegistry* PluginRegistry::msRegistry = NULL;

PluginRegistry::~PluginRegistry()
{
	RegistryEntryMap::iterator iter = msRegistry->mRegistryEntries.begin();
	RegistryEntryMap::iterator iterEnd = msRegistry->mRegistryEntries.end();
	
	for( ; iter != iterEnd; )
	{
		RegistryEntryMap::iterator temp = iter++;
		delete temp->second;
	}
}

// ============================================================================

/*static*/
void PluginRegistry::registerHandler( const PluginCreatorBase* inCreator )
{
	if( msRegistry == NULL )
		msRegistry = new PluginRegistry();
	
	msRegistry->mRegistryEntries[inCreator->GetUID()] = inCreator;
}

// ============================================================================

/*static*/
PluginBase* PluginRegistry::create( const std::string& uid, const std::string& filePath, XMP_Uns32 openFlags, XMP_Uns32 format, XMP_Uns32 handlerFlags )
{
	if( msRegistry != NULL )
	{
		RegistryEntryMap::const_iterator iter = msRegistry->mRegistryEntries.find(uid);
		if( iter != msRegistry->mRegistryEntries.end() )
			return iter->second->create(filePath, openFlags, format, handlerFlags);
	}
	
	return NULL;
}

// ============================================================================

/*static*/
bool PluginRegistry::checkFileFormat( const std::string& uid, const std::string& filePath, const IOAdapter& file )
{
	if( msRegistry != NULL )
	{
		RegistryEntryMap::const_iterator iter = msRegistry->mRegistryEntries.find(uid);
		if( iter != msRegistry->mRegistryEntries.end() )
			return iter->second->checkFileFormat( filePath, file );
	}
	
	return false;
}

// ============================================================================

/*static*/
bool PluginRegistry::checkFolderFormat( const std::string& uid, const std::string& rootPath, const std::string& gpName, const std::string& parentName, const std::string& leafName )
{
	if( msRegistry != NULL )
	{
		RegistryEntryMap::const_iterator iter = msRegistry->mRegistryEntries.find(uid);
		if( iter != msRegistry->mRegistryEntries.end() )
			return iter->second->checkFolderFormat( rootPath, gpName, parentName, leafName );
	}
	
	return false;
}

// ============================================================================

/*static*/
bool PluginRegistry::initialize()
{
	if( msRegistry != NULL )
	{			
		RegistryEntryMap::const_iterator iter = msRegistry->mRegistryEntries.begin();
		RegistryEntryMap::const_iterator iterEnd = msRegistry->mRegistryEntries.end();
		
		for( ; iter != iterEnd; ++iter )
		{
			if( iter->second->initialize() == false )
				return false;
		}
	}
	
	return true;
}

// ============================================================================

/*static*/
bool PluginRegistry::terminate()
{
	if( msRegistry != NULL )
	{
		RegistryEntryMap::const_iterator iter = msRegistry->mRegistryEntries.begin();
		RegistryEntryMap::const_iterator iterEnd = msRegistry->mRegistryEntries.end();
		
		for( ; iter != iterEnd; ++iter )
		{
			iter->second->terminate();
		}
		
		delete msRegistry;
	}
	return true;
}

} //namespace XMP_PLUGIN
