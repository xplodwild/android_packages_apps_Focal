#ifndef __PostScript_Handler_hpp__
#define __PostScript_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/FormatSupport/PostScript_Support.hpp"



// =================================================================================================
/// \file PostScript_Handler.hpp
/// \brief File format handler for PostScript and EPS files.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * PostScript_MetaHandlerCTor ( XMPFiles * parent );

extern bool PostScript_CheckFormat ( XMP_FileFormat format,
									 XMP_StringPtr  filePath,
			                         XMP_IO *       fileRef,
			                         XMPFiles *     parent );

static const XMP_OptionBits kPostScript_HandlerFlags = (
		kXMPFiles_CanInjectXMP
		|kXMPFiles_CanExpand
		|kXMPFiles_CanRewrite
		|kXMPFiles_PrefersInPlace
		|kXMPFiles_CanReconcile
		|kXMPFiles_AllowsOnlyXMP
		|kXMPFiles_ReturnsRawPacket
		|kXMPFiles_AllowsSafeUpdate 
		|kXMPFiles_CanNotifyProgress );

class PostScript_MetaHandler : public XMPFileHandler
{
public:

	PostScript_MetaHandler ( XMPFiles * parent );
	~PostScript_MetaHandler();

	void CacheFileData(); 
	void UpdateFile ( bool doSafeUpdate );
	void ProcessXMP ( );
	void WriteTempFile ( XMP_IO* tempRef );

	int psHint;
	/* Structure used to keep
		Track of Tokens in 
		EPS files
	*/
	struct TokenLocation{
		//offset from the begining of the file 
		// at which the token string starts
		XMP_Int64 offsetStart;
		//Total length of the token string
		XMP_Int64 tokenlen;
		TokenLocation():offsetStart(-1),tokenlen(0)
		{}
	};
protected:
	//Determines the postscript hint in the DSC comments
	int FindPostScriptHint();

	// Helper methods to get the First or the Last packet from the 
	// PS file based upon the PostScript hint that is present in the PS file
	bool FindFirstPacket();
	bool FindLastPacket();

	
	//Facilitates read time reconciliation of PS native metadata
	void ReconcileXMP( const std::string &xmpStr, std::string *outStr );

	//Facilitates reading of XMP packet , if one exists
	void ReadXMPPacket ( std::string & xmpPacket);

	// Parses the PS file to record th epresence and location of 
	// XMP packet and native metadata in the file
	void ParsePSFile();

	// Helper function to record the native metadata key/avlue pairs 
	// when parsing the PS file
	void RegisterKeyValue(std::string& key, std::string& value);

	// Helper Function to record the location and length of the Tokens
	// in the opened PS file
	void setTokenInfo(TokenFlag tFlag,XMP_Int64 offset,XMP_Int64 length);

	// Getter to get the location of a token ina PS file.
	TokenLocation& getTokenInfo(TokenFlag tFlag);

	//modifies the Binary Header of a PS file as per the modifications
	void modifyHeader(XMP_IO* fileRef,XMP_Int64 extrabytes,XMP_Int64 offset );

	//Extract the values for different DSC comments
	bool ExtractDSCCommentValue(IOBuffer &ioBuf,NativeMetadataIndex index);

	//Extract value for ADO_ContainsXMP Comment
	bool ExtractContainsXMPHint(IOBuffer &ioBuf,XMP_Int64 containsXMPStartpos);
	
	//Extract values from DocInfo Dict
	bool ExtractDocInfoDict(IOBuffer &ioBuf);

	//Determine the update method to be used 
	UpdateMethod DetermineUpdateMethod(std::string & outStr);
	void DetermineInsertionOffsets(XMP_Int64& ADOhintOffset,XMP_Int64& InjectData1Offset,
							XMP_Int64& InjectData3Offset);
	//Different update methods
	void InplaceUpdate (std::string &outStr,XMP_IO* &tempRef, bool doSafeUpdate);
	void ExpandingSFDFilterUpdate (std::string &outStr,XMP_IO* &tempRef, bool doSafeUpdate );
	void InsertNewUpdate ( std::string &outStr,XMP_IO* &tempRef, bool doSafeUpdate );
private:
	//Flag tracks DSC comments 
	XMP_Uns32 dscFlags;
	//Flag tracks DOCINFO keys
	XMP_Uns32 docInfoFlags;
	//stores the native metadata values. Index values an enum var NativeMetadataIndex
	std::string nativeMeta[kPS_MaxNativeIndexValue];
	//all offsets are to the end of the comment after atleast one whitespace
	TokenLocation fileTokenInfo[25];
	//Indicates the presence of both XMP hint and XMP
	bool containsXMPHint;
	//Keeps track whether a PS or EPS
	XMP_FileFormat fileformat;
	//keep the first packet info
	XMP_PacketInfo firstPacketInfo;	
	//keep the last packet info
	XMP_PacketInfo lastPacketInfo;	

};	// PostScript_MetaHandler

// =================================================================================================

#endif /* __PostScript_Handler_hpp__ */
