// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! Must be the first #include!
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include <vector>
#include <string.h>

#include "source/UnicodeConversions.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/HandlerRegistry.h"

#if EnablePluginManager
	#include "XMPFiles/source/PluginHandler/PluginManager.h"
#endif

#include "XMPFiles/source/FormatSupport/ID3_Support.hpp"

#if EnablePacketScanning
	#include "XMPFiles/source/FileHandlers/Scanner_Handler.hpp"
#endif

// =================================================================================================
/// \file XMPFiles.cpp
/// \brief High level support to access metadata in files of interest to Adobe applications.
///
/// This header ...
///
// =================================================================================================

using namespace Common;

#if EnablePluginManager
	using namespace XMP_PLUGIN;
#endif

// =================================================================================================

XMP_Int32 sXMPFilesInitCount = 0;

static XMP_ProgressTracker::CallbackInfo sProgressDefault;
static XMPFiles::ErrorCallbackInfo sDefaultErrorCallback;


#if GatherPerformanceData
	APIPerfCollection* sAPIPerf = 0;
#endif

// These are embedded version strings.

#if XMP_DebugBuild
	#define kXMPFiles_DebugFlag 1
#else
	#define kXMPFiles_DebugFlag 0
#endif

#define kXMPFiles_VersionNumber	( (kXMPFiles_DebugFlag << 31)    |	\
                                  (XMP_API_VERSION_MAJOR << 24) |	\
						          (XMP_API_VERSION_MINOR << 16) |	\
						          (XMP_API_VERSION_MICRO << 8) )

	#define kXMPFilesName "XMP Files"
	#define kXMPFiles_VersionMessage kXMPFilesName " " XMPFILES_API_VERSION_STRING
const char * kXMPFiles_EmbeddedVersion   = kXMPFiles_VersionMessage;
const char * kXMPFiles_EmbeddedCopyright = kXMPFilesName " " kXMP_CopyrightStr;

#define EMPTY_FILE_PATH ""
#define XMP_FILES_STATIC_START try { int a;
#define XMP_FILES_STATIC_END1(severity) a = 1; } catch ( XMP_Error & error ) { sDefaultErrorCallback.NotifyClient ( (severity), error, EMPTY_FILE_PATH ); }
#define XMP_FILES_STATIC_END2(filePath, severity) a = 1; } catch ( XMP_Error & error ) { sDefaultErrorCallback.NotifyClient ( (severity), error, (filePath) ); }
#define XMP_FILES_START try { int b;
#define XMP_FILES_END1(severity) b = 1; } catch ( XMP_Error & error ) { errorCallback.NotifyClient ( (severity), error, this->filePath.c_str() ); }
#define XMP_FILES_END2(filePath, severity) b = 1; } catch ( XMP_Error & error ) { errorCallback.NotifyClient ( (severity), error, (filePath) ); }
#define XMP_FILES_STATIC_NOTIFY_ERROR(errorCallbackPtr, filePath, severity, error)							\
	if ( (errorCallbackPtr) != NULL ) (errorCallbackPtr)->NotifyClient ( (severity), (error), (filePath) );


// =================================================================================================

#if EnablePacketScanning
	static XMPFileHandlerInfo kScannerHandlerInfo ( kXMP_UnknownFile, kScanner_HandlerFlags,
													(CheckFileFormatProc)0, Scanner_MetaHandlerCTor );
#endif

// =================================================================================================

/* class-static */
void
XMPFiles::GetVersionInfo ( XMP_VersionInfo * info )
{
	XMP_FILES_STATIC_START
	memset ( info, 0, sizeof(XMP_VersionInfo) );

	info->major   = XMPFILES_API_VERSION_MAJOR;
	info->minor   = XMPFILES_API_VERSION_MINOR;
	info->micro   = 0; //no longer used
	info->isDebug = kXMPFiles_DebugFlag;
	info->flags   = 0;	// ! None defined yet.
	info->message = kXMPFiles_VersionMessage;
	XMP_FILES_STATIC_END1 ( kXMPErrSev_OperationFatal )
}	// XMPFiles::GetVersionInfo

// =================================================================================================

#if XMP_TraceFilesCalls
	FILE * xmpFilesLog = stderr;
#endif

#if UseGlobalLibraryLock & (! XMP_StaticBuild )
	XMP_BasicMutex sLibraryLock;	// ! Handled in XMPMeta for static builds.
#endif

/* class static */
bool
XMPFiles::Initialize( XMP_OptionBits options, const char* pluginFolder, const char* plugins /* = NULL */ )
{
	XMP_FILES_STATIC_START
	++sXMPFilesInitCount;
	if ( sXMPFilesInitCount > 1 ) return true;

	#if XMP_TraceFilesCallsToFile
		xmpFilesLog = fopen ( "XMPFilesLog.txt", "w" );
		if ( xmpFilesLog == 0 ) xmpFilesLog = stderr;
	#endif

	#if UseGlobalLibraryLock & (! XMP_StaticBuild )
		InitializeBasicMutex ( sLibraryLock );	// ! Handled in XMPMeta for static builds.
	#endif

	SXMPMeta::Initialize();	// Just in case the client does not.

	if ( ! Initialize_LibUtils() ) return false;
	if ( ! ID3_Support::InitializeGlobals() ) return false;

	#if GatherPerformanceData
		sAPIPerf = new APIPerfCollection;
	#endif

	XMP_Uns16 endianInt  = 0x00FF;
	XMP_Uns8  endianByte = *((XMP_Uns8*)&endianInt);
	if ( kBigEndianHost ) {
		if ( endianByte != 0 ) XMP_Throw ( "Big endian flag mismatch", kXMPErr_InternalFailure );
	} else {
		if ( endianByte != 0xFF ) XMP_Throw ( "Little endian flag mismatch", kXMPErr_InternalFailure );
	}

	XMP_Assert ( kUTF8_PacketHeaderLen == strlen ( "<?xpacket begin='xxx' id='W5M0MpCehiHzreSzNTczkc9d'" ) );
	XMP_Assert ( kUTF8_PacketTrailerLen == strlen ( (const char *) kUTF8_PacketTrailer ) );

	HandlerRegistry::getInstance().initialize();

	InitializeUnicodeConversions();

	ignoreLocalText = XMP_OptionIsSet ( options, kXMPFiles_IgnoreLocalText );
	#if XMP_UNIXBuild
		if ( ! ignoreLocalText ) XMP_Throw ( "Generic UNIX clients must pass kXMPFiles_IgnoreLocalText", kXMPErr_EnforceFailure );
	#endif

	#if EnablePluginManager
		if ( pluginFolder != 0 ) {
			std::string pluginList;
			if ( plugins != 0 ) pluginList.assign ( plugins );
			PluginManager::initialize ( pluginFolder, pluginList );  // Load file handler plugins.
		}
	#endif

	// Make sure the embedded info strings are referenced and kept.
	if ( (kXMPFiles_EmbeddedVersion[0] == 0) || (kXMPFiles_EmbeddedCopyright[0] == 0) ) return false;
	// Verify critical type sizes.
	XMP_Assert ( sizeof(XMP_Int8) == 1 );
	XMP_Assert ( sizeof(XMP_Int16) == 2 );
	XMP_Assert ( sizeof(XMP_Int32) == 4 );
	XMP_Assert ( sizeof(XMP_Int64) == 8 );
	XMP_Assert ( sizeof(XMP_Uns8) == 1 );
	XMP_Assert ( sizeof(XMP_Uns16) == 2 );
	XMP_Assert ( sizeof(XMP_Uns32) == 4 );
	XMP_Assert ( sizeof(XMP_Uns64) == 8 );
	XMP_Assert ( sizeof(XMP_Bool) == 1 );
	XMP_FILES_STATIC_END1 ( kXMPErrSev_ProcessFatal )
	return true;

}	// XMPFiles::Initialize

