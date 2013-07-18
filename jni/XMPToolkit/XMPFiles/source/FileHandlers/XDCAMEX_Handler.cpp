// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
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

#include "XMPFiles/source/FileHandlers/XDCAMEX_Handler.hpp"
#include "XMPFiles/source/FormatSupport/XDCAM_Support.hpp"
#include "third-party/zuid/interfaces/MD5.h"
#include "XMPFiles/source/FormatSupport/PackageFormat_Support.hpp"

using namespace std;

// =================================================================================================
/// \file XDCAMEX_Handler.cpp
/// \brief Folder format handler for XDCAMEX.
///
/// This handler is for the XDCAMEX video format.
///
/// .../MyMovie/
///		BPAV/
///			MEDIAPRO.XML
///			MEDIAPRO.BUP
///			CLPR/
///				709_001_01/
///					709_001_01.SMI
///					709_001_01.MP4
///					709_001_01M01.XML
///					709_001_01R01.BIM
///					709_001_01I01.PPN
///				709_001_02/
///				709_002_01/
///				709_003_01/
///			TAKR/
///				709_001/
///					709_001.SMI
///					709_001M01.XML
///
/// The Backup files (.BUP) are optional. No files or directories other than those listed are
/// allowed in the BPAV directory. The CLPR (clip root) directory may contain only clip directories,
/// which may only contain the clip files listed. The TAKR (take root) direcory may contail only
/// take directories, which may only contain take files. The take root directory can be empty.
/// MEDIPRO.XML contains information on clip and take management.
///
/// Each clip directory contains a media file (.MP4), a clip info file (.SMI), a real time metadata
/// file (.BIM), a non real time metadata file (.XML), and a picture pointer file (.PPN). A take
/// directory conatins a take info and non real time take metadata files.
// =================================================================================================

// =================================================================================================
// XDCAMEX_CheckFormat
// ===================
//
// This version checks for the presence of a top level BPAV directory, and the required files and
// directories immediately within it. The CLPR and TAKR subfolders are required, as is MEDIAPRO.XML.
//
// The state of the string parameters depends on the form of the path passed by the client. If the
// client passed a logical clip path, like ".../MyMovie/012_3456_01", the parameters are:
//   rootPath   - ".../MyMovie"
//   gpName     - empty
//   parentName - empty
//   leafName   - "012_3456_01"
// If the client passed a full file path, like ".../MyMovie/BPAV/CLPR/012_3456_01/012_3456_01M01.XML", they are:
//   rootPath   - ".../MyMovie/BPAV"
//   gpName     - "CLPR"
//   parentName - "012_3456_01"
//   leafName   - "012_3456_01M01"

// ! The common code has shifted the gpName, parentName, and leafName strings to upper case. It has
// ! also made sure that for a logical clip path the rootPath is an existing folder, and that the
// ! file exists for a full file path.

// ! Using explicit '/' as a separator when creating paths, it works on Windows.

bool XDCAMEX_CheckFormat ( XMP_FileFormat format,
						   const std::string & _rootPath,
						   const std::string & gpName,
						   const std::string & parentName,
						   const std::string & _leafName,
						   XMPFiles * parent )
{
	std::string rootPath = _rootPath;
	std::string clipName = _leafName;
	std::string grandGPName;

	std::string bpavPath ( rootPath );

	// Do some initial checks on the gpName and parentName.

	if ( gpName.empty() != parentName.empty() ) return false;	// Must be both empty or both non-empty.

	if ( gpName.empty() ) {

		// This is the logical clip path case. Make sure .../MyMovie/BPAV/CLPR is a folder.
		bpavPath += kDirChar;	// The rootPath was just ".../MyMovie".
		bpavPath += "BPAV";
		if ( Host_IO::GetChildMode ( bpavPath.c_str(), "CLPR" ) != Host_IO::kFMode_IsFolder ) return false;

	} else {

		// This is the explicit file case. Make sure the ancestry is OK, compare using the parent's
		// length since the file can have a suffix like "M01". Use the leafName as the clipName to
		// preserve lower case, but truncate to the parent's length to remove any suffix.

		if ( gpName != "CLPR" ) return false;
		XIO::SplitLeafName ( &rootPath, &grandGPName );
		MakeUpperCase ( &grandGPName );
		if ( grandGPName != "BPAV" ) return false;

		if ( ! XMP_LitNMatch ( parentName.c_str(), clipName.c_str(), parentName.size() ) ) {
			std::string tempName = clipName;
			MakeUpperCase ( &tempName );
			if ( ! XMP_LitNMatch ( parentName.c_str(), tempName.c_str(), parentName.size() ) ) return false;
		}
		
		clipName.erase ( parentName.size() );

	}

	// Check the rest of the required general structure.
	if ( Host_IO::GetChildMode ( bpavPath.c_str(), "TAKR" ) != Host_IO::kFMode_IsFolder ) return false;
	if ( Host_IO::GetChildMode ( bpavPath.c_str(), "MEDIAPRO.XML" ) != Host_IO::kFMode_IsFile ) return false;

	// Make sure the clip's .MP4 and .SMI files exist.
	std::string tempPath ( bpavPath );
	tempPath += kDirChar;
	tempPath += "CLPR";
	tempPath += kDirChar;
	tempPath += clipName;
	tempPath += kDirChar;
	tempPath += clipName;
	tempPath += ".MP4";
	if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFile ) return false;
	tempPath.erase ( tempPath.size()-3 );
	tempPath += "SMI";
	if ( Host_IO::GetFileMode ( tempPath.c_str() ) != Host_IO::kFMode_IsFile ) return false;

	// And now save the psuedo path for the handler object.
	tempPath = rootPath;
	tempPath += kDirChar;
	tempPath += clipName;
	size_t pathLen = tempPath.size() + 1;	// Include a terminating nul.
	parent->tempPtr = malloc ( pathLen );
	if ( parent->tempPtr == 0 ) XMP_Throw ( "No memory for XDCAMEX clip info", kXMPErr_NoMemory );
	memcpy ( parent->tempPtr, tempPath.c_str(), pathLen );

	return true;

}	// XDCAMEX_CheckFormat

