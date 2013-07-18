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

#include "XMPFiles/source/FileHandlers/P2_Handler.hpp"
#include "XMPFiles/source/FormatSupport/PackageFormat_Support.hpp"
#include "third-party/zuid/interfaces/MD5.h"

#include <cmath>

using namespace std;

// =================================================================================================
/// \file P2_Handler.cpp
/// \brief Folder format handler for P2.
///
/// This handler is for the P2 video format. This is a pseudo-package, visible files but with a very
/// well-defined layout and naming rules. A typical P2 example looks like:
///
/// .../MyMovie
/// 	CONTENTS/
/// 		CLIP/
/// 			0001AB.XML
/// 			0001AB.XMP
/// 			0002CD.XML
/// 			0002CD.XMP
/// 		VIDEO/
/// 			0001AB.MXF
/// 			0002CD.MXF
/// 		AUDIO/
/// 			0001AB00.MXF
/// 			0001AB01.MXF
/// 			0002CD00.MXF
/// 			0002CD01.MXF
/// 		ICON/
/// 			0001AB.BMP
/// 			0002CD.BMP
/// 		VOICE/
/// 			0001AB.WAV
/// 			0002CD.WAV
/// 		PROXY/
/// 			0001AB.MP4
/// 			0002CD.MP4
///
/// From the user's point of view, .../MyMovie contains P2 stuff, in this case 2 clips whose raw
/// names are 0001AB and 0002CD. There may be mapping information for nicer clip names to the raw
/// names, but that can be ignored for now. Each clip is stored as a collection of files, each file
/// holding some specific aspect of the clip's data.
///
/// The P2 handler operates on clips. The path from the client of XMPFiles can be either a logical
/// clip path, like ".../MyMovie/0001AB", or a full path to one of the files. In the latter case the
/// handler must figure out the intended clip, it must not blindly use the named file.
///
/// Once the P2 structure and intended clip are identified, the handler only deals with the .XMP and
/// .XML files in the CLIP folder. The .XMP file, if present, contains the XMP for the clip. The .XML
/// file must be present to define the existance of the clip. It contains a variety of information
/// about the clip, including some legacy metadata.
///
// =================================================================================================

static const char * kContentFolderNames[] = { "CLIP", "VIDEO", "AUDIO", "ICON", "VOICE", "PROXY", 0 };
static int kNumRequiredContentFolders = 6;	// All 6 of the above.

static inline bool CheckContentFolderName ( const std::string & folderName )
{
	for ( int i = 0; kContentFolderNames[i] != 0; ++i ) {
		if ( folderName == kContentFolderNames[i] ) return true;
	}
	return false;
}

// =================================================================================================
// InternalMakeClipFilePath
// ========================
//
// P2_CheckFormat can't use the member function.

static void InternalMakeClipFilePath ( std::string * path,
									   const std::string & rootPath,
									   const std::string & clipName,
									   XMP_StringPtr suffix )
{

	*path = rootPath;
	*path += kDirChar;
	*path += "CONTENTS";
	*path += kDirChar;
	*path += "CLIP";
	*path += kDirChar;
	*path += clipName;
	*path += suffix;

}	// InternalMakeClipFilePath

// =================================================================================================
// P2_CheckFormat
// ==============
//
// This version does fairly simple checks. The top level folder (.../MyMovie) must have a child
// folder called CONTENTS. This must have a subfolder called CLIP. It may also have subfolders
// called VIDEO, AUDIO, ICON, VOICE, and PROXY. Any mixture of these additional folders is allowed,
// but no other children are allowed in CONTENTS. The CLIP folder must contain a .XML file for the
// desired clip. The name checks are case insensitive.
//
// The state of the string parameters depends on the form of the path passed by the client. If the
// client passed a logical clip path, like ".../MyMovie/0001AB", the parameters are:
//   rootPath   - ".../MyMovie"
//   gpName     - empty
//   parentName - empty
//   leafName   - "0001AB"
// If the client passed a full file path, like ".../MyMovie/CONTENTS/VOICE/0001AB.WAV", they are:
//   rootPath   - ".../MyMovie"
//   gpName     - "CONTENTS"
//   parentName - "VOICE"
//   leafName   - "0001AB"
//
// For most of the content files the base file name is the raw clip name. Files in the AUDIO and
// VOICE folders have an extra 2 digits appended to the raw clip name. These must be trimmed.

// ! The common code has shifted the gpName, parentName, and leafName strings to upper case. It has
// ! also made sure that for a logical clip path the rootPath is an existing folder, and that the
// ! file exists for a full file path.

bool P2_CheckFormat ( XMP_FileFormat format,
					  const std::string & rootPath,
					  const std::string & gpName,
					  const std::string & parentName,
					  const std::string & leafName,
					  XMPFiles * parent )
{
	Host_IO::AutoFolder aFolder;
	std::string tempPath, childName;

	std::string clipName = leafName;

	// Do some basic checks on the gpName and parentName.

	if ( gpName.empty() != parentName.empty() ) return false;	// Must be both empty or both non-empty.

	if ( ! gpName.empty() ) {

		if ( gpName != "CONTENTS" ) return false;
		if ( ! CheckContentFolderName ( parentName ) ) return false;

		if ( (parentName == "AUDIO") | (parentName == "VOICE") ) {
			if ( clipName.size() < 3 ) return false;
			clipName.erase ( clipName.size() - 2 );
		}

	}

	tempPath = rootPath;
	tempPath += kDirChar;
	tempPath += "CONTENTS";
	if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFolder ) return false;

	aFolder.folder = Host_IO::OpenFolder ( tempPath.c_str() );
	int numChildrenFound = 0;
	std::string childPath;

	while ( ( Host_IO::GetNextChild ( aFolder.folder, &childName ) && ( numChildrenFound < kNumRequiredContentFolders ) ) ) {	// Make sure the children of CONTENTS are legit.
		if ( CheckContentFolderName ( childName ) ) {
			childPath = tempPath;
			childPath += kDirChar;
			childPath += childName;
			if ( Host_IO::GetFileMode ( childPath.c_str() ) != Host_IO::kFMode_IsFolder ) return false;
			++numChildrenFound;
		}
	}
	aFolder.Close();

	// Make sure the clip's .XML file exists.

	InternalMakeClipFilePath ( &tempPath, rootPath, clipName, ".XML" );
	if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFile ) return false;

	// Make a bogus path to pass the root path and clip name to the handler. A bit of a hack, but
	// the only way to get info from here to there.


	tempPath = rootPath;
	tempPath += kDirChar;
	tempPath += clipName;

	size_t pathLen = tempPath.size() + 1;	// Include a terminating nul.
	parent->tempPtr = malloc ( pathLen );
	if ( parent->tempPtr == 0 ) XMP_Throw ( "No memory for P2 clip path", kXMPErr_NoMemory );
	memcpy ( parent->tempPtr, tempPath.c_str(), pathLen );	// AUDIT: Safe, allocated above.

	return true;

}	// P2_CheckFormat

// =================================================================================================

