// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"
#include "source/IOUtils.hpp"

#include "XMPFiles/source/FileHandlers/XDCAM_Handler.hpp"
#include "XMPFiles/source/FormatSupport/XDCAM_Support.hpp"
#include "XMPFiles/source/FormatSupport/PackageFormat_Support.hpp"
#include "third-party/zuid/interfaces/MD5.h"

using namespace std;

// =================================================================================================
/// \file XDCAM_Handler.cpp
/// \brief Folder format handler for XDCAM.
///
/// This handler is for the XDCAM video format. This is a pseudo-package, visible files but with a very
/// well-defined layout and naming rules. There are 2 different layouts for XDCAM, called FAM and SAM.
/// The FAM layout is used by "normal" XDCAM devices. The SAM layout is used by XDCAM-EX devices.
///
/// A typical FAM layout looks like (note mixed case for the nested folders):
///
/// .../MyMovie/
/// 	INDEX.XML
/// 	DISCMETA.XML
/// 	MEDIAPRO.XML
/// 	General/
/// 		unknown files
/// 	Clip/
/// 		C0001.MXF
/// 		C0001M01.XML
/// 		C0001M01.XMP
/// 		C0002.MXF
/// 		C0002M01.XML
/// 		C0002M01.XMP
/// 	Sub/
/// 		C0001S01.MXF
/// 		C0002S01.MXF
/// 	Edit/
/// 		E0001E01.SMI
/// 		E0001M01.XML
/// 		E0002E01.SMI
/// 		E0002M01.XML
///
/// A typical FAM XMPilot layout looks like (note mixed case for the nested folders):
///
/// .../MyMovie/
/// 	DISCMETA.XML
/// 	MEDIAPRO.XML
/// 	General/
/// 	Clip/
/// 		Office_0001.MXF
/// 		Office_0001M01.XML
/// 		Office_0001M01.XMP
/// 		Office_0002.MXF
/// 		Office_0002M01.XML
/// 		Office_0002M01.XMP
/// 	Sub/
/// 		Office_0001S01.MXF
/// 		Office_0002S01.MXF
/// 	Edit/
///     UserData/
///         unknown files
///
/// A typical FAM XDCAM Memory SxS layout looks like (note mixed case for the nested folders):
///
/// .../MyMovie/
/// 	DISCMETA.XML
/// 	MEDIAPRO.XML
///     CUEUP.XML
/// 	General/
/// 	Clip/
/// 		C0001.MXF
/// 		C0001M01.XML
/// 		C0001M01.XMP
/// 		C0001R01.BIM
/// 		C0002.MXF
/// 		C0002M01.XML
/// 		C0002M01.XMP
/// 		C0001R01.BIM
/// 	Sub/
/// 		C0001S01.MXF
/// 		C0002S01.MXF
/// 	Edit/
///		Take/
/// 		T0001.SMI
/// 		T0001M01.XML
///     UserData/
///
/// A typical SAM layout looks like:
///
/// .../MyMovie/
/// 	GENERAL/
/// 		unknown files
/// 	PROAV/
/// 		INDEX.XML
/// 		INDEX.BUP
/// 		DISCMETA.XML
/// 		DISCINFO.XML
/// 		DISCINFO.BUP
/// 		CLPR/
/// 			C0001/
/// 				C0001C01.SMI
/// 				C0001V01.MXF
/// 				C0001A01.MXF
/// 				C0001A02.MXF
/// 				C0001R01.BIM
/// 				C0001I01.PPN
/// 				C0001M01.XML
/// 				C0001M01.XMP
/// 				C0001S01.MXF
/// 			C0002/
/// 				...
/// 		EDTR/
/// 			E0001/
/// 				E0001E01.SMI
/// 				E0001M01.XML
/// 			E0002/
/// 				...
///
/// Note that the Sony documentation uses the folder names "General", "Clip", "Sub", and "Edit". We
/// use all caps here. Common code has already shifted the names, we want to be case insensitive.
///
/// From the user's point of view, .../MyMovie contains XDCAM stuff, in this case 2 clips whose raw
/// names are C0001 and C0002. There may be mapping information for nicer clip names to the raw
/// names, but that can be ignored for now. Each clip is stored as a collection of files, each file
/// holding some specific aspect of the clip's data.
///
/// The XDCAM handler operates on clips. The path from the client of XMPFiles can be either a logical
/// clip path, like ".../MyMovie/C0001", or a full path to one of the files. In the latter case the
/// handler must figure out the intended clip, it must not blindly use the named file.
///
/// Once the XDCAM structure and intended clip are identified, the handler only deals with the .XMP
/// and .XML files in the CLIP or CLPR/<clip> folders. The .XMP file, if present, contains the XMP
/// for the clip. The .XML file must be present to define the existance of the clip. It contains a
/// variety of information about the clip, including some legacy metadata.
///
// =================================================================================================

// =================================================================================================
// XDCAM_CheckFormat
// =================
//
// This version does fairly simple checks. The top level folder (.../MyMovie) must have exactly 1
// child, a folder called CONTENTS. This must have a subfolder called CLIP. It may also have
// subfolders called VIDEO, AUDIO, ICON, VOICE, and PROXY. Any mixture of these additional folders
// is allowed, but no other children are allowed in CONTENTS. The CLIP folder must contain a .XML
// file for the desired clip. The name checks are case insensitive.
//
// The state of the string parameters depends on the form of the path passed by the client. If the
// client passed a logical clip path, like ".../MyMovie/C0001", the parameters are:
//   rootPath   - ".../MyMovie"
//   gpName     - empty
//   parentName - empty
//   leafName   - "C0001"
//
// If the client passed a FAM file path, like ".../MyMovie/Edit/E0001E01.SMI", they are:
//   rootPath   - "..."
//   gpName     - "MyMovie"
//   parentName - "EDIT"	(common code has shifted the case)
//   leafName   - "E0001E01"
//
// If the client passed a SAM file path, like ".../MyMovie/PROAV/CLPR/C0001/C0001A02.MXF", they are:
//   rootPath   - ".../MyMovie/PROAV"
//   gpName     - "CLPR"
//   parentName - "C0001"
//   leafName   - "C0001A02"
//
// For both FAM and SAM the leading character of the leafName for an existing file might be coerced
// to 'C' to form the logical clip name. And suffix such as "M01" must be removed for FAM. We don't
// need to worry about that for SAM, that uses the <clip> folder name.

// ! The FAM format supports general clip file names through an ALIAS.XML mapping file. The simple
// ! existence check has an edge case bug, left to be fixed later. If the ALIAS.XML file exists, but
// ! some of the clips still have "raw" names, and we're passed an existing file path in the EDIT
// ! folder, we will fail to do the leading 'E' to 'C' coercion. We might also erroneously remove a
// ! suffix from a mapped essence file with a name like ClipX01.MXF.

// ! The common code has shifted the gpName, parentName, and leafName strings to uppercase. It has
// ! also made sure that for a logical clip path the rootPath is an existing folder, and that the
// ! file exists for a full file path.

