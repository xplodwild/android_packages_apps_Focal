#ifndef __MOOV_Support_hpp__
#define __MOOV_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"

#include "source/EndianUtils.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include <vector>

#define moovBoxSizeLimit 100*1024*1024

// =================================================================================================
// MOOV_Manager
// ============

class MOOV_Manager {
public:

	// ---------------------------------------------------------------------------------------------
	// Types and constants

	enum {	// Values for fileMode.
		kFileIsNormalISO = 0,		// A "normal" MPEG-4 file, no 'qt  ' compatible brand.
		kFileIsModernQT  = 1,		// Has an 'ftyp' box and 'qt  ' compatible brand.
		kFileIsTraditionalQT = 2	// Old QuickTime, no 'ftyp' box.
	};

	typedef const void * BoxRef;	// Valid until a sibling or higher box is added or deleted.

	struct BoxInfo {
		XMP_Uns32 boxType;			// In memory as native endian, compares work with ISOMedia::k_* constants.
		XMP_Uns32 childCount;		// ! A 'meta' box has both content (version/flags) and children!
		XMP_Uns32 contentSize;		// Does not include the size of nested boxes.
		const XMP_Uns8 * content;	// Null if contentSize is zero.
		BoxInfo() : boxType(0), childCount(0), contentSize(0), content(0) {};
	};

	// ---------------------------------------------------------------------------------------------
	// GetBox - Pick a box given a '/' separated list of box types. Picks the 1st of each type.
	// GetBoxInfo - Get the info if we already have the ref.
	// GetNthChild - Pick the overall n-th child of the parent, zero based.
	// GetTypeChild - Pick the first child of the given type.
	// GetParsedOffset - Get the box's offset in the parsed tree, 0 if changed since parsing.
	// GetHeaderSize - Get the box's header size in the parsed tree, 0 if changed since parsing.

	BoxRef GetBox ( const char * boxPath, BoxInfo * info ) const;
	
	void GetBoxInfo ( const BoxRef ref, BoxInfo * info ) const;

	BoxRef GetNthChild  ( BoxRef parentRef, size_t childIndex, BoxInfo * info ) const;
	BoxRef GetTypeChild ( BoxRef parentRef, XMP_Uns32 childType, BoxInfo * info ) const;

	XMP_Uns32 GetParsedOffset ( BoxRef ref ) const;
	XMP_Uns32 GetHeaderSize ( BoxRef ref ) const;

	// ---------------------------------------------------------------------------------------------
	// NoteChange - Note overall change, value was directly replaced.
	// SetBox(ref) - Replace the content with a copy of the given data.
	// SetBox(path) - Like above, but creating path to the box if necessary.
	// AddChildBox - Add a child of the given type, using a copy of the given data (may be null)

	void NoteChange();

	void SetBox ( BoxRef theBox, const void* dataPtr, XMP_Uns32 size );
	void SetBox ( const char * boxPath, const void* dataPtr, XMP_Uns32 size );

	BoxRef AddChildBox ( BoxRef parentRef, XMP_Uns32 childType, const void * dataPtr, XMP_Uns32 size );

	// ---------------------------------------------------------------------------------------------
	// DeleteNthChild - Delete the overall n-th child, return true if there was one.
	// DeleteTypeChild - Delete the first child of the given type, return true if there was one.

	bool DeleteNthChild ( BoxRef parentRef, size_t childIndex );
	bool DeleteTypeChild ( BoxRef parentRef, XMP_Uns32 childType );

	// ---------------------------------------------------------------------------------------------

	bool IsChanged() const { return this->moovNode.changed; };

	// ---------------------------------------------------------------------------------------------
	// The client is expected to fill in fullSubtree before calling ParseMemoryTree, and directly
	// use fullSubtree after calling UpdateMemoryTree.
	//
	// IMPORTANT: We only support cases where the 'moov' subtree is significantly less than 4 GB, in
	// particular with a threshold of probably 100 MB. This has 2 big impacts: we can safely use
	// 32-bit offsets and sizes, and comfortably assume everything will fit in available heap space.

	RawDataBlock fullSubtree;	// The entire 'moov' box, straight from the file or from UpdateMemoryTree.

	void ParseMemoryTree ( XMP_Uns8 fileMode );
	void UpdateMemoryTree();

	// ---------------------------------------------------------------------------------------------

	#pragma pack (push, 1)	// ! These must match the file layout!

	struct Content_mvhd_0 {
		XMP_Uns32 vFlags;			//   0
		XMP_Uns32 creationTime;		//   4
		XMP_Uns32 modificationTime;	//   8
		XMP_Uns32 timescale;		//  12
		XMP_Uns32 duration;			//  16
		XMP_Int32 rate;				//  20
		XMP_Int16 volume;			//  24
		XMP_Uns16 pad_1;			//  26
		XMP_Uns32 pad_2, pad_3;		//  28
		XMP_Int32 matrix [9];		//  36
		XMP_Uns32 preDef [6];		//  72
		XMP_Uns32 nextTrackID;		//  96
	};								// 100

