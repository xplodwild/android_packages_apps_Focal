#ifndef __RIFF_hpp__
#define __RIFF_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
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

#include <vector>
#include <map>

// ahead declaration:
class RIFF_MetaHandler;

namespace RIFF {

	enum ChunkType {
			chunk_GENERAL,	//unknown or not relevant
			chunk_CONTAINER,
			chunk_XMP,
			chunk_VALUE,
			chunk_JUNK,
			NO_CHUNK // used as precessor to first chunk, etc.
		};

	// ahead declarations
	class Chunk;
	class ContainerChunk;
	class ValueChunk;
	class XMPChunk;

	// (scope: only used in RIFF_Support and RIFF_Handler.cpp
	//   ==> no need to overspecify with lengthy names )

	typedef std::vector<Chunk*> chunkVect;  // coulddo: narrow down toValueChunk (could give trouble with JUNK though)
	typedef chunkVect::iterator chunkVectIter;  // or refactor ??

	typedef std::vector<ContainerChunk*> containerVect;
	typedef containerVect::iterator containerVectIter;

	typedef std::map<XMP_Uns32,ValueChunk*> valueMap;
	typedef valueMap::iterator valueMapIter;


	// format chunks+types
	const XMP_Uns32 kChunk_RIFF = 0x46464952;
	const XMP_Uns32 kType_AVI_  = 0x20495641;
	const XMP_Uns32 kType_AVIX  = 0x58495641;
	const XMP_Uns32 kType_WAVE  = 0x45564157;

	const XMP_Uns32 kChunk_JUNK  = 0x4B4E554A;
	const XMP_Uns32 kChunk_JUNQ  = 0x514E554A;

	// other container chunks
	const XMP_Uns32 kChunk_LIST = 0x5453494C;
	const XMP_Uns32 kType_INFO = 0x4F464E49;
	const XMP_Uns32 kType_Tdat = 0x74616454;

	// other relevant chunks
	const XMP_Uns32 kChunk_XMP  = 0x584D505F; // "_PMX"

	// relevant for Index Correction
	// LIST:
	const XMP_Uns32 kType_hdrl =  0x6C726468;
	const XMP_Uns32 kType_strl =  0x6C727473;
	const XMP_Uns32 kChunk_indx =  0x78646E69;
	const XMP_Uns32 kChunk_ixXX =  0x58587869;
	const XMP_Uns32 kType_movi =  0x69766F6D;

	//should occur only in AVI
	const XMP_Uns32 kChunk_Cr8r = 0x72387243;
	const XMP_Uns32 kChunk_PrmL = 0x4C6D7250;

	//should occur only in WAV
	const XMP_Uns32 kChunk_DISP = 0x50534944;
	const XMP_Uns32 kChunk_bext  = 0x74786562;

	// LIST/INFO constants
	const XMP_Uns32 kPropChunkIART = 0x54524149;
	const XMP_Uns32 kPropChunkICMT = 0x544D4349;
	const XMP_Uns32 kPropChunkICOP = 0x504F4349;
	const XMP_Uns32 kPropChunkICRD = 0x44524349;
	const XMP_Uns32 kPropChunkIENG = 0x474E4549;
	const XMP_Uns32 kPropChunkIGNR = 0x524E4749;
	const XMP_Uns32 kPropChunkINAM = 0x4D414E49;
	const XMP_Uns32 kPropChunkISFT = 0x54465349;
	const XMP_Uns32 kPropChunkIARL = 0x4C524149;

	const XMP_Uns32 kPropChunkIMED = 0x44454D49;
	const XMP_Uns32 kPropChunkISRF = 0x46525349;
	const XMP_Uns32 kPropChunkICMS = 0x4C524149;
	const XMP_Uns32 kPropChunkIPRD = 0x534D4349;
	const XMP_Uns32 kPropChunkISRC = 0x44525049;
	const XMP_Uns32 kPropChunkITCH = 0x43525349;

	const XMP_Uns32 kPropChunk_tc_O =0x4F5F6374;
	const XMP_Uns32 kPropChunk_tc_A =0x415F6374;
	const XMP_Uns32 kPropChunk_rn_O =0x4F5F6E72;
	const XMP_Uns32 kPropChunk_rn_A =0x415F6E72;

	///////////////////////////////////////////////////////////////

	enum PropType { // from a simplified, opinionated legacy angle
			prop_SIMPLE,
			prop_TIMEVALUE,
			prop_LOCALIZED_TEXT,
			prop_ARRAYITEM,           // ( here: a solitary one)
		};

