// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef __WAVE_Handler_hpp__
#define __WAVE_Handler_hpp__	1

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/IFF/ChunkController.h"
#include "XMPFiles/source/FormatSupport/IFF/IChunkBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/IChunkData.h"
#include "source/Endian.h"
#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "XMPFiles/source/FormatSupport/WAVE/BEXTMetadata.h"
#include "XMPFiles/source/FormatSupport/WAVE/CartMetadata.h"
#include "XMPFiles/source/FormatSupport/WAVE/DISPMetadata.h"
#include "XMPFiles/source/FormatSupport/WAVE/INFOMetadata.h"
#include "source/XIO.hpp"
#include "XMPFiles/source/XMPFiles_Impl.hpp"

using namespace IFF_RIFF;

// =================================================================================================
/// \file WAV_Handler.hpp
/// \brief File format handler for AIFF.
// =================================================================================================

/**
 * Contructor for the handler.
 */
extern XMPFileHandler * WAVE_MetaHandlerCTor ( XMPFiles * parent );

/**
 * Checks the format of the file, see common code.
 */
extern bool WAVE_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
			                  XMP_IO*    fileRef,
			                  XMPFiles *     parent );

/** WAVE does not need kXMPFiles_CanRewrite as we can always use UpdateFile to either do
 *  in-place update or append to the file. */
static const XMP_OptionBits kWAVE_HandlerFlags = (kXMPFiles_CanInjectXMP | 
												  kXMPFiles_CanExpand | 
												  kXMPFiles_PrefersInPlace |
												  kXMPFiles_CanReconcile |
												  kXMPFiles_ReturnsRawPacket |
												  kXMPFiles_AllowsSafeUpdate |
												  kXMPFiles_CanNotifyProgress
												 );

/**
 * Main class for the the WAVE file handler.
 */
class WAVE_MetaHandler : public XMPFileHandler
{
public:
	WAVE_MetaHandler ( XMPFiles* parent );
	~WAVE_MetaHandler();

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	/**
	* Checks if the first 4 bytes of the given buffer are either type RIFF or RF64
	* @param buffer a byte buffer that must contain at least 4 bytes and point to the correct byte
	* @return Either kChunk_RIFF, kChunk_RF64 0 if no type could be determined
	*/
	static XMP_Uns32 whatRIFFFormat( XMP_Uns8* buffer );

private:
	/**
	 * Updates/creates/deletes a given legacy chunk depending on the given new legacy value
	 * If the Chunk exists and is not empty, it is updated. If it is empty the 
	 * Chunk is removed from the tree. If the Chunk does not exist but a value is given, it is created
	 * and initialized with that value
	 *
	 * @param chunk OUT pointer to the legacy chunk
	 * @param chunkID Id of the Chunk if it needs to be created
	 * @param chunkType Type of the Chunk if it needs to be created
	 * @param legacyData the new legacy metadata object (can be empty)
	 */
	void updateLegacyChunk( IChunkData **chunk, XMP_Uns32 chunkID, XMP_Uns32 chunkType, IMetadata &legacyData  );

	
	/** private standard Ctor, not to be used */
	WAVE_MetaHandler (): mChunkController(NULL), mChunkBehavior(NULL), 
						mINFOMeta(), mBEXTMeta(), mCartMeta(), mDISPMeta(),
						mXMPChunk(NULL), mINFOChunk(NULL),
						mBEXTChunk(NULL), mCartChunk(NULL), mDISPChunk(NULL) {};

	// ----- MEMBERS ----- //
	
	/** Controls the parsing and writing of the passed stream. */
	ChunkController *mChunkController;
	/** Represents the rules how chunks are added, removed or rearranged */
	IChunkBehavior *mChunkBehavior;
	/** container for Legacy metadata */
	INFOMetadata mINFOMeta;
	BEXTMetadata mBEXTMeta;
	CartMetadata mCartMeta;
	DISPMetadata mDISPMeta;

	// cr8r is not yet required for WAVE
	// Cr8rMetadata mCr8rMeta;

	/** pointer to the XMP chunk */
	IChunkData *mXMPChunk;
	/** pointer to legacy chunks */
	IChunkData *mINFOChunk;
	IChunkData *mBEXTChunk;
	IChunkData *mCartChunk;
	IChunkData *mDISPChunk;

	// cr8r is not yet required for WAVE
	// IChunkData *mCr8rChunk;

	// ----- CONSTANTS ----- //
	
	/** Chunk path identifier of interest in WAVE */
	static const ChunkIdentifier kRIFFXMP[2];
	static const ChunkIdentifier kRIFFInfo[2];
	static const ChunkIdentifier kRIFFDisp[2];
	static const ChunkIdentifier kRIFFBext[2];
	static const ChunkIdentifier kRIFFCart[2];

	// cr8r is not yet required for WAVE
	// static const ChunkIdentifier kWAVECr8r[2];

	/** Chunk path identifier of interest in RF64 */
	static const ChunkIdentifier kRF64XMP[2];
	static const ChunkIdentifier kRF64Info[2];
	static const ChunkIdentifier kRF64Disp[2];
	static const ChunkIdentifier kRF64Bext[2];
	static const ChunkIdentifier kRF64Cart[2];

	// cr8r is not yet required for WAVE
	// static const ChunkIdentifier kRF64Cr8r[2];
	
	/** Path to XMP chunk */
	ChunkPath mWAVEXMPChunkPath;

	/** Path to INFO chunk */
	ChunkPath mWAVEInfoChunkPath;

	/** Path to DISP chunk */
	ChunkPath mWAVEDispChunkPath;

	/** Path to BEXT chunk */
	ChunkPath mWAVEBextChunkPath;

	/** Path to cart chunk */
	ChunkPath mWAVECartChunkPath;

	//cr8r is not yet required for WAVE
	///** Path to Cr8r chunk */
	//const ChunkPath mWAVECr8rChunkPath;

};	// WAVE_MetaHandler

// =================================================================================================

#endif /* __WAVE_Handler_hpp__ */