// =================================================================================================

static void* CreatePseudoClipPath ( const std::string & clientPath ) 
{

	// Used to create the clip pseudo path when the CheckFormat function is skipped.
	
	std::string pseudoPath = clientPath;

	size_t pathLen;
	void* tempPtr = 0;
	
	if ( Host_IO::Exists ( pseudoPath.c_str() ) ) {
	
		// The client passed a physical path. The logical clip name is the last folder name, the
		// parent of the file. This is best since some files have suffixes.
		
		std::string clipName, ignored;
		
		XIO::SplitLeafName ( &pseudoPath, &ignored );	// Split the file name.
		XIO::SplitLeafName ( &pseudoPath, &clipName );	// Use the parent folder name.

		XIO::SplitLeafName ( &pseudoPath, &ignored );	// Remove the 2 intermediate folder levels.
		XIO::SplitLeafName ( &pseudoPath, &ignored );
		
		pseudoPath += kDirChar;
		pseudoPath += clipName;
	
	}

	pathLen = pseudoPath.size() + 1;	// Include a terminating nul.
	tempPtr = malloc ( pathLen );
	if ( tempPtr == 0 ) XMP_Throw ( "No memory for XDCAMEX clip info", kXMPErr_NoMemory );
	memcpy ( tempPtr, pseudoPath.c_str(), pathLen );
	
	return tempPtr;

}	// CreatePseudoClipPath

// =================================================================================================
// XDCAMEX_MetaHandlerCTor
// =======================

XMPFileHandler * XDCAMEX_MetaHandlerCTor ( XMPFiles * parent )
{
	return new XDCAMEX_MetaHandler ( parent );

}	// XDCAMEX_MetaHandlerCTor

// =================================================================================================
// XDCAMEX_MetaHandler::XDCAMEX_MetaHandler
// ========================================

XDCAMEX_MetaHandler::XDCAMEX_MetaHandler ( XMPFiles * _parent ) : expat(0),clipMetadata(0)
{
	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kXDCAMEX_HandlerFlags;
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

}	// XDCAMEX_MetaHandler::XDCAMEX_MetaHandler

// =================================================================================================
// XDCAMEX_MetaHandler::~XDCAMEX_MetaHandler
// =========================================

XDCAMEX_MetaHandler::~XDCAMEX_MetaHandler()
{

	this->CleanupLegacyXML();
	if ( this->parent->tempPtr != 0 ) {
		free ( this->parent->tempPtr );
		this->parent->tempPtr = 0;
	}

}	// XDCAMEX_MetaHandler::~XDCAMEX_MetaHandler

// =================================================================================================
// XDCAMEX_MetaHandler::MakeClipFilePath
// =====================================