	struct Mapping {
		XMP_Uns32 chunkID;
		const char* ns;
		const char* prop;
		PropType propType;
	};

	// bext Mappings, piece-by-piece:
	static Mapping bextDescription =			{ 0, kXMP_NS_BWF,  "description",			prop_SIMPLE };
	static Mapping bextOriginator =				{ 0, kXMP_NS_BWF,  "originator",			prop_SIMPLE };
	static Mapping bextOriginatorRef =			{ 0, kXMP_NS_BWF,  "originatorReference",	prop_SIMPLE };
	static Mapping bextOriginationDate =		{ 0, kXMP_NS_BWF,  "originationDate",		prop_SIMPLE };
	static Mapping bextOriginationTime =		{ 0, kXMP_NS_BWF,  "originationTime",		prop_SIMPLE };
	static Mapping bextTimeReference =			{ 0, kXMP_NS_BWF,  "timeReference",			prop_SIMPLE };
	static Mapping bextVersion =				{ 0, kXMP_NS_BWF,  "version",				prop_SIMPLE };
	static Mapping bextUMID =					{ 0, kXMP_NS_BWF,  "umid",					prop_SIMPLE };
	static Mapping bextCodingHistory =			{ 0, kXMP_NS_BWF,  "codingHistory",			prop_SIMPLE };

	// LIST:INFO properties
	static Mapping listInfoProps[] = {
		// reconciliations CS4 and before:
		{ kPropChunkIART, kXMP_NS_DM,	"artist" ,		prop_SIMPLE },
		{ kPropChunkICMT, kXMP_NS_DM,	"logComment" ,	prop_SIMPLE  },
		{ kPropChunkICOP, kXMP_NS_DC,	"rights" ,		prop_LOCALIZED_TEXT  },
		{ kPropChunkICRD, kXMP_NS_XMP,	"CreateDate" ,	prop_SIMPLE  },
		{ kPropChunkIENG, kXMP_NS_DM,	"engineer" ,	prop_SIMPLE  },
		{ kPropChunkIGNR, kXMP_NS_DM,	"genre" ,		prop_SIMPLE  },
		{ kPropChunkINAM, kXMP_NS_DC,	"title" ,		prop_LOCALIZED_TEXT  }, // ( was wrongly dc:album in pre-CS4)
		{ kPropChunkISFT, kXMP_NS_XMP,	"CreatorTool",	prop_SIMPLE },

		// RIFF/*/LIST/INFO properties, new in CS5, both AVI and WAV

		{ kPropChunkIMED, kXMP_NS_DC,	"source" ,		prop_SIMPLE },
		{ kPropChunkISRF, kXMP_NS_DC,	"type" ,		prop_ARRAYITEM },
		// TO ENABLE { kPropChunkIARL, kXMP_NS_DC,	"subject" ,		prop_SIMPLE }, // array !! (not x-default language alternative)
		//{ kPropChunkICMS, to be decided,	"" ,		prop_SIMPLE },
		//{ kPropChunkIPRD, to be decided,	"" ,		prop_SIMPLE },
		//{ kPropChunkISRC, to be decided,	"" ,		prop_SIMPLE },
		//{ kPropChunkITCH, to be decided,	"" ,		prop_SIMPLE },

		{ 0, 0, 0 }	// sentinel
	};

	static Mapping listTdatProps[] = {
		// reconciliations CS4 and before:
		{ kPropChunk_tc_O, kXMP_NS_DM,	"startTimecode"	, prop_TIMEVALUE  },  // special: must end up in dm:timeValue child
		{ kPropChunk_tc_A, kXMP_NS_DM,	"altTimecode"	, prop_TIMEVALUE },   // special: must end up in dm:timeValue child
		{ kPropChunk_rn_O, kXMP_NS_DM,	"tapeName"		, prop_SIMPLE  },
		{ kPropChunk_rn_A, kXMP_NS_DM,	"altTapeName"	, prop_SIMPLE  },
		{ 0, 0, 0 }	// sentinel
	};

	// =================================================================================================
	// ImportCr8rItems
	// ===============
#if SUNOS_SPARC || SUNOS_X86
	#pragma pack ( 1 )
#else
	#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86
	struct PrmLBoxContent {
		XMP_Uns32 magic;
		XMP_Uns32 size;
		XMP_Uns16 verAPI;
		XMP_Uns16 verCode;
		XMP_Uns32 exportType;
		XMP_Uns16 MacVRefNum;
		XMP_Uns32 MacParID;
		char filePath[260];
	};