static void* CreatePseudoClipPath ( const std::string & clientPath ) {

	// Used to create the clip pseudo path when the CheckFormat function is skipped.
	
	std::string pseudoPath = clientPath;

	size_t pathLen;
	void* tempPtr = 0;
	
	if ( Host_IO::Exists ( pseudoPath.c_str() ) ) {
	
		// The client passed a physical path. The logical clip name is the leaf name, with the
		// extension removed. Files in the AUDIO and VOICE folders have an extra 2 digits appended
		// to the clip name. The movie root path ends two levels up.
		
		std::string clipName, parentName, ignored;
		
		XIO::SplitLeafName ( &pseudoPath, &clipName );	// Extract the logical clip name.
		XIO::SplitFileExtension ( &clipName, &ignored );

		XIO::SplitLeafName ( &pseudoPath, &parentName );	// Remove the 2 intermediate folder levels.
		XIO::SplitLeafName ( &pseudoPath, &ignored );

		if ( (parentName == "AUDIO") | (parentName == "VOICE") ) {
			if ( clipName.size() >= 3 ) clipName.erase ( clipName.size() - 2 );
		}
		
		pseudoPath += kDirChar;
		pseudoPath += clipName;
	
	}

	pathLen = pseudoPath.size() + 1;	// Include a terminating nul.
	tempPtr = malloc ( pathLen );
	if ( tempPtr == 0 ) XMP_Throw ( "No memory for P2 clip info", kXMPErr_NoMemory );
	memcpy ( tempPtr, pseudoPath.c_str(), pathLen );
	
	return tempPtr;

}	// CreatePseudoClipPath

// =================================================================================================
// P2_MetaHandlerCTor
// ==================

XMPFileHandler * P2_MetaHandlerCTor ( XMPFiles * parent )
{
	return new P2_MetaHandler ( parent );

}	// P2_MetaHandlerCTor

// =================================================================================================
// P2_MetaHandler::P2_MetaHandler
// ==============================

P2_MetaHandler::P2_MetaHandler ( XMPFiles * _parent ) : expat(0), clipMetadata(0), clipContent(0)
{

	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kP2_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	// Extract the root path and clip name from tempPtr.

	if ( this->parent->tempPtr == 0 ) {
		// The CheckFormat call might have been skipped.
		this->parent->tempPtr = CreatePseudoClipPath ( this->parent->GetFilePath() );
	}

	this->rootPath.assign ( (char*) this->parent->tempPtr );
	free ( this->parent->tempPtr );
	this->parent->tempPtr = 0;

	XIO::SplitLeafName ( &this->rootPath, &this->clipName );

}	// P2_MetaHandler::P2_MetaHandler

// =================================================================================================
// P2_MetaHandler::~P2_MetaHandler
// ===============================

P2_MetaHandler::~P2_MetaHandler()
{

	this->CleanupLegacyXML();
	if ( this->parent->tempPtr != 0 ) {
		free ( this->parent->tempPtr );
		this->parent->tempPtr = 0;
	}

}	// P2_MetaHandler::~P2_MetaHandler

// =================================================================================================
// P2_MetaHandler::MakeClipFilePath
// ================================

bool P2_MetaHandler::MakeClipFilePath ( std::string * path, XMP_StringPtr suffix, bool checkFile /* = false */ )
{

	InternalMakeClipFilePath ( path, this->rootPath, this->clipName, suffix );
	if ( ! checkFile ) return true;
	
	return Host_IO::Exists ( path->c_str() );

}	// P2_MetaHandler::MakeClipFilePath

// =================================================================================================
// P2_MetaHandler::CleanupLegacyXML
// ================================

void P2_MetaHandler::CleanupLegacyXML()
{

	if ( this->expat != 0 ) { delete ( this->expat ); this->expat = 0; }

	clipMetadata = 0;	// ! Was a pointer into the expat tree.
	clipContent = 0;	// ! Was a pointer into the expat tree.

}	// P2_MetaHandler::CleanupLegacyXML

// =================================================================================================
// P2_MetaHandler::DigestLegacyItem
// ================================

void P2_MetaHandler::DigestLegacyItem ( MD5_CTX & md5Context, XML_NodePtr legacyContext, XMP_StringPtr legacyPropName )
{
	XML_NodePtr legacyProp = legacyContext->GetNamedElement ( this->p2NS.c_str(), legacyPropName );

	if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() && (! legacyProp->content.empty()) ) {
		const XML_Node * xmlValue = legacyProp->content[0];
		MD5Update ( &md5Context, (XMP_Uns8*)xmlValue->value.c_str(), (unsigned int)xmlValue->value.size() );
	}

}	// P2_MetaHandler::DigestLegacyItem

// =================================================================================================
// P2_MetaHandler::DigestLegacyRelations
// =====================================

void P2_MetaHandler::DigestLegacyRelations ( MD5_CTX & md5Context )
{
	XMP_StringPtr p2NS = this->p2NS.c_str();
	XML_Node * legacyContext = this->clipContent->GetNamedElement ( p2NS, "Relation" );

	if ( legacyContext != 0 ) {

		this->DigestLegacyItem ( md5Context, legacyContext, "GlobalShotID" );
		XML_Node * legacyConnectionContext = legacyContext = this->clipContent->GetNamedElement ( p2NS, "Connection" );

		if ( legacyConnectionContext != 0 ) {

			legacyContext = legacyConnectionContext->GetNamedElement ( p2NS, "Top" );

			if ( legacyContext != 0 ) {
				this->DigestLegacyItem ( md5Context, legacyContext, "GlobalClipID" );
			}

			legacyContext = legacyConnectionContext->GetNamedElement ( p2NS, "Previous" );

			if ( legacyContext != 0 ) {
				this->DigestLegacyItem ( md5Context, legacyContext, "GlobalClipID" );
			}

			legacyContext = legacyConnectionContext->GetNamedElement ( p2NS, "Next" );

			if ( legacyContext != 0 ) {
				this->DigestLegacyItem ( md5Context, legacyContext, "GlobalClipID" );
			}

		}

	}

}	// P2_MetaHandler::DigestLegacyRelations

// =================================================================================================
// P2_MetaHandler::SetXMPPropertyFromLegacyXML
// ===========================================

void P2_MetaHandler::SetXMPPropertyFromLegacyXML ( bool digestFound,
												   XML_NodePtr legacyContext,
												   XMP_StringPtr schemaNS,
												   XMP_StringPtr propName,
												   XMP_StringPtr legacyPropName,
												   bool isLocalized )
{

	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( schemaNS, propName )) ) {

		XMP_StringPtr p2NS = this->p2NS.c_str();
		XML_NodePtr legacyProp = legacyContext->GetNamedElement ( p2NS, legacyPropName );

		if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {
			if ( isLocalized ) {
				this->xmpObj.SetLocalizedText ( schemaNS, propName, "", "x-default", legacyProp->GetLeafContentValue(), kXMP_DeleteExisting );
			} else {
				this->xmpObj.SetProperty ( schemaNS, propName, legacyProp->GetLeafContentValue(), kXMP_DeleteExisting );
			}
			this->containsXMP = true;
		}

	}

}	// P2_MetaHandler::SetXMPPropertyFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetRelationsFromLegacyXML
// =========================================