bool XDCAM_CheckFormat ( XMP_FileFormat format,
						 const std::string & _rootPath,
						 const std::string & _gpName,
						 const std::string & parentName,
						 const std::string & leafName,
						 XMPFiles * parent )
{
	std::string rootPath = _rootPath;	// ! Need tweaking in the existing file cases (FAM and SAM).
	std::string gpName   = _gpName;

	bool isFAM = false;

	std::string tempPath, childName;

	std::string clipName = leafName;

	// Do some basic checks on the root path and component names. Decide if this is FAM or SAM.

	if ( gpName.empty() != parentName.empty() ) return false;	// Must be both empty or both non-empty.

	if ( gpName.empty() ) {

		// This is the logical clip path case. Just look for PROAV to see if this is FAM or SAM.
		if ( Host_IO::GetChildMode ( rootPath.c_str(), "PROAV" ) != Host_IO::kFMode_IsFolder ) isFAM = true;

	} else {

		// This is the existing file case. See if this is FAM or SAM, tweak the clip name as needed.

		if ( (parentName == "CLIP") || (parentName == "EDIT") || (parentName == "SUB") ) {
			// ! The standard says Clip/Edit/Sub, but the caller has already shifted to upper case.
			isFAM = true;
		} else if ( (gpName != "CLPR") && (gpName != "EDTR") ) {
			return false;
		}

		if ( isFAM ) {

			// Put the proper root path together. Clean up the clip name if needed.

			if ( ! rootPath.empty() ) rootPath += kDirChar;
			rootPath += gpName;
			gpName.erase();

			// XMPilot has no ALIAS.XML, but does have a UserData folder, don't change the first
			// letter of the clip name for XMPilot.
			if ( (Host_IO::GetChildMode ( rootPath.c_str(), "ALIAS.XML" ) != Host_IO::kFMode_IsFile) &&
				 (Host_IO::GetChildMode ( rootPath.c_str(), "UserData" ) != Host_IO::kFMode_IsFolder) ) {
				clipName[0] = 'C';	// ! See notes above about pending bug.
			}

			if ( clipName.size() > 3 ) {
				size_t clipMid = clipName.size() - 3;
				char c1 = clipName[clipMid];
				char c2 = clipName[clipMid+1];
				char c3 = clipName[clipMid+2];
				if ( ('A' <= c1) && (c1 <= 'Z') &&
					 ('0' <= c2) && (c2 <= '9') && ('0' <= c3) && (c3 <= '9') ) {
					clipName.erase ( clipMid );
				}
			}

		} else {

			// Fix the clip name. Check for and strip the "PROAV" suffix on the root path.

			clipName = parentName;	// ! We have a folder with the (almost) exact clip name.
			clipName[0] = 'C';

			std::string proav;
			XIO::SplitLeafName ( &rootPath, &proav );
			MakeUpperCase ( &proav );
			if ( (rootPath.empty()) || (proav != "PROAV") ) return false;

		}

	}

	// Make sure the general XDCAM package structure is legit. Set tempPath as a bogus path of the
	// form <root>/<FAM-or-SAM>/<clip>, e.g. ".../MyMovie/FAM/C0001". This is passed the handler via
	// the tempPtr hackery.

	if ( isFAM ) {

		if ( (format != kXMP_XDCAM_FAMFile) && (format != kXMP_UnknownFile) ) return false;

		tempPath = rootPath;

		// XMPilot does not have INDEX.XML but does have UserData.
        if ( (Host_IO::GetChildMode ( tempPath.c_str(), "INDEX.XML" ) != Host_IO::kFMode_IsFile) &&
			 !((Host_IO::GetChildMode ( rootPath.c_str(), "UserData" ) == Host_IO::kFMode_IsFolder)
			 // Changes introduced by Sony for XDCAM Memory SxS format in the FAM file structure are 
			 //	1) There is no INDEX.XML in the root directory for XDCAM Memory SxS.
			 //	2) There is a new Take folder(similar to XDCAMEX) in the root directory.
			 || (Host_IO::GetChildMode ( tempPath.c_str(), "Take" ) == Host_IO::kFMode_IsFolder))) return false;
		if ( Host_IO::GetChildMode ( tempPath.c_str(), "DISCMETA.XML" ) != Host_IO::kFMode_IsFile ) return false;
		if ( Host_IO::GetChildMode ( tempPath.c_str(), "MEDIAPRO.XML" ) != Host_IO::kFMode_IsFile ) return false;

		tempPath += kDirChar;
		tempPath += "Clip";	// ! Yes, mixed case.
		tempPath += kDirChar;
		tempPath += clipName;
		tempPath += "M01.XML";
		if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFile ) return false;

		tempPath = rootPath;
		tempPath += kDirChar;
		tempPath += "FAM";
		tempPath += kDirChar;
		tempPath += clipName;

	} else {

		if ( (format != kXMP_XDCAM_SAMFile) && (format != kXMP_UnknownFile) ) return false;

		// We already know about the PROAV folder, just check below it.

		tempPath = rootPath;
		tempPath += kDirChar;
		tempPath += "PROAV";

		if ( Host_IO::GetChildMode ( tempPath.c_str(), "INDEX.XML" ) != Host_IO::kFMode_IsFile ) return false;
		if ( Host_IO::GetChildMode ( tempPath.c_str(), "DISCMETA.XML" ) != Host_IO::kFMode_IsFile ) return false;
		if ( Host_IO::GetChildMode ( tempPath.c_str(), "DISCINFO.XML" ) != Host_IO::kFMode_IsFile ) return false;
		if ( Host_IO::GetChildMode ( tempPath.c_str(), "CLPR" ) != Host_IO::kFMode_IsFolder ) return false;

		tempPath += kDirChar;
		tempPath += "CLPR";
		tempPath += kDirChar;
		tempPath += clipName;
		if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFolder ) return false;

		tempPath += kDirChar;
		tempPath += clipName;
		tempPath += "M01.XML";
		if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFile ) return false;

		tempPath = rootPath;
		tempPath += kDirChar;
		tempPath += "SAM";
		tempPath += kDirChar;
		tempPath += clipName;

	}

	// Save the pseudo-path for the handler object. A bit of a hack, but the only way to get info
	// from here to there.

	size_t pathLen = tempPath.size() + 1;	// Include a terminating nul.
	parent->tempPtr = malloc ( pathLen );
	if ( parent->tempPtr == 0 ) XMP_Throw ( "No memory for XDCAM clip info", kXMPErr_NoMemory );
	memcpy ( parent->tempPtr, tempPath.c_str(), pathLen );	// AUDIT: Safe, allocated above.

	return true;

}	// XDCAM_CheckFormat

// =================================================================================================

static void* CreatePseudoClipPath ( const std::string & clientPath ) {

	// Used to create the clip pseudo path when the CheckFormat function is skipped.
	
	std::string pseudoPath = clientPath;
	std::string clipName;
	bool isSAM;
	
	size_t pathLen;
	void* tempPtr = 0;
	
	if ( ! Host_IO::Exists ( pseudoPath.c_str() ) ) {

		// This is the logical clip path case. Look for PROAV to see if this is FAM or SAM.

		XIO::SplitLeafName ( &pseudoPath, &clipName );	// Extract the logical clip name, no extension.
		isSAM = ( Host_IO::GetChildMode ( pseudoPath.c_str(), "PROAV" ) == Host_IO::kFMode_IsFolder );
	
	} else {
	
		// The client passed a physical path. We have separate cases for FAM and SAM. If the last
		// folder, the parent of the file, is Clip, Edit, or Sub (ignoring case) then this is FAM
		// and things are a bit messy. For SAM, the parent folder is the almost clip name.
		
		std::string parentName, ignored;
		
		XIO::SplitLeafName ( &pseudoPath, &clipName );	// Extract the logical clip name.
		XIO::SplitFileExtension ( &clipName, &ignored );

		XIO::SplitLeafName ( &pseudoPath, &parentName );
		MakeUpperCase ( &parentName );
		isSAM = ( (parentName != "CLIP") && (parentName != "EDIT") && (parentName != "SUB") );
		
		if ( isSAM ) {
		
			// SAM is easy, the parent name is almost the clip name, the first letter gets coerced
			// to 'C'. There are 2 other folders to remove from the path.

			clipName = parentName;
			clipName[0] = 'C';
			XIO::SplitLeafName ( &pseudoPath, &ignored );	// Remove the 2 intermediate folder levels.
			XIO::SplitLeafName ( &pseudoPath, &ignored );

		} else {
		
			// FAM is a bit messy, study the comments and code of XDCAM_CheckFormat for details.

			if ( Host_IO::GetChildMode ( pseudoPath.c_str(), "ALIAS.XML" ) != Host_IO::kFMode_IsFile ) {
				clipName[0] = 'C';	// ! See notes in XDCAM_CheckFormat about pending bug.
			}

			if ( clipName.size() > 3 ) {
				size_t clipMid = clipName.size() - 3;
				char c1 = clipName[clipMid];
				char c2 = clipName[clipMid+1];
				char c3 = clipName[clipMid+2];
				if ( ('A' <= c1) && (c1 <= 'Z') &&
					 ('0' <= c2) && (c2 <= '9') && ('0' <= c3) && (c3 <= '9') ) {
					clipName.erase ( clipMid );
				}
			}
		
		}
	
	}
	
	pseudoPath += kDirChar;
	if ( isSAM ) {
		pseudoPath += "SAM";
	} else {
		pseudoPath += "FAM";
	}
	pseudoPath += kDirChar;
	pseudoPath += clipName;

	pathLen = pseudoPath.size() + 1;	// Include a terminating nul.
	tempPtr = malloc ( pathLen );
	if ( tempPtr == 0 ) XMP_Throw ( "No memory for XDCAM clip info", kXMPErr_NoMemory );
	memcpy ( tempPtr, pseudoPath.c_str(), pathLen );
	
	return tempPtr;

}	// CreatePseudoClipPath

