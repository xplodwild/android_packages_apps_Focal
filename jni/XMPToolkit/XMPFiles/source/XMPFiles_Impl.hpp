#ifndef __XMPFiles_Impl_hpp__
#define __XMPFiles_Impl_hpp__	1

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
#include "build/XMP_BuildInfo.h"
#include "source/XMP_LibUtils.hpp"
#include "source/EndianUtils.hpp"

#include <string>
#define TXMP_STRING_TYPE std::string
#define XMP_INCLUDE_XMPFILES 1
#include "public/include/XMP.hpp"

#include "XMPFiles/source/XMPFiles.hpp"
#include "public/include/XMP_IO.hpp"
#include "source/Host_IO.hpp"

#include <vector>
#include <map>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#if XMP_WinBuild

	#define snprintf _snprintf

#else

	#if XMP_MacBuild
		#include <CoreServices/CoreServices.h>
	#endif
	
	// POSIX headers for both Mac and generic UNIX.
	#include <fcntl.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <sys/stat.h>
	#include <sys/types.h>

#endif

// =================================================================================================
// General global variables and macros
// ===================================

typedef std::vector<XMP_Uns8> RawDataBlock;

extern bool ignoreLocalText;

#ifndef EnablePhotoHandlers
	#define EnablePhotoHandlers 1
#endif

#ifndef EnableDynamicMediaHandlers
	#define EnableDynamicMediaHandlers 1
#endif

#ifndef EnableMiscHandlers
	#define EnableMiscHandlers 1
#endif

#ifndef EnablePacketScanning
	#define EnablePacketScanning 1
#endif

#ifndef EnablePluginManager
	#if XMP_iOSBuild
		#define EnablePluginManager 0
	#else
		#define EnablePluginManager 1
	#endif
#endif

extern XMP_Int32 sXMPFilesInitCount;

#ifndef GatherPerformanceData
	#define GatherPerformanceData 0
#endif

#if ! GatherPerformanceData

	#define StartPerfCheck(proc,info)	/* do nothing */
	#define EndPerfCheck(proc)	/* do nothing */

#else

	#include "PerfUtils.hpp"
	
	enum {
		kAPIPerf_OpenFile,
		kAPIPerf_CloseFile,
		kAPIPerf_GetXMP,
		kAPIPerf_PutXMP,
		kAPIPerf_CanPutXMP,
		kAPIPerfProcCount	// Last, count of the procs.
	};
	
	static const char* kAPIPerfNames[] =
	{ "OpenFile", "CloseFile", "GetXMP", "PutXMP", "CanPutXMP", 0 };
	
	struct APIPerfItem {
		XMP_Uns8    whichProc;
		double      elapsedTime;
		XMPFilesRef xmpFilesRef;
		std::string extraInfo;
		APIPerfItem ( XMP_Uns8 proc, double time, XMPFilesRef ref, const char * info )
			: whichProc(proc), elapsedTime(time), xmpFilesRef(ref), extraInfo(info) {};
	};
	
	typedef std::vector<APIPerfItem> APIPerfCollection;
	
	extern APIPerfCollection* sAPIPerf;
	
	#define StartPerfCheck(proc,info)	\
		sAPIPerf->push_back ( APIPerfItem ( proc, 0.0, xmpFilesRef, info ) );	\
		APIPerfItem & thisPerf = sAPIPerf->back();								\
		PerfUtils::MomentValue startTime, endTime;								\
		try {																	\
		startTime = PerfUtils::NoteThisMoment();
	
	#define EndPerfCheck(proc)	\
		endTime = PerfUtils::NoteThisMoment();										\
		thisPerf.elapsedTime = PerfUtils::GetElapsedSeconds ( startTime, endTime );	\
	} catch ( ... ) {																\
		endTime = PerfUtils::NoteThisMoment();										\
		thisPerf.elapsedTime = PerfUtils::GetElapsedSeconds ( startTime, endTime );	\
		thisPerf.extraInfo += "  ** THROW **";										\
		throw;																		\
	}

#endif