bool XDCAMEX_MetaHandler::MakeClipFilePath ( std::string * path, XMP_StringPtr suffix, bool checkFile /* = false */ )
{

	*path = this->rootPath;
	*path += kDirChar;
	*path += "BPAV";
	*path += kDirChar;
	*path += "CLPR";
	*path += kDirChar;
	*path += this->clipName;
	*path += kDirChar;
	*path += this->clipName;
	*path += suffix;
	
	if ( ! checkFile ) return true;
	return Host_IO::Exists ( path->c_str() );

}	// XDCAMEX_MetaHandler::MakeClipFilePath

// =================================================================================================
// XDCAMEX_MetaHandler::MakeMediaproPath
// =====================================

bool XDCAMEX_MetaHandler::MakeMediaproPath ( std::string * path, bool checkFile /* = false */ )
{

	*path = this->rootPath;
	*path += kDirChar;
	*path += "BPAV";
	*path += kDirChar;
	*path += "MEDIAPRO.XML";
	
	if ( ! checkFile ) return true;
	return Host_IO::Exists ( path->c_str() );

}	// XDCAMEX_MetaHandler::MakeMediaproPath

// =================================================================================================
// XDCAMEX_MetaHandler::MakeLegacyDigest
// =====================================

// *** Early hack version.

#define kHexDigits "0123456789ABCDEF"

void XDCAMEX_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
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

}	// XDCAMEX_MetaHandler::MakeLegacyDigest

// =================================================================================================
// XDCAMEX_MetaHandler::CleanupLegacyXML
// =====================================

void XDCAMEX_MetaHandler::CleanupLegacyXML()
{

	delete this->expat;
	this->expat = 0;

	clipMetadata = 0;	// ! Was a pointer into the expat tree.

}	// XDCAMEX_MetaHandler::CleanupLegacyXML

// =================================================================================================
// XDCAMEX_MetaHandler::GetFileModDate
// ===================================

static inline bool operator< ( const XMP_DateTime & left, const XMP_DateTime & right ) {
	int compare = SXMPUtils::CompareDateTime ( left, right );
	return (compare < 0);
}

bool XDCAMEX_MetaHandler::GetFileModDate ( XMP_DateTime * modDate )
{

	// The XDCAM EX locations of metadata:
	//	BPAV/
	//		MEDIAPRO.XML	// Has non-XMP metadata.
	//		CLPR/
	//			709_3001_01:
	//				709_3001_01M01.XML	// Has non-XMP metadata.
	//				709_3001_01M01.XMP

	bool ok, haveDate = false;
	std::string fullPath;
	XMP_DateTime oneDate, junkDate;
	if ( modDate == 0 ) modDate = &junkDate;

	ok = this->MakeMediaproPath ( &fullPath, true /* checkFile */ );
	if ( ok ) ok = Host_IO::GetModifyDate ( fullPath.c_str(), &oneDate );
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

}	// XDCAMEX_MetaHandler::GetFileModDate

// Adds all the associated resources for the specified clip only (not related spanned ones) 
static void FillClipAssociatedResources( std::vector<std::string> * resourceList, std::string &clipPath, std::string &clipName )
{
	std::string filePath;
	std::string spannedClipFolderPath = clipPath + clipName + kDirChar;
	
	std::string clipPathNoExt = spannedClipFolderPath + clipName;
	// Get the files present inside clip folder.
	std::vector<std::string> regExpStringVec;
	std::string regExpString;
	regExpString = "^" + clipName + ".MP4$";
	regExpStringVec.push_back(regExpString);
	regExpString = "^" + clipName + "M\\d\\d.XMP$";
	regExpStringVec.push_back(regExpString);
	regExpString = "^" + clipName + "M\\d\\d.XML$";
	regExpStringVec.push_back(regExpString);
	regExpString = "^" + clipName + "I\\d\\d.PPN$";
	regExpStringVec.push_back(regExpString);
	regExpString = "^" + clipName + "R\\d\\d.BIM$";
	regExpStringVec.push_back(regExpString);
	regExpString = "^" + clipName + ".SMI$";
	regExpStringVec.push_back(regExpString);

	IOUtils::GetMatchingChildren (*resourceList, spannedClipFolderPath, regExpStringVec, false, true, true );

}