// =================================================================================================
// XDCAM_MetaHandlerCTor
// =====================

XMPFileHandler * XDCAM_MetaHandlerCTor ( XMPFiles * parent )
{
	return new XDCAM_MetaHandler ( parent );

}	// XDCAM_MetaHandlerCTor


// =================================================================================================
// XDCAM_MetaHandler::SetSidecarPath
// ====================================
void XDCAM_MetaHandler::SetSidecarPath()
{
	// Here, we set the appropriate sidecar name for this format.
	// If, the format if XMPilot (no INDEX.XML but UserData folder present) or
	// SxS (no INDEX.XML but Take folder present) then sidecar name will be
	// old name used by MXFHandler i.e, {clipName}.MXF.xmp or {clipname}.mxf.xmp
	// For all other cases, new side car name i.e, {clipname}M01.XMP will be used.
	
	try
	{
		if(this->isFAM &&  Host_IO::GetChildMode ( this->rootPath.c_str(), "INDEX.XML" ) != Host_IO::kFMode_IsFile &&
			(Host_IO::GetChildMode ( rootPath.c_str(), "UserData" ) == Host_IO::kFMode_IsFolder 
			|| Host_IO::GetChildMode ( this->rootPath.c_str(), "Take" ) == Host_IO::kFMode_IsFolder) )
		{
			// this is either XMPilot or SxS format.
			XMP_VarString mxfFilePath;
			if(MakeClipFilePath ( &mxfFilePath , ".MXF", true ) ||  MakeClipFilePath ( &mxfFilePath , ".mxf", true ) )
			{
				Host_IO::FileRef hostRef = Host_IO::Open ( mxfFilePath.c_str(), Host_IO::openReadOnly );
				if ( hostRef != Host_IO::noFileRef )
				{
						
					XMPFiles_IO mxfFile ( hostRef, mxfFilePath.c_str() , Host_IO::openReadOnly );	

					if ( Host_IO::Length(hostRef) >= 16 )
					{
						XMP_Uns8 buffer[16];
						Host_IO::Seek(hostRef, 0, kXMP_SeekFromStart);
						XMP_Uns32 readBytes = Host_IO::Read(hostRef, buffer, 16 );	

						if ( ( readBytes == 16 ) && 
							( GetUns32BE(&buffer[0]) == 0x060E2B34 ) &&
							( GetUns32BE(&buffer[4]) == 0x02050101 ) &&
							( GetUns32BE(&buffer[8]) == 0x0D010201 ) &&
							( ( GetUns32BE(&buffer[12]) & 0xFFFF00FF ) == 0x01020000 )
							)
						{
							// If cached MXF file name is present then use it otherwise 
							// side car generated on case insensitive OS may not be read on case sensitive OS.
							// For example, if file name is X.MXF then windows says X.mxf is same as X.MXF so
							// we may land up generating side car name as X.mxf.xmp which will not be read on 
							// Mac which will search specifically for X.MXF.xmp
							XMP_VarString filePath = this->parent->GetFilePath();
							XMP_VarString ext;
							XIO::SplitFileExtension(&filePath, &ext);
							if(ext == "MXF" || ext == "mxf")
							{
								this->sidecarPath = this->parent->GetFilePath() + ".xmp";
							}
							else
							{
								this->sidecarPath = mxfFilePath + ".xmp";
							}
						}
					}
				}
			}
		}
	}
	catch( ... )
	{
		// Use new side car name.
	}
	if(this->sidecarPath.empty())
	{
		MakeClipFilePath ( &this->sidecarPath , "M01.XMP", false ) ;
	}
}// XDCAM_MetaHandler::SetSidecarPath

// =================================================================================================
// XDCAM_MetaHandler::XDCAM_MetaHandler
// ====================================

XDCAM_MetaHandler::XDCAM_MetaHandler ( XMPFiles * _parent ) : isFAM(false), expat(0)
{

	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kXDCAM_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	// Extract the root path, clip name, and FAM/SAM flag from tempPtr.

	if ( this->parent->tempPtr == 0 ) {
		// The CheckFormat call might have been skipped.
		this->parent->tempPtr = CreatePseudoClipPath ( this->parent->GetFilePath() );
	}

	this->rootPath.assign ( (char*) this->parent->tempPtr );
	free ( this->parent->tempPtr );
	this->parent->tempPtr = 0;

	XIO::SplitLeafName ( &this->rootPath, &this->clipName );

	std::string temp;
	XIO::SplitLeafName ( &this->rootPath, &temp );
	XMP_Assert ( (temp == "FAM") || (temp == "SAM") );
	if ( temp == "FAM" ) this->isFAM = true;
	// backward compatibility ensured for XMPilot Clips
	// XMPilot is FAM
	this->SetSidecarPath();
	XMP_Assert ( this->isFAM ? (this->parent->format == kXMP_XDCAM_FAMFile) : (this->parent->format == kXMP_XDCAM_SAMFile) );

}	// XDCAM_MetaHandler::XDCAM_MetaHandler

// =================================================================================================
// XDCAM_MetaHandler::~XDCAM_MetaHandler
// =====================================

XDCAM_MetaHandler::~XDCAM_MetaHandler()
{

	this->CleanupLegacyXML();
	if ( this->parent->tempPtr != 0 ) {
		free ( this->parent->tempPtr );
		this->parent->tempPtr = 0;
	}

}	// XDCAM_MetaHandler::~XDCAM_MetaHandler

// =================================================================================================
// XDCAM_MetaHandler::MakeClipFilePath
// ===================================

bool XDCAM_MetaHandler::MakeClipFilePath ( std::string * path, XMP_StringPtr suffix, bool checkFile /* = false */ )
{

	*path = this->rootPath;
	*path += kDirChar;

	if ( this->isFAM ) {
		*path += "Clip";	// ! Yes, mixed case.
	} else {
		*path += "PROAV";
		*path += kDirChar;
		*path += "CLPR";
		*path += kDirChar;
		*path += this->clipName;
	}

	*path += kDirChar;
	*path += this->clipName;
	*path += suffix;
	
	if ( ! checkFile ) return true;
	return Host_IO::Exists ( path->c_str() );

}	// XDCAM_MetaHandler::MakeClipFilePath

// =================================================================================================
// XDCAM_MetaHandler::MakeMediaproPath
// ===================================

bool XDCAM_MetaHandler::MakeMediaproPath ( std::string * path, bool checkFile /* = false */ )
{

	*path = this->rootPath;
	*path += kDirChar;
	*path += "MEDIAPRO.XML";
	
	if ( ! checkFile ) return true;
	return Host_IO::Exists ( path->c_str() );

}	// XDCAM_MetaHandler::MakeMediaproPath

