// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef __PLUGIN_CONST_H__
#define __PLUGIN_CONST_H__

#include "XMP_Const.h"

typedef void * StringVectorRef;
typedef void (* SetStringVectorProc) ( StringVectorRef vectorRef, XMP_StringPtr * arrayPtr, XMP_Uns32 stringCount );

enum 
{
	/// Plugin-internal failures
	kXMPErr_PluginInternal			      = 500,
	/// 
	kXMPErr_PluginInitialized		      = 501,
	/// 
	kXMPErr_PluginTerminate			      = 502,
	/// 
	kXMPErr_PluginSessionInit		      = 503,
	/// 
	kXMPErr_PluginSessionTerm		      = 504,
	/// 
	kXMPErr_PluginCacheFileData		      = 505,
	/// 
	kXMPErr_PluginUpdateFile		      = 506,
	/// 
	kXMPErr_PluginWriteTempFile		      = 507,
	/// 
	kXMPErr_PluginImportToXMP		      = 508,
	/// 
	kXMPErr_PluginExportFromXMP		      = 509,
	/// 
	kXMPErr_PluginCheckFileFormat	      = 510,
	/// 
	kXMPErr_PluginCheckFolderFormat	      = 511,
	///
	kXMPErr_SetHostAPI	                  = 512,
	///
	kXMPErr_PluginGetFileModDate          = 513,
	///
	kXMPErr_PluginFillMetadataFiles       = 514,
	///
	kXMPErr_PluginFillAssociatedResources = 515,
	///
	kXMPErr_PluginIsMetadataWritable	  = 516,
	
	/// last plugin error, please add new errors before this one
	kXMPErr_PluginLastError
};

#endif	// __PLUGIN_CONST_H__