// =================================================================================================
// XDCAMEX_MetaHandler::FillAssociatedResources
// ======================================
void XDCAMEX_MetaHandler::FillAssociatedResources ( std::vector<std::string> * resourceList )
{
	// The possible associated resources:
	//	BPAV/
	//		MEDIAPRO.XML	
	//		CUEUP.XML
	//		CLPR/
	//			MIXXXX_YY:			MI is MachineID, XXXX is TakeSerial, 
	//								YY is ClipSuffix(as single take can be divided across multiple clips.)
	//								In case of spanning, all the clip folders starting from "MIXXXX_" are looked for.
	//				MIXXXX_YY.MP4
	//				MIXXXX_YYMNN.XML	NN is a counter which will start from from 01 and can go upto 99 based 
	//									on number of files present in this folder with same extension.
	//				MIXXXX_YYMNN.XMP
	//				MIXXXX_YYINN.PPN
	//				MIXXXX_YYRNN.BIM
	//				MXXXX_YY.SMI
	//		TAKR/
	//			MIXXXX:
	//				MIXXXXMNN.XML		NN is a counter which will start from from 01 and can go upto 99 based 
	//									on number of files present in this folder with same extension.
	//				MIXXXX.SMI
	//				MIXXXXUNN.SMI		NN is a counter which goes from 01 to N-1 where N is number of media, this
	//									take is divided into. For Nth, MIXXXX.SMI shall be picked up.
	XMP_VarString bpavPath = this->rootPath + kDirChar + "BPAV" + kDirChar;
	XMP_VarString filePath;
	//Add RootPath
	filePath = this->rootPath + kDirChar;
	PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );

	// Get the files present directly inside BPAV folder.
	filePath = bpavPath + "MEDIAPRO.XML";
	PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	filePath = bpavPath + "MEDIAPRO.BUP";
	PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	filePath = bpavPath + "CUEUP.XML";
	PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
	filePath = bpavPath + "CUEUP.BUP";
	PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );

	XMP_VarString clipPath = bpavPath + "CLPR" + kDirChar;
	size_t clipSuffixIndex = this->clipName.find_last_of('_');
	XMP_VarString takeName = this->clipName.substr(0, clipSuffixIndex);

	// Add spanned clip files. 
	// Here, we iterate over all the folders present inside "/BPAV/CLPR/" and whose name starts from
	// "MIXXXX_". All valid files present inside such folders are added to the list.
	XMP_VarString regExpString;
	regExpString = "^" + takeName + "_\\d\\d$";
	XMP_StringVector list;

	IOUtils::GetMatchingChildren ( list, clipPath, regExpString, true, false, false );
	size_t spaningClipsCount = list.size();
	for ( size_t index = 0; index < spaningClipsCount; index++ ) {
		FillClipAssociatedResources ( resourceList, clipPath, list[index] );
	}
	list.clear();

	size_t sizeWithoutTakeFiles = resourceList->size();
	XMP_VarString takeFolderPath = bpavPath + "TAKR" + kDirChar + takeName + kDirChar;
	XMP_StringVector regExpStringVec;

	// Get the files present inside take folder.
	regExpString = "^" + takeName + "M\\d\\d.XML$";
	regExpStringVec.push_back ( regExpString );
	regExpString = "^" + takeName + "U\\d\\d.SMI$";
	regExpStringVec.push_back ( regExpString );
	regExpString = "^" + takeName + ".SMI$";
	regExpStringVec.push_back ( regExpString );
	IOUtils::GetMatchingChildren ( *resourceList, takeFolderPath, regExpStringVec, false, true, true );

	if ( sizeWithoutTakeFiles == resourceList->size() )
	{
		// no Take files added to resource list. But "TAKR" folder is necessary to recognize this format
		// so let's add it to the list.
		filePath = bpavPath + "TAKR" + kDirChar;
		PackageFormat_Support::AddResourceIfExists(resourceList, filePath);
	}
}	// XDCAMEX_MetaHandler::FillAssociatedResources

// =================================================================================================
// XDCAMEX_MetaHandler::FillMetadataFiles
// ======================================
void XDCAMEX_MetaHandler::FillMetadataFiles ( std::vector<std::string> * metadataFiles )
{
	std::string noExtPath, filePath;

	noExtPath = rootPath + kDirChar + "BPAV" + kDirChar + "CLPR" +
						   kDirChar + clipName + kDirChar + clipName;

	filePath = noExtPath + "M01.XMP";
	metadataFiles->push_back ( filePath );
	filePath = noExtPath + "M01.XML";
	metadataFiles->push_back ( filePath );
	filePath = rootPath + kDirChar + "BPAV" + kDirChar + "MEDIAPRO.XML";
	metadataFiles->push_back ( filePath );

}	// 	FillMetadataFiles_XDCAM_EX

// =================================================================================================
// XDCAMEX_MetaHandler::IsMetadataWritable
// =======================================