// =================================================================================================
// XDCAM_MetaHandler::MakeLegacyDigest
// ===================================

// *** Early hack version.

#define kHexDigits "0123456789ABCDEF"

void XDCAM_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
{
	digestStr->erase();
	if ( this->clipMetadata == 0 ) return;	// Bail if we don't have any legacy XML.
	XMP_Assert ( this->expat != 0 );

	XMP_StringPtr xdcNS = this->xdcNS.c_str();
	XML_NodePtr legacyContext, legacyProp;

	legacyContext = this->clipMetadata->GetNamedElement ( xdcNS, "Access" );
	if ( legacyContext == 0 ) return;

	MD5_CTX    context;
	unsigned char digestBin [16];
	MD5Init ( &context );

	legacyProp = legacyContext->GetNamedElement ( xdcNS, "Creator" );
	if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() && (! legacyProp->content.empty()) ) {
		const XML_Node * xmlValue = legacyProp->content[0];
		MD5Update ( &context, (XMP_Uns8*)xmlValue->value.c_str(), (unsigned int)xmlValue->value.size() );
	}

	legacyProp = legacyContext->GetNamedElement ( xdcNS, "CreationDate" );
	if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() && (! legacyProp->content.empty()) ) {
		const XML_Node * xmlValue = legacyProp->content[0];
		MD5Update ( &context, (XMP_Uns8*)xmlValue->value.c_str(), (unsigned int)xmlValue->value.size() );
	}

	legacyProp = legacyContext->GetNamedElement ( xdcNS, "LastUpdateDate" );
	if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() && (! legacyProp->content.empty()) ) {
		const XML_Node * xmlValue = legacyProp->content[0];
		MD5Update ( &context, (XMP_Uns8*)xmlValue->value.c_str(), (unsigned int)xmlValue->value.size() );
	}

	MD5Final ( digestBin, &context );

	char buffer [40];
	for ( int in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digestBin[in];
		buffer[out]   = kHexDigits [ byte >> 4 ];
		buffer[out+1] = kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;
	digestStr->append ( buffer );

}	// XDCAM_MetaHandler::MakeLegacyDigest

// =================================================================================================
// P2_MetaHandler::CleanupLegacyXML
// ================================

void XDCAM_MetaHandler::CleanupLegacyXML()
{

	if ( this->expat != 0 ) { delete ( this->expat ); this->expat = 0; }

	clipMetadata = 0;	// ! Was a pointer into the expat tree.

}	// XDCAM_MetaHandler::CleanupLegacyXML

void  XDCAM_MetaHandler::readXMLFile( XMP_StringPtr filePath, ExpatAdapter* &expat )
{
	Host_IO::FileRef hostRef = Host_IO::Open ( filePath, Host_IO::openReadOnly );
	if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
	XMPFiles_IO xmlFile ( hostRef, filePath, Host_IO::openReadOnly );

	expat = XMP_NewExpatAdapter ( ExpatAdapter::kUseLocalNamespaces );
	if ( expat == 0 ) XMP_Throw ( "XDCAM_MetaHandler: Can't create Expat adapter", kXMPErr_NoMemory );

	XMP_Uns8 buffer [64*1024];
	while ( true ) {
		XMP_Int32 ioCount = xmlFile.Read ( buffer, sizeof(buffer) );
		if ( ioCount == 0 ) break;
		expat->ParseBuffer ( buffer, ioCount, false /* not the end */ );
	}
	expat->ParseBuffer ( 0, 0, true );	// End the parse.

	xmlFile.Close();
}

// =================================================================================================
// XDCAM_MetaHandler::GetFileModDate
// =================================

static inline bool operator< ( const XMP_DateTime & left, const XMP_DateTime & right ) {
	int compare = SXMPUtils::CompareDateTime ( left, right );
	return (compare < 0);
}

