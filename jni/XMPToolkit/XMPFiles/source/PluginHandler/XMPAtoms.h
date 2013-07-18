// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _H_XMPATOMS
#define _H_XMPATOMS
#include "FileHandler.h"
#include "source/ExpatAdapter.hpp"
#include <set>

namespace XMP_PLUGIN
{

/** @brief An enum required to parse the resource file of plugin.
 */
enum
{
	emptyStr_K = 0, //Empty string
	
	// Mandatory keys in the resource file
	Handler_K,
	Extensions_K,
	Extension_K,
	FormatIDs_K,
	FormatID_K,
	HandlerType_K,
	OverwriteHdl_K,
	HandlerFlags_K,
	HandlerFlag_K,
	SerializeOptions_K,
	SerializeOption_K,
	Version_K,
	CheckFormat_K,
	Name_K,
	Offset_K,
	Length_K,
	ByteSeq_K,

	// Handler types
	NormalHandler_K,
	OwningHandler_K,
	FolderHandler_K,

	// Handler flags
	kXMPFiles_CanInjectXMP_K,
	kXMPFiles_CanExpand_K,
	kXMPFiles_CanRewrite_K,
	kXMPFiles_PrefersInPlace_K,
	kXMPFiles_CanReconcile_K,
	kXMPFiles_AllowsOnlyXMP_K,
	kXMPFiles_ReturnsRawPacket_K,
	kXMPFiles_HandlerOwnsFile_K,
	kXMPFiles_AllowsSafeUpdate_K,
	kXMPFiles_NeedsReadOnlyPacket_K,
	kXMPFiles_UsesSidecarXMP_K,
	kXMPFiles_FolderBasedFormat_K,
	kXMPFiles_NeedsPreloading_K,

    // Serialize option
    kXMP_OmitPacketWrapper_K,
    kXMP_ReadOnlyPacket_K,
    kXMP_UseCompactFormat_K,
    kXMP_UseCanonicalFormat_K,
    kXMP_IncludeThumbnailPad_K,
    kXMP_ExactPacketLength_K,
    kXMP_OmitAllFormatting_K,
    kXMP_OmitXMPMetaElement_K,
    kXMP_EncodingMask_K,
    kXMP_EncodeUTF8_K,
    kXMP_EncodeUTF16Big_K,
    kXMP_EncodeUTF16Little_K,
    kXMP_EncodeUTF32Big_K,
    kXMP_EncodeUTF32Little_K,
	
	//Last element
	lastfinal_K
};

#define XMPAtomNull emptyStr_K

struct StringCompare : std::binary_function<const std::string &, const std::string &, bool>
{
	bool operator() (const std::string & a, const std::string & b) const
	{
		return ( a.compare(b) < 0 );
	}
};
typedef std::map<std::string, XMPAtom,			StringCompare>		XMPAtomsMap;


/** @class ResourceParser
 *  @brief Class to parse resource file of the plugin.
 */
class ResourceParser
{
public:
	ResourceParser(ModuleSharedPtr module)
		: mModule(module), mFlags(0), mSerializeOption(0), mType(0), mVersion(0.0), mOverwriteHandler(false) {}

	/** 
	 *  Initialize the XMPAtoms which will be used in parsing resource files.
	 *  @return true on success.
	 */
	static bool initialize();
	static void terminate();
	
	/** @brief Return file format corresponding to file extension \a fileExt.
	 *
	 *  It is similar to GetXMPFileFormat except that it also searches in PluginManager's 
	 *  private file formats. If the extension is not a public file format and \a AddIfNotFound is true
	 *  then it also adds the private fileformat. The priavte 4-byte file format is created by converting 
	 *  extension to upper case and appending space '0x20' to make it 4-byte. For i.e
	 *  "pdf"  -> 'PDF ' (0x50444620)
	 *  "tmp"  -> 'TMP ' (0x54415020)
	 *  "temp" -> 'TEMP' (0x54454150)
	 * 
	 *  @param fileExt Extension string of the file. For i.e "pdf"
	 *  @param AddIfNotFound If AddIfNotFound is true and public format is not found then 
	 *  priavte format is added.
	 *  @return File format correspoding to file extension \a fileExt.
	 */
	static XMP_FileFormat getPluginFileFormat( const std::string & fileExt, bool AddIfNotFound );

	/** 
	 *  Parse the XML node's children recursively.
	 *  @param xmlParent XMLNode which will be parsed. 
	 *  @return Void.
	 */
	void parseElementList( const XML_Node * xmlParent, bool isTopLevel );

private:
	void clear();		
	void addHandler();

	/** 
	 *  Parse the XML node's attribute.
	 *  @param xmlNode XMLNode which will be parsed. 
	 *  @return true if xmlNode's name is Handler_K.
	 */
	bool parseElementAttrs( const XML_Node * xmlNode, bool isTopLevel );
	
	/** 
	 *  Parse the XML node.
	 *  @param xmlNode XMLNode which will be parsed. 
	 *  @return Void.
	 */
	void parseElement( const XML_Node * xmlNode, bool isTopLevel );

	/** 
	 *  Return XMPAtom corresponding to string.
	 *  @param stringAtom std::string whose XMPAtom will be return.
	 *  @return XMPAtom corresponding to string.
	 */
	static XMPAtom getXMPAtomFromString( const std::string & stringAtom );
	
	/** 
	 *  Return Handler flag corresponding to XMPAtom string.
	 *  @param stringAtom Handler flag string
	 *  @return Handler flag.
	 */
	static XMP_OptionBits getHandlerFlag( const std::string & stringAtom );

	/** 
	 *  Return Serialize option corresponding to XMPAtom string.
	 *  @param stringAtom Handler flag string
	 *  @return Serialize option.
	 */
	static XMP_OptionBits getSerializeOption( const std::string & stringAtom );

	static inline bool isValidXMPAtom( XMPAtom atom )
	{
		return ( atom > emptyStr_K && atom < lastfinal_K );
	}

	static inline bool isValidHandlerType(XMPAtom atom)
	{
		return ( atom >= NormalHandler_K && atom <= FolderHandler_K );
	}

	ModuleSharedPtr       mModule;
	std::string           mUID;
	FileHandlerType       mType;
	XMP_OptionBits        mFlags;
	XMP_OptionBits        mSerializeOption;
	double                mVersion;
	bool				  mOverwriteHandler;
	CheckFormat           mCheckFormat;
	std::set<XMP_FileFormat> mFileExtensions;
	std::set<XMP_FileFormat> mFormatIDs;
	FileHandlerSharedPtr mHandler;

	static XMPAtomsMap *  msXMPAtoms;
};

} //namespace XMP_PLUGIN
#endif	// _H_XMPATOMS
