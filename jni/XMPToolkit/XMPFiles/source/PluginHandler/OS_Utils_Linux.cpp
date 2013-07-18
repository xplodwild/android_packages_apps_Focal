// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "ModuleUtils.h"
#include "source/UnicodeConversions.hpp"
#include "source/XIO.hpp"

#include <iostream>
#include <map>
#include <limits>
#include <dlfcn.h>
#include <errno.h>

namespace XMP_PLUGIN
{

// global map of loaded modules (handle, full path)
typedef std::map<OS_ModuleRef, std::string>			ModuleRefToPathMap;
static ModuleRefToPathMap							sMapModuleRefToPath;
//typedef std::map<void*, std::string>				ResourceFileToPathMap;
typedef std::map<OS_ModuleRef, std::string>			ResourceFileToPathMap;
static ResourceFileToPathMap						sMapResourceFileToPath;
static XMP_ReadWriteLock							sMapModuleRWLock;

typedef std::tr1::shared_ptr<int>					FilePtr;

static std::string GetModulePath( OS_ModuleRef inOSModule );
/** ************************************************************************************************************************
** CloseFile()
*/
void CloseFile(
	int* inFilePtr)
{
	close(*inFilePtr);
	delete inFilePtr;
}

/** ************************************************************************************************************************
** OpenResourceFile()
*/
static FilePtr OpenResourceFile(
	OS_ModuleRef inOSModule,
	const std::string& inResourceName,
	const std::string& inResourceType)
{
	// It is assumed, that all resources reside in a folder with
	// the same name as the shared object plus '.resources' extension
	std::string path( GetModulePath(inOSModule) );
	
	XMP_StringPtr extPos = path.c_str() + path.size();
	for ( ; (extPos != path.c_str()) && (*extPos != '.'); --extPos ) {}
	path.erase( extPos - path.c_str() ); // Remove extension
	
	path += ".resources";
	path += kDirChar;
	path += inResourceName + "." + inResourceType;

	FilePtr file;
	if( Host_IO::GetFileMode(path.c_str()) == Host_IO::kFMode_IsFile )
	{
		int fileRef = ::open( path.c_str(), O_RDONLY );
		if (fileRef != -1)
		{
			file.reset(new int(fileRef), CloseFile);
		}
	}
	return file;
}

OS_ModuleRef LoadModule( const std::string & inModulePath, bool inOnlyResourceAccess)
{
	OS_ModuleRef result = NULL;
	if( inOnlyResourceAccess )
	{
		int fileHandle = open(reinterpret_cast<const char*>(inModulePath.c_str()), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if( !fileHandle )
		{
			std::cerr << "Cannot open library for resource access: " << strerror(errno) << std::endl;
		}
		else
		{	// success !
			result = (void*) fileHandle;
			ResourceFileToPathMap::const_iterator iter = sMapResourceFileToPath.find(result);
			if (iter == sMapResourceFileToPath.end())
			{
				// not found, so insert
				sMapResourceFileToPath.insert(std::make_pair(result, inModulePath));
			}
		}

	}
	else
	{
		result = dlopen(reinterpret_cast<const char*>(inModulePath.c_str()), RTLD_LAZY/*RTLD_NOW*/);

		if( !result )
		{
			std::cerr << "Cannot open library: " << dlerror() << std::endl;
		}
		else
		{	// success !
			XMP_AutoLock writeLock ( &sMapModuleRWLock, kXMP_WriteLock );
			ModuleRefToPathMap::const_iterator iter = sMapModuleRefToPath.find(result);
			if( iter == sMapModuleRefToPath.end() )
			{
				// not found, so insert
				sMapModuleRefToPath.insert(std::make_pair(result, inModulePath));
			}
		}
	}

	return result;
}

void UnloadModule( OS_ModuleRef inModule, bool inOnlyResourceAccess )
{
	if( inModule != NULL )
	{
		// we bluntly assume, that only one instance of the same library is loaded ad therefore added to the global map !
		if( inOnlyResourceAccess )
		{
			ResourceFileToPathMap::iterator iter = sMapResourceFileToPath.find(inModule);
			if( iter != sMapResourceFileToPath.end() )
			{
				close((long) inModule);
				sMapResourceFileToPath.erase(iter);
			}
			else
			{
				XMP_Throw("OS_Utils_Linux::UnloadModule called with invalid module handle", kXMPErr_InternalFailure);
			}
		}
		else
		{
			XMP_AutoLock writeLock ( &sMapModuleRWLock, kXMP_WriteLock );
			ModuleRefToPathMap::iterator iter = sMapModuleRefToPath.find(inModule);
			if( iter != sMapModuleRefToPath.end() )
			{
				dlclose(inModule);
				sMapModuleRefToPath.erase(iter);
			}
			else
			{
				XMP_Throw("OS_Utils_Linux::UnloadModule called with invalid module handle", kXMPErr_InternalFailure);
			}
		}
	}
}

/** ************************************************************************************************************************
** GetModulePath()
*/
static std::string GetModulePath(
	OS_ModuleRef inOSModule)
{
	std::string result;

	if( inOSModule != NULL )
	{
		ModuleRefToPathMap::const_iterator iter;
		{
			XMP_AutoLock readLock ( &sMapModuleRWLock, kXMP_ReadLock );
			iter = sMapModuleRefToPath.find(inOSModule);
		}

		ResourceFileToPathMap::const_iterator iter2 = sMapResourceFileToPath.find(inOSModule);
		if( (iter != sMapModuleRefToPath.end()) && (iter2 != sMapResourceFileToPath.end()) )
		{
			XMP_Throw("OS_Utils_Linux::GetModulePath: Module handle is present in both global maps", kXMPErr_InternalFailure);
		}
		if (iter != sMapModuleRefToPath.end())
		{
			result = iter->second;
		}
		else if (iter2 != sMapResourceFileToPath.end())
		{
			result = iter2->second;
		}
		else
		{
			XMP_Throw("OS_Utils_Linux::GetModulePath: Failed to find inOSModule in global map !", kXMPErr_InternalFailure);
		}
	}
	
	return result;
}

void* GetFunctionPointerFromModuleImpl(	OS_ModuleRef inOSModule, const char* inSymbol )
{
	void* proc = NULL;
	if( inOSModule != NULL )
	{
		proc = dlsym(inOSModule, inSymbol);
		if( !proc )
		{
			std::cerr << "Cannot get function " << inSymbol << " : " << dlerror() << std::endl;
		}
	}
	return proc;
}

bool GetResourceDataFromModule(
	OS_ModuleRef inOSModule,
	const std::string & inResourceName,
	const std::string & inResourceType,
	std::string & outBuffer)
{
	bool success = false;
	FilePtr file = OpenResourceFile( inOSModule, inResourceName, inResourceType );

	if( file )
	{
		ssize_t	size = 0;
		ssize_t	file_size = ::lseek(*file.get(), 0, SEEK_END);
		// presumingly we don't want to load more than 2GByte at once (!)
		if( file_size < std::numeric_limits<XMP_Int32>::max() )
		{
			size = file_size;
			if( size > 0 )
			{
				outBuffer.resize(size);

				::lseek(*file.get(), 0, SEEK_SET);
				ssize_t bytesRead = ::read( *file.get(), (unsigned char*)&outBuffer[0], size );
				success = bytesRead == size;
			}
		}
	}
	return success;
}

} //namespace XMP_PLUGIN