void P2_MetaHandler::SetRelationsFromLegacyXML ( bool digestFound )
{
	XMP_StringPtr p2NS = this->p2NS.c_str();
	XML_NodePtr legacyRelationContext = this->clipContent->GetNamedElement ( p2NS, "Relation" );

	// P2 Relation blocks are optional -- they're only present when a clip is part of a multi-clip shot.

	if ( legacyRelationContext != 0 ) {

		if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DC, "relation" )) ) {

			XML_NodePtr legacyProp = legacyRelationContext->GetNamedElement ( p2NS, "GlobalShotID" );
			std::string relationString;

			if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {

				this->xmpObj.DeleteProperty ( kXMP_NS_DC, "relation" );
				relationString = std::string("globalShotID:") + legacyProp->GetLeafContentValue();
				this->xmpObj.AppendArrayItem ( kXMP_NS_DC, "relation", kXMP_PropArrayIsUnordered, relationString );
				this->containsXMP = true;

				XML_NodePtr legacyConnectionContext = legacyRelationContext->GetNamedElement ( p2NS, "Connection" );

				if ( legacyConnectionContext != 0 ) {

					XML_NodePtr legacyContext = legacyConnectionContext->GetNamedElement ( p2NS, "Top" );

					if ( legacyContext != 0 ) {
						legacyProp = legacyContext->GetNamedElement ( p2NS, "GlobalClipID" );

						if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {
							relationString = std::string("topGlobalClipID:") + legacyProp->GetLeafContentValue();
							this->xmpObj.AppendArrayItem ( kXMP_NS_DC, "relation", kXMP_PropArrayIsUnordered, relationString );
						}
					}

					legacyContext = legacyConnectionContext->GetNamedElement ( p2NS, "Previous" );

					if ( legacyContext != 0 ) {
						legacyProp = legacyContext->GetNamedElement ( p2NS, "GlobalClipID" );

						if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {
							relationString = std::string("previousGlobalClipID:") + legacyProp->GetLeafContentValue();
							this->xmpObj.AppendArrayItem ( kXMP_NS_DC, "relation", kXMP_PropArrayIsUnordered, relationString );
						}
					}

					legacyContext = legacyConnectionContext->GetNamedElement ( p2NS, "Next" );

					if ( legacyContext != 0 ) {
						legacyProp = legacyContext->GetNamedElement ( p2NS, "GlobalClipID" );

						if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {
							relationString = std::string("nextGlobalClipID:") + legacyProp->GetLeafContentValue();
							this->xmpObj.AppendArrayItem ( kXMP_NS_DC, "relation", kXMP_PropArrayIsUnordered, relationString );
						}
					}

				}

			}

		}

	}

}	// P2_MetaHandler::SetRelationsFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetAudioInfoFromLegacyXML
// =========================================

void P2_MetaHandler::SetAudioInfoFromLegacyXML ( bool digestFound )
{
	XMP_StringPtr p2NS = this->p2NS.c_str();
	XML_NodePtr legacyAudioContext = this->clipContent->GetNamedElement ( p2NS, "EssenceList" );

	if ( legacyAudioContext != 0 ) {

		legacyAudioContext = legacyAudioContext->GetNamedElement ( p2NS, "Audio" );

		if ( legacyAudioContext != 0 ) {

			this->SetXMPPropertyFromLegacyXML ( digestFound, legacyAudioContext, kXMP_NS_DM, "audioSampleRate", "SamplingRate", false );

			if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "audioSampleType" )) ) {
				XML_NodePtr legacyProp = legacyAudioContext->GetNamedElement ( p2NS, "BitsPerSample" );

				if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {

					const std::string p2BitsPerSample = legacyProp->GetLeafContentValue();
					std::string dmSampleType;

					if ( p2BitsPerSample == "16" ) {
						dmSampleType = "16Int";
					} else if ( p2BitsPerSample == "24" ) {
						dmSampleType = "32Int";
					}

					if ( ! dmSampleType.empty() ) {
						this->xmpObj.SetProperty ( kXMP_NS_DM, "audioSampleType", dmSampleType, kXMP_DeleteExisting );
						this->containsXMP = true;
					}

				}

			}

		}

	}

}	// P2_MetaHandler::SetAudioInfoFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetVideoInfoFromLegacyXML
// =========================================

void P2_MetaHandler::SetVideoInfoFromLegacyXML ( bool digestFound )
{
	XMP_StringPtr p2NS = this->p2NS.c_str();
	XML_NodePtr legacyVideoContext = this->clipContent->GetNamedElement ( p2NS, "EssenceList" );

	if ( legacyVideoContext != 0 ) {

		legacyVideoContext = legacyVideoContext->GetNamedElement ( p2NS, "Video" );

		if ( legacyVideoContext != 0 ) {
			this->SetVideoFrameInfoFromLegacyXML ( legacyVideoContext, digestFound );
			this->SetStartTimecodeFromLegacyXML ( legacyVideoContext, digestFound );
			this->SetXMPPropertyFromLegacyXML ( digestFound, legacyVideoContext, kXMP_NS_DM, "videoFrameRate", "FrameRate", false );
		}

	}

}	// P2_MetaHandler::SetVideoInfoFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetDurationFromLegacyXML
// ========================================

void P2_MetaHandler::SetDurationFromLegacyXML ( bool digestFound )
{

	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "duration" )) ) {

		XMP_StringPtr p2NS = this->p2NS.c_str();
		XML_NodePtr legacyDurationProp = this->clipContent->GetNamedElement ( p2NS, "Duration" );
		XML_NodePtr legacyEditUnitProp = this->clipContent->GetNamedElement ( p2NS, "EditUnit" );

		if ( (legacyDurationProp != 0) && ( legacyEditUnitProp != 0 ) &&
			 legacyDurationProp->IsLeafContentNode() && legacyEditUnitProp->IsLeafContentNode() ) {

			this->xmpObj.DeleteProperty ( kXMP_NS_DM, "duration" );
			this->xmpObj.SetStructField ( kXMP_NS_DM, "duration",
										  kXMP_NS_DM, "value", legacyDurationProp->GetLeafContentValue() );

			this->xmpObj.SetStructField ( kXMP_NS_DM, "duration",
										  kXMP_NS_DM, "scale", legacyEditUnitProp->GetLeafContentValue() );
			this->containsXMP = true;

		}

	}

}	// P2_MetaHandler::SetDurationFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetVideoFrameInfoFromLegacyXML
// ==============================================