// =================================================================================================

#if GatherPerformanceData

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

#include "PerfUtils.cpp"

static void ReportPerformanceData()
{
	struct SummaryInfo {
		size_t callCount;
		double totalTime;
		SummaryInfo() : callCount(0), totalTime(0.0) {};
	};

	SummaryInfo perfSummary [kAPIPerfProcCount];

	XMP_DateTime now;
	SXMPUtils::CurrentDateTime ( &now );
	std::string nowStr;
	SXMPUtils::ConvertFromDate ( now, &nowStr );

	#if XMP_WinBuild
		#define kPerfLogPath "C:\\XMPFilesPerformanceLog.txt"
	#else
		#define kPerfLogPath "/XMPFilesPerformanceLog.txt"
	#endif
	FILE * perfLog = fopen ( kPerfLogPath, "ab" );
	if ( perfLog == 0 ) return;

	fprintf ( perfLog, "\n\n// =================================================================================================\n\n" );
	fprintf ( perfLog, "XMPFiles performance data\n" );
	fprintf ( perfLog, "   %s\n", kXMPFiles_VersionMessage );
	fprintf ( perfLog, "   Reported at %s\n", nowStr.c_str() );
	fprintf ( perfLog, "   %s\n", PerfUtils::GetTimerInfo() );

	// Gather and report the summary info.

	for ( size_t i = 0; i < sAPIPerf->size(); ++i ) {
		SummaryInfo& summaryItem = perfSummary [(*sAPIPerf)[i].whichProc];
		++summaryItem.callCount;
		summaryItem.totalTime += (*sAPIPerf)[i].elapsedTime;
	}

	fprintf ( perfLog, "\nSummary data:\n" );

	for ( size_t i = 0; i < kAPIPerfProcCount; ++i ) {
		long averageTime = 0;
		if ( perfSummary[i].callCount != 0 ) {
			averageTime = (long) (((perfSummary[i].totalTime/perfSummary[i].callCount) * 1000.0*1000.0) + 0.5);
		}
		fprintf ( perfLog, "   %s, %d total calls, %d average microseconds per call\n",
				  kAPIPerfNames[i], perfSummary[i].callCount, averageTime );
	}

	fprintf ( perfLog, "\nPer-call data:\n" );

	// Report the info for each call.

	for ( size_t i = 0; i < sAPIPerf->size(); ++i ) {
		long time = (long) (((*sAPIPerf)[i].elapsedTime * 1000.0*1000.0) + 0.5);
		fprintf ( perfLog, "   %s, %d microSec, ref %.8X, \"%s\"\n",
				  kAPIPerfNames[(*sAPIPerf)[i].whichProc], time,
				  (*sAPIPerf)[i].xmpFilesRef, (*sAPIPerf)[i].extraInfo.c_str() );
	}

	fclose ( perfLog );

}	// ReportAReportPerformanceDataPIPerformance

#endif

// =================================================================================================

/* class static */
void
XMPFiles::Terminate()
{
	XMP_FILES_STATIC_START
	--sXMPFilesInitCount;
	if ( sXMPFilesInitCount != 0 ) return;	// Not ready to terminate, or already terminated.

	#if GatherPerformanceData
		ReportPerformanceData();
		EliminateGlobal ( sAPIPerf );
	#endif

	#if EnablePluginManager
		PluginManager::terminate();
	#endif

	HandlerRegistry::terminate();

	SXMPMeta::Terminate();	// Just in case the client does not.

	ID3_Support::TerminateGlobals();
	Terminate_LibUtils();

	#if UseGlobalLibraryLock & (! XMP_StaticBuild )
		TerminateBasicMutex ( sLibraryLock );	// ! Handled in XMPMeta for static builds.
	#endif

	#if XMP_TraceFilesCallsToFile
		if ( xmpFilesLog != stderr ) fclose ( xmpFilesLog );
		xmpFilesLog = stderr;
	#endif

	XMP_FILES_STATIC_END1 ( kXMPErrSev_ProcessFatal )
}	// XMPFiles::Terminate

// =================================================================================================

XMPFiles::XMPFiles()
	: clientRefs(0)
	, format(kXMP_UnknownFile)
	, ioRef(0)
	, openFlags(0)
	, handler(0)
	, tempPtr(0)
	, tempUI32(0)
	, abortProc(0)
	, abortArg(0)
	, progressTracker(0)
{
	XMP_FILES_START
	if ( sProgressDefault.clientProc != 0 ) {
		this->progressTracker = new XMP_ProgressTracker ( sProgressDefault );
		if (this->progressTracker == 0) XMP_Throw ( "XMPFiles: Unable to allocate memory for Progress Tracker", kXMPErr_NoMemory );
	}

	if ( sDefaultErrorCallback.clientProc != 0 ) {
		this->errorCallback.wrapperProc = sDefaultErrorCallback.wrapperProc;
		this->errorCallback.clientProc = sDefaultErrorCallback.clientProc;
		this->errorCallback.context = sDefaultErrorCallback.context;
		this->errorCallback.limit = sDefaultErrorCallback.limit;
	}
	XMP_FILES_END1 ( kXMPErrSev_OperationFatal );
}	// XMPFiles::XMPFiles

// =================================================================================================

static inline void CloseLocalFile ( XMPFiles* thiz )
{
	if ( thiz->UsesLocalIO() ) {

		XMPFiles_IO* localFile = (XMPFiles_IO*)thiz->ioRef;

		if ( localFile != 0 ) {
			localFile->Close();
			delete localFile;
			thiz->ioRef = 0;
		}

	}

}	// CloseLocalFile

// =================================================================================================

XMPFiles::~XMPFiles()
{
	XMP_FILES_START
	XMP_Assert ( this->clientRefs <= 0 );

	if ( this->handler != 0 ) {
		delete this->handler;
		this->handler = 0;
	}

	CloseLocalFile ( this );

	if ( this->progressTracker != 0 ) delete this->progressTracker;
	if ( this->tempPtr != 0 ) free ( this->tempPtr );	// ! Must have been malloc-ed!
	XMP_FILES_END1 ( kXMPErrSev_OperationFatal )

}	// XMPFiles::~XMPFiles

// =================================================================================================

/* class static */
bool
XMPFiles::GetFormatInfo ( XMP_FileFormat   format,
                          XMP_OptionBits * flags /* = 0 */ )
{
	XMP_FILES_STATIC_START
	return HandlerRegistry::getInstance().getFormatInfo ( format, flags );
	XMP_FILES_STATIC_END1 ( kXMPErrSev_OperationFatal )
	return false;

}	// XMPFiles::GetFormatInfo

// =================================================================================================

