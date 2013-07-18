// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPAtoms.h"
#include "XMPFiles/source/HandlerRegistry.h"

using namespace Common;

namespace XMP_PLUGIN
{

struct XMPAtomMapping
{
	XMP_StringPtr	name;
	XMPAtom			atom;
};

const XMPAtomMapping kXMPAtomVec[] =
{
	{ "",							emptyStr_K },
	{ "Handler",					Handler_K },
	{ "Extensions",					Extensions_K },
	{ "Extension",					Extension_K },
	{ "FormatIDs",					FormatIDs_K },
	{ "FormatID",					FormatID_K },
	{ "HandlerType",				HandlerType_K },
	{ "Priority",					OverwriteHdl_K },
	{ "HandlerFlags",				HandlerFlags_K },
	{ "HandlerFlag",				HandlerFlag_K },
	{ "SerializeOptions",			SerializeOptions_K },
	{ "SerializeOption",			SerializeOption_K },
	{ "Version",					Version_K },
	{ "CheckFormat",				CheckFormat_K },
	{ "Name",						Name_K },
	{ "Offset",						Offset_K },
	{ "Length",						Length_K },
	{ "ByteSeq",					ByteSeq_K },

	// Handler types
	{ "NormalHandler",				NormalHandler_K },
	{ "OwningHandler",				OwningHandler_K },
	{ "FolderHandler",				FolderHandler_K },

	// Handler flags
	{ "kXMPFiles_CanInjectXMP",			kXMPFiles_CanInjectXMP_K },
	{ "kXMPFiles_CanExpand",			kXMPFiles_CanExpand_K },
	{ "kXMPFiles_CanRewrite",			kXMPFiles_CanRewrite_K },
	{ "kXMPFiles_PrefersInPlace",		kXMPFiles_PrefersInPlace_K },
	{ "kXMPFiles_CanReconcile",			kXMPFiles_CanReconcile_K },
	{ "kXMPFiles_AllowsOnlyXMP",		kXMPFiles_AllowsOnlyXMP_K },
	{ "kXMPFiles_ReturnsRawPacket",		kXMPFiles_ReturnsRawPacket_K },
	{ "kXMPFiles_HandlerOwnsFile",		kXMPFiles_HandlerOwnsFile_K },
	{ "kXMPFiles_AllowsSafeUpdate",		kXMPFiles_AllowsSafeUpdate_K },
	{ "kXMPFiles_NeedsReadOnlyPacket",	kXMPFiles_NeedsReadOnlyPacket_K },
	{ "kXMPFiles_UsesSidecarXMP",		kXMPFiles_UsesSidecarXMP_K },
	{ "kXMPFiles_FolderBasedFormat",	kXMPFiles_FolderBasedFormat_K },
	{ "kXMPFiles_NeedsPreloading",		kXMPFiles_NeedsPreloading_K },

	// Serialize option
	{ "kXMP_OmitPacketWrapper",			kXMP_OmitPacketWrapper_K },
	{ "kXMP_ReadOnlyPacket",			kXMP_ReadOnlyPacket_K },
	{ "kXMP_UseCompactFormat",			kXMP_UseCompactFormat_K },
	{ "kXMP_UseCanonicalFormat",		kXMP_UseCanonicalFormat_K },
	{ "kXMP_IncludeThumbnailPad",		kXMP_IncludeThumbnailPad_K },
	{ "kXMP_ExactPacketLength",			kXMP_ExactPacketLength_K },
	{ "kXMP_OmitAllFormatting",			kXMP_OmitAllFormatting_K },
	{ "kXMP_OmitXMPMetaElement",		kXMP_OmitXMPMetaElement_K },
	{ "kXMP_EncodingMask",				kXMP_EncodingMask_K },
	{ "kXMP_EncodeUTF8",				kXMP_EncodeUTF8_K },
	{ "kXMP_EncodeUTF16Big",			kXMP_EncodeUTF16Big_K },
	{ "kXMP_EncodeUTF16Little",			kXMP_EncodeUTF16Little_K },
	{ "kXMP_EncodeUTF32Big",			kXMP_EncodeUTF32Big_K },
	{ "kXMP_EncodeUTF32Little",			kXMP_EncodeUTF32Little_K }
	
};


XMPAtomsMap* ResourceParser::msXMPAtoms = NULL;
void ResourceParser::clear()
{
	mUID.clear();
	mFileExtensions.clear();
	mFormatIDs.clear();
	mCheckFormat.clear();
	mHandler = FileHandlerSharedPtr();
	mFlags = mSerializeOption = mType = 0;
	mVersion = 0.0;
}

void ResourceParser::addHandler()
{
	if( mUID.empty() || ( mFileExtensions.empty() && mFormatIDs.empty() ) || !isValidHandlerType( mType ) || mFlags == 0 )
	{
		XMP_Throw( "Atleast one of uid, format, ext, typeStr, flags non-valid ...", kXMPErr_Unavailable );
	}
	else
	{
		mHandler->setHandlerFlags( mFlags );
		mHandler->setHandlerType( mType );
		mHandler->setSerializeOption( mSerializeOption );
		mHandler->setOverwriteHandler( mOverwriteHandler );
		if( mVersion != 0.0)
		{
			mHandler->setVersion( mVersion );
		}
		
		// A plugin could define the XMP_FileFormat value in manifest file through keyword "FormatID" and
		// file extensions for NormalHandler and OwningHandler through keyword "Extension".
		// If Both are defined then give priority to FormatID.

		std::set<XMP_FileFormat> formatIDs = mFormatIDs.empty() ? mFileExtensions : mFormatIDs;
		for( std::set<XMP_FileFormat>::const_iterator it = formatIDs.begin(); it != formatIDs.end(); ++it )
		{
			PluginManager::addFileHandler(*it, mHandler);
		}
	}
}

bool ResourceParser::parseElementAttrs( const XML_Node * xmlNode, bool isTopLevel )
{
	XMPAtom nodeAtom = getXMPAtomFromString( xmlNode->name );
	if( nodeAtom == Handler_K )
		this->clear();

	XML_cNodePos currAttr = xmlNode->attrs.begin();
	XML_cNodePos endAttr  = xmlNode->attrs.end();

	for( ; currAttr != endAttr; ++currAttr ) 
	{
		XMP_OptionBits oneflag=0;
		XMP_FileFormat formatID = kXMP_UnknownFile;
		std::string formatStr;
		XMPAtom attrAtom = getXMPAtomFromString( (*currAttr)->name );
		switch(nodeAtom)
		{
		case Handler_K:
			switch(attrAtom)
			{
			case Name_K:
				this->mUID = (*currAttr)->value;
				mHandler = FileHandlerSharedPtr( new FileHandler( this->mUID, 0, 0, mModule ) );
				break;
			case Version_K:
				sscanf( (*currAttr)->value.c_str(), "%lf", &this->mVersion );
				break;
			case HandlerType_K:
				this->mType = getXMPAtomFromString( (*currAttr)->value );
				break;
			case OverwriteHdl_K:
				this->mOverwriteHandler = ( (*currAttr)->value == "true" );
				break;
			default:
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//XMP_Throw( "Invalid Attr in Handler encountered in resource file", kXMPErr_Unavailable );
				break;
			}
			break;
		case CheckFormat_K:
			switch(attrAtom)
			{
			case Offset_K:
				this->mCheckFormat.mOffset = atoi( (*currAttr)->value.c_str() );
				break;
			case Length_K:
				this->mCheckFormat.mLength = atoi( (*currAttr)->value.c_str() );
				break;
			case ByteSeq_K:
				this->mCheckFormat.mByteSeq = (*currAttr)->value;
				break;
			default:
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//XMP_Throw( "Invalid Attr in CheckFormat encountered in resource file", kXMPErr_Unavailable );
				break;
			}
			break;
		case Extension_K:
			switch(attrAtom)
			{
			case Name_K:
				this->mFileExtensions.insert( HandlerRegistry::getInstance().getFileFormat( (*currAttr)->value, true) );
				break;
			default:
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//XMP_Throw( "Invalid Attr in Extension encountered in resource file", kXMPErr_Unavailable );
				break;
			}
			break;
		case FormatID_K:
			switch(attrAtom)
			{
			case Name_K:
				formatStr.assign( (*currAttr)->value );
				
				// Convert string into 4-byte string by appending space '0x20' character.
				for( size_t j=formatStr.size(); j<4; j++ )
					formatStr.push_back(' ');
				
				formatID = GetUns32BE( formatStr.c_str() );
				this->mFormatIDs.insert( formatID );
				break;
			default:
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//XMP_Throw( "Invalid Attr in FormatID encountered in resource file", kXMPErr_Unavailable );
				break;
			}
			break;
		case HandlerFlag_K:
			switch(attrAtom)
			{
			case Name_K:
				oneflag = getHandlerFlag( (*currAttr)->value );
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//if( 0 == oneflag )
				//	XMP_Throw( "Invalid handler flag found in resource file...", kXMPErr_Unavailable );
				this->mFlags |= oneflag;
				break;
			default:
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//XMP_Throw( "Invalid handler flag found in resource file...", kXMPErr_Unavailable );
				break;
			}
			break;
		case SerializeOption_K:
			switch(attrAtom)
			{
			case Name_K:
				oneflag = getSerializeOption( (*currAttr)->value );
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//if( 0 == oneflag )
				//	XMP_Throw( "Invalid serialize option found in resource file...", kXMPErr_Unavailable );
				this->mSerializeOption |= oneflag;
				break;
			default:
				//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
				//XMP_Throw( "Invalid handler flag found in resource file...", kXMPErr_Unavailable );
				break;
			}
			break;
		default:
			break;
		}
	}

	if( nodeAtom == CheckFormat_K )
		mHandler->addCheckFormat( mCheckFormat );

	return (nodeAtom == Handler_K) ? true : false;
}	// parseElementAttrs

void ResourceParser::parseElement( const XML_Node * xmlNode, bool isTopLevel )
{
	bool HandlerFound = this->parseElementAttrs( xmlNode, isTopLevel );
	this->parseElementList ( xmlNode, false );

	if(HandlerFound)
	{
		this->addHandler();
	}
}

void ResourceParser::parseElementList( const XML_Node * xmlParent, bool isTopLevel )
{
	initialize(); //Make sure XMPAtoms are initialized.

	XML_cNodePos currChild = xmlParent->content.begin();
	XML_cNodePos endChild  = xmlParent->content.end();

	for( ; currChild != endChild; ++currChild )
	{
		if( (*currChild)->IsWhitespaceNode() ) continue;
		this->parseElement( *currChild, isTopLevel );
	}
}

bool ResourceParser::initialize()
{
	//Check if already populated.
	if( msXMPAtoms == NULL )
	{
		msXMPAtoms = new XMPAtomsMap();
		
		//Create XMPAtomMap from XMPAtomVec, as we need to do a lot of searching while processing the resource file.
		int count = sizeof(kXMPAtomVec)/sizeof(XMPAtomMapping);
		for( int i=0; i<count; i++ )
		{
			XMP_StringPtr name = kXMPAtomVec[i].name;
			XMPAtom atom = kXMPAtomVec[i].atom;
			(*msXMPAtoms)[name] = atom;
		}
	}
	return true;
}

void ResourceParser::terminate()
{
	delete msXMPAtoms;
	msXMPAtoms = NULL;
}

XMPAtom ResourceParser::getXMPAtomFromString( const std::string & stringAtom )
{
	XMPAtomsMap::const_iterator it = msXMPAtoms->find(stringAtom);

	if( it != msXMPAtoms->end() )
		return it->second;

	return XMPAtomNull;
}

//static 
XMP_OptionBits ResourceParser::getHandlerFlag( const std::string & stringAtom )
{
	XMPAtom atom = getXMPAtomFromString( stringAtom );

	if( !isValidXMPAtom(atom) )
		return 0;

	switch( atom )
	{
	case kXMPFiles_CanInjectXMP_K:
		return kXMPFiles_CanInjectXMP;
	case kXMPFiles_CanExpand_K:
		return kXMPFiles_CanExpand;
	case kXMPFiles_CanRewrite_K:
		return kXMPFiles_CanRewrite;
	case kXMPFiles_PrefersInPlace_K:
		return kXMPFiles_PrefersInPlace;
	case kXMPFiles_CanReconcile_K:
		return kXMPFiles_CanReconcile;
	case kXMPFiles_AllowsOnlyXMP_K:
		return kXMPFiles_AllowsOnlyXMP;
	case kXMPFiles_ReturnsRawPacket_K:
		return kXMPFiles_ReturnsRawPacket;
	case kXMPFiles_HandlerOwnsFile_K:
		return kXMPFiles_HandlerOwnsFile;
	case kXMPFiles_AllowsSafeUpdate_K:
		return kXMPFiles_AllowsSafeUpdate;
	case kXMPFiles_NeedsReadOnlyPacket_K:
		return kXMPFiles_NeedsReadOnlyPacket;
	case kXMPFiles_UsesSidecarXMP_K:
		return kXMPFiles_UsesSidecarXMP;
	case kXMPFiles_FolderBasedFormat_K:
		return kXMPFiles_FolderBasedFormat;
	case kXMPFiles_NeedsPreloading_K:
		return kXMPFiles_NeedsPreloading;
	default:
		//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
		//XMP_Throw( "Invalid PluginhandlerFlag ...", kXMPErr_Unavailable );
		return 0;
	}
}

//static 
XMP_OptionBits ResourceParser::getSerializeOption( const std::string & stringAtom )
{
	XMPAtom atom = getXMPAtomFromString( stringAtom );

	if( !isValidXMPAtom(atom) )
		return 0;

	switch( atom )
	{
	case kXMP_OmitPacketWrapper_K:
		return kXMP_OmitPacketWrapper;
    case kXMP_ReadOnlyPacket_K:
		return kXMP_ReadOnlyPacket;
    case kXMP_UseCompactFormat_K:
		return kXMP_UseCompactFormat;
    case kXMP_UseCanonicalFormat_K:
		return kXMP_UseCanonicalFormat;
    case kXMP_IncludeThumbnailPad_K:
		return kXMP_IncludeThumbnailPad;
    case kXMP_ExactPacketLength_K:
		return kXMP_ExactPacketLength;
    case kXMP_OmitAllFormatting_K:
		return kXMP_OmitAllFormatting;
    case kXMP_OmitXMPMetaElement_K:
		return kXMP_OmitXMPMetaElement;
    case kXMP_EncodingMask_K:
		return kXMP_EncodingMask;
    case kXMP_EncodeUTF8_K:
		return kXMP_EncodeUTF8;
    case kXMP_EncodeUTF16Big_K:
		return kXMP_EncodeUTF16Big;
    case kXMP_EncodeUTF16Little_K:
		return kXMP_EncodeUTF16Little;
    case kXMP_EncodeUTF32Big_K:
		return kXMP_EncodeUTF32Big;
    case kXMP_EncodeUTF32Little_K:
		return kXMP_EncodeUTF32Little;
	default:
		//fix for bug 3565147: Plugin Architecture: If any unknown node is present in the Plugin Manifest then the Plugin is not loaded
		//XMP_Throw( "Invalid Serialize Option ...", kXMPErr_Unavailable );
		return 0;
	}
}

XMP_FileFormat ResourceParser::getPluginFileFormat( const std::string & fileExt, bool AddIfNotFound )
{
	XMP_FileFormat format = kXMP_UnknownFile;

	if( msXMPAtoms )
	{
		XMPAtomsMap::const_iterator iter = msXMPAtoms->find( fileExt );
		if( iter != msXMPAtoms->end() )
		{
			format = iter->second;
		}
		else if( AddIfNotFound )
		{
			std::string formatStr(fileExt);
			MakeUpperCase(&formatStr);
			for( size_t j=formatStr.size(); j<4; j++ )	// Convert "pdf" to "PDF "
				formatStr.push_back(' ');
			format = GetUns32BE( formatStr.c_str() ); //Convert "PDF " into kXMP_PDFFile
			(*msXMPAtoms)[ fileExt ] = format;
		}
	}
	
	return format;
}

} //namespace XMP_PLUGIN
