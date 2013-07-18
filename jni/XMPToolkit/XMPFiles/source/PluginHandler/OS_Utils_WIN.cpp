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

namespace XMP_PLUGIN
{

OS_ModuleRef LoadModule( const std::string & inModulePath, bool inOnlyResourceAccess)
{
	std::string wideString;
	ToUTF16 ( (UTF8Unit*)inModulePath.c_str(), inModulePath.size() + 1, &wideString, false ); // need +1 character otherwise \0 won't be converted into UTF16

	DWORD flags = inOnlyResourceAccess ? LOAD_LIBRARY_AS_IMAGE_RESOURCE : 0;
	OS_ModuleRef result = ::LoadLibraryEx((WCHAR*) wideString.c_str(), NULL, flags);

	// anything below indicates error in LoadLibrary
	if((result != NULL) && (result < OS_ModuleRef(32)))
	{
		result = NULL;
	}
	return result;
}

void UnloadModule( OS_ModuleRef inModule, bool inOnlyResourceAccess )
{
	::FreeLibrary(inModule);
}

void* GetFunctionPointerFromModuleImpl(	OS_ModuleRef inOSModule, const char* inSymbol )
{
	return reinterpret_cast<void*>(	::GetProcAddress( inOSModule, inSymbol ) );
}

bool GetResourceDataFromModule(
	OS_ModuleRef inOSModule,
	const std::string & inResourceName,
	const std::string & inResourceType,
	std::string & outBuffer)
{
	HRSRC src = ::FindResourceA(inOSModule, reinterpret_cast<LPCSTR>(inResourceName.c_str()), reinterpret_cast<LPCSTR>(inResourceType.c_str()));
	if( src != NULL )
	{
		HGLOBAL resource = (::LoadResource(inOSModule, src));
		HGLOBAL data = (::LockResource(resource));
		std::size_t size = (std::size_t)(::SizeofResource(inOSModule, src));
		if (size)
		{
			outBuffer.assign((const char*)data, size);
		}
		UnlockResource(data);
		::FreeResource(resource);

		return true;
	}
	return false;
}

} //namespace XMP_PLUGIN