/* class static */
XMP_FileFormat
XMPFiles::CheckFileFormat ( XMP_StringPtr clientPath )
{
	XMP_FILES_STATIC_START
	if ( (clientPath == 0) || (*clientPath == 0) ) return kXMP_UnknownFile;

	XMPFiles bogus;	// Needed to provide context to SelectSmartHandler.
	bogus.SetFilePath ( clientPath ); // So that XMPFiles destructor cleans up the XMPFiles_IO object.
	XMPFileHandlerInfo * handlerInfo = HandlerRegistry::getInstance().selectSmartHandler(&bogus, clientPath, kXMP_UnknownFile, kXMPFiles_OpenForRead);

	if ( handlerInfo == 0 ) {
		if ( !Host_IO::Exists ( clientPath ) ) {
			XMP_Error error ( kXMPErr_NoFile, "XMPFiles: file does not exist" );
			XMP_FILES_STATIC_NOTIFY_ERROR ( &sDefaultErrorCallback, clientPath, kXMPErrSev_Recoverable, error );
		}
		return kXMP_UnknownFile;
	}
	return handlerInfo->format;
	XMP_FILES_STATIC_END2 ( clientPath, kXMPErrSev_OperationFatal )
	return kXMP_UnknownFile;

}	// XMPFiles::CheckFileFormat

// =================================================================================================

/* class static */
XMP_FileFormat
XMPFiles::CheckPackageFormat ( XMP_StringPtr folderPath )
{
	XMP_FILES_STATIC_START
	// This is called with a path to a folder, and checks to see if that folder is the top level of
	// a "package" that should be recognized by one of the folder-oriented handlers. The checks here
	// are not overly extensive, but hopefully enough to weed out false positives.
	//
	// Since we don't have many folder handlers, this is simple hardwired code.

	#if ! EnableDynamicMediaHandlers
		return kXMP_UnknownFile;
	#else
		Host_IO::FileMode folderMode = Host_IO::GetFileMode ( folderPath );
		if ( folderMode != Host_IO::kFMode_IsFolder ) return kXMP_UnknownFile;
		return HandlerRegistry::checkTopFolderName ( std::string ( folderPath ) );
	#endif
	XMP_FILES_STATIC_END2 ( folderPath, kXMPErrSev_OperationFatal )
	return kXMP_UnknownFile;

}	// XMPFiles::CheckPackageFormat

// =================================================================================================

static bool FileIsExcluded (
	XMP_StringPtr clientPath,
	std::string * fileExt,
	Host_IO::FileMode * clientMode,
	const XMPFiles::ErrorCallbackInfo * _errorCallbackInfoPtr = NULL )
{
	// ! Return true for excluded files, false for OK files.
	
	*clientMode = Host_IO::GetFileMode ( clientPath );

	if ( (*clientMode == Host_IO::kFMode_IsFolder) || (*clientMode == Host_IO::kFMode_IsOther) ) {
		XMP_Error error ( kXMPErr_FilePathNotAFile, "XMPFiles: path specified is not a file" );
		XMP_FILES_STATIC_NOTIFY_ERROR ( _errorCallbackInfoPtr, clientPath, kXMPErrSev_Recoverable, error );
		return true;
	}

	XMP_Assert ( (*clientMode == Host_IO::kFMode_IsFile) || (*clientMode == Host_IO::kFMode_DoesNotExist) );

	if ( *clientMode == Host_IO::kFMode_IsFile ) {

		// Find the file extension. OK to be "wrong" for something like "C:\My.dir\file". Any
		// filtering looks for matches with real extensions, "dir\file" won't match any of these.
		XMP_StringPtr extPos = clientPath + strlen ( clientPath );
		for ( ; (extPos != clientPath) && (*extPos != '.'); --extPos ) {}
		if ( *extPos == '.' ) {
			fileExt->assign ( extPos+1 );
			MakeLowerCase ( fileExt );
		}

		// See if this file is one that XMPFiles should never process.
		for ( size_t i = 0; kKnownRejectedFiles[i] != 0; ++i ) {
			if ( *fileExt == kKnownRejectedFiles[i] ) {
				XMP_Error error ( kXMPErr_RejectedFileExtension, "XMPFiles: rejected file extension specified" );
				XMP_FILES_STATIC_NOTIFY_ERROR ( _errorCallbackInfoPtr, clientPath, kXMPErrSev_Recoverable, error );
				return true;
			}
		}

	}
	
	return false;

}	// FileIsExcluded

static XMPFileHandlerInfo* CreateFileHandlerInfo (
	XMPFiles* dummyParent,
	XMP_FileFormat * format,
	XMP_OptionBits   options,
	const XMPFiles::ErrorCallbackInfo * _errorCallbackInfoPtr = NULL )
{
	Host_IO::FileMode clientMode;
	std::string fileExt;	// Used to check for excluded files.
	bool excluded = FileIsExcluded ( dummyParent->GetFilePath().c_str(), &fileExt, &clientMode, &sDefaultErrorCallback );	// ! Fills in fileExt and clientMode.
	if ( excluded ) return 0;

	XMPFileHandlerInfo * handlerInfo  = 0;

	XMP_FileFormat dummyFormat = kXMP_UnknownFile;
	if ( format == 0 ) format = &dummyFormat;

	options |= kXMPFiles_OpenForRead;
	handlerInfo = HandlerRegistry::getInstance().selectSmartHandler ( dummyParent, dummyParent->GetFilePath().c_str(), *format, options );

	if ( handlerInfo == 0 ) {
		if ( clientMode == Host_IO::kFMode_DoesNotExist ) {
			XMP_Error error ( kXMPErr_NoFile, "XMPFiles: file does not exist" );
			XMP_FILES_STATIC_NOTIFY_ERROR ( _errorCallbackInfoPtr, dummyParent->GetFilePath().c_str(), kXMPErrSev_Recoverable, error );
		}else{
			XMP_Error error ( kXMPErr_NoFileHandler, "XMPFiles: No smart file handler available to handle file" );
			XMP_FILES_STATIC_NOTIFY_ERROR ( _errorCallbackInfoPtr, dummyParent->GetFilePath().c_str(), kXMPErrSev_Recoverable, error );
		}
		return 0;
	}
	return handlerInfo;
}

// =================================================================================================