void P2_MetaHandler::SetVideoFrameInfoFromLegacyXML ( XML_NodePtr legacyVideoContext, bool digestFound )
{

	//	Map the P2 Codec field to various dynamic media schema fields.
	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "videoFrameSize" )) ) {

		XMP_StringPtr p2NS = this->p2NS.c_str();
		XML_NodePtr legacyProp = legacyVideoContext->GetNamedElement ( p2NS, "Codec" );

		if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {

			const std::string p2Codec = legacyProp->GetLeafContentValue();
			std::string dmPixelAspectRatio, dmVideoCompressor, dmWidth, dmHeight;

			if ( p2Codec == "DV25_411" ) {
				dmWidth = "720";
				dmVideoCompressor = "DV25 4:1:1";
			} else if ( p2Codec == "DV25_420" ) {
				dmWidth = "720";
				dmVideoCompressor = "DV25 4:2:0";
			} else if ( p2Codec == "DV50_422" ) {
				dmWidth = "720";
				dmVideoCompressor = "DV50 4:2:2";
			} else if ( ( p2Codec == "DV100_1080/59.94i" ) || ( p2Codec == "DV100_1080/50i" ) ) {
				dmVideoCompressor = "DV100";
				dmHeight = "1080";

				if ( p2Codec == "DV100_1080/59.94i" ) {
					dmWidth = "1280";
					dmPixelAspectRatio = "3/2";
				} else {
					dmWidth = "1440";
					dmPixelAspectRatio = "1920/1440";
				}
			} else if ( ( p2Codec == "DV100_720/59.94p" ) || ( p2Codec == "DV100_720/50p" ) ) {
				dmVideoCompressor = "DV100";
				dmHeight = "720";
				dmWidth = "960";
				dmPixelAspectRatio = "1920/1440";
			} else if ( ( p2Codec.compare ( 0, 6, "AVC-I_" ) == 0 ) ) {

				// This is AVC-Intra footage. The framerate and PAR depend on the "class" attribute in the P2 XML.
				const XMP_StringPtr codecClass = legacyProp->GetAttrValue( "Class" );

				if ( XMP_LitMatch ( codecClass, "100" ) ) {

						dmVideoCompressor = "AVC-Intra 100";
						dmPixelAspectRatio = "1/1";

					   if ( p2Codec.compare ( 6, 4, "1080" ) == 0 ) {
						   dmHeight = "1080";
						   dmWidth = "1920";
					   } else if ( p2Codec.compare ( 6, 3, "720" ) == 0 ) {
						   dmHeight = "720";
						   dmWidth = "1280";
					   }

				} else if ( XMP_LitMatch ( codecClass, "50" ) ) {

					dmVideoCompressor = "AVC-Intra 50";
					dmPixelAspectRatio = "1920/1440";

					if ( p2Codec.compare ( 6, 4, "1080" ) == 0 ) {
						dmHeight = "1080";
						dmWidth = "1440";
					} else if ( p2Codec.compare ( 6, 3, "720" ) == 0 ) {
						dmHeight = "720";
						dmWidth = "960";
					}

				} else {
					//	Unknown codec class -- we don't have enough info to determine the
					//	codec, PAR, or aspect ratio
					dmVideoCompressor = "AVC-Intra";
				}
			}

			if ( dmWidth == "720" ) {

				//	This is SD footage -- calculate the frame height and pixel aspect ratio using the legacy P2
				//	FrameRate and AspectRatio fields.

				legacyProp = legacyVideoContext->GetNamedElement ( p2NS, "FrameRate" );
				if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {

					const std::string p2FrameRate = legacyProp->GetLeafContentValue();

					legacyProp = legacyVideoContext->GetNamedElement ( p2NS, "AspectRatio" );

					if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {
						const std::string p2AspectRatio = legacyProp->GetLeafContentValue();

						if ( p2FrameRate == "50i" ) {
							//	Standard Definition PAL.
							dmHeight = "576";
							if ( p2AspectRatio == "4:3" ) {
								dmPixelAspectRatio = "768/702";
							} else if ( p2AspectRatio == "16:9" ) {
								dmPixelAspectRatio = "1024/702";
							}
						} else if ( p2FrameRate == "59.94i" ) {
							//	Standard Definition NTSC.
							dmHeight = "480";
							if ( p2AspectRatio == "4:3" ) {
								dmPixelAspectRatio = "10/11";
							} else if ( p2AspectRatio == "16:9" ) {
								dmPixelAspectRatio = "40/33";
							}
						}

					}
				}
			}

			if ( ! dmPixelAspectRatio.empty() ) {
				this->xmpObj.SetProperty ( kXMP_NS_DM, "videoPixelAspectRatio", dmPixelAspectRatio, kXMP_DeleteExisting );
				this->containsXMP = true;
			}

			if ( ! dmVideoCompressor.empty() ) {
				this->xmpObj.SetProperty ( kXMP_NS_DM, "videoCompressor", dmVideoCompressor, kXMP_DeleteExisting );
				this->containsXMP = true;
			}

			if ( ( ! dmWidth.empty() ) && ( ! dmHeight.empty() ) ) {
				this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "w", dmWidth, 0 );
				this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "h", dmHeight, 0 );
				this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "unit", "pixel", 0 );
				this->containsXMP = true;
			}

		}

	}

}	// P2_MetaHandler::SetVideoFrameInfoFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetStartTimecodeFromLegacyXML
// =============================================

void P2_MetaHandler::SetStartTimecodeFromLegacyXML ( XML_NodePtr legacyVideoContext, bool digestFound )
{

	//	Translate start timecode to the format specified by the dynamic media schema.
	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "startTimecode" )) ) {

		XMP_StringPtr p2NS = this->p2NS.c_str();
		XML_NodePtr legacyProp = legacyVideoContext->GetNamedElement ( p2NS, "StartTimecode" );

		if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {

			std::string p2StartTimecode = legacyProp->GetLeafContentValue();

			legacyProp = legacyVideoContext->GetNamedElement ( p2NS, "FrameRate" );

			if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {

				const std::string p2FrameRate = legacyProp->GetLeafContentValue();
				XMP_StringPtr p2DropFrameFlag = legacyProp->GetAttrValue ( "DropFrameFlag" );
				if ( p2DropFrameFlag == 0 ) p2DropFrameFlag = "";	// Make tests easier.
				std::string dmTimeFormat;

				if ( ( p2FrameRate == "50i" ) || ( p2FrameRate == "25p" ) ) {

					dmTimeFormat = "25Timecode";

				} else if ( p2FrameRate == "23.98p" ) {

					dmTimeFormat = "23976Timecode";

				} else if ( p2FrameRate == "50p" ) {

					dmTimeFormat = "50Timecode";

				} else if ( p2FrameRate == "59.94p" ) {

					if ( XMP_LitMatch ( p2DropFrameFlag, "true" ) ) {
						dmTimeFormat = "5994DropTimecode";
					} else if ( XMP_LitMatch ( p2DropFrameFlag, "false" ) ) {
						dmTimeFormat = "5994NonDropTimecode";
					}

				} else if ( (p2FrameRate == "59.94i") || (p2FrameRate == "29.97p") ) {

					if ( p2DropFrameFlag != 0 ) {

						if ( XMP_LitMatch ( p2DropFrameFlag, "false" ) ) {

							dmTimeFormat = "2997NonDropTimecode";

						} else if ( XMP_LitMatch ( p2DropFrameFlag, "true" ) ) {

							//	Drop frame NTSC timecode uses semicolons instead of colons as separators.
							std::string::iterator currCharIt = p2StartTimecode.begin();
							const std::string::iterator charsEndIt = p2StartTimecode.end();

							for ( ; currCharIt != charsEndIt; ++currCharIt ) {
								if ( *currCharIt == ':' ) *currCharIt = ';';
							}

							dmTimeFormat = "2997DropTimecode";

						}

					}

				}

				if ( ( ! p2StartTimecode.empty() ) && ( ! dmTimeFormat.empty() ) ) {
					this->xmpObj.SetStructField ( kXMP_NS_DM, "startTimecode", kXMP_NS_DM, "timeValue", p2StartTimecode, 0 );
					this->xmpObj.SetStructField ( kXMP_NS_DM, "startTimecode", kXMP_NS_DM, "timeFormat", dmTimeFormat, 0 );
					this->containsXMP = true;
				}

			}

		}

	}

}	// P2_MetaHandler::SetStartTimecodeFromLegacyXML


