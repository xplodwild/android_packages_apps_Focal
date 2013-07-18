// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _ChunkPath_h_
#define _ChunkPath_h_

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"

#include <limits.h>	// For UINT_MAX.
#include <vector>

namespace IFF_RIFF
{

/**
	A ChunkPath describes one certain chunk in the hierarchy of chunks
	of the IFF/RIFF file format.
	Each chunks gets identified by a structure of the type ChunkIdentifier.
	Which consists of the 4byte ID of the chunk and, if applicable, the 4byte
	type of the chunk.
*/

// IFF/RIFF ids
enum {
	// invalid ID
	kChunk_NONE = UINT_MAX,

	// format chunks
	kChunk_RIFF = 0x52494646,
	kChunk_RF64 = 0x52463634,
	kChunk_FORM = 0x464F524D,
	kChunk_JUNK = 0x4A554E4B,
	kChunk_JUNQ = 0x4A554E51,
	

	// other container chunks
	kChunk_LIST = 0x4C495354,

	// other relevant chunks
	kChunk_XMP  = 0x5F504D58, // "_PMX"

	kChunk_data	= 0x64617461,

	//should occur only in AVI
	kChunk_Cr8r = 0x43723872,
	kChunk_PrmL = 0x50726D4C,

	//should occur only in WAV
	kChunk_DISP = 0x44495350,
	kChunk_bext = 0x62657874,
	kChunk_cart = 0x63617274,
	kChunk_ds64 = 0x64733634,

	// AIFF
	kChunk_APPL	= 0x4150504C,
	kChunk_NAME = 0x4E414D45,
	kChunk_AUTH = 0x41555448,
	kChunk_CPR  = 0x28632920,
	kChunk_ANNO = 0x414E4E4F
};

// IFF/RIFF types
enum {
	kType_AVI_ = 0x41564920,
	kType_AVIX = 0x41564958,
	kType_WAVE = 0x57415645,
	kType_AIFF = 0x41494646,
	kType_AIFC = 0x41494643,
	kType_INFO = 0x494E464F,
	kType_Tdat = 0x54646174,

	// AIFF
	kType_XMP  = 0x584D5020,
	kType_FREE = 0x46524545,

	kType_NONE = UINT_MAX
};


struct ChunkIdentifier
{
   XMP_Uns32 id;
   XMP_Uns32 type;
};

/**
* calculates the size of a ChunkIdentifier array.
* Has to be a macro as the sizeof operator does nto work for pointer function parameters
*/
#define SizeOfCIArray(ciArray) ( sizeof(ciArray) / sizeof(ChunkIdentifier) )


class ChunkPath
{
public:
	/**
	ctor/dtor
	*/
							ChunkPath( const ChunkIdentifier* path = NULL, XMP_Uns32 size = 0 );
							ChunkPath( const ChunkPath& path );
							ChunkPath( const ChunkIdentifier& identifier );
						   ~ChunkPath();

	ChunkPath &				operator=( const ChunkPath &rhs );

	/**
	Append a ChunkIdentifier to the end of the path

	@param	id		4byte id of chunk
	@param	type	4byte type of chunk
	*/
	void					append( XMP_Uns32 id, XMP_Uns32 type = kType_NONE );
	void					append( const ChunkIdentifier& identifier );
	
	/**
	Append a whole path
	
	@param path Array of ChunkIdentifiert objects
	@param size number of elements in the given array
	*/
	void					append( const ChunkIdentifier* path = NULL, XMP_Uns32 size = 0 );

	/**
	Insert an identifier
	
	@param identifier	id and type
	@param pos			position within the path
	 */
	void					insert( const ChunkIdentifier& identifier, XMP_Uns32 pos = 0 );

	/**
	Remove the endmost ChunkIdentifier from the path
	*/
	void					remove();
	/**
	Remove the ChunkIdentifier at the passed position in the path.
	Throw exception if the position is out of range.

	@param	pos		Position of ChunkIdentifier in the path
	*/
	void					removeAt( XMP_Int32 pos );

	/**
	Return ChunkIdentifier at the passed position

	@param	pos		Position of ChunkIdentifier in the path
	@return			ChunkIdentifier at passed position (throw exception if
					the position is out of range)
	*/
	const ChunkIdentifier&	identifier( XMP_Int32 pos ) const;

	/**
	Return the number of ChunkIdentifier's in the path
	*/
	XMP_Int32				length() const;

	/**
	Remove all ChunkIdentifier's from the path
	*/
	void					clear();

	/**
	Compare the passed ChunkPath with this path.

	@param	path	Path to compare with this path
	@return			Match result
	*/
	enum MatchResult
	{
		kNoMatch	= 0,
		kPartMatch	= 1,
		kFullMatch	= 2
	};

	MatchResult				match( const ChunkPath& path ) const;

private:
	std::vector<ChunkIdentifier>	mPath;
};

}

#endif