/* class static */
bool
XMPFiles::GetFileModDate ( XMP_StringPtr    clientPath,
						   XMP_DateTime *   modDate,
						   XMP_FileFormat * format, /* = 0 */
						   XMP_OptionBits   options /* = 0 */ )
{
	XMP_FILES_STATIC_START
	// ---------------------------------------------------------------
	// First try to select a smart handler. Return false if not found.
	
	XMPFiles dummyParent;	// GetFileModDate is static, but the handler needs a parent.
	dummyParent.SetFilePath ( clientPath );
	
	
	XMPFileHandlerInfo * handlerInfo  = 0;
	handlerInfo = CreateFileHandlerInfo ( &dummyParent, format, options, &sDefaultErrorCallback );
	if ( handlerInfo == 0 ) return false;
	
	// -------------------------------------------------------------------------
	// Fill in the format output. Call the handler to get the modification date.

	dummyParent.format = handlerInfo->format;
	if ( format ) *format = handlerInfo->format;

	dummyParent.handler = handlerInfo->handlerCTor ( &dummyParent );

	bool ok = false;

	std::vector <std::string> resourceList;
	try{
		XMP_DateTime lastModDate;
		XMP_DateTime junkDate;
		if ( modDate == 0 ) modDate = &junkDate;
		dummyParent.handler->FillAssociatedResources ( &resourceList );
		size_t countRes = resourceList.size();
		for ( size_t index = 0; index < countRes ; ++index ){
			XMP_StringPtr curFilePath = resourceList[index].c_str();
			if( Host_IO::GetFileMode ( curFilePath ) != Host_IO::kFMode_IsFile ) continue;// only interested in files
			Host_IO::GetModifyDate ( curFilePath, &lastModDate );
			if ( ! ok || ( SXMPUtils::CompareDateTime ( *modDate , lastModDate ) < 0 ) ) 
			{
				*modDate = lastModDate;
				ok = true;
			}
		}
	}
	catch(...){
		// Fallback to the old way 
		// eventually this method would go away as 
		// soon as the implementation for
		// GetAssociatedResources is added to all 
		// the file/Plugin Handlers
		ok = dummyParent.handler->GetFileModDate ( modDate );
	}
	delete dummyParent.handler;
	dummyParent.handler = 0;

	return ok;
	XMP_FILES_STATIC_END2 ( clientPath, kXMPErrSev_OperationFatal )
	return false;

}	// XMPFiles::GetFileModDate

// =================================================================================================

/* class static */
bool
XMPFiles::GetAssociatedResources ( 
		XMP_StringPtr              filePath,
        std::vector<std::string> * resourceList,
        XMP_FileFormat             format  /* = kXMP_UnknownFile */, 
        XMP_OptionBits             options /*  = 0 */ )
{
	XMP_FILES_STATIC_START
	XMP_Assert ( (resourceList != 0) && resourceList->empty() );	// Ensure that the glue passes in an empty local.

	// Try to select a handler.

	if ( (filePath == 0) || (*filePath == 0) ) return false;

	XMPFiles dummyParent;	// GetAssociatedResources is static, but the handler needs a parent.
	dummyParent.SetFilePath ( filePath );

	XMPFileHandlerInfo * handlerInfo  = 0;
	handlerInfo = CreateFileHandlerInfo ( &dummyParent, &format, options, &sDefaultErrorCallback );
	if ( handlerInfo == 0 ) return false;

	// -------------------------------------------------------------------------
	// Fill in the format output. Call the handler to get the Associated Resources.

	dummyParent.format = handlerInfo->format;

	dummyParent.handler = handlerInfo->handlerCTor ( &dummyParent );
	
	try {
		dummyParent.handler->FillAssociatedResources ( resourceList );
	} catch ( XMP_Error& error ) {
		if ( error.GetID() == kXMPErr_Unimplemented ) {
			XMP_FILES_STATIC_NOTIFY_ERROR ( &sDefaultErrorCallback, filePath, kXMPErrSev_Recoverable, error );
			return false;
		} else {
			throw;
		}
	}
	XMP_Assert ( ! resourceList->empty() );

	delete dummyParent.handler;
	dummyParent.handler = 0;
	
	return true;

	XMP_FILES_STATIC_END2 ( filePath, kXMPErrSev_OperationFatal )
	return false;

}	// XMPFiles::GetAssociatedResources

bool 
XMPFiles::IsMetadataWritable (
		XMP_StringPtr  filePath,
        XMP_Bool *     writable,
        XMP_FileFormat format  /* = kXMP_UnknownFile */,
        XMP_OptionBits options  /* = 0 */ )
{
	XMP_FILES_STATIC_START
	// Try to select a handler.
	
	if ( (filePath == 0) || (*filePath == 0) ) return false;
	
	XMPFiles dummyParent;	// IsMetadataWritable is static, but the handler needs a parent.
	dummyParent.SetFilePath ( filePath );

	XMPFileHandlerInfo * handlerInfo  = 0;
	handlerInfo = CreateFileHandlerInfo ( &dummyParent, &format, options, &sDefaultErrorCallback );
	if ( handlerInfo == 0 ) return false;

	if ( writable == 0 ) {
		XMP_Throw("Boolean parameter is required for IsMetadataWritable() API.", kXMPErr_BadParam);
	} else {
		*writable = kXMP_Bool_False;
	}


	dummyParent.format = handlerInfo->format;
	dummyParent.handler = handlerInfo->handlerCTor ( &dummyParent );
	
	// We don't require any of the files to be opened at this point.
	// Also, if We don't close them then this will be a problem for embedded handlers because we will be checking
	// write permission on the same file which could be open (in some mode) already.
	CloseLocalFile(&dummyParent);
	
	try {
		*writable = ConvertBoolToXMP_Bool( dummyParent.handler->IsMetadataWritable() );
	}
     catch ( XMP_Error& error ) {	
		if ( error.GetID() == kXMPErr_Unimplemented ) {
			XMP_FILES_STATIC_NOTIFY_ERROR ( &sDefaultErrorCallback, filePath, kXMPErrSev_Recoverable, error );
			return false;
		} else {
			throw;
		}
	}
	delete dummyParent.handler;
	dummyParent.handler = 0;
	XMP_FILES_STATIC_END2 ( filePath, kXMPErrSev_OperationFatal )
	return true;
} // XMPFiles::IsMetadataWritable 


// =================================================================================================