bool XDCAM_MetaHandler::GetFileModDate ( XMP_DateTime * modDate )
{

	// The XDCAM FAM locations of metadata:
	//	MEDIAPRO.XML	// Has non-XMP metadata.
	//	Clip:
	//		C0001_50i_DVCAM_43_4chM01.XML	// Has non-XMP metadata.
	//		C0001_50i_DVCAM_43_4chM01.XMP

	// The XDCAM SAM locations of metadata:
	//	PROAV:
	//		CLPR:
	//			C0001:
	//				C0001M01.XML	// Has non-XMP metadata.
	//				C0001M01.XMP

	bool ok, haveDate = false;
	std::string fullPath;
	XMP_DateTime oneDate, junkDate;
	if ( modDate == 0 ) modDate = &junkDate;

	std::string mediaproPath;
	ok = MakeMediaproPath ( &mediaproPath, true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( mediaproPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	ok = this->MakeClipFilePath ( &fullPath, "M01.XML", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	ok = this->MakeClipFilePath ( &fullPath, "M01.XMP", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	return haveDate;

}	// XDCAM_MetaHandler::GetFileModDate


// =================================================================================================
// XDCAM_MetaHandler::GetClipUmid
// ==============================
bool XDCAM_MetaHandler::GetClipUmid ( std::string &clipUmid ) 
{
	std::string clipInfoPath;
	ExpatAdapter* clipInfoExpat = 0 ;
	bool umidFound = false;
	XMP_StringPtr nameSpace = 0;
	try {
		this->MakeClipFilePath ( &clipInfoPath, "C01.SMI" ) ;
		readXMLFile( clipInfoPath.c_str(), clipInfoExpat );
		if ( clipInfoExpat != 0 )
		{	
			XML_Node & xmlTree = clipInfoExpat->tree;
			XML_NodePtr rootElem = 0;

			for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
				if ( xmlTree.content[i]->kind == kElemNode ) {
					rootElem = xmlTree.content[i];
				}
			}
			if ( rootElem != 0 )
			{
				XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
				
				if ( XMP_LitMatch ( rootLocalName, "smil" ) ) 
				{
					XMP_StringPtr umidValue = rootElem->GetAttrValue ( "umid" );
					if ( umidValue != 0 ) {
						clipUmid = umidValue;
						umidFound = true;
					}
				}
			}
		}
		if( ! umidFound )
		{	//try to get the umid from the NRT metadata
			delete ( clipInfoExpat ) ; clipInfoExpat = 0;
			this->MakeClipFilePath ( &clipInfoPath, "M01.XML" ) ;
			readXMLFile( clipInfoPath.c_str(), clipInfoExpat ) ;		
			if ( clipInfoExpat != 0 )
			{	
				XML_Node & xmlTree = clipInfoExpat->tree;
				XML_NodePtr rootElem = 0;
				for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
					if ( xmlTree.content[i]->kind == kElemNode ) {
						rootElem = xmlTree.content[i];
					}
				}
				if ( rootElem != 0 )
				{
					XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
					
					if ( XMP_LitMatch ( rootLocalName, "NonRealTimeMeta" ) ) 
					{	
						nameSpace = rootElem->ns.c_str() ;
						XML_NodePtr targetProp = rootElem->GetNamedElement ( nameSpace, "TargetMaterial" );
						if ( (targetProp != 0) && targetProp->IsEmptyLeafNode() ) {
							XMP_StringPtr umidValue = targetProp->GetAttrValue ( "umidRef" );
							if ( umidValue != 0 ) {
								clipUmid = umidValue;
								umidFound = true;
							}
						}
					}
				}
			}
		}
	} catch ( ... ) {
	}
	delete ( clipInfoExpat ) ;
	return umidFound;
}// XDCAM_MetaHandler::GetClipUmid

// =================================================================================================
// XDCAM_MetaHandler::IsClipsPlanning
// ==================================
bool XDCAM_MetaHandler::IsClipsPlanning ( std::string clipUmid , XMP_StringPtr planPath ) 
{
	ExpatAdapter* planniingExpat = 0 ;
	XMP_StringPtr nameSpace = 0 ;
	try {
		readXMLFile( planPath, planniingExpat );
		if ( planniingExpat != 0 )
		{	
			XML_Node & xmlTree = planniingExpat->tree;
			XML_NodePtr rootElem = 0;

			for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
				if ( xmlTree.content[i]->kind == kElemNode ) {
					rootElem = xmlTree.content[i];
				}
			}
			if ( rootElem != 0 )
			{
				XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
				
				if ( XMP_LitMatch ( rootLocalName, "PlanningMetadata" ) ) 
				{
					nameSpace = rootElem->ns.c_str() ;
					size_t noOfMaterialGroups = rootElem->CountNamedElements ( nameSpace, "MaterialGroup" ) ;
					while( noOfMaterialGroups-- )
					{
						XML_NodePtr mgNode = rootElem->GetNamedElement(  nameSpace, "MaterialGroup" );
						size_t noOfMaterialElements = mgNode->CountNamedElements ( nameSpace, "Material" ) ;
						while( noOfMaterialElements-- )
						{
							XML_NodePtr materialNode = mgNode->GetNamedElement(  nameSpace, "Material" );
							XMP_StringPtr materialType = materialNode->GetAttrValue ( "type" );
							if ( XMP_LitMatch( materialType , "clip" ) ) 
							{
								XMP_StringPtr umidValue = materialNode->GetAttrValue ( "umidRef" );
								if ( umidValue != 0 &&  XMP_LitMatch( umidValue , clipUmid.c_str()  ) )
								{
									delete ( planniingExpat ) ;
									return true;
								}
							}
							
						}
					}
				}
			}
		}
		
	} catch ( ... ) {
	}
	delete ( planniingExpat ) ;
	return false;
} // XDCAM_MetaHandler::IsClipsPlanning


// =================================================================================================
// XDCAM_MetaHandler::RefersClipUmid
// ==================================
bool XDCAM_MetaHandler::RefersClipUmid ( std::string clipUmid , XMP_StringPtr editInfoPath ) 
{
	ExpatAdapter* editInfoExpat = 0 ;
	XMP_StringPtr nameSpace = 0 ;
	try {
		readXMLFile( editInfoPath, editInfoExpat );
		if ( editInfoExpat != 0 )
		{	
			XML_Node & xmlTree = editInfoExpat->tree;
			XML_NodePtr rootElem = 0;

			for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
				if ( xmlTree.content[i]->kind == kElemNode ) {
					rootElem = xmlTree.content[i];
				}
			}
			if ( rootElem != 0 )
			{
				XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
				
				if ( XMP_LitMatch ( rootLocalName, "smil" ) ) 
				{
					nameSpace = rootElem->ns.c_str() ;
					size_t noOfBodyElements = rootElem->CountNamedElements ( nameSpace, "body" ) ;
					while( noOfBodyElements-- )
					{
						XML_NodePtr bodyNode = rootElem->GetNamedElement(  nameSpace, "body" );
						size_t noOfParElements = bodyNode->CountNamedElements ( nameSpace, "par" ) ;
						while( noOfParElements-- )
						{
							XML_NodePtr parNode = bodyNode->GetNamedElement(  nameSpace, "par" );
							size_t noOfRefElements = parNode->CountNamedElements ( nameSpace, "ref" ) ;
							size_t whichElem = 0;
							while( noOfRefElements-- )
							{
								XML_NodePtr refNode = parNode->GetNamedElement(  nameSpace, "ref" ,whichElem++ );
								XMP_StringPtr umidValue = refNode->GetAttrValue ( "src" );
								if ( umidValue != 0 && 
										( XMP_LitMatch( umidValue , clipUmid.c_str() ) || 
											( strlen(umidValue) > 15 && XMP_LitMatch( &umidValue[15] , clipUmid.c_str() ) ) 
										)
									)
									{
									delete ( editInfoExpat ) ;
									return true;
								}
							}
						}
					}
				}
			}
		}
		
	} catch ( ... ) {
	}
	delete ( editInfoExpat ) ;
	return false;
} // XDCAM_MetaHandler::RefersClipUmid

inline bool IsDigit( char c )
{
	return c >= '0' && c <= '9';
}


// =================================================================================================
// XDCAM_MetaHandler::GetEditInfoFilesSAM
// ======================================
bool XDCAM_MetaHandler::GetEditInfoFilesSAM ( std::vector<std::string> &editInfoList ) 
{
	std::string clipUmid;
	bool found = false;

	if( GetClipUmid ( clipUmid ) )
	{
		std::string editFolderPath = this->rootPath + kDirChar + "PROAV" + kDirChar + "EDTR"  + kDirChar ;
		if ( Host_IO::Exists( editFolderPath.c_str() ) && 
			Host_IO::GetFileMode( editFolderPath.c_str() ) == Host_IO::kFMode_IsFolder 
			)
		{
			Host_IO::AutoFolder edtrFolder, editFolder;
			std::string edtrChildName, edlistChild;

			edtrFolder.folder = Host_IO::OpenFolder ( editFolderPath.c_str() );
			while (  Host_IO::GetNextChild ( edtrFolder.folder, &edtrChildName ) ) {
				size_t childLen = edtrChildName.size();
				std::string editListFolderPath = editFolderPath + edtrChildName + kDirChar ;
				if ( ! ( childLen == 5 &&
					edtrChildName[0] == 'E' &&
					IsDigit( edtrChildName[1] ) &&
					IsDigit( edtrChildName[2] ) &&
					IsDigit( edtrChildName[3] ) &&
					IsDigit( edtrChildName[4] ) &&
					Host_IO::GetFileMode( editListFolderPath.c_str() ) == Host_IO::kFMode_IsFolder
					) ) continue;
				
				editFolder.folder = Host_IO::OpenFolder ( editListFolderPath.c_str() );
				while (  Host_IO::GetNextChild ( editFolder.folder, &edlistChild ) ) {
					size_t filenamelen = edlistChild.size();
					std::string editListFilePath = editListFolderPath + edlistChild ;
					if ( ! ( filenamelen == 12 &&
						edlistChild.compare ( filenamelen - 4, 4 , ".SMI" ) == 0 &&
						edlistChild.compare ( 0, edtrChildName.size(), edtrChildName ) == 0 &&
						Host_IO::GetFileMode( editListFilePath.c_str() ) == Host_IO::kFMode_IsFile
					) ) continue;
					if( RefersClipUmid ( clipUmid , editListFilePath.c_str() )  )
					{
						found = true ;
						editInfoList.push_back( editListFilePath );
					}
				}
			}
		}
	}
	return found;
} // XDCAM_MetaHandler::GetEditInfoFilesSAM

// =================================================================================================
// XDCAM_MetaHandler::GetInfoFilesFAM
// ==================================
bool XDCAM_MetaHandler::GetInfoFilesFAM ( std::vector<std::string> &editInfoList, std::string pathToFolder) 
{
	std::string clipUmid;
	bool found = false;

	if( GetClipUmid ( clipUmid ) )
	{
		if ( Host_IO::Exists( pathToFolder.c_str() ) && 
			Host_IO::GetFileMode( pathToFolder.c_str() ) == Host_IO::kFMode_IsFolder 
			)
		{
			Host_IO::AutoFolder editFolder;
			std::string  edlistChild;

			editFolder.folder = Host_IO::OpenFolder ( pathToFolder.c_str() );
			while (  Host_IO::GetNextChild ( editFolder.folder, &edlistChild ) ) {
				size_t filenamelen = edlistChild.size();
				std::string editListFilePath = pathToFolder + edlistChild ;
				if ( ! ( filenamelen > 7 && 
					edlistChild.compare ( filenamelen - 4, 4 , ".SMI" ) == 0 &&
					Host_IO::GetFileMode( editListFilePath.c_str() ) == Host_IO::kFMode_IsFile
				) ) continue;
				if( RefersClipUmid ( clipUmid , editListFilePath.c_str() )  )
				{
					found = true ;
					editInfoList.push_back( editListFilePath );
				}
			}
		}
	}
	return found;
} // XDCAM_MetaHandler::GetInfoFilesFAM