// =================================================================================================
// P2_MetaHandler::SetGPSPropertyFromLegacyXML
// ===========================================

void P2_MetaHandler::SetGPSPropertyFromLegacyXML  ( XML_NodePtr legacyLocationContext, bool digestFound, XMP_StringPtr propName, XMP_StringPtr legacyPropName )
{

	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_EXIF, propName )) ) {

		XMP_StringPtr p2NS = this->p2NS.c_str();
		XML_NodePtr legacyGPSProp = legacyLocationContext->GetNamedElement ( p2NS, legacyPropName );

		if ( ( legacyGPSProp != 0 ) && legacyGPSProp->IsLeafContentNode() ) {

			this->xmpObj.DeleteProperty ( kXMP_NS_EXIF, propName );

			const std::string legacyGPSValue = legacyGPSProp->GetLeafContentValue();

			if ( ! legacyGPSValue.empty() ) {

				//	Convert from decimal to sexagesimal GPS coordinates
				char direction = '\0';
				double degrees = 0.0;
				const int numFieldsRead = sscanf ( legacyGPSValue.c_str(), "%c%lf", &direction, &degrees );

				if ( numFieldsRead == 2 ) {
					double wholeDegrees = 0.0;
					const double fractionalDegrees = modf ( degrees, &wholeDegrees );
					const double minutes = fractionalDegrees * 60.0;
					char xmpValue [128];

					sprintf ( xmpValue, "%d,%.5lf%c", static_cast<int>(wholeDegrees), minutes, direction );
					this->xmpObj.SetProperty ( kXMP_NS_EXIF, propName, xmpValue );
					this->containsXMP = true;

				}

			}

		}

	}

}	// P2_MetaHandler::SetGPSPropertyFromLegacyXML

// =================================================================================================
// P2_MetaHandler::SetAltitudeFromLegacyXML
// ========================================

void P2_MetaHandler::SetAltitudeFromLegacyXML  ( XML_NodePtr legacyLocationContext, bool digestFound )
{

	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_EXIF, "GPSAltitude" )) ) {

		XMP_StringPtr p2NS = this->p2NS.c_str();
		XML_NodePtr legacyAltitudeProp = legacyLocationContext->GetNamedElement ( p2NS, "Altitude" );

		if ( ( legacyAltitudeProp != 0 ) && legacyAltitudeProp->IsLeafContentNode() ) {

			this->xmpObj.DeleteProperty ( kXMP_NS_EXIF, "GPSAltitude" );

			const std::string legacyGPSValue = legacyAltitudeProp->GetLeafContentValue();

			if ( ! legacyGPSValue.empty() ) {

				int altitude = 0;

				if ( sscanf ( legacyGPSValue.c_str(), "%d", &altitude ) == 1) {

					if ( altitude >= 0 ) {
						// At or above sea level.
						this->xmpObj.SetProperty ( kXMP_NS_EXIF, "GPSAltitudeRef", "0" );
					} else {
						// Below sea level.
						altitude = -altitude;
						this->xmpObj.SetProperty ( kXMP_NS_EXIF, "GPSAltitudeRef", "1" );
					}

					char xmpValue [128];

					sprintf ( xmpValue, "%d/1", altitude );
					this->xmpObj.SetProperty ( kXMP_NS_EXIF, "GPSAltitude", xmpValue );
					this->containsXMP = true;

				}

			}

		}

	}

}	// P2_MetaHandler::SetAltitudeFromLegacyXML

// =================================================================================================
// P2_MetaHandler::ForceChildElement
// =================================

XML_Node * P2_MetaHandler::ForceChildElement ( XML_Node * parent, XMP_StringPtr localName, XMP_Int32 indent , XMP_Bool insertAtFront  )
{
	XML_Node * wsNodeBefore, * wsNodeAfter;
	XML_Node * childNode = parent->GetNamedElement ( this->p2NS.c_str(), localName );
	//

	if ( childNode == 0 ) {

		// The indenting is a hack, assuming existing 2 spaces per level.

		wsNodeBefore = new XML_Node ( parent, "", kCDataNode );
		wsNodeBefore->value = "  ";	// Add 2 spaces to the existing WS before the parent's close tag.

		childNode = new XML_Node ( parent, localName, kElemNode );
		childNode->ns = parent->ns;
		childNode->nsPrefixLen = parent->nsPrefixLen;
		childNode->name.insert ( 0, parent->name, 0, parent->nsPrefixLen );

		wsNodeAfter = new XML_Node ( parent, "", kCDataNode );
		wsNodeAfter->value = '\n';
		for ( ; indent > 1; --indent ) wsNodeAfter->value += "  ";	// Indent less 1, to "outdent" the parent's close.

		if(insertAtFront){
			// we are asked to insert this child as the first child of it's parent.So if P is the parent and B,C are children
			// already present. Then if we add a new child A as it's first child then we need to first add new line character right
			// after "<P>" and then proper indentation to bring them on the level of other children.
			//<P>
			//	  <B>
			//	  <C>
			//</P>
			std::vector<XML_Node *> indentedNode;
			indentedNode.push_back(wsNodeAfter);
			indentedNode.push_back(wsNodeBefore);
			indentedNode.push_back(childNode);
			parent->content.insert(parent->content.begin(), indentedNode.begin(), indentedNode.end());
		}
		else{
			parent->content.push_back(wsNodeBefore);
			parent->content.push_back(childNode);
			parent->content.push_back(wsNodeAfter);
		}

	}

	return childNode;

}	// P2_MetaHandler::ForceChildElement

// =================================================================================================
// P2_MetaHandler::MakeLegacyDigest
// =================================

// *** Early hack version.

#define kHexDigits "0123456789ABCDEF"