static bool
DoOpenFile ( XMPFiles *     thiz,
			 XMP_IO *       clientIO,
			 XMP_StringPtr  clientPath,
	         XMP_FileFormat format /* = kXMP_UnknownFile */,
	         XMP_OptionBits openFlags /* = 0 */ )
{
	XMP_Assert ( (clientIO == 0) ? (clientPath[0] != 0) : (clientPath[0] == 0) );
	
	openFlags &= ~kXMPFiles_ForceGivenHandler;	// Don't allow this flag for OpenFile.

	if ( thiz->handler != 0 ) XMP_Throw ( "File already open", kXMPErr_BadParam );
	CloseLocalFile ( thiz );	// Sanity checks if prior call failed.

	thiz->ioRef = clientIO;
	thiz->SetFilePath ( clientPath );

	thiz->format = kXMP_UnknownFile;	// Make sure it is preset for later check.
	thiz->openFlags = openFlags;

	bool readOnly = XMP_OptionIsClear ( openFlags, kXMPFiles_OpenForUpdate );

	Host_IO::FileMode clientMode;
	std::string fileExt;	// Used to check for camera raw files and OK to scan files.

	if ( thiz->UsesClientIO() ) {
		clientMode = Host_IO::kFMode_IsFile;
	} else {
		bool excluded = FileIsExcluded ( clientPath, &fileExt, &clientMode, &thiz->errorCallback );	// ! Fills in fileExt and clientMode.
		if ( excluded ) return false;
	}

	// Find the handler, fill in the XMPFiles member variables, cache the desired file data.

	XMPFileHandlerInfo * handlerInfo  = 0;
	XMPFileHandlerCTor   handlerCTor  = 0;
	XMP_OptionBits       handlerFlags = 0;

	if ( ! (openFlags & kXMPFiles_OpenUsePacketScanning) ) {
		handlerInfo = HandlerRegistry::getInstance().selectSmartHandler( thiz, clientPath, format, openFlags );
	}

	#if ! EnablePacketScanning

		if ( handlerInfo == 0 ) {
			if ( clientMode == Host_IO::kFMode_DoesNotExist ) {
				XMP_Error error ( kXMPErr_NoFile, "XMPFiles: file does not exist" );
				XMP_FILES_STATIC_NOTIFY_ERROR ( &thiz->errorCallback, clientPath, kXMPErrSev_Recoverable, error );
			}
			return false;
		}

	#else

		if ( handlerInfo == 0 ) {

			// No smart handler, packet scan if appropriate.
			if ( clientMode == Host_IO::kFMode_DoesNotExist ) {
				XMP_Error error ( kXMPErr_NoFile, "XMPFiles: file does not exist" );
				XMP_FILES_STATIC_NOTIFY_ERROR ( &thiz->errorCallback, clientPath, kXMPErrSev_Recoverable, error );
				return false;
			} else if ( clientMode != Host_IO::kFMode_IsFile ) {
				return false;
			}

			if ( openFlags & kXMPFiles_OpenUseSmartHandler ) {
				XMP_Error error ( kXMPErr_NoFileHandler, "XMPFiles: No smart file handler available to handle file" );
				XMP_FILES_STATIC_NOTIFY_ERROR ( &thiz->errorCallback, clientPath, kXMPErrSev_Recoverable, error );
				return false;
			}

			if ( openFlags & kXMPFiles_OpenLimitedScanning ) {
				bool scanningOK = false;
				for ( size_t i = 0; kKnownScannedFiles[i] != 0; ++i ) {
					if ( fileExt == kKnownScannedFiles[i] ) { scanningOK = true; break; }
				}
				if ( ! scanningOK ) return false;
			}

			handlerInfo = &kScannerHandlerInfo;
			if ( thiz->ioRef == 0 ) {	// Normally opened in SelectSmartHandler, but might not be open yet.
				thiz->ioRef = XMPFiles_IO::New_XMPFiles_IO ( clientPath, readOnly );
				if ( thiz->ioRef == 0 ) return false;
			}

		}

	#endif

	XMP_Assert ( handlerInfo != 0 );
	handlerCTor  = handlerInfo->handlerCTor;
	handlerFlags = handlerInfo->flags;

	XMP_Assert ( (thiz->ioRef != 0) ||
				 (handlerFlags & kXMPFiles_UsesSidecarXMP) ||
				 (handlerFlags & kXMPFiles_HandlerOwnsFile) ||
				 (handlerFlags & kXMPFiles_FolderBasedFormat) );

	if ( thiz->format == kXMP_UnknownFile ) thiz->format = handlerInfo->format;	// ! The CheckProc might have set it.
	XMPFileHandler* handler = (*handlerCTor) ( thiz );
	XMP_Assert ( handlerFlags == handler->handlerFlags );

	thiz->handler = handler;

	try {
		handler->CacheFileData();
	} catch ( ... ) {
		delete thiz->handler;
		thiz->handler = 0;
		if ( ! (handlerFlags & kXMPFiles_HandlerOwnsFile) ) CloseLocalFile ( thiz );
		throw;
	}

	if ( handler->containsXMP ) FillPacketInfo ( handler->xmpPacket, &handler->packetInfo );

	if ( (! (openFlags & kXMPFiles_OpenForUpdate)) && (! (handlerFlags & kXMPFiles_HandlerOwnsFile)) ) {
		// Close the disk file now if opened for read-only access.
		CloseLocalFile ( thiz );
	}

	return true;

}	// DoOpenFile

// =================================================================================================

static bool DoOpenFile( XMPFiles* thiz, 
						const Common::XMPFileHandlerInfo& hdlInfo, 
						XMP_IO* clientIO, 
						XMP_StringPtr clientPath, 
						XMP_OptionBits openFlags )
{
	XMP_Assert ( (clientIO == 0) ? (clientPath[0] != 0) : (clientPath[0] == 0) );
	
	openFlags &= ~kXMPFiles_ForceGivenHandler;	// Don't allow this flag for OpenFile.


	if ( thiz->handler != 0 ) XMP_Throw ( "File already open", kXMPErr_BadParam );

	//
	// setup members
	//
	thiz->ioRef		= clientIO;
	thiz->SetFilePath ( clientPath );
	thiz->format	= hdlInfo.format;
	thiz->openFlags = openFlags;

	//
	// create file handler instance
	//
	XMPFileHandlerCTor handlerCTor	= hdlInfo.handlerCTor;
	XMP_OptionBits handlerFlags		= hdlInfo.flags;

	XMPFileHandler* handler			= (*handlerCTor) ( thiz );
	XMP_Assert ( handlerFlags == handler->handlerFlags );

	thiz->handler = handler;
	bool readOnly = XMP_OptionIsClear ( openFlags, kXMPFiles_OpenForUpdate );

	if ( thiz->ioRef == 0 ) {	//Need to open the file if not done already
		thiz->ioRef = XMPFiles_IO::New_XMPFiles_IO ( clientPath, readOnly );
		if ( thiz->ioRef == 0 ) return false;
	}
	//
	// try to read metadata
	//
	try 
	{
		handler->CacheFileData();

		if( handler->containsXMP ) 
		{
			FillPacketInfo( handler->xmpPacket, &handler->packetInfo );
		}

		if( (! (openFlags & kXMPFiles_OpenForUpdate)) && (! (handlerFlags & kXMPFiles_HandlerOwnsFile)) ) 
		{
			// Close the disk file now if opened for read-only access.
			CloseLocalFile ( thiz );
		}
	} 
	catch( ... ) 
	{
		delete thiz->handler;
		thiz->handler = NULL;

		if( ! (handlerFlags & kXMPFiles_HandlerOwnsFile) ) 
		{
			CloseLocalFile( thiz );
		}

		throw;
	}

	return true;
}

// =================================================================================================

bool
XMPFiles::OpenFile ( XMP_StringPtr  clientPath,
	                 XMP_FileFormat format /* = kXMP_UnknownFile */,
	                 XMP_OptionBits openFlags /* = 0 */ )
{
	XMP_FILES_START
	return DoOpenFile ( this, 0, clientPath, format, openFlags );
	XMP_FILES_END2 ( clientPath, kXMPErrSev_FileFatal )
	return false;
}

// =================================================================================================

#if XMP_StaticBuild	// ! Client XMP_IO objects can only be used in static builds.

bool
XMPFiles::OpenFile ( XMP_IO*        clientIO,
	                 XMP_FileFormat format /* = kXMP_UnknownFile */,
	                 XMP_OptionBits openFlags /* = 0 */ )
{
	XMP_FILES_START
	this->progressTracker = 0;	// Progress tracking is not supported for client-managed I/O.
	return DoOpenFile ( this, clientIO, "", format, openFlags );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
	return false;

}	// XMPFiles::OpenFile

bool XMPFiles::OpenFile ( const Common::XMPFileHandlerInfo&	hdlInfo,
						 XMP_IO*							clientIO,
						 XMP_OptionBits						openFlags /*= 0*/ )
{
	XMP_FILES_START
	this->progressTracker = 0;	// Progress tracking is not supported for client-managed I/O.
	return DoOpenFile ( this, hdlInfo, clientIO, NULL, openFlags );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
	return false;

}