	enum { kExportTypeMovie = 0, kExportTypeStill = 1, kExportTypeAudio = 2, kExportTypeCustom = 3 };

	struct Cr8rBoxContent {
		XMP_Uns32 magic;
		XMP_Uns32 size;
		XMP_Uns16 majorVer;
		XMP_Uns16 minorVer;
		XMP_Uns32 creatorCode;
		XMP_Uns32 appleEvent;
		char fileExt[16];
		char appOptions[16];
		char appName[32];
	};
#if SUNOS_SPARC || SUNOS_X86
	#pragma pack (  )
#else
	#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

	// static getter, determines appropriate chunkType (peeking)and returns
	// the respective constructor. It's the caller's responsibility to
	// delete obtained chunk.
	Chunk* getChunk ( ContainerChunk* parent, RIFF_MetaHandler* handler );

	class Chunk
	{
	public:
		ChunkType			chunkType;	// set by constructor
		ContainerChunk*		parent;		// 0 on top-level

		XMP_Uns32	id;		// the first four bytes, first byte of highest value
		XMP_Int64	oldSize;	// actual chunk size INCLUDING the 8/12 header bytes,
		XMP_Int64	oldPos;	// file position of this chunk

		// both set as part of changesAndSize()
		XMP_Int64	newSize;
		bool		hasChange;
		bool		needSizeFix;		// used in changesAndSize() only

		// Constructors ///////////////////////
		// parsing
		Chunk( ContainerChunk* parent, RIFF_MetaHandler* handler, bool skip, ChunkType c /*= chunk_GENERAL*/ );
		// ad-hoc creation
		Chunk( ContainerChunk* parent, ChunkType c, XMP_Uns32 id );

		/* returns true, if something has changed in chunk (which needs specific write-out,
		   this->newSize is expected to be set by this routine */
		virtual void changesAndSize( RIFF_MetaHandler* handler );
		virtual std::string toString(XMP_Uns8 level = 0);
		virtual void write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk = false );

		virtual ~Chunk();

	}; // class Chunk

	class XMPChunk : public Chunk
	{
	public:
		XMPChunk( ContainerChunk* parent );
		XMPChunk( ContainerChunk* parent, RIFF_MetaHandler* handler );

		void changesAndSize( RIFF_MetaHandler* handler );
		void write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk = false );

	};

	// any chunk, whose value should be stored, e.g. LIST:INFO, LIST:Tdat
	class ValueChunk : public Chunk
	{
	public:
		std::string oldValue, newValue;

		// for ad-hoc creation (upon write)
		ValueChunk( ContainerChunk* parent, std::string value, XMP_Uns32 id );

		// for parsing
		ValueChunk( ContainerChunk* parent, RIFF_MetaHandler* handler );

		enum { kNULisOptional = true };

		void SetValue( std::string value, bool optionalNUL = false );
		void changesAndSize( RIFF_MetaHandler* handler );
		void write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk = false  );

		// own destructor not needed.
	};

	// relevant (level 1) JUNQ and JUNK chunks...
	class JunkChunk : public Chunk
	{
	public:
		// construction
		JunkChunk( ContainerChunk* parent, XMP_Int64 size );
		// parsing
		JunkChunk( ContainerChunk* parent, RIFF_MetaHandler* handler );

		// own destructor not needed.

		void changesAndSize( RIFF_MetaHandler* handler );
		void write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk = false  );
	};


	class ContainerChunk : public Chunk
	{
	public:
		XMP_Uns32	containerType;					// e.g. kType_INFO as in "LIST:INFO"

		chunkVect children;		// used for cleanup/destruction, ordering...
		valueMap childmap;		// only for efficient *value* access (inside LIST), *not* used for other containers

		// construct
		ContainerChunk( ContainerChunk* parent, XMP_Uns32 id, XMP_Uns32 containerType );
		// parse
		ContainerChunk( ContainerChunk* parent, RIFF_MetaHandler* handler );

		bool removeValue( XMP_Uns32	id );

		/* returns iterator to (first) occurence of this chunk.
		   iterator to the end of the map if chunk pointer is not found */
		chunkVectIter getChild( Chunk* needle );

		void replaceChildWithJunk( Chunk* child, bool deleteChild = true );

		void changesAndSize( RIFF_MetaHandler* handler );
		std::string toString(XMP_Uns8 level = 0);
		void write( RIFF_MetaHandler* handler, XMP_IO* file, bool isMainChunk = false );

		// destroy
		void release(); // used by destructor and on error in constructor
		~ContainerChunk();

	}; // class ContainerChunk

} // namespace RIFF


#endif	// __RIFF_hpp__