void P2_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
{
	digestStr->erase();
	if ( this->clipMetadata == 0 ) return;	// Bail if we don't have any legacy XML.
	XMP_Assert ( this->expat != 0 );

	XMP_StringPtr p2NS = this->p2NS.c_str();
	XML_NodePtr legacyContext;
	MD5_CTX    md5Context;
	unsigned char digestBin [16];
	MD5Init ( &md5Context );

	legacyContext = this->clipContent;
	this->DigestLegacyItem ( md5Context, legacyContext, "ClipName" );
	this->DigestLegacyItem ( md5Context, legacyContext, "GlobalClipID" );
	this->DigestLegacyItem ( md5Context, legacyContext, "Duration" );
	this->DigestLegacyItem ( md5Context, legacyContext, "EditUnit" );
	this->DigestLegacyRelations ( md5Context );

	legacyContext = this->clipContent->GetNamedElement ( p2NS, "EssenceList" );

	if ( legacyContext != 0 ) {

		XML_NodePtr videoContext = legacyContext->GetNamedElement ( p2NS, "Video" );

		if ( videoContext != 0 ) {
			this->DigestLegacyItem ( md5Context, videoContext, "AspectRatio" );
			this->DigestLegacyItem ( md5Context, videoContext, "Codec" );
			this->DigestLegacyItem ( md5Context, videoContext, "FrameRate" );
			this->DigestLegacyItem ( md5Context, videoContext, "StartTimecode" );
		}

		XML_NodePtr audioContext = legacyContext->GetNamedElement ( p2NS, "Audio" );

		if ( audioContext != 0 ) {
			this->DigestLegacyItem ( md5Context, audioContext, "SamplingRate" );
			this->DigestLegacyItem ( md5Context, audioContext, "BitsPerSample" );
		}

	}

	legacyContext = this->clipMetadata;
	this->DigestLegacyItem ( md5Context, legacyContext, "UserClipName" );
	this->DigestLegacyItem ( md5Context, legacyContext, "ShotMark" );

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Access" );
	/* Rather return than create the digest because the "Access" element is listed as "required" in the P2 spec.
	So a P2 file without an "Access" element does not follow the spec and might be corrupt.*/
	if ( legacyContext == 0 ) return;

	this->DigestLegacyItem ( md5Context, legacyContext, "Creator" );
	this->DigestLegacyItem ( md5Context, legacyContext, "CreationDate" );
	this->DigestLegacyItem ( md5Context, legacyContext, "LastUpdateDate" );

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Shoot" );

	if ( legacyContext != 0 ) {
		this->DigestLegacyItem ( md5Context, legacyContext, "Shooter" );

		legacyContext = legacyContext->GetNamedElement ( p2NS, "Location" );

		if ( legacyContext != 0 ) {
			this->DigestLegacyItem ( md5Context, legacyContext, "PlaceName" );
			this->DigestLegacyItem ( md5Context, legacyContext, "Longitude" );
			this->DigestLegacyItem ( md5Context, legacyContext, "Latitude" );
			this->DigestLegacyItem ( md5Context, legacyContext, "Altitude" );
		}
	}

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Scenario" );

	if ( legacyContext != 0 ) {
		this->DigestLegacyItem ( md5Context, legacyContext, "SceneNo." );
		this->DigestLegacyItem ( md5Context, legacyContext, "TakeNo." );
	}

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Device" );

	if ( legacyContext != 0 ) {
		this->DigestLegacyItem ( md5Context, legacyContext, "Manufacturer" );
		this->DigestLegacyItem ( md5Context, legacyContext, "SerialNo." );
		this->DigestLegacyItem ( md5Context, legacyContext, "ModelName" );
	}

	MD5Final ( digestBin, &md5Context );

	char buffer [40];
	for ( int in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digestBin[in];
		buffer[out]   = kHexDigits [ byte >> 4 ];
		buffer[out+1] = kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;
	digestStr->append ( buffer );

}	// P2_MetaHandler::MakeLegacyDigest

// =================================================================================================
// P2_MetaHandler::GetFileModDate
// ==============================

static inline bool operator< ( const XMP_DateTime & left, const XMP_DateTime & right ) {
	int compare = SXMPUtils::CompareDateTime ( left, right );
	return (compare < 0);
}

bool P2_MetaHandler::GetFileModDate ( XMP_DateTime * modDate )
{

	// The P2 locations of metadata:
	//	CONTENTS/
	// 		CLIP/
	// 			0001AB.XML
	// 			0001AB.XMP

	bool ok, haveDate = false;
	std::string fullPath;
	XMP_DateTime oneDate, junkDate;
	if ( modDate == 0 ) modDate = &junkDate;

	ok = this->MakeClipFilePath ( &fullPath, ".XML", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	ok = this->MakeClipFilePath ( &fullPath, ".XMP", true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
	if ( ok ) {
		if ( (! haveDate) || (*modDate < oneDate) ) *modDate = oneDate;
		haveDate = true;
	}

	return haveDate;

}	// P2_MetaHandler::GetFileModDate

// =================================================================================================
// P2_MetaHandler::FillMetadataFiles
// =================================
void P2_MetaHandler::FillMetadataFiles ( std::vector<std::string>* metadataFiles )
{
	std::string noExtPath, filePath;

	noExtPath = rootPath + kDirChar + "CONTENTS" + kDirChar + "CLIP" + kDirChar + clipName;

	filePath = noExtPath + ".XMP";
	metadataFiles->push_back ( filePath );
	filePath = noExtPath + ".XML";
	metadataFiles->push_back ( filePath );

}	// 	FillMetadataFiles_P2

// =================================================================================================
// P2_MetaHandler::IsMetadataWritable
// =======================================

bool P2_MetaHandler::IsMetadataWritable ( )
{
	std::vector<std::string> metadataFiles;
	FillMetadataFiles(&metadataFiles);
	std::vector<std::string>::iterator itr = metadataFiles.begin();
	// Check whether sidecar is writable, if not then check if it can be created.
	bool xmpWritable = Host_IO::Writable( itr->c_str(), true );
	// Check if legacy XML is writable.
	bool xmlWritable = Host_IO::Writable( (++itr)->c_str(), false );
	return (xmlWritable && xmpWritable);
}// P2_MetaHandler::IsMetadataWritable


// =================================================================================================
// P2_MetaHandler::FillAssociatedResources
// ======================================
void P2_MetaHandler::FillAssociatedResources ( std::vector<std::string> * resourceList )
{
	// The possible associated resources:
	//	CONTENTS/
	//		CLIP/
	//			XXXXXX.XML			XXXXXX is clip name
	//			XXXXXX.XMP
	//		VIDEO/
	//			XXXXXX.MXF
	//		AUDIO/
	//			XXXXXXNN.MXF	NN is a counter which can go from 00 to 15.
	//		ICON/
	//			XXXXXX.BMP
	//		VOICE/
	//			XXXXXXNN.WAV	NN is a counter which can go from 00 to 99.
	//		PROXY/
	//			XXXXXX.MP4
	//			XXXXXX.BIN

	XMP_VarString contentsPath = this->rootPath + kDirChar + "CONTENTS" + kDirChar;
	XMP_VarString path;
	
	//Add RootPath
	path = this->rootPath + kDirChar;
	PackageFormat_Support::AddResourceIfExists(resourceList, path);

	std::string clipPathNoExt = contentsPath + "CLIP" + kDirChar + this->clipName;
	// Get the files present inside CLIP folder.
	path = clipPathNoExt + ".XML";
	PackageFormat_Support::AddResourceIfExists(resourceList, path);
	path = clipPathNoExt + ".XMP";
	PackageFormat_Support::AddResourceIfExists(resourceList, path);

	// Get the files present inside VIDEO folder.
	path = contentsPath + "VIDEO" + kDirChar + this->clipName + ".MXF";
	PackageFormat_Support::AddResourceIfExists(resourceList, path);

	// Get the files present inside AUDIO folder.
	path = contentsPath + "AUDIO" + kDirChar;
	XMP_VarString regExp;
	regExp = "^" + this->clipName + "\\d\\d.MXF$";
	IOUtils::GetMatchingChildren ( *resourceList, path, regExp, false, true, true );

	// Get the files present inside ICON folder.
	path = contentsPath + "ICON" + kDirChar + this->clipName + ".BMP";
	PackageFormat_Support::AddResourceIfExists(resourceList, path);

	// Get the files present inside VOICE folder.
	path = contentsPath + "VOICE" + kDirChar;
	regExp = "^" + clipName + "\\d\\d.WAV$";
	IOUtils::GetMatchingChildren ( *resourceList, path, regExp, false, true, true );

	// Get the files present inside PROXY folder.
	std::string proxyPathNoExt = contentsPath + "PROXY" + kDirChar + this->clipName;

	path = proxyPathNoExt + ".MP4";
	PackageFormat_Support::AddResourceIfExists(resourceList, path);
	path = proxyPathNoExt + ".BIN";
	PackageFormat_Support::AddResourceIfExists(resourceList, path);
}	// P2_MetaHandler::FillAssociatedResources

// =================================================================================================
// P2_MetaHandler::CacheFileData
// =============================

void P2_MetaHandler::CacheFileData()
{
	XMP_Assert ( ! this->containsXMP );

	if ( this->parent->UsesClientIO() ) {
		XMP_Throw ( "P2 cannot be used with client-managed I/O", kXMPErr_InternalFailure );
	}

	// Make sure the clip's .XMP file exists.

	std::string xmpPath;
	this->MakeClipFilePath ( &xmpPath, ".XMP" );
	if ( ! Host_IO::Exists ( xmpPath.c_str() ) ) return;	// No XMP.

	// Read the entire .XMP file. We know the XMP exists, New_XMPFiles_IO is supposed to return 0
	// only if the file does not exist.

	bool readOnly = XMP_OptionIsClear ( this->parent->openFlags, kXMPFiles_OpenForUpdate );

	XMP_Assert ( this->parent->ioRef == 0 );
	XMPFiles_IO* xmpFile =  XMPFiles_IO::New_XMPFiles_IO ( xmpPath.c_str(), readOnly );
	if ( xmpFile == 0 ) XMP_Throw ( "P2 XMP file open failure", kXMPErr_InternalFailure );
	this->parent->ioRef = xmpFile;

	XMP_Int64 xmpLen = xmpFile->Length();
	if ( xmpLen > 100*1024*1024 ) {
		XMP_Throw ( "P2 XMP is outrageously large", kXMPErr_InternalFailure );	// Sanity check.
	}

	this->xmpPacket.erase();
	this->xmpPacket.append ( (size_t)xmpLen, ' ' );

	XMP_Int32 ioCount = xmpFile->ReadAll ( (void*)this->xmpPacket.data(), (XMP_Int32)xmpLen );

	this->packetInfo.offset = 0;
	this->packetInfo.length = (XMP_Int32)xmpLen;
	FillPacketInfo ( this->xmpPacket, &this->packetInfo );

	this->containsXMP = true;

}	// P2_MetaHandler::CacheFileData

// =================================================================================================
// P2_MetaHandler::ProcessXMP
// ==========================

void P2_MetaHandler::ProcessXMP()
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

	std::string xmlPath;
	this->MakeClipFilePath ( &xmlPath, ".XML" );

	Host_IO::FileRef hostRef = Host_IO::Open ( xmlPath.c_str(), Host_IO::openReadOnly );
	if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
	XMPFiles_IO xmlFile ( hostRef, xmlPath.c_str(), Host_IO::openReadOnly );

	this->expat = XMP_NewExpatAdapter ( ExpatAdapter::kUseLocalNamespaces );
	if ( this->expat == 0 ) XMP_Throw ( "P2_MetaHandler: Can't create Expat adapter", kXMPErr_NoMemory );

	XMP_Uns8 buffer [64*1024];
	while ( true ) {
		XMP_Int32 ioCount = xmlFile.Read ( buffer, sizeof(buffer) );
		if ( ioCount == 0 ) break;
		this->expat->ParseBuffer ( buffer, ioCount, false /* not the end */ );
	}
	this->expat->ParseBuffer ( 0, 0, true );	// End the parse.

	xmlFile.Close();

	// The root element should be P2Main in some namespace. At least 2 different namespaces are in
	// use (ending in "v3.0" and "v3.1"). Take whatever this file uses.

	XML_Node & xmlTree = this->expat->tree;
	XML_NodePtr rootElem = 0;

	for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
		if ( xmlTree.content[i]->kind == kElemNode ) {
			rootElem = xmlTree.content[i];
		}
	}

	if ( rootElem == 0 ) CleanupAndExit
	XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
	if ( ! XMP_LitMatch ( rootLocalName, "P2Main" ) ) CleanupAndExit

	this->p2NS = rootElem->ns;

	// Now find ClipMetadata element and check the legacy digest.

	XMP_StringPtr p2NS = this->p2NS.c_str();
	XML_NodePtr legacyContext, legacyProp;

	legacyContext = rootElem->GetNamedElement ( p2NS, "ClipContent" );
	if ( legacyContext == 0 ) CleanupAndExit

	this->clipContent = legacyContext;	// ! Save the ClipContext pointer for other use.

	legacyContext = legacyContext->GetNamedElement ( p2NS, "ClipMetadata" );
	if ( legacyContext == 0 ) CleanupAndExit

	this->clipMetadata = legacyContext;	// ! Save the ClipMetadata pointer for other use.

	std::string oldDigest, newDigest;
	bool digestFound = this->xmpObj.GetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "P2", &oldDigest, 0 );
	if ( digestFound ) {
		this->MakeLegacyDigest ( &newDigest );
		if ( oldDigest == newDigest ) CleanupAndExit
	}

	// If we get here we need find and import the actual legacy elements using the current namespace.
	// Either there is no old digest in the XMP, or the digests differ. In the former case keep any
	// existing XMP, in the latter case take new legacy values.
	this->SetXMPPropertyFromLegacyXML ( digestFound, this->clipContent, kXMP_NS_DC, "title", "ClipName", true );
	this->SetXMPPropertyFromLegacyXML ( digestFound, this->clipContent, kXMP_NS_DC, "identifier", "GlobalClipID", false );
	this->SetDurationFromLegacyXML (digestFound );
	this->SetRelationsFromLegacyXML ( digestFound );
	this->SetXMPPropertyFromLegacyXML ( digestFound, this->clipMetadata, kXMP_NS_DM, "shotName", "UserClipName", false );
	this->SetAudioInfoFromLegacyXML ( digestFound );
	this->SetVideoInfoFromLegacyXML ( digestFound );

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Access" );
	if ( legacyContext == 0 ) CleanupAndExit

	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DC, "creator" )) ) {
		legacyProp = legacyContext->GetNamedElement ( p2NS, "Creator" );
		if ( (legacyProp != 0) && legacyProp->IsLeafContentNode() ) {
			this->xmpObj.DeleteProperty ( kXMP_NS_DC, "creator" );
			this->xmpObj.AppendArrayItem ( kXMP_NS_DC, "creator", kXMP_PropArrayIsOrdered,
										   legacyProp->GetLeafContentValue() );
			this->containsXMP = true;
		}
	}

	this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_XMP, "CreateDate", "CreationDate", false );
	this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_XMP, "ModifyDate", "LastUpdateDate", false );

	if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "good" )) ) {
		legacyProp = this->clipMetadata->GetNamedElement ( p2NS, "ShotMark" );
		if ( (legacyProp == 0) || (! legacyProp->IsLeafContentNode()) ) {
			this->xmpObj.DeleteProperty ( kXMP_NS_DM, "good" );
		} else {
			XMP_StringPtr markValue = legacyProp->GetLeafContentValue();
			if ( markValue == 0 ) {
				this->xmpObj.DeleteProperty ( kXMP_NS_DM, "good" );
			} else if ( XMP_LitMatch ( markValue, "true" ) || XMP_LitMatch ( markValue, "1" ) ) {
				this->xmpObj.SetProperty_Bool ( kXMP_NS_DM, "good", true, kXMP_DeleteExisting );
				this->containsXMP = true;
			} else if ( XMP_LitMatch ( markValue, "false" ) || XMP_LitMatch ( markValue, "0" ) ) {
				this->xmpObj.SetProperty_Bool ( kXMP_NS_DM, "good", false, kXMP_DeleteExisting );
				this->containsXMP = true;
			}
		}
	}

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Shoot" );
	if ( legacyContext != 0 ) {
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_TIFF, "Artist", "Shooter", false );
		legacyContext = legacyContext->GetNamedElement ( p2NS, "Location" );
	}

	if ( legacyContext != 0 ) {
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_DM, "shotLocation", "PlaceName", false );
		this->SetGPSPropertyFromLegacyXML ( legacyContext, digestFound, "GPSLongitude", "Longitude" );
		this->SetGPSPropertyFromLegacyXML ( legacyContext, digestFound, "GPSLatitude", "Latitude" );
		this->SetAltitudeFromLegacyXML ( legacyContext, digestFound );
	}

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Device" );
	if ( legacyContext != 0 ) {
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_TIFF, "Make", "Manufacturer", false );
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_EXIF_Aux, "SerialNumber", "SerialNo.", false );
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_TIFF, "Model", "ModelName", false );
	}

	legacyContext = this->clipMetadata->GetNamedElement ( p2NS, "Scenario" );
	if ( legacyContext != 0 ) {
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_DM, "scene", "SceneNo.", false );
		this->SetXMPPropertyFromLegacyXML ( digestFound, legacyContext, kXMP_NS_DM, "takeNumber", "TakeNo.", false );
	}

	CleanupAndExit
	#undef CleanupAndExit

}	// P2_MetaHandler::ProcessXMP