bool XDCAMEX_MetaHandler::IsMetadataWritable ( )
{
	std::vector<std::string> metadataFiles;
	FillMetadataFiles(&metadataFiles);
	std::vector<std::string>::iterator itr = metadataFiles.begin();
	// Check whether sidecar is writable, if not then check if it can be created.
	XMP_Bool xmpWritable = Host_IO::Writable( itr->c_str(), true );
	// Check for legacy metadata file.
	XMP_Bool xmlWritable = Host_IO::Writable( (++itr)->c_str(), false );
	return ( xmlWritable && xmpWritable );
}// XDCAMEX_MetaHandler::IsMetadataWritable

// =================================================================================================
// XDCAMEX_MetaHandler::CacheFileData
// ==================================

void XDCAMEX_MetaHandler::CacheFileData()
{
	XMP_Assert ( ! this->containsXMP );

	if ( this->parent->UsesClientIO() ) {
		XMP_Throw ( "XDCAMEX cannot be used with client-managed I/O", kXMPErr_InternalFailure );
	}

	// See if the clip's .XMP file exists.

	std::string xmpPath;
	this->MakeClipFilePath ( &xmpPath, "M01.XMP" );
	if ( ! Host_IO::Exists ( xmpPath.c_str() ) ) return;	// No XMP.

	// Read the entire .XMP file. We know the XMP exists, New_XMPFiles_IO is supposed to return 0
	// only if the file does not exist.

	bool readOnly = XMP_OptionIsClear ( this->parent->openFlags, kXMPFiles_OpenForUpdate );

	XMP_Assert ( this->parent->ioRef == 0 );
	XMPFiles_IO* xmpFile =  XMPFiles_IO::New_XMPFiles_IO ( xmpPath.c_str(), readOnly );
	if ( xmpFile == 0 ) XMP_Throw ( "XDCAMEX XMP file open failure", kXMPErr_InternalFailure );
	this->parent->ioRef = xmpFile;

	XMP_Int64 xmpLen = xmpFile->Length();
	if ( xmpLen > 100*1024*1024 ) {
		XMP_Throw ( "XDCAMEX XMP is outrageously large", kXMPErr_InternalFailure );	// Sanity check.
	}

	this->xmpPacket.erase();
	this->xmpPacket.append ( (size_t)xmpLen, ' ' );

	XMP_Int32 ioCount = xmpFile->ReadAll ( (void*)this->xmpPacket.data(), (XMP_Int32)xmpLen );

	this->packetInfo.offset = 0;
	this->packetInfo.length = (XMP_Int32)xmpLen;
	FillPacketInfo ( this->xmpPacket, &this->packetInfo );

	this->containsXMP = true;

}	// XDCAMEX_MetaHandler::CacheFileData

// =================================================================================================
// XDCAMEX_MetaHandler::GetTakeDuration
// ====================================

void XDCAMEX_MetaHandler::GetTakeDuration ( const std::string & takeURI, std::string & duration )
{

	// Some versions of gcc can't tolerate goto's across declarations.
	// *** Better yet, avoid this cruft with self-cleaning objects.
	#define CleanupAndExit	\
		{									\
			delete expatMediaPro;	        \
			takeXMLFile.Close();			\
			return;							\
		}

	duration.clear();

	// Build a directory string to the take .xml file.

	std::string takeDir ( takeURI );
	takeDir.erase ( 0, 1 );	// Change the leading "//" to "/", then all '/' to kDirChar.
	if ( kDirChar != '/' ) {
		for ( size_t i = 0, limit = takeDir.size(); i < limit; ++i ) {
			if ( takeDir[i] == '/' ) takeDir[i] = kDirChar;
		}
	}

	std::string takePath ( this->rootPath );
	takePath += kDirChar;
	takePath += "BPAV";
	takePath += takeDir;

	// Replace .SMI with M01.XML.
	if ( takePath.size() > 4 ) {
		takePath.erase ( takePath.size() - 4, 4 );
		takePath += "M01.XML";
	}

	// Parse MEDIAPRO.XML

	XML_NodePtr takeRootElem = 0;
	XML_NodePtr context = 0;

	Host_IO::FileRef hostRef = Host_IO::Open ( takePath.c_str(), Host_IO::openReadOnly );
	if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
	XMPFiles_IO takeXMLFile ( hostRef, takePath.c_str(), Host_IO::openReadOnly );

	ExpatAdapter * expatMediaPro = XMP_NewExpatAdapter ( ExpatAdapter::kUseLocalNamespaces );
	if ( expatMediaPro == 0 ) return;

	XMP_Uns8 buffer [64*1024];
	while ( true ) {
		XMP_Int32 ioCount = takeXMLFile.Read ( buffer, sizeof(buffer) );
		if ( ioCount == 0 ) break;
		expatMediaPro->ParseBuffer ( buffer, ioCount, false /* not the end */ );
	}

	expatMediaPro->ParseBuffer ( 0, 0, true );	// End the parse.
	takeXMLFile.Close();

	// Get the root node of the XML tree.

	XML_Node & mediaproXMLTree = expatMediaPro->tree;
	for ( size_t i = 0, limit = mediaproXMLTree.content.size(); i < limit; ++i ) {
		if ( mediaproXMLTree.content[i]->kind == kElemNode ) {
			takeRootElem = mediaproXMLTree.content[i];
		}
	}
	if ( takeRootElem == 0 ) CleanupAndExit

	XMP_StringPtr rlName = takeRootElem->name.c_str() + takeRootElem->nsPrefixLen;
	if ( ! XMP_LitMatch ( rlName, "NonRealTimeMeta" ) ) CleanupAndExit

	// MediaProfile, Contents
	XMP_StringPtr ns = takeRootElem->ns.c_str();
	context = takeRootElem->GetNamedElement ( ns, "Duration" );
	if ( context != 0 ) {
		XMP_StringPtr durationValue = context->GetAttrValue ( "value" );
		if ( durationValue != 0 ) duration = durationValue;
	}

	CleanupAndExit
	#undef CleanupAndExit

}	// XDCAMEX_MetaHandler::GetTakeDuration