// =================================================================================================
// XDCAM_MetaHandler::GetPlanningFilesFAM
// ======================================
bool XDCAM_MetaHandler::GetPlanningFilesFAM ( std::vector<std::string> &planInfoList, std::string pathToFolder) 
{
	std::string clipUmid;
	bool found = false;

	if( GetClipUmid ( clipUmid ) )
	{
		if ( Host_IO::Exists( pathToFolder.c_str() ) && 
			Host_IO::GetFileMode( pathToFolder.c_str() ) == Host_IO::kFMode_IsFolder 
			)
		{
			Host_IO::AutoFolder planFolder;
			std::string  listChild;

			planFolder.folder = Host_IO::OpenFolder ( pathToFolder.c_str() );
			while (  Host_IO::GetNextChild ( planFolder.folder, &listChild ) ) {
				size_t filenamelen = listChild.size();
				std::string listFilePath = pathToFolder + listChild ;
				if ( ! ( filenamelen > 4 && 
					( listChild.compare ( filenamelen - 4, 4 , ".XML" ) == 0 
					|| 
					listChild.compare ( filenamelen - 4, 4 , ".xml" ) == 0
					)
					&&
					Host_IO::GetFileMode( listFilePath.c_str() ) == Host_IO::kFMode_IsFile
				) ) continue;
				if( IsClipsPlanning ( clipUmid , listFilePath.c_str() )  )
				{
					found = true ;
					planInfoList.push_back( listFilePath );
				}
			}
		}
	}
	return found;
} // XDCAM_MetaHandler::GetPlanningFilesFAM

// =================================================================================================
// XDCAM_MetaHandler::IsMetadataWritable
// =======================================

bool XDCAM_MetaHandler::IsMetadataWritable ( )
{
	std::vector<std::string> metadataFiles;
	FillMetadataFiles(&metadataFiles);
	std::vector<std::string>::iterator itr = metadataFiles.begin();
	// Check whether sidecar is writable, if not then check if it can be created.
	bool xmpWritable = Host_IO::Writable( itr->c_str(), true );
	// Check for legacy metadata file.
	bool xmlWritable = Host_IO::Writable( (++itr)->c_str(), false );
	return ( xmlWritable && xmpWritable );
}// XDCAM_MetaHandler::IsMetadataWritable

// =================================================================================================
// XDCAM_MetaHandler::FillFAMAssociatedResources
// =============================================
void XDCAM_MetaHandler::FillFAMAssociatedResources ( std::vector<std::string> * resourceList )
{
	// The possible associated resources:
    //  .../MyMovie/
	//  	ALIAS.XML
	//  	INDEX.XML
	//  	DISCMETA.XML
	//  	MEDIAPRO.XML
	//  	MEDIAPRO.BUP
	//      CUEUP.XML
	//      CUEUP.BUP
	//  	Clip/
	//  		AAAAA.MXF                   AAAAA is the clipname with clipserial
	//										XX is a counter which will start from from 01 and can go upto 99 based 
	//				 					    on number of files present in this folder with same extension and same clipname/editListName/Takename.
	//  		AAAAAMXX.XML  
	//  		AAAAAMXX.XMP
	//  		AAAAARXX.BIM
	//  	Sub/
	//  		AAAAASXX.MXF
	//  	Local/
	//  		AAAAACXX.SMI
	//  		AAAAACXX.PPN
	//  	Edit/                           DDDDD is the editListName
	//  		DDDDDEXX.SMI
	//  		DDDDDMXX.XML
	// 		Take/							TTTTT is the Takename
	//  		TTTTT.SMI
	//			TTTTTUNN.SMI			NN is a counter which goes from 01 to N-1 where N is number of media, this
	//									take is divided into. For Nth, TTTTT.SMI shall be picked up.
	//  		TTTTTMXX.XML
	//  	General/
	//			Sony/
	//				Planning/				AAAAA is the clipname without clipserial
	//										YYYYMMDDHHMISS is DateTime
	//						BBBBB_YYYYMMDDHHMISS.xml 
	//      UserData/
	//

	//Add RootPath
	std::string filePath = rootPath + kDirChar;
	PackageFormat_Support::AddResourceIfExists( resourceList, filePath );

	// Get the files present directly inside root folder.
	filePath = rootPath + kDirChar + "ALIAS.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	filePath = rootPath + kDirChar + "INDEX.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	filePath = rootPath + kDirChar + "DISCMETA.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	filePath = rootPath + kDirChar + "MEDIAPRO.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	filePath = rootPath + kDirChar + "MEDIAPRO.BUP";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	filePath = rootPath + kDirChar + "CUEUP.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	filePath = rootPath + kDirChar + "CUEUP.BUP";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	
	// Add the UserData folder which is used to identify the format in any way
	filePath = rootPath + kDirChar + "UserData" + kDirChar;
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	
	XMP_VarString clipPath = rootPath + kDirChar + "Clip" + kDirChar ;

	size_t oldCount = resourceList->size();
	// Get the files present inside clip folder.
	XMP_VarString regExp;
	XMP_StringVector regExpVec;
	
	regExp = "^" + clipName + ".MXF$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "M\\d\\d.XML$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "R\\d\\d.BIM$";
	regExpVec.push_back ( regExp );
	IOUtils::GetMatchingChildren ( *resourceList, clipPath, regExpVec, false, true, true ); 
	PackageFormat_Support::AddResourceIfExists(resourceList, this->sidecarPath);
	if ( resourceList->size() <= oldCount )
	{
		PackageFormat_Support::AddResourceIfExists(resourceList, clipPath);
	}

	//Get the files Under Sub folder
	clipPath = rootPath + kDirChar + "Sub" + kDirChar ;
	regExpVec.clear();
	regExp = "^" + clipName + "S\\d\\d.MXF$";
	regExpVec.push_back ( regExp );
	oldCount = resourceList->size();
	IOUtils::GetMatchingChildren ( *resourceList, clipPath, regExpVec, false, true, true );
	// Add Sub folder if no file inside this, was added.
	if ( resourceList->size() <= oldCount )
	{
		PackageFormat_Support::AddResourceIfExists(resourceList, clipPath);
	}

	//Get the files Under Local folder
	clipPath = rootPath + kDirChar + "Local" + kDirChar ;
	regExpVec.clear();
	regExp = "^" + clipName + "C\\d\\d.SMI$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "I\\d\\d.PPN$";
	regExpVec.push_back ( regExp );
	oldCount = resourceList->size();
	IOUtils::GetMatchingChildren ( *resourceList, clipPath, regExpVec, false, true, true );
	
	//Add the Edit lists associated to this clip
	XMP_StringVector editInfoList;
	bool atLeastOneFileAdded = false;
	clipPath = rootPath + kDirChar + "Edit" + kDirChar ;
	if ( GetInfoFilesFAM ( editInfoList , clipPath ) )
	{
		size_t noOfEditInfoFiles = editInfoList.size() ;
		for( size_t count = 0; count < noOfEditInfoFiles; count++ )
		{
			atLeastOneFileAdded = PackageFormat_Support::AddResourceIfExists(resourceList, editInfoList[count]) ? true : atLeastOneFileAdded;
			std::string editNRTFile = editInfoList[count] ;
			size_t filenamelen = editInfoList[count].length() ;
			editNRTFile[ filenamelen - 7 ] = 'M';
			editNRTFile[ filenamelen - 3 ] = 'X';
			editNRTFile[ filenamelen - 2 ] = 'M';
			editNRTFile[ filenamelen - 1 ] = 'L';
			atLeastOneFileAdded = PackageFormat_Support::AddResourceIfExists(resourceList, editNRTFile ) ? true : atLeastOneFileAdded;
		}
	}
	// Add Edit folder if no file inside this, was added.
	if ( !atLeastOneFileAdded )
	{
		PackageFormat_Support::AddResourceIfExists(resourceList, clipPath);
	}

	atLeastOneFileAdded = false;

	//Add the Takes associated to this clip
	XMP_StringVector takeList;
	clipPath = rootPath + kDirChar + "Take" + kDirChar ;
	if( GetInfoFilesFAM ( takeList  , clipPath ) )
	{
		size_t noOfTakes = takeList.size() ;
		for( size_t count = 0; count < noOfTakes; count++ )
		{
			atLeastOneFileAdded = PackageFormat_Support::AddResourceIfExists(resourceList, takeList[count]) ? true : atLeastOneFileAdded;
			XMP_VarString takeNRTFile = takeList[count] ;
			size_t filenamelen = takeList[count].length() ;
			if ( takeNRTFile[ filenamelen - 7 ] == 'U' 
				&& IsDigit( takeNRTFile[ filenamelen - 6 ] ) 
				&& IsDigit( takeNRTFile[ filenamelen - 5 ] ) )
			{
				takeNRTFile.erase( takeNRTFile.begin() + filenamelen - 7, takeNRTFile.end() ) ;
			}
			else
			{
				takeNRTFile.erase( takeNRTFile.begin() + filenamelen - 4, takeNRTFile.end() ) ;
			}

			XMP_VarString fileName;
			size_t pos = takeNRTFile.find_last_of ( kDirChar );
			fileName = takeNRTFile.substr ( pos + 1 );
			XMP_VarString regExp = "^" + fileName + "M\\d\\d.XML$";
			oldCount = resourceList->size();
			IOUtils::GetMatchingChildren ( *resourceList, clipPath, regExp, false, true, true );
			atLeastOneFileAdded = resourceList->size() > oldCount;
		}
	}
	// Add Take folder if no file inside this, was added.
	if(!atLeastOneFileAdded)
	{
		filePath = rootPath + kDirChar + "Take" + kDirChar;
		PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	}

	//Add the Planning Metadata Files associated to this clip
	XMP_StringVector planList;
	clipPath = rootPath + kDirChar + "General" + kDirChar + "Sony" + kDirChar+ "Planning" + kDirChar;
	if( GetPlanningFilesFAM ( planList  , clipPath ) )
	{
		size_t noOfPlans = planList.size() ;
		for( size_t count = 0; count < noOfPlans; count++ )
		{
			resourceList->push_back( planList[count] );
		}
	}
} // XDCAM_MetaHandler::FillFAMAssociatedResources