extern XMP_FileFormat voidFileFormat;	// Used as sink for unwanted output parameters.
extern XMP_PacketInfo voidPacketInfo;
extern void *         voidVoidPtr;
extern XMP_StringPtr  voidStringPtr;
extern XMP_StringLen  voidStringLen;
extern XMP_OptionBits voidOptionBits;

static const XMP_Uns8 * kUTF8_PacketStart = (const XMP_Uns8 *) "<?xpacket begin=";
static const XMP_Uns8 * kUTF8_PacketID    = (const XMP_Uns8 *) "W5M0MpCehiHzreSzNTczkc9d";
static const size_t     kUTF8_PacketHeaderLen = 51;	// ! strlen ( "<?xpacket begin='xxx' id='W5M0MpCehiHzreSzNTczkc9d'" )

static const XMP_Uns8 * kUTF8_PacketTrailer    = (const XMP_Uns8 *) "<?xpacket end=\"w\"?>";
static const size_t     kUTF8_PacketTrailerLen = 19;	// ! strlen ( kUTF8_PacketTrailer )

struct FileExtMapping {
	XMP_StringPtr  ext;
	XMP_FileFormat format;
};

extern const FileExtMapping kFileExtMap[];
extern const char * kKnownScannedFiles[];
extern const char * kKnownRejectedFiles[];

class CharStarLess {	// Comparison object for the genre code maps.
public:
	bool operator() ( const char * left, const char * right ) const {
		int order = strcmp ( left, right );
		return order < 0;
	}
};

typedef std::map < const char *, const char *, CharStarLess >		ID3GenreMap;
extern ID3GenreMap* kMapID3GenreCodeToName;	// Storage defined in ID3_Support.cpp.
extern ID3GenreMap* kMapID3GenreNameToCode;

#define Uns8Ptr(p) ((XMP_Uns8 *) (p))

#define IsNewline( ch )    ( ((ch) == kLF) || ((ch) == kCR) )
#define IsSpaceOrTab( ch ) ( ((ch) == ' ') || ((ch) == kTab) )
#define IsWhitespace( ch ) ( IsSpaceOrTab ( ch ) || IsNewline ( ch ) )

static inline void MakeLowerCase ( std::string * str )
{
	for ( size_t i = 0, limit = str->size(); i < limit; ++i ) {
		char ch = (*str)[i];
		if ( ('A' <= ch) && (ch <= 'Z') ) (*str)[i] += 0x20;
	}
}

static inline void MakeUpperCase ( std::string * str )
{
	for ( size_t i = 0, limit = str->size(); i < limit; ++i ) {
		char ch = (*str)[i];
		if ( ('a' <= ch) && (ch <= 'z') ) (*str)[i] -= 0x20;
	}
}

#define XMP_LitMatch(s,l)		(strcmp((s),(l)) == 0)
#define XMP_LitNMatch(s,l,n)	(strncmp((s),(l),(n)) == 0)

// =================================================================================================
// Support for call tracing
// ========================

#ifndef XMP_TraceFilesCalls
	#define XMP_TraceFilesCalls			0
	#define XMP_TraceFilesCallsToFile	0
#endif

#if XMP_TraceFilesCalls

	#undef AnnounceThrow
	#undef AnnounceCatch
	
	#undef AnnounceEntry
	#undef AnnounceNoLock
	#undef AnnounceExit
	
	extern FILE * xmpFilesLog;
	
	#define AnnounceThrow(msg)	\
		fprintf ( xmpFilesLog, "XMP_Throw: %s\n", msg ); fflush ( xmpFilesLog )
	#define AnnounceCatch(msg)	\
		fprintf ( xmpFilesLog, "Catch in %s: %s\n", procName, msg ); fflush ( xmpFilesLog )
	
	#define AnnounceEntry(proc)			\
		const char * procName = proc;	\
		fprintf ( xmpFilesLog, "Entering %s\n", procName ); fflush ( xmpFilesLog )
	#define AnnounceNoLock(proc)		\
		const char * procName = proc;	\
		fprintf ( xmpFilesLog, "Entering %s (no lock)\n", procName ); fflush ( xmpFilesLog )
	#define AnnounceExit()	\
		fprintf ( xmpFilesLog, "Exiting %s\n", procName ); fflush ( xmpFilesLog )