// =================================================================================================
// XDCAMEX_MetaHandler::GetMediaProMetadata
// ========================================

bool XDCAMEX_MetaHandler::GetMediaProMetadata ( SXMPMeta * xmpObjPtr,
											  	const std::string& clipUMID,
											  	bool digestFound )
{
	// Build a directory string to the MEDIAPRO file.

	std::string mediaproPath;
	this->MakeMediaproPath ( &mediaproPath );
	return XDCAM_Support::GetMediaProLegacyMetadata ( xmpObjPtr, clipUMID, mediaproPath, digestFound );

}	// XDCAMEX_MetaHandler::GetMediaProMetadata

// =================================================================================================
// XDCAMEX_MetaHandler::GetTakeUMID
// ================================

void XDCAMEX_MetaHandler::GetTakeUMID ( const std::string& clipUMID,
										std::string&	   takeUMID,
										std::string&	   takeXMLURI )
{

	// Some versions of gcc can't tolerate goto's across declarations.
	// *** Better yet, avoid this cruft with self-cleaning objects.
	#define CleanupAndExit	\
		{									\
			delete expatMediaPro;	        \
			mediaproXMLFile.Close();		\
			return;							\
		}

	takeUMID.clear();
	takeXMLURI.clear();

	// Build a directory string to the MEDIAPRO file.

	std::string mediapropath ( this->rootPath );
	mediapropath += kDirChar;
	mediapropath += "BPAV";
	mediapropath += kDirChar;
	mediapropath += "MEDIAPRO.XML";

	// Parse MEDIAPRO.XML.

	XML_NodePtr mediaproRootElem = 0;
	XML_NodePtr contentContext = 0, materialContext = 0;

	Host_IO::FileRef hostRef = Host_IO::Open ( mediapropath.c_str(), Host_IO::openReadOnly );
	if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
	XMPFiles_IO mediaproXMLFile ( hostRef, mediapropath.c_str(), Host_IO::openReadOnly );

	ExpatAdapter * expatMediaPro = XMP_NewExpatAdapter ( ExpatAdapter::kUseLocalNamespaces );
	if ( expatMediaPro == 0 ) return;

	XMP_Uns8 buffer [64*1024];
	while ( true ) {
		XMP_Int32 ioCount = mediaproXMLFile.Read ( buffer, sizeof(buffer) );
		if ( ioCount == 0 ) break;
		expatMediaPro->ParseBuffer ( buffer, ioCount, false /* not the end */ );
	}

	expatMediaPro->ParseBuffer ( 0, 0, true );	// End the parse.
	mediaproXMLFile.Close();

	// Get the root node of the XML tree.

	XML_Node & mediaproXMLTree = expatMediaPro->tree;
	for ( size_t i = 0, limit = mediaproXMLTree.content.size(); i < limit; ++i ) {
		if ( mediaproXMLTree.content[i]->kind == kElemNode ) {
			mediaproRootElem = mediaproXMLTree.content[i];
		}
	}

	if ( mediaproRootElem == 0 ) CleanupAndExit
	XMP_StringPtr rlName = mediaproRootElem->name.c_str() + mediaproRootElem->nsPrefixLen;
	if ( ! XMP_LitMatch ( rlName, "MediaProfile" ) ) CleanupAndExit

	//  MediaProfile, Contents

	XMP_StringPtr ns = mediaproRootElem->ns.c_str();
	contentContext = mediaproRootElem->GetNamedElement ( ns, "Contents" );

	if ( contentContext != 0 ) {

		size_t numMaterialElems = contentContext->CountNamedElements ( ns, "Material" );

		for ( size_t i = 0; i < numMaterialElems; ++i ) {	// Iterate over Material tags.

			XML_NodePtr materialElement = contentContext->GetNamedElement ( ns, "Material", i );
			XMP_Assert ( materialElement != 0 );

			XMP_StringPtr umid = materialElement->GetAttrValue ( "umid" );
			XMP_StringPtr uri = materialElement->GetAttrValue ( "uri" );

			if ( umid == 0 ) umid = "";
			if ( uri == 0 ) uri = "";

			size_t numComponents = materialElement->CountNamedElements ( ns, "Component" );

			for ( size_t j = 0; j < numComponents; ++j ) {

				XML_NodePtr componentElement = materialElement->GetNamedElement ( ns, "Component", j );
				XMP_Assert ( componentElement != 0 );

				XMP_StringPtr compUMID = componentElement->GetAttrValue ( "umid" );

				if ( (compUMID != 0) && (compUMID == clipUMID) ) {
					takeUMID = umid;
					takeXMLURI = uri;
					break;
				}

			}

			if ( ! takeUMID.empty() ) break;

		}

	}

	CleanupAndExit
	#undef CleanupAndExit

}

