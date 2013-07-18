#ifndef __RIFF_Support_hpp__
#define __RIFF_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include <vector>
#include "XMPFiles/source/XMPFiles_Impl.hpp"

// ahead declaration:
class RIFF_MetaHandler;

namespace RIFF {

	// declare ahead
	class Chunk;
	class ContainerChunk;
	class ValueChunk;
	class XMPChunk;
	
	/* This rountines imports the properties found into the 
		xmp packet. Use after parsing. */
	void importProperties( RIFF_MetaHandler* handler );

	/* This rountines exports XMP properties to the respective Chunks,
	   creating those if needed. No writing to file here. */
	void exportAndRemoveProperties( RIFF_MetaHandler* handler );

	/* will relocated a wrongly placed  chunk (one of XMP, LIST:Info, LIST:Tdat=
	   from RIFF::avix back to main chunk. Chunk itself not touched. */
	void relocateWronglyPlacedXMPChunk( RIFF_MetaHandler* handler );

} // namespace RIFF

#endif	// __RIFF_Support_hpp__