// =================================================================================================
// XDCAM_MetaHandler::FillSAMAssociatedResources
// =============================================
void XDCAM_MetaHandler::FillSAMAssociatedResources ( std::vector<std::string> * resourceList )
{
	// The possible associated resources:
    //  .../MyMovie/
	//	    PROAV/
	//  	    INDEX.XML
	//  	    INDEX.BUP
	//  	    DISCMETA.XML
	//  	    DISCINFO.XML
	//  	    DISCINFO.BUP
	//		    CLPR/
	//			    CXXXX/			 XXXX is ClipSerial and NN is a counter which will start from from 01 and can go upto 99 based 
	//				 					on number of files present in this folder with same extension.
	//				    CXXXXCNN.SMI
	//				    CXXXXVNN.MXF
	//				    CXXXXANN.MXF
	//				    CXXXXRNN.BIM
	//				    CXXXXINN.PPN
	//				    CXXXXMNN.XML
	//				    CXXXXSNN.MXF
	//		    EDTR/
	//			    EXXXX:
	//				    EXXXXENN.SMI	
	//				    EXXXXMNN.XML		
	//					
	std::string proavPath = rootPath + kDirChar + "PROAV" + kDirChar;
	std::string filePath;
	//Add RootPath
	filePath = rootPath + kDirChar;
	PackageFormat_Support::AddResourceIfExists( resourceList, filePath );

	// Get the files present directly inside PROAV folder.
	filePath = proavPath + "INDEX.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	filePath = proavPath + "INDEX.BUP";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	filePath = proavPath + "DISCINFO.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	filePath = proavPath + "DISCINFO.BUP";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	filePath = proavPath + "DISCMETA.XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, filePath);

	XMP_VarString clipPath = proavPath + "CLPR" + kDirChar + clipName + kDirChar;
	XMP_VarString regExp;
	XMP_StringVector regExpVec;

	regExp = "^" + clipName + "C\\d\\d.SMI$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "M\\d\\d.XML$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "V\\d\\d.MXF$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "A\\d\\d.MXF$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "R\\d\\d.BIM$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "I\\d\\d.PPN$";
	regExpVec.push_back ( regExp );
	regExp = "^" + clipName + "S\\d\\d.MXF$";
	regExpVec.push_back ( regExp );
	IOUtils::GetMatchingChildren ( *resourceList, clipPath, regExpVec, false, true, true );
	PackageFormat_Support::AddResourceIfExists(resourceList, this->sidecarPath);
	//Add the Edit lists that refer this clip
	std::vector<std::string> editInfoList;
	if( GetEditInfoFilesSAM ( editInfoList ) )
	{
		size_t noOfEditInfoFiles = editInfoList.size() ;
		for( size_t count = 0; count < noOfEditInfoFiles; count++ )
		{
			PackageFormat_Support::AddResourceIfExists(resourceList, editInfoList[count]);
			std::string editNRTFile = editInfoList[count].c_str() ;
			size_t filenamelen = editInfoList[count].length() ;
			editNRTFile[ filenamelen - 7 ] = 'M';
			editNRTFile[ filenamelen - 3 ] = 'X';
			editNRTFile[ filenamelen - 2 ] = 'M';
			editNRTFile[ filenamelen - 1 ] = 'L';
			PackageFormat_Support::AddResourceIfExists(resourceList, editNRTFile );
		}
	}
}// XDCAM_MetaHandler::FillSAMAssociatedResources

// =================================================================================================
// XDCAM_MetaHandler::FillAssociatedResources
// ======================================
void XDCAM_MetaHandler::FillAssociatedResources ( std::vector<std::string> * resourceList )
{
	if( this->isFAM )
		 FillFAMAssociatedResources ( resourceList );
	else
		 FillSAMAssociatedResources ( resourceList );
}
// =================================================================================================
// XDCAM_MetaHandler::FillMetadataFiles
// ====================================
void XDCAM_MetaHandler::FillMetadataFiles ( std::vector<std::string> * metadataFiles )
{
	std::string noExtPath, filePath;

	if(this->isFAM) {
		noExtPath = rootPath + kDirChar + "Clip" + kDirChar + clipName;
	} else {
		noExtPath = rootPath + kDirChar + "PROAV" + kDirChar + "CLPR" +
						   kDirChar + clipName + kDirChar + clipName;
	}

	metadataFiles->push_back ( this->sidecarPath );
	filePath = noExtPath + "M01.XML";
	metadataFiles->push_back ( filePath );

}	// XDCAM_MetaHandler::FillMetadataFiles

// =================================================================================================
// XDCAM_MetaHandler::CacheFileData
// ================================