// =================================================================================================
// XDCAMEX_MetaHandler::ProcessXMP
// ===============================

void XDCAMEX_MetaHandler::ProcessXMP()
{

	// Some versions of gcc can't tolerate goto's across declarations.
	// *** Better yet, avoid this cruft with self-cleaning objects.
	#define CleanupAndExit	\
		{																								\
			bool openForUpdate = XMP_OptionIsSet ( this->parent->openFlags, kXMPFiles_OpenForUpdate );	\
			if ( ! openForUpdate ) this->CleanupLegacyXML();											\
			xmlFile.Close();																			\
			return;																						\
		}

	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.

	if ( this->containsXMP ) {
		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	}

	// NonRealTimeMeta -> XMP by schema.
	std::string thisUMID, takeUMID, takeXMLURI, takeDuration;
	std::string xmlPath;
	this->MakeClipFilePath ( &xmlPath, "M01.XML" );

	Host_IO::FileRef hostRef = Host_IO::Open ( xmlPath.c_str(), Host_IO::openReadOnly );
	if ( hostRef == Host_IO::noFileRef ) return;	// The open failed.
	XMPFiles_IO xmlFile ( hostRef, xmlPath.c_str(), Host_IO::openReadOnly );

	this->expat = XMP_NewExpatAdapter ( ExpatAdapter::kUseLocalNamespaces );
	if ( this->expat == 0 ) XMP_Throw ( "XDCAMEX_MetaHandler: Can't create Expat adapter", kXMPErr_NoMemory );

	XMP_Uns8 buffer [64*1024];
	while ( true ) {
		XMP_Int32 ioCount = xmlFile.Read ( buffer, sizeof(buffer) );
		if ( ioCount == 0 ) break;
		this->expat->ParseBuffer ( buffer, ioCount, false /* not the end */ );
	}
	this->expat->ParseBuffer ( 0, 0, true );	// End the parse.

	xmlFile.Close();

	// The root element should be NonRealTimeMeta in some namespace. Take whatever this file uses.

	XML_Node & xmlTree = this->expat->tree;
	XML_NodePtr rootElem = 0;

	for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i ) {
		if ( xmlTree.content[i]->kind == kElemNode ) rootElem = xmlTree.content[i];
	}

	if ( rootElem == 0 ) CleanupAndExit
	XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;
	if ( ! XMP_LitMatch ( rootLocalName, "NonRealTimeMeta" ) ) CleanupAndExit

	this->legacyNS = rootElem->ns;

	// Check the legacy digest.

	XMP_StringPtr legacyNS = this->legacyNS.c_str();
	this->clipMetadata = rootElem;	// ! Save the NonRealTimeMeta pointer for other use.

	std::string oldDigest, newDigest;
	bool digestFound = this->xmpObj.GetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "XDCAMEX", &oldDigest, 0 );
	if ( digestFound ) {
		this->MakeLegacyDigest ( &newDigest );
		if ( oldDigest == newDigest ) CleanupAndExit
	}

	// If we get here we need find and import the actual legacy elements using the current namespace.
	// Either there is no old digest in the XMP, or the digests differ. In the former case keep any
	// existing XMP, in the latter case take new legacy values.
	this->containsXMP = XDCAM_Support::GetLegacyMetadata ( &this->xmpObj, rootElem, legacyNS, digestFound, thisUMID );

	// If this clip is part of a take, add the take number to the relation field, and get the
	// duration from the take metadata.
	GetTakeUMID ( thisUMID, takeUMID, takeXMLURI );

	// If this clip is part of a take, update the duration to reflect the take duration rather than
	// the clip duration, and add the take name as a shot name.

	if ( ! takeXMLURI.empty() ) {

		// Update duration. This property already exists from clip legacy metadata.
		GetTakeDuration ( takeXMLURI, takeDuration );
		if ( ! takeDuration.empty() ) {
			this->xmpObj.SetStructField ( kXMP_NS_DM, "duration", kXMP_NS_DM, "value", takeDuration );
			containsXMP = true;
		}

		if ( digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "shotName" )) ) {

			std::string takeName;
			XIO::SplitLeafName ( &takeXMLURI, &takeName );

			// Check for the xml suffix, and delete if it exists.
			size_t pos = takeName.rfind(".SMI");
			if ( pos != std::string::npos ) {

				takeName.erase ( pos );

				// delete the take number suffix if it exists.
				if ( takeName.size() > 3 ) {

					size_t suffix = takeName.size() - 3;
					char c1 = takeName[suffix];
					char c2 = takeName[suffix+1];
					char c3 = takeName[suffix+2];
					if ( ('U' == c1) && ('0' <= c2) && (c2 <= '9') && ('0' <= c3) && (c3 <= '9') ) {
						takeName.erase ( suffix );
					}

					this->xmpObj.SetProperty ( kXMP_NS_DM, "shotName", takeName, kXMP_DeleteExisting );
					containsXMP = true;

				}

			}

		}

	}

	if ( (! takeUMID.empty()) &&
		 (digestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DC, "relation" ))) ) {
		this->xmpObj.DeleteProperty ( kXMP_NS_DC, "relation" );
		this->xmpObj.AppendArrayItem ( kXMP_NS_DC, "relation", kXMP_PropArrayIsUnordered, takeUMID );
		this->containsXMP = true;
	}

	this->containsXMP |= GetMediaProMetadata ( &this->xmpObj, thisUMID, digestFound );

	CleanupAndExit
	#undef CleanupAndExit

}	// XDCAMEX_MetaHandler::ProcessXMP