#endif	// XMP_StaticBuild

// =================================================================================================

bool XMPFiles::OpenFile ( const Common::XMPFileHandlerInfo&	hdlInfo,
						 XMP_StringPtr						filePath,
						 XMP_OptionBits						openFlags /*= 0*/ )
{
	XMP_FILES_START
	return DoOpenFile ( this, hdlInfo, NULL, filePath, openFlags );
	XMP_FILES_END2 (filePath, kXMPErrSev_FileFatal )
	return false;
}

// =================================================================================================

void
XMPFiles::CloseFile ( XMP_OptionBits closeFlags /* = 0 */ )
{
	XMP_FILES_START
	if ( this->handler == 0 ) return;	// Return if there is no open file (not an error).

	bool needsUpdate = this->handler->needsUpdate;
	XMP_OptionBits handlerFlags = this->handler->handlerFlags;

	// Decide if we're doing a safe update. If so, make sure the handler supports it. All handlers
	// that don't own the file tolerate safe update using common code below.

	bool doSafeUpdate = XMP_OptionIsSet ( closeFlags, kXMPFiles_UpdateSafely );
	#if GatherPerformanceData
		if ( doSafeUpdate ) sAPIPerf->back().extraInfo += ", safe update";	// Client wants safe update.
	#endif

	if ( ! (this->openFlags & kXMPFiles_OpenForUpdate) ) doSafeUpdate = false;
	if ( ! needsUpdate ) doSafeUpdate = false;

	bool safeUpdateOK = ( (handlerFlags & kXMPFiles_AllowsSafeUpdate) ||
						  (! (handlerFlags & kXMPFiles_HandlerOwnsFile)) );
	if ( doSafeUpdate && (! safeUpdateOK) ) {
		XMP_Throw ( "XMPFiles::CloseFile - Safe update not supported", kXMPErr_Unavailable );
	}

	if ( (this->progressTracker != 0) && this->UsesLocalIO() ) {
		XMPFiles_IO * localFile = (XMPFiles_IO*)this->ioRef;
		localFile->SetProgressTracker ( this->progressTracker );
	}

	// Try really hard to make sure the file is closed and the handler is deleted.

	try {

		if ( (! doSafeUpdate) || (handlerFlags & kXMPFiles_HandlerOwnsFile) ) {	// ! Includes no update case.

			// Close the file without doing common crash-safe writing. The handler might do it.

			if ( needsUpdate ) {
				#if GatherPerformanceData
					sAPIPerf->back().extraInfo += ", direct update";
				#endif
				this->handler->UpdateFile ( doSafeUpdate );
			}

			delete this->handler;
			this->handler = 0;
			CloseLocalFile ( this );

		} else {

			// Update the file in a crash-safe manner using common control of a temp file.

			XMP_IO* tempFileRef = this->ioRef->DeriveTemp();
			if ( tempFileRef == 0 ) XMP_Throw ( "XMPFiles::CloseFile, cannot create temp", kXMPErr_InternalFailure );

			if ( handlerFlags & kXMPFiles_CanRewrite ) {

				// The handler can rewrite an entire file based on the original.

				#if GatherPerformanceData
					sAPIPerf->back().extraInfo += ", file rewrite";
				#endif

				this->handler->WriteTempFile ( tempFileRef );

			} else {

				// The handler can only update an existing file. Copy to the temp then update.


				#if GatherPerformanceData
					sAPIPerf->back().extraInfo += ", copy update";
				#endif


				XMP_IO* origFileRef = this->ioRef;

				origFileRef->Rewind();
				if ( this->progressTracker != 0 && (this->handler->handlerFlags & kXMPFiles_CanNotifyProgress) ) progressTracker->BeginWork ( (float) origFileRef->Length() );
				XIO::Copy ( origFileRef, tempFileRef, origFileRef->Length(), abortProc, abortArg );

				try {

					this->ioRef = tempFileRef;
					this->handler->UpdateFile ( false );	// We're doing the safe update, not the handler.
					this->ioRef = origFileRef;

				} catch ( ... ) {

					this->ioRef->DeleteTemp();
					this->ioRef = origFileRef;
					throw;

				}

				if ( progressTracker != 0 && (this->handler->handlerFlags & kXMPFiles_CanNotifyProgress)) progressTracker->WorkComplete();

			}

			this->ioRef->AbsorbTemp();
			CloseLocalFile ( this );

			delete this->handler;
			this->handler = 0;

		}

	} catch ( ... ) {

		// *** Don't delete the temp or copy files, not sure which is best.

		try {
			if ( this->handler != 0 ) delete this->handler;
		} catch ( ... ) { /* Do nothing, throw the outer exception later. */ }

		if ( this->ioRef ) this->ioRef->DeleteTemp();
		CloseLocalFile ( this );
		this->ClearFilePath();

		this->handler   = 0;
		this->format    = kXMP_UnknownFile;
		this->ioRef     = 0;
		this->openFlags = 0;

		if ( this->tempPtr != 0 ) free ( this->tempPtr );	// ! Must have been malloc-ed!
		this->tempPtr  = 0;
		this->tempUI32 = 0;

		throw;

	}

	// Clear the XMPFiles member variables.

	CloseLocalFile ( this );
	this->ClearFilePath();

	this->handler   = 0;
	this->format    = kXMP_UnknownFile;
	this->ioRef     = 0;
	this->openFlags = 0;

	if ( this->tempPtr != 0 ) free ( this->tempPtr );	// ! Must have been malloc-ed!
	this->tempPtr  = 0;
	this->tempUI32 = 0;
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )

}	// XMPFiles::CloseFile

// =================================================================================================

bool
XMPFiles::GetFileInfo ( XMP_StringPtr *  filePath /* = 0 */,
                        XMP_StringLen *  pathLen /* = 0 */,
	                    XMP_OptionBits * openFlags /* = 0 */,
	                    XMP_FileFormat * format /* = 0 */,
	                    XMP_OptionBits * handlerFlags /* = 0 */ ) const
{
	XMP_FILES_START
	if ( this->handler == 0 ) return false;
	XMPFileHandler * handler = this->handler;

	if ( filePath == 0 ) filePath = &voidStringPtr;
	if ( pathLen == 0 ) pathLen = &voidStringLen;
	if ( openFlags == 0 ) openFlags = &voidOptionBits;
	if ( format == 0 ) format = &voidFileFormat;
	if ( handlerFlags == 0 ) handlerFlags = &voidOptionBits;

	*filePath     = this->filePath.c_str();
	*pathLen      = (XMP_StringLen) this->filePath.size();
	*openFlags    = this->openFlags;
	*format       = this->format;
	*handlerFlags = this->handler->handlerFlags;
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
	return true;

}	// XMPFiles::GetFileInfo

// =================================================================================================

void
XMPFiles::SetAbortProc ( XMP_AbortProc abortProc,
						 void *        abortArg )
{
	XMP_FILES_START
	this->abortProc = abortProc;
	this->abortArg  = abortArg;

	XMP_Assert ( (abortProc != (XMP_AbortProc)0) || (abortArg != (void*)(unsigned long long)0xDEADBEEFULL) );	// Hack to test the assert callback.
	XMP_FILES_END1 ( kXMPErrSev_OperationFatal )
}	// XMPFiles::SetAbortProc