#endif

// =================================================================================================
// Support for memory leak tracking
// ================================

#ifndef TrackMallocAndFree
	#define TrackMallocAndFree 0
#endif

#if TrackMallocAndFree

	static void* ChattyMalloc ( size_t size )
	{
		void* ptr = malloc ( size );
		fprintf ( stderr, "Malloc %d bytes @ %.8X\n", size, ptr );
		return ptr;
	}
	
	static void ChattyFree ( void* ptr )
	{
		fprintf ( stderr, "Free @ %.8X\n", ptr );
		free ( ptr );
	}
	
	#define malloc(s) ChattyMalloc ( s )
	#define free(p) ChattyFree ( p )

#endif

// =================================================================================================
// FileHandler declarations
// ========================

extern void ReadXMPPacket ( XMPFileHandler * handler );

extern void FillPacketInfo ( const XMP_VarString & packet, XMP_PacketInfo * info );

class XMPFileHandler {	// See XMPFiles.hpp for usage notes.
public:

#define DefaultCTorPresets							\
	handlerFlags(0), stdCharForm(kXMP_CharUnknown),	\
	containsXMP(false), processedXMP(false), needsUpdate(false)

	XMPFileHandler() : parent(0), DefaultCTorPresets {};
	XMPFileHandler (XMPFiles * _parent) : parent(_parent), DefaultCTorPresets
	{
		xmpObj.SetErrorCallback(ErrorCallbackForXMPMeta, &parent->errorCallback);
	};

	virtual ~XMPFileHandler() {};	// ! The specific handler is responsible for tnailInfo.tnailImage.
	
	virtual bool GetFileModDate ( XMP_DateTime * modDate );	// The default implementation is for embedding handlers.
	virtual void FillMetadataFiles ( std::vector<std::string> * metadataFiles );
	virtual void FillAssociatedResources ( std::vector<std::string> * resourceList );
	virtual bool IsMetadataWritable ( );

	virtual void CacheFileData() = 0;
	virtual void ProcessXMP();		// The default implementation just parses the XMP.

	virtual XMP_OptionBits GetSerializeOptions();	// The default is compact.

	virtual void UpdateFile ( bool doSafeUpdate ) = 0;
	virtual void WriteTempFile ( XMP_IO* tempRef ) = 0;

	// ! Leave the data members public so common code can see them.

	XMPFiles *     parent;			// Let's the handler see the file info.
	XMP_OptionBits handlerFlags;	// Capabilities of this handler.
	XMP_Uns8       stdCharForm;		// The standard character form for output.

	bool containsXMP;		// True if the file has XMP or PutXMP has been called.
	bool processedXMP;		// True if the XMP is parsed and reconciled.
	bool needsUpdate;		// True if the file needs to be updated.

	XMP_PacketInfo packetInfo;	// ! This is always info about the packet in the file, if any!
	std::string    xmpPacket;	// ! This is the current XMP, updated by XMPFiles::PutXMP.
	SXMPMeta       xmpObj;

};	// XMPFileHandler

typedef XMPFileHandler * (* XMPFileHandlerCTor) ( XMPFiles * parent );

typedef bool (* CheckFileFormatProc ) ( XMP_FileFormat format,
									   XMP_StringPtr  filePath,
									   XMP_IO *       fileRef,
									   XMPFiles *     parent );

typedef bool (*CheckFolderFormatProc ) ( XMP_FileFormat format,
										const std::string & rootPath,
										const std::string & gpName,
										const std::string & parentName,
										const std::string & leafName,
										XMPFiles * parent );

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static inline bool CheckBytes ( const void * left, const void * right, size_t length )
{
	return (memcmp ( left, right, length ) == 0);
}

// -------------------------------------------------------------------------------------------------

static inline bool CheckCString ( const void * left, const void * right )
{
	return (strcmp ( (char*)left, (char*)right ) == 0);
}

#endif /* __XMPFiles_Impl_hpp__ */