	struct Content_mvhd_1 {
		XMP_Uns32 vFlags;			//   0
		XMP_Uns64 creationTime;		//   4
		XMP_Uns64 modificationTime;	//  12
		XMP_Uns32 timescale;		//  20
		XMP_Uns64 duration;			//  24
		XMP_Int32 rate;				//  32
		XMP_Int16 volume;			//  36
		XMP_Uns16 pad_1;			//  38
		XMP_Uns32 pad_2, pad_3;		//  40
		XMP_Int32 matrix [9];		//  48
		XMP_Uns32 preDef [6];		//  84
		XMP_Uns32 nextTrackID;		// 108
	};								// 112

	struct Content_hdlr {	// An 'hdlr' box as defined by ISO 14496-12. Maps OK to the QuickTime box.
		XMP_Uns32 versionFlags;	//  0
		XMP_Uns32 preDef;		//  4
		XMP_Uns32 handlerType;	//  8
		XMP_Uns32 reserved [3];	// 12
		// Plus optional component name string, null terminated UTF-8.
	};							// 24

	struct Content_stsd_entry {
		XMP_Uns32 entrySize;		//  0
		XMP_Uns32 format;			//  4
		XMP_Uns8  reserved_1 [6];	//  8
		XMP_Uns16 dataRefIndex;		// 14
		XMP_Uns32 reserved_2;		// 16
		XMP_Uns32 flags;			// 20
		XMP_Uns32 timeScale;		// 24
		XMP_Uns32 frameDuration;	// 28
		XMP_Uns8  frameCount;		// 32
		XMP_Uns8  reserved_3;		// 33
		// Plus optional trailing ISO boxes.
	};								// 34

	struct Content_stsc_entry {
		XMP_Uns32 firstChunkNumber;	//  0
		XMP_Uns32 samplesPerChunk;	//  4
		XMP_Uns32 sampleDescrID;	//  8
	};								// 12

	#pragma pack( pop )

#if SUNOS_SPARC
	#pragma pack( )
#endif //#if SUNOS_SPARC

	// ---------------------------------------------------------------------------------------------

	MOOV_Manager() : fileMode(0)
	{
		XMP_Assert ( sizeof ( Content_mvhd_0 ) == 100 );	// Make sure the structs really are packed.
		XMP_Assert ( sizeof ( Content_mvhd_1 ) == 112 );
		XMP_Assert ( sizeof ( Content_hdlr ) == 24 );
		XMP_Assert ( sizeof ( Content_stsd_entry ) == 34 );
		XMP_Assert ( sizeof ( Content_stsc_entry ) == 12 );
	};

	virtual ~MOOV_Manager() {};

private:

	struct BoxNode;
	typedef std::vector<BoxNode> BoxList;
	typedef BoxList::iterator    BoxListPos;

	struct BoxNode {
		// ! Don't have a parent link, it will get destroyed by vector growth!

		XMP_Uns32 offset;		// The offset in the fullSubtree, 0 if not in the parse.
		XMP_Uns32 boxType;
		XMP_Uns32 headerSize;	// The actual header size in the fullSubtree, 0 if not in the parse.
		XMP_Uns32 contentSize;	// The current content size, does not include nested boxes.
		BoxList   children;
		RawDataBlock changedContent;	// Might be empty even if changed is true.
		bool changed;	// If true, the content is in changedContent, else in fullSubtree.

		BoxNode() : offset(0), boxType(0), headerSize(0), contentSize(0), changed(false) {};
		BoxNode ( XMP_Uns32 _offset, XMP_Uns32 _boxType, XMP_Uns32 _headerSize, XMP_Uns32 _contentSize )
			: offset(_offset), boxType(_boxType), headerSize(_headerSize), contentSize(_contentSize), changed(false) {};

	};

	XMP_Uns8 fileMode;
	BoxNode moovNode;

	void ParseNestedBoxes ( BoxNode * parentNode, const std::string & parentPath, bool ignoreMetaBoxes );

	XMP_Uns8 * PickContentPtr ( const BoxNode & node ) const;
	void FillBoxInfo ( const BoxNode & node, BoxInfo * info ) const;

	XMP_Uns32  NewSubtreeSize ( const BoxNode & node, const std::string & parentPath );
	XMP_Uns8 * AppendNewSubtree ( const BoxNode & node, const std::string & parentPath,
										 XMP_Uns8 * newPtr, XMP_Uns8 * newEnd );

};	// MOOV_Manager

#endif	// __MOOV_Support_hpp__