// This function adds a dummy attribute to the clipContent/clipMetadata (whichever is non-null)
// with empty value and namespace as xmlns:xsi=http://www.w3.org/2001/XMLSchema-instance
static XML_Node* AddXSINamespace(XML_Node *clipContent, XML_Node *clipMetadata){

	XML_Node *parent = clipContent ? clipContent : clipMetadata;
	if(parent){
		XML_Node *attrWithXSINamespace = new XML_Node ( parent, "xsi:", kCDataNode );
		attrWithXSINamespace->value="";
		attrWithXSINamespace->ns="http://www.w3.org/2001/XMLSchema-instance";
		parent->attrs.push_back(attrWithXSINamespace);
		return parent;
	}

	return NULL;

}

// =================================================================================================
// P2_MetaHandler::UpdateFile
// ==========================
//
// Note that UpdateFile is only called from XMPFiles::CloseFile, so it is OK to close the file here.

void P2_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	this->needsUpdate = false;	// Make sure only called once.

	XMP_Assert ( this->parent->UsesLocalIO() );

	// Update the internal legacy XML tree if we have one, and set the digest in the XMP.

	bool updateLegacyXML = false;

	if ( this->clipMetadata != 0 ) {

		XMP_Assert ( this->expat != 0 );

		bool xmpFound;
		std::string xmpValue;
		XML_Node * xmlNode;

		xmpFound = this->xmpObj.GetLocalizedText ( kXMP_NS_DC, "title", "", "x-default", 0, &xmpValue, 0 );

		if ( xmpFound ) {

			xmlNode = this->ForceChildElement ( this->clipContent, "ClipName", 3, false );

			if ( xmpValue != xmlNode->GetLeafContentValue() ) {
				xmlNode->SetLeafContentValue ( xmpValue.c_str() );
				updateLegacyXML = true;
			}

		}

		xmpFound = this->xmpObj.GetArrayItem ( kXMP_NS_DC, "creator", 1, &xmpValue, 0 );

		if ( xmpFound ) {
			xmlNode = this->ForceChildElement ( this->clipMetadata, "Access", 3, false );

			// "Creator" must be first child of "Access" node else Panasonic P2 Viewer gives an error.
			xmlNode = this->ForceChildElement ( xmlNode, "Creator", 4 , true);
			if ( xmpValue != xmlNode->GetLeafContentValue() ) {
				xmlNode->SetLeafContentValue ( xmpValue.c_str() );
				updateLegacyXML = true;
			}
		}

	}

	std::string newDigest;
	this->MakeLegacyDigest ( &newDigest );
	this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "P2", newDigest.c_str(), kXMP_DeleteExisting );

	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, this->GetSerializeOptions() );

	// -----------------------------------------------------------------------
	// Update the XMP file first, don't let legacy XML failures block the XMP.

	std::string xmpPath;
	this->MakeClipFilePath ( &xmpPath, ".XMP" );

	bool haveXMP = Host_IO::Exists ( xmpPath.c_str() );
	if ( ! haveXMP ) {
		XMP_Assert ( this->parent->ioRef == 0 );
		Host_IO::Create ( xmpPath.c_str() );
		this->parent->ioRef = XMPFiles_IO::New_XMPFiles_IO ( xmpPath.c_str(), Host_IO::openReadWrite );
		if ( this->parent->ioRef == 0 ) XMP_Throw ( "Failure opening P2 XMP file", kXMPErr_ExternalFailure );
	}

	XMP_IO* xmpFile = this->parent->ioRef;
	XMP_Assert ( xmpFile != 0 );
	XIO::ReplaceTextFile ( xmpFile, this->xmpPacket, (haveXMP & doSafeUpdate) );

	// --------------------------------------------
	// Now update the legacy XML file if necessary.

	if ( updateLegacyXML ) {

		std::string legacyXML, xmlPath;

		/*bug # 3217688: xmlns:xsi=http://www.w3.org/2001/XMLSchema-instance namespace must be defined at the 
		root node "P2Main" in legacy XML else Panasonic P2 Viewer gives an error. So we are adding a 
		dummy attribute with this namespace to clipContent/clipMetadata (whichever is non-null) before
		serializing the XML tree. We are also undoing it below after serialization.*/

		XML_Node *parentNode = AddXSINamespace(this->clipContent, this->clipMetadata);
		this->expat->tree.Serialize ( &legacyXML );
		if(parentNode){
			// Remove the dummy attribute added to clipContent/clipMetadata.
			delete parentNode->attrs[parentNode->attrs.size()-1];
			parentNode->attrs.pop_back();
		}

		this->MakeClipFilePath ( &xmlPath, ".XML" );

		bool haveXML = Host_IO::Exists ( xmlPath.c_str() );
		if ( ! haveXML ) Host_IO::Create ( xmlPath.c_str() );

		Host_IO::FileRef hostRef = Host_IO::Open ( xmlPath.c_str(), Host_IO::openReadWrite );
		if ( hostRef == Host_IO::noFileRef ) XMP_Throw ( "Failure opening P2 legacy XML file", kXMPErr_ExternalFailure );
		XMPFiles_IO origXML ( hostRef, xmlPath.c_str(), Host_IO::openReadWrite );
		XIO::ReplaceTextFile ( &origXML, legacyXML, (haveXML & doSafeUpdate) );
		origXML.Close();

	}

}	// P2_MetaHandler::UpdateFile

// =================================================================================================
// P2_MetaHandler::WriteTempFile
// =============================

void P2_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{

	// ! WriteTempFile is not supposed to be called for handlers that own the file.
	XMP_Throw ( "P2_MetaHandler::WriteTempFile should not be called", kXMPErr_InternalFailure );

}	// P2_MetaHandler::WriteTempFile

// =================================================================================================