// =================================================================================================
// SetClientPacketInfo
// ===================
//
// Set the packet info returned to the client. This is the internal packet info at first, which
// tells what is in the file. But once the file needs update (PutXMP has been called), we return
// info about the latest XMP. The internal packet info is left unchanged since it is needed when
// the file is updated to locate the old packet in the file.

static void
SetClientPacketInfo ( XMP_PacketInfo * clientInfo, const XMP_PacketInfo & handlerInfo,
					  const std::string & xmpPacket, bool needsUpdate )
{

	if ( clientInfo == 0 ) return;

	if ( ! needsUpdate ) {
		*clientInfo = handlerInfo;
	} else {
		clientInfo->offset = kXMPFiles_UnknownOffset;
		clientInfo->length = (XMP_Int32) xmpPacket.size();
		FillPacketInfo ( xmpPacket, clientInfo );
	}

}	// SetClientPacketInfo

// =================================================================================================

bool
XMPFiles::GetXMP ( SXMPMeta *       xmpObj /* = 0 */,
                   XMP_StringPtr *  xmpPacket /* = 0 */,
                   XMP_StringLen *  xmpPacketLen /* = 0 */,
                   XMP_PacketInfo * packetInfo /* = 0 */ )
{
	XMP_FILES_START
	if ( this->handler == 0 ) XMP_Throw ( "XMPFiles::GetXMP - No open file", kXMPErr_BadObject );

	XMP_OptionBits applyTemplateFlags = kXMPTemplate_AddNewProperties | kXMPTemplate_IncludeInternalProperties;

	if ( ! this->handler->processedXMP ) {
		try {
			this->handler->ProcessXMP();
		} catch ( ... ) {
			// Return the outputs then rethrow the exception.
			if ( xmpObj != 0 ) {
				// ! Don't use Clone, that replaces the internal ref in the local xmpObj, leaving the client unchanged!
				xmpObj->Erase();
				SXMPUtils::ApplyTemplate ( xmpObj, this->handler->xmpObj, applyTemplateFlags );
			}
			if ( xmpPacket != 0 ) *xmpPacket = this->handler->xmpPacket.c_str();
			if ( xmpPacketLen != 0 ) *xmpPacketLen = (XMP_StringLen) this->handler->xmpPacket.size();
			SetClientPacketInfo ( packetInfo, this->handler->packetInfo,
								  this->handler->xmpPacket, this->handler->needsUpdate );
			throw;
		}
	}

	if ( ! this->handler->containsXMP ) return false;

	#if 0	// *** See bug 1131815. A better way might be to pass the ref up from here.
		if ( xmpObj != 0 ) *xmpObj = this->handler->xmpObj.Clone();
	#else
		if ( xmpObj != 0 ) {
			// ! Don't use Clone, that replaces the internal ref in the local xmpObj, leaving the client unchanged!
			xmpObj->Erase();
			SXMPUtils::ApplyTemplate ( xmpObj, this->handler->xmpObj, applyTemplateFlags );
		}
	#endif

	if ( xmpPacket != 0 ) *xmpPacket = this->handler->xmpPacket.c_str();
	if ( xmpPacketLen != 0 ) *xmpPacketLen = (XMP_StringLen) this->handler->xmpPacket.size();
	SetClientPacketInfo ( packetInfo, this->handler->packetInfo,
						  this->handler->xmpPacket, this->handler->needsUpdate );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
	return true;

}	// XMPFiles::GetXMP

// =================================================================================================

static bool
DoPutXMP ( XMPFiles * thiz, const SXMPMeta & xmpObj, const bool doIt )
{
	// Check some basic conditions to see if the Put should be attempted.

	if ( thiz->handler == 0 ) XMP_Throw ( "XMPFiles::PutXMP - No open file", kXMPErr_BadObject );
	if ( ! (thiz->openFlags & kXMPFiles_OpenForUpdate) ) {
		XMP_Throw ( "XMPFiles::PutXMP - Not open for update", kXMPErr_BadObject );
	}

	XMPFileHandler * handler      = thiz->handler;
	XMP_OptionBits   handlerFlags = handler->handlerFlags;
	XMP_PacketInfo & packetInfo   = handler->packetInfo;
	std::string &    xmpPacket    = handler->xmpPacket;

	if ( ! handler->processedXMP ) handler->ProcessXMP();	// Might have Open/Put with no GetXMP.

	size_t oldPacketOffset = (size_t)packetInfo.offset;
	size_t oldPacketLength = packetInfo.length;

	if ( oldPacketOffset == (size_t)kXMPFiles_UnknownOffset ) oldPacketOffset = 0;	// ! Simplify checks.
	if ( oldPacketLength == (size_t)kXMPFiles_UnknownLength ) oldPacketLength = 0;

	bool fileHasPacket = (oldPacketOffset != 0) && (oldPacketLength != 0);

	if ( ! fileHasPacket ) {
		if ( ! (handlerFlags & kXMPFiles_CanInjectXMP) ) {
			XMP_Throw ( "XMPFiles::PutXMP - Can't inject XMP", kXMPErr_Unavailable );
		}
		if ( handler->stdCharForm == kXMP_CharUnknown ) {
			XMP_Throw ( "XMPFiles::PutXMP - No standard character form", kXMPErr_InternalFailure );
		}
	}

	// Serialize the XMP and update the handler's info.

	XMP_Uns8 charForm = handler->stdCharForm;
	if ( charForm == kXMP_CharUnknown ) charForm = packetInfo.charForm;

	XMP_OptionBits options = handler->GetSerializeOptions() | XMP_CharToSerializeForm ( charForm );
	if ( handlerFlags & kXMPFiles_NeedsReadOnlyPacket ) options |= kXMP_ReadOnlyPacket;
	if ( fileHasPacket && (thiz->format == kXMP_UnknownFile) && (! packetInfo.writeable) ) options |= kXMP_ReadOnlyPacket;

	bool preferInPlace = ((handlerFlags & kXMPFiles_PrefersInPlace) != 0);
	bool tryInPlace    = (fileHasPacket & preferInPlace) || (! (handlerFlags & kXMPFiles_CanExpand));

	if ( handlerFlags & kXMPFiles_UsesSidecarXMP ) tryInPlace = false;

	if ( tryInPlace ) {
		try {
			xmpObj.SerializeToBuffer ( &xmpPacket, (options | kXMP_ExactPacketLength), (XMP_StringLen) oldPacketLength );
			XMP_Assert ( xmpPacket.size() == oldPacketLength );
		} catch ( ... ) {
			if ( preferInPlace ) {
				tryInPlace = false;	// ! Try again, out of place this time.
			} else {
				if ( ! doIt ) return false;
				throw;
			}
		}
	}

	if ( ! tryInPlace ) {
		try {
			xmpObj.SerializeToBuffer ( &xmpPacket, options );
		} catch ( ... ) {
			if ( ! doIt ) return false;
			throw;
		}
	}

	if ( doIt ) {
		handler->xmpObj = xmpObj.Clone();
		handler->containsXMP = true;
		handler->processedXMP = true;
		handler->needsUpdate = true;
	}

	return true;

}	// DoPutXMP

// =================================================================================================