void XDCAM_MetaHandler::CacheFileData()
{
	XMP_Assert ( ! this->containsXMP );

	if ( this->parent->UsesClientIO() ) {
		XMP_Throw ( "XDCAM cannot be used with client-managed I/O", kXMPErr_InternalFailure );
	}

	// See if the clip's .XMP file exists.

	if ( ! Host_IO::Exists ( this->sidecarPath.c_str() ) ) return;	// No XMP.

	// Read the entire .XMP file. We know the XMP exists, New_XMPFiles_IO is supposed to return 0
	// only if the file does not exist.

	bool readOnly = XMP_OptionIsClear ( this->parent->openFlags, kXMPFiles_OpenForUpdate );

	XMP_Assert ( this->parent->ioRef == 0 );
	XMPFiles_IO* xmpFile =  XMPFiles_IO::New_XMPFiles_IO ( this->sidecarPath.c_str(), readOnly );
	if ( xmpFile == 0 ) XMP_Throw ( "XDCAM XMP file open failure", kXMPErr_InternalFailure );
	this->parent->ioRef = xmpFile;

	XMP_Int64 xmpLen = xmpFile->Length();
	if ( xmpLen > 100*1024*1024 ) {
		XMP_Throw ( "XDCAM XMP is outrageously large", kXMPErr_InternalFailure );	// Sanity check.
	}

	this->xmpPacket.erase();
	this->xmpPacket.append ( (size_t)xmpLen, ' ' );

	XMP_Int32 ioCount = xmpFile->ReadAll ( (void*)this->xmpPacket.data(), (XMP_Int32)xmpLen );

	this->packetInfo.offset = 0;
	this->packetInfo.length = (XMP_Int32)xmpLen;
	FillPacketInfo ( this->xmpPacket, &this->packetInfo );

	this->containsXMP = true;

}	// XDCAM_MetaHandler::CacheFileData

// =================================================================================================
// XDCAM_MetaHandler::GetMediaProMetadata
// ======================================

bool XDCAM_MetaHandler::GetMediaProMetadata ( SXMPMeta * xmpObjPtr,
											  const std::string& clipUMID,
											  bool digestFound )
{
	if (!this->isFAM) return false;

	// Build a directory string to the MEDIAPRO file.

	std::string mediaproPath;
	MakeMediaproPath ( &mediaproPath );
	return XDCAM_Support::GetMediaProLegacyMetadata ( xmpObjPtr, clipUMID, mediaproPath, digestFound );

}

// =================================================================================================
// XDCAM_MetaHandler::ProcessXMP
// =============================

void XDCAM_MetaHandler::ProcessXMP()
{

	// Some versions of gcc can't tolerate goto's across declarations.
	// *** Better yet, avoid this cruft with self-cleaning objects.
	#define CleanupAndExit	\
		{																								\
			bool openForUpdate = XMP_OptionIsSet ( this->parent->openFlags, kXMPFiles_OpenForUpdate );	\
			if ( ! openForUpdate ) this->CleanupLegacyXML();											\
			return;																						\
		}

	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.

	if ( this->containsXMP ) {
		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	}

	// NonRealTimeMeta -> XMP by schema
	std::string xmlPath, umid;
	this->MakeClipFilePath ( &xmlPath, "M01.XML" );

	readXMLFile( xmlPath.c_str(),this->expat );
	if ( this->expat == 0 ) return;

	// The root element should be NonRealTimeMeta in some namespace. Take whatever this file uses.

	XML_Node & xmlTree = this->expat->tree;
	XML_NodePtr rootElem = 0;

	for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
		if ( xmlTree.content[i]->kind == kElemNode ) {
			rootElem = xmlTree.content[i];
		}
	}

	if ( rootElem == 0 ) CleanupAndExit
	XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
	if ( ! XMP_LitMatch ( rootLocalName, "NonRealTimeMeta" ) ) CleanupAndExit

	this->legacyNS = rootElem->ns;

	// Check the legacy digest.

	XMP_StringPtr legacyNS = this->legacyNS.c_str();

	this->clipMetadata = rootElem;	// ! Save the NonRealTimeMeta pointer for other use.

	std::string oldDigest, newDigest;
	bool digestFound = this->xmpObj.GetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "XDCAM", &oldDigest, 0 );
	if ( digestFound ) {
		this->MakeLegacyDigest ( &newDigest );
		if ( oldDigest == newDigest ) CleanupAndExit
	}

	// If we get here we need find and import the actual legacy elements using the current namespace.
	// Either there is no old digest in the XMP, or the digests differ. In the former case keep any
	// existing XMP, in the latter case take new legacy values.

	this->containsXMP = XDCAM_Support::GetLegacyMetadata ( &this->xmpObj, rootElem, legacyNS, digestFound, umid );
	this->containsXMP |= GetMediaProMetadata ( &this->xmpObj, umid, digestFound );

	CleanupAndExit
	#undef CleanupAndExit

}	// XDCAM_MetaHandler::ProcessXMP

// =================================================================================================
// XDCAM_MetaHandler::UpdateFile
// =============================
//
// Note that UpdateFile is only called from XMPFiles::CloseFile, so it is OK to close the file here.

void XDCAM_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	this->needsUpdate = false;	// Make sure only called once.

	XMP_Assert ( this->parent->UsesLocalIO() );

	// Update the internal legacy XML tree if we have one, and set the digest in the XMP.

	bool updateLegacyXML = false;

	if ( this->clipMetadata != 0 ) {
		updateLegacyXML = XDCAM_Support::SetLegacyMetadata ( this->clipMetadata, &this->xmpObj, this->legacyNS.c_str());
	}

	std::string newDigest;
	this->MakeLegacyDigest ( &newDigest );
	this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "XDCAM", newDigest.c_str(), kXMP_DeleteExisting );
	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, this->GetSerializeOptions() );

	// -----------------------------------------------------------------------
	// Update the XMP file first, don't let legacy XML failures block the XMP.

	

	bool haveXMP = Host_IO::Exists ( this->sidecarPath.c_str() );
	if ( ! haveXMP ) {
		XMP_Assert ( this->parent->ioRef == 0 );
		Host_IO::Create ( this->sidecarPath.c_str() );
		this->parent->ioRef = XMPFiles_IO::New_XMPFiles_IO ( this->sidecarPath.c_str(), Host_IO::openReadWrite );
		if ( this->parent->ioRef == 0 ) XMP_Throw ( "Failure opening XDCAM XMP file", kXMPErr_ExternalFailure );
	}

	XMP_IO* xmpFile = this->parent->ioRef;
	XMP_Assert ( xmpFile != 0 );
	XIO::ReplaceTextFile ( xmpFile, this->xmpPacket, (haveXMP & doSafeUpdate) );

	// --------------------------------------------
	// Now update the legacy XML file if necessary.

	if ( updateLegacyXML ) {

		std::string legacyXML, xmlPath;
		this->expat->tree.Serialize ( &legacyXML );
		this->MakeClipFilePath ( &xmlPath, "M01.XML" );

		bool haveXML = Host_IO::Exists ( xmlPath.c_str() );
		if ( ! haveXML ) Host_IO::Create ( xmlPath.c_str() );

		Host_IO::FileRef hostRef = Host_IO::Open ( xmlPath.c_str(), Host_IO::openReadWrite );
		if ( hostRef == Host_IO::noFileRef ) XMP_Throw ( "Failure opening XDCAM XML file", kXMPErr_ExternalFailure );
		XMPFiles_IO origXML ( hostRef, xmlPath.c_str(), Host_IO::openReadWrite );
		XIO::ReplaceTextFile ( &origXML, legacyXML, (haveXML & doSafeUpdate) );
		origXML.Close();

	}

}	// XDCAM_MetaHandler::UpdateFile

// =================================================================================================
// XDCAM_MetaHandler::WriteTempFile
// ================================

void XDCAM_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{

	// ! WriteTempFile is not supposed to be called for handlers that own the file.
	XMP_Throw ( "XDCAM_MetaHandler::WriteTempFile should not be called", kXMPErr_InternalFailure );

}	// XDCAM_MetaHandler::WriteTempFile

// =================================================================================================
