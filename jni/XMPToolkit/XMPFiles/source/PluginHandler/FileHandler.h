// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef PLUGINHANDLER_H
#define PLUGINHANDLER_H
#include "Module.h"

namespace XMP_PLUGIN
{

/** @struct CheckFormat
 *  @brief Record of byte sequences.
 *
 *  It is a static information of the file handler provided in the resource file, if the format 
 *  can be identified by one or more sequences of fixed bytes at a fixed location within the format.
 */
struct CheckFormat
{
	XMP_Int64 mOffset;
	XMP_Uns32 mLength;
	std::string mByteSeq;

	CheckFormat(): mOffset( 0 ), mLength( 0 ) { }
	
	inline void clear()
	{
		mOffset = 0; mLength = 0; mByteSeq.clear(); 
	}
	
	inline bool empty() 
	{ 
		return ( mLength == 0 || mByteSeq.empty() );
	}	
};

/** @class FileHandler
 *  @brief File handler exposed through plugin architecture.
 *
 *  At the initialization time of XMPFiles only static information from all the avalbale plugins
 *  is populated by creating instance of this class FileHandler. It's loaded later when it's actually 
 *  required to get information from the format.
 */
class FileHandler
{
public:
	
	FileHandler(std::string & uid, XMP_OptionBits handlerFlags, FileHandlerType type, ModuleSharedPtr module):
	  mVersion(0), mUID(uid), mHandlerFlags(handlerFlags), mOverwrite(false), mType(type), mModule(module) {}

	virtual ~FileHandler(){}

	inline double getVersion() const { return mVersion; }
	inline void setVersion( double version ) { mVersion = version; }

	inline const std::string& getUID() const { return mUID; }
	inline XMP_OptionBits getHandlerFlags() const { return mHandlerFlags; }
	inline void setHandlerFlags( XMP_OptionBits flags ) { mHandlerFlags = flags; }
	
	inline XMP_OptionBits getSerializeOption() const { return mSerializeOption; }
	inline void setSerializeOption( XMP_OptionBits option ) { mSerializeOption = option; }

	inline bool getOverwriteHandler() const { return mOverwrite; }
	inline void setOverwriteHandler( bool overwrite ) { mOverwrite = overwrite; }
	
	inline FileHandlerType getHandlerType() const { return mType; }
	inline void setHandlerType( FileHandlerType type ) { mType = type; }

	inline bool load() { return mModule->load(); }
	inline ModuleSharedPtr getModule() const { return mModule; }

	inline void addCheckFormat( const CheckFormat & checkFormat ) { mCheckFormatVec.push_back( checkFormat ); }
	inline XMP_Uns32 getCheckFormatSize() const { return (XMP_Uns32) mCheckFormatVec.size(); }
	inline CheckFormat getCheckFormat( XMP_Uns32 index ) const 
	{
		CheckFormat checkFormat;
		if( index < mCheckFormatVec.size() )
			checkFormat = mCheckFormatVec[index];
		return checkFormat;
	}
	
private:
	typedef std::vector<CheckFormat> CheckFormatVec;

	CheckFormatVec				mCheckFormatVec;
	double						mVersion;
	std::string					mUID;
	XMP_OptionBits				mHandlerFlags;
	XMP_OptionBits				mSerializeOption;
	bool						mOverwrite;
	FileHandlerType				mType;
	ModuleSharedPtr				mModule;
};

} //namespace XMP_PLUGIN
#endif //PLUGINHANDLER_H