void
XMPFiles::PutXMP ( const SXMPMeta & xmpObj )
{
	XMP_FILES_START
	(void) DoPutXMP ( this, xmpObj, true );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )

}	// XMPFiles::PutXMP

// =================================================================================================

void
XMPFiles::PutXMP ( XMP_StringPtr xmpPacket,
                   XMP_StringLen xmpPacketLen /* = kXMP_UseNullTermination */ )
{
	XMP_FILES_START
	SXMPMeta xmpObj;
	xmpObj.SetErrorCallback ( ErrorCallbackForXMPMeta, &errorCallback );
	xmpObj.ParseFromBuffer ( xmpPacket, xmpPacketLen );
	this->PutXMP ( xmpObj );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
}	// XMPFiles::PutXMP

// =================================================================================================

bool
XMPFiles::CanPutXMP ( const SXMPMeta & xmpObj )
{
	XMP_FILES_START
	if ( this->handler == 0 ) XMP_Throw ( "XMPFiles::CanPutXMP - No open file", kXMPErr_BadObject );

	if ( ! (this->openFlags & kXMPFiles_OpenForUpdate) ) return false;

	if ( this->handler->handlerFlags & kXMPFiles_CanInjectXMP ) return true;
	if ( ! this->handler->containsXMP ) return false;
	if ( this->handler->handlerFlags & kXMPFiles_CanExpand ) return true;

	return DoPutXMP ( this, xmpObj, false );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
	return false;

}	// XMPFiles::CanPutXMP

// =================================================================================================

bool
XMPFiles::CanPutXMP ( XMP_StringPtr xmpPacket,
                      XMP_StringLen xmpPacketLen /* = kXMP_UseNullTermination */ )
{
	XMP_FILES_START
	SXMPMeta xmpObj;
	xmpObj.SetErrorCallback ( ErrorCallbackForXMPMeta, &errorCallback );
	xmpObj.ParseFromBuffer ( xmpPacket, xmpPacketLen );
	return this->CanPutXMP ( xmpObj );
	XMP_FILES_END1 ( kXMPErrSev_FileFatal )
	return false;

}	// XMPFiles::CanPutXMP

// =================================================================================================

/* class-static */
void
XMPFiles::SetDefaultProgressCallback ( const XMP_ProgressTracker::CallbackInfo & cbInfo )
{
	XMP_FILES_STATIC_START
	XMP_Assert ( cbInfo.wrapperProc != 0 );	// ! Should be provided by the glue code.

	sProgressDefault = cbInfo;
	XMP_FILES_STATIC_END1 ( kXMPErrSev_OperationFatal )

}	// XMPFiles::SetDefaultProgressCallback

// =================================================================================================

void
XMPFiles::SetProgressCallback ( const XMP_ProgressTracker::CallbackInfo & cbInfo )
{
	XMP_FILES_START
	XMP_Assert ( cbInfo.wrapperProc != 0 );	// ! Should be provided by the glue code.

	if ( (this->handler != 0) && this->UsesClientIO() ) return;	// Can't use progress tracking.

	if ( this->progressTracker != 0 ) {
		delete this->progressTracker;	// ! Delete any existing tracker.
		this->progressTracker = 0;
	}
	
	if ( cbInfo.clientProc != 0 ) {
		this->progressTracker = new XMP_ProgressTracker ( cbInfo );
	}
	XMP_FILES_END1 ( kXMPErrSev_OperationFatal )

}	// XMPFiles::SetProgressCallback

// =================================================================================================
// Error notifications
// ===================

// -------------------------------------------------------------------------------------------------
// SetDefaultErrorCallback
// -----------------------

/* class-static */ 
void XMPFiles::SetDefaultErrorCallback ( XMPFiles_ErrorCallbackWrapper	wrapperProc,
										 XMPFiles_ErrorCallbackProc		clientProc,
										 void *							context,
										 XMP_Uns32						limit )
{
	XMP_FILES_STATIC_START
	XMP_Assert ( wrapperProc != 0 );	// Must always be set by the glue;

	sDefaultErrorCallback.wrapperProc = wrapperProc;
	sDefaultErrorCallback.clientProc = clientProc;
	sDefaultErrorCallback.context = context;
	sDefaultErrorCallback.limit = limit;
	XMP_FILES_STATIC_END1 ( kXMPErrSev_OperationFatal )
}	// SetDefaultErrorCallback

// -------------------------------------------------------------------------------------------------
// SetErrorCallback
// ----------------

void XMPFiles::SetErrorCallback ( XMPFiles_ErrorCallbackWrapper	wrapperProc,
								  XMPFiles_ErrorCallbackProc	clientProc,
								  void *						context,
								  XMP_Uns32						limit )
{
	XMP_FILES_START
	XMP_Assert ( wrapperProc != 0 );	// Must always be set by the glue;

	this->errorCallback.Clear();
	this->errorCallback.wrapperProc = wrapperProc;
	this->errorCallback.clientProc = clientProc;
	this->errorCallback.context = context;
	this->errorCallback.limit = limit;
	XMP_FILES_END1 ( kXMPErrSev_OperationFatal )
}	// SetErrorCallback

// -------------------------------------------------------------------------------------------------
// ResetErrorCallbackLimit
// -----------------------

void XMPFiles::ResetErrorCallbackLimit ( XMP_Uns32 limit ) {
	XMP_FILES_START
	this->errorCallback.limit = limit;
	this->errorCallback.notifications = 0;
	this->errorCallback.topSeverity = kXMPErrSev_Recoverable;
	XMP_FILES_END1 ( kXMPErrSev_OperationFatal )
}	// ResetErrorCallbackLimit

// -------------------------------------------------------------------------------------------------
// ErrorCallbackInfo::CanNotify
// -------------------------------
//
// This is const just to be usable from const XMPMeta functions.

bool 
	XMPFiles::ErrorCallbackInfo::CanNotify() const
{
	XMP_Assert ( (this->clientProc == 0) || (this->wrapperProc != 0) );
	return ( this->clientProc != 0 );
}

// -------------------------------------------------------------------------------------------------
// ErrorCallbackInfo::ClientCallbackWrapper
// -------------------------------
//
// This is const just to be usable from const XMPMeta functions.

bool
	XMPFiles::ErrorCallbackInfo::ClientCallbackWrapper ( XMP_StringPtr filePath,
														 XMP_ErrorSeverity severity,
														 XMP_Int32 cause,
														 XMP_StringPtr messsage ) const
{
	
	XMP_StringPtr filePathPtr = filePath;
	if ( filePathPtr == 0 ) {
		filePathPtr = this->filePath.c_str();
	}

	XMP_Bool retValue = (*this->wrapperProc) ( this->clientProc, this->context, filePathPtr, severity, cause, messsage );
	return ConvertXMP_BoolToBool(retValue);
}

// -------------------------------------------------------------------------------------------------
// ErrorCallbackForXMPMeta
// -------------------------------

bool
	ErrorCallbackForXMPMeta ( void * context, XMP_ErrorSeverity severity, 
							   XMP_Int32 cause,
							   XMP_StringPtr message)
{
	//typecast context to GenericErrorCallback
	GenericErrorCallback * callback = ( GenericErrorCallback * ) context;
	XMP_Error error ( cause, message );
	callback->NotifyClient ( severity, error );
	return !kXMP_Bool_False;
}

// =================================================================================================