// =================================================================================================
// XDCAMEX_MetaHandler::UpdateFile
// ===============================
//
// Note that UpdateFile is only called from XMPFiles::CloseFile, so it is OK to close the file here.

void XDCAMEX_MetaHandler::UpdateFile ( bool doSafeUpdate )
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
	this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "XDCAMEX", newDigest.c_str(), kXMP_DeleteExisting );
	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, this->GetSerializeOptions() );

	// -----------------------------------------------------------------------
	// Update the XMP file first, don't let legacy XML failures block the XMP.

	std::string xmpPath;
	this->MakeClipFilePath ( &xmpPath, "M01.XMP" );

	bool haveXMP = Host_IO::Exists ( xmpPath.c_str() );
	if ( ! haveXMP ) {
		XMP_Assert ( this->parent->ioRef == 0 );
		Host_IO::Create ( xmpPath.c_str() );
		this->parent->ioRef = XMPFiles_IO::New_XMPFiles_IO ( xmpPath.c_str(), Host_IO::openReadWrite );
		if ( this->parent->ioRef == 0 ) XMP_Throw ( "Failure opening XDCAMEX XMP file", kXMPErr_ExternalFailure );
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
		if ( hostRef == Host_IO::noFileRef ) XMP_Throw ( "Failure opening XDCAMEX legacy XML file", kXMPErr_ExternalFailure );
		XMPFiles_IO origXML ( hostRef, xmlPath.c_str(), Host_IO::openReadWrite );
		XIO::ReplaceTextFile ( &origXML, legacyXML, (haveXML & doSafeUpdate) );
		origXML.Close();

	}

}	// XDCAMEX_MetaHandler::UpdateFile

// =================================================================================================
// XDCAMEX_MetaHandler::WriteTempFile
// ==================================

void XDCAMEX_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{

	// ! WriteTempFile is not supposed to be called for handlers that own the file.
	XMP_Throw ( "XDCAMEX_MetaHandler::WriteTempFile should not be called", kXMPErr_InternalFailure );

}	// XDCAMEX_MetaHandler::WriteTempFile

// =================================================================================================
