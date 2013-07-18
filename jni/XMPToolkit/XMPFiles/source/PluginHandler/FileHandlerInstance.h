// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef PLUGINHANDLERINSTANCE_H
#define PLUGINHANDLERINSTANCE_H
#include "FileHandler.h"

namespace XMP_PLUGIN
{

/** @class FileHandlerInstance
 *  @brief This class is equivalent to the native file handlers like JPEG_MetaHandler or the simialr one.
 *
 *  This class is equivalent to the native file handler. This class is supposed to support all the 
 *  function which a native file handler support.
 *  As of now, it support only the functions required for OwningFileHandler.
 */
class FileHandlerInstance : public XMPFileHandler
{
public:
	FileHandlerInstance ( SessionRef object, FileHandlerSharedPtr handler, XMPFiles * parent );
	virtual ~FileHandlerInstance();

	virtual bool GetFileModDate ( XMP_DateTime * modDate );

	virtual void CacheFileData();
	virtual void ProcessXMP();
	//virtual XMP_OptionBits GetSerializeOptions(); //It should not be needed as its required only inside updateFile.
	virtual void UpdateFile ( bool doSafeUpdate );
	virtual void WriteTempFile ( XMP_IO* tempRef );
	virtual void FillMetadataFiles ( std::vector<std::string> * metadataFiles );
	virtual void FillAssociatedResources ( std::vector<std::string> * resourceList );
	virtual bool IsMetadataWritable ( );

	inline SessionRef				GetSession() const	{ return mObject; }
	inline FileHandlerSharedPtr		GetHandlerInfo() const	{ return mHandler; }

private:
	SessionRef			mObject;
	FileHandlerSharedPtr	mHandler;
};

} //namespace XMP_PLUGIN
#endif
