// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef MODULEUTILS_H
#define MODULEUTILS_H
#include "XMPFiles/source/XMPFiles_Impl.hpp"

#if XMP_WinBuild
#include <Windows.h>
typedef HMODULE OS_ModuleRef;
#elif XMP_MacBuild
#include <CoreFoundation/CFBundle.h>
#include <tr1/memory>
typedef CFBundleRef OS_ModuleRef;
#elif XMP_UNIXBuild
#include <tr1/memory>
typedef void* OS_ModuleRef;
#else
#error	Unsupported operating system
#endif

namespace XMP_PLUGIN
{

/**
 * Platform implementation to retrieve a function pointer of the name \param inSymbol from a module \param inOSModule
 */
void* GetFunctionPointerFromModuleImpl(	OS_ModuleRef inOSModule, const char* inSymbol );

/**
 *  @return true if @param inModulePath points to a valid shared library
*/
#if XMP_MacBuild
bool IsValidLibrary( const std::string & inModulePath );
#endif

/**
 *  Load module specified by absolute path \param inModulePath
 *
 * Win:
 * If \param inOnlyResourceAccess = true, only the image is loaded, no referenced dlls are loaded nor initialization code is executed.
 * If the module is already loaded and executable, it behaves as \param inOnlyResourceAccess = false.
 * The reference count is increased, so don't forget to call UnloadModule.
 *
 * Mac:
 * If \param inOnlyResourceAccess = true, only the CFBundleRef is created. No code is loaded and executed.
 */
OS_ModuleRef LoadModule( const std::string & inModulePath, bool inOnlyResourceAccess = false );

/**
 *  Unload module
 *  @param inModule
 *  @param inOnlyResourceAccess = true, close resource file (only relevant for Linux !!).
 */
void UnloadModule( OS_ModuleRef inModule, bool inOnlyResourceAccess = false );

/** @brief Read resource file and fill the data in outBuffer
 *  @param inOSModule Handle of the module.
 *  @param inResourceName Name of the resource file which needs to be read.
 *  @param inResourceType Type/Extension of the resource file.
 *  @param outBuffer Output buffer where data read from the resource file will be stored.
 *  @return true on success otherwise false
 */
bool GetResourceDataFromModule(
	OS_ModuleRef inOSModule,
	const std::string & inResourceName,
	const std::string & inResourceType,
	std::string & outBuffer);

} //namespace XMP_PLUGIN
#endif
