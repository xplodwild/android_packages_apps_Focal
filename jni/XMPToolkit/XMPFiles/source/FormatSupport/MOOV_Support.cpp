// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/FormatSupport/MOOV_Support.hpp"

#include "XMPFiles/source/FormatSupport/ISOBaseMedia_Support.hpp"

#include <string.h>

// =================================================================================================
/// \file MOOV_Support.cpp
/// \brief XMPFiles support for the 'moov' box in MPEG-4 and QuickTime files.
// =================================================================================================

// =================================================================================================
// =================================================================================================
// MOOV_Manager - The parsing and reading routines are all commmon
// =================================================================================================
// =================================================================================================

#ifndef TraceParseMoovTree
	#define TraceParseMoovTree 0
#endif

#ifndef TraceUpdateMoovTree
	#define TraceUpdateMoovTree 0
#endif

// =================================================================================================
// MOOV_Manager::PickContentPtr
// ============================

XMP_Uns8 * MOOV_Manager::PickContentPtr ( const BoxNode & node ) const
{
	if ( node.contentSize == 0 ) {
		return 0;
	} else if ( node.changed ) {
		return (XMP_Uns8*) &node.changedContent[0];
	} else {
		return (XMP_Uns8*) &this->fullSubtree[0] + node.offset + node.headerSize;
	}
}	// MOOV_Manager::PickContentPtr

// =================================================================================================
// MOOV_Manager::FillBoxInfo
// =========================

void MOOV_Manager::FillBoxInfo ( const BoxNode & node, BoxInfo * info ) const
{
	if ( info == 0 ) return;

	info->boxType = node.boxType;
	info->childCount = (XMP_Uns32)node.children.size();
	info->contentSize = node.contentSize;
	info->content = PickContentPtr ( node );

}	// MOOV_Manager::FillBoxInfo

// =================================================================================================
// MOOV_Manager::GetBoxInfo
// =========================

void MOOV_Manager::GetBoxInfo ( const BoxRef ref, BoxInfo * info ) const
{

	this->FillBoxInfo ( *((BoxNode*)ref), info );

}	// MOOV_Manager::GetBoxInfo

// =================================================================================================
// MOOV_Manager::GetBox
// ====================
//
// Find a box given the type path. Pick the first child of each type.

MOOV_Manager::BoxRef MOOV_Manager::GetBox ( const char * boxPath, BoxInfo * info ) const
{
	size_t pathLen = strlen(boxPath);
	XMP_Assert ( (pathLen >= 4) && XMP_LitNMatch ( boxPath, "moov", 4 ) );
	if ( info != 0 ) memset ( info, 0, sizeof(BoxInfo) );
	
	const char * pathPtr = boxPath + 5;	// Skip the "moov/" portion.
	const char * pathEnd = boxPath + pathLen;
	
	BoxRef currRef = &this->moovNode;
	
	while ( pathPtr < pathEnd ) {
	
		XMP_Assert ( (pathEnd - pathPtr) >= 4 );		
		XMP_Uns32 boxType = GetUns32BE ( pathPtr );
		pathPtr += 5;	// ! Don't care that the last step goes 1 too far.
		
		currRef = this->GetTypeChild ( currRef, boxType, 0 );
		if ( currRef == 0 ) return 0;
	
	}

	this->FillBoxInfo ( *((BoxNode*)currRef), info );
	return currRef;
	
}	// MOOV_Manager::GetBox

// =================================================================================================
// MOOV_Manager::GetNthChild
// =========================

MOOV_Manager::BoxRef MOOV_Manager::GetNthChild ( BoxRef parentRef, size_t childIndex, BoxInfo * info ) const
{
	XMP_Assert ( parentRef != 0 );
	const BoxNode & parent = *((BoxNode*)parentRef);
	if ( info != 0 ) memset ( info, 0, sizeof(BoxInfo) );
	
	if ( childIndex >= parent.children.size() ) return 0;

	const BoxNode & currNode = parent.children[childIndex];

	this->FillBoxInfo ( currNode, info );
	return &currNode;

}	// MOOV_Manager::GetNthChild

// =================================================================================================
// MOOV_Manager::GetTypeChild
// ==========================

MOOV_Manager::BoxRef MOOV_Manager::GetTypeChild ( BoxRef parentRef, XMP_Uns32 childType, BoxInfo * info ) const
{
	XMP_Assert ( parentRef != 0 );
	const BoxNode & parent = *((BoxNode*)parentRef);
	if ( info != 0 ) memset ( info, 0, sizeof(BoxInfo) );
	if ( parent.children.empty() ) return 0;

	size_t i = 0, limit = parent.children.size();
	for ( ; i < limit; ++i ) {
		const BoxNode & currNode = parent.children[i];
		if ( currNode.boxType == childType ) {
			this->FillBoxInfo ( currNode, info );
			return &currNode;
		}
	}
	
	return 0;

}	// MOOV_Manager::GetTypeChild

// =================================================================================================
// MOOV_Manager::GetParsedOffset
// =============================

XMP_Uns32 MOOV_Manager::GetParsedOffset ( BoxRef ref ) const
{
	XMP_Assert ( ref != 0 );
	const BoxNode & node = *((BoxNode*)ref);
	
	if ( node.changed ) return 0;
	return node.offset;
	
}	// MOOV_Manager::GetParsedOffset

// =================================================================================================
// MOOV_Manager::GetHeaderSize
// ===========================

XMP_Uns32 MOOV_Manager::GetHeaderSize ( BoxRef ref ) const
{
	XMP_Assert ( ref != 0 );
	const BoxNode & node = *((BoxNode*)ref);
	
	if ( node.changed ) return 0;
	return node.headerSize;
	
}	// MOOV_Manager::GetHeaderSize

// =================================================================================================
// MOOV_Manager::ParseMemoryTree
// =============================
//
// Parse the fullSubtree data, building the BoxNode tree for the stuff that we care about. Tolerate
// errors like content ending too soon, make a best effoert to parse what we can.

void MOOV_Manager::ParseMemoryTree ( XMP_Uns8 fileMode )
{
	this->fileMode = fileMode;
	
	this->moovNode.offset = this->moovNode.boxType = 0;
	this->moovNode.headerSize = this->moovNode.contentSize = 0;
	this->moovNode.children.clear();
	this->moovNode.changedContent.clear();
	this->moovNode.changed = false;

	if ( this->fullSubtree.empty() ) return;
	
	ISOMedia::BoxInfo moovInfo;
	const XMP_Uns8 * moovOrigin = &this->fullSubtree[0];
	const XMP_Uns8 * moovLimit  = moovOrigin + this->fullSubtree.size();

	(void) ISOMedia::GetBoxInfo ( moovOrigin, moovLimit, &moovInfo );
	XMP_Enforce ( moovInfo.boxType == ISOMedia::k_moov );
	
	XMP_Uns64 fullMoovSize = moovInfo.headerSize + moovInfo.contentSize;
	if ( fullMoovSize > moovBoxSizeLimit ) {	// From here on we know 32-bit offsets are safe.
		XMP_Throw ( "Oversize 'moov' box", kXMPErr_EnforceFailure );
	}
	
	this->moovNode.boxType = ISOMedia::k_moov;
	this->moovNode.headerSize = moovInfo.headerSize;
	this->moovNode.contentSize = (XMP_Uns32)moovInfo.contentSize;

	bool ignoreMetaBoxes = (fileMode == kFileIsTraditionalQT);	// ! Don't want, these don't follow ISO spec.
	#if TraceParseMoovTree
		fprintf ( stderr, "Parsing 'moov' subtree, moovNode @ 0x%X, ignoreMetaBoxes = %d\n",
				  &this->moovNode, ignoreMetaBoxes );
	#endif
	this->ParseNestedBoxes ( &this->moovNode, "moov", ignoreMetaBoxes );
	
}	// MOOV_Manager::ParseMemoryTree

// =================================================================================================
// MOOV_Manager::ParseNestedBoxes
// ==============================
//
// Add the current level of child boxes to the parent node, recurse as appropriate.

void MOOV_Manager::ParseNestedBoxes ( BoxNode * parentNode, const std::string & parentPath, bool ignoreMetaBoxes )
{
	ISOMedia::BoxInfo isoInfo;
	const XMP_Uns8 * moovOrigin = &this->fullSubtree[0];
	const XMP_Uns8 * childOrigin = moovOrigin + parentNode->offset + parentNode->headerSize;
	const XMP_Uns8 * childLimit  = childOrigin + parentNode->contentSize;
	const XMP_Uns8 * nextChild;
	
	parentNode->contentSize = 0;	// Exclude nested box size.
	if ( parentNode->boxType == ISOMedia::k_meta ) {	// ! The 'meta' box is a FullBox.
		parentNode->contentSize = 4;
		childOrigin += 4;
	}

	for ( const XMP_Uns8 * currChild = childOrigin; currChild < childLimit; currChild = nextChild ) {
	
		nextChild = ISOMedia::GetBoxInfo ( currChild, childLimit, &isoInfo );
		if ( (isoInfo.boxType == 0) &&
			 (isoInfo.headerSize < 8) &&
			 (isoInfo.contentSize == 0) ) continue;	// Skip trailing padding that QT sometimes writes.
		
		XMP_Uns32 childOffset = (XMP_Uns32) (currChild - moovOrigin);
		parentNode->children.push_back ( BoxNode ( childOffset, isoInfo.boxType, isoInfo.headerSize, (XMP_Uns32)isoInfo.contentSize ) );
		BoxNode * newChild = &parentNode->children.back();
		
		#if TraceParseMoovTree
			size_t depth = (parentPath.size()+1) / 5;
			for ( size_t i = 0; i < depth; ++i ) fprintf ( stderr, "  " );
			XMP_Uns32 be32 = MakeUns32BE ( newChild->boxType );
			XMP_Uns32 addr32 = (XMP_Uns32) this->PickContentPtr ( *newChild );
			fprintf ( stderr, "  Parsed %s/%.4s, offset 0x%X, size %d, content @ 0x%X, BoxNode @ 0x%X\n",
					  parentPath.c_str(), &be32, newChild->offset, newChild->contentSize, addr32, newChild );
		#endif
		
		const char * pathSuffix = 0;	// Set to non-zero for boxes of interest.
		char buffer[6];	buffer[0] = 0;
		
			switch ( isoInfo.boxType ) {	// Want these boxes regardless of parent.
				case ISOMedia::k_udta : pathSuffix = "/udta"; break;
				case ISOMedia::k_meta : pathSuffix = "/meta"; break;
				case ISOMedia::k_ilst : pathSuffix = "/ilst"; break;
				case ISOMedia::k_trak : pathSuffix = "/trak"; break;
				case ISOMedia::k_edts : pathSuffix = "/edts"; break;
				case ISOMedia::k_mdia : pathSuffix = "/mdia"; break;
				case ISOMedia::k_minf : pathSuffix = "/minf"; break;
				case ISOMedia::k_dinf : pathSuffix = "/dinf"; break;
				case ISOMedia::k_stbl : pathSuffix = "/stbl"; break;
			}
		if ( pathSuffix != 0 ) {
			this->ParseNestedBoxes ( newChild, (parentPath + pathSuffix), ignoreMetaBoxes );
		}
	
	}

}	// MOOV_Manager::ParseNestedBoxes

// =================================================================================================
// MOOV_Manager::NoteChange
// ========================

void MOOV_Manager::NoteChange()
{

	this->moovNode.changed = true;

}	// MOOV_Manager::NoteChange

// =================================================================================================
// MOOV_Manager::SetBox
// ====================
//
// Save the new data, set this box's changed flag, and set the top changed flag.

void MOOV_Manager::SetBox ( BoxRef theBox, const void* dataPtr, XMP_Uns32 size )
{
	XMP_Enforce ( size < moovBoxSizeLimit );
	BoxNode * node = (BoxNode*)theBox;
	
	if ( node->contentSize == size ) {
	
		XMP_Uns8 * oldContent = PickContentPtr ( *node );
		if ( memcmp ( oldContent, dataPtr, size ) == 0 ) return;	// No change.
		memcpy ( oldContent, dataPtr, size );	// Update the old content in-place
		this->moovNode.changed = true;
		
		#if TraceUpdateMoovTree
			XMP_Uns32 be32 = MakeUns32BE ( node->boxType );
			fprintf ( stderr, "Updated '%.4s', parse offset 0x%X, same size\n", &be32, node->offset );
		#endif
	
	} else {
	
		node->changedContent.assign ( size, 0 );	// Fill with 0's first to get the storage.
		memcpy ( &node->changedContent[0], dataPtr, size );
		node->contentSize = size;
		node->changed = true;
		this->moovNode.changed = true;
		
		#if TraceUpdateMoovTree
			XMP_Uns32 be32 = MakeUns32BE ( node->boxType );
			XMP_Uns32 addr32 = (XMP_Uns32) this->PickContentPtr ( *node );
			fprintf ( stderr, "Updated '%.4s', parse offset 0x%X, new size %d, new content @ 0x%X\n",
					  &be32, node->offset, node->contentSize, addr32 );
		#endif
	
	}

}	// MOOV_Manager::SetBox

// =================================================================================================
// MOOV_Manager::SetBox
// ====================
//
// Like above, but create the path to the box if necessary.

void MOOV_Manager::SetBox ( const char * boxPath, const void* dataPtr, XMP_Uns32 size )
{
	XMP_Enforce ( size < moovBoxSizeLimit );

	size_t pathLen = strlen(boxPath);
	XMP_Assert ( (pathLen >= 4) && XMP_LitNMatch ( boxPath, "moov", 4 ) );
	
	const char * pathPtr = boxPath + 5;	// Skip the "moov/" portion.
	const char * pathEnd = boxPath + pathLen;
	
	BoxRef parentRef = 0;
	BoxRef currRef   = &this->moovNode;
	
	while ( pathPtr < pathEnd ) {
	
		XMP_Assert ( (pathEnd - pathPtr) >= 4 );		
		XMP_Uns32 boxType = GetUns32BE ( pathPtr );
		pathPtr += 5;	// ! Don't care that the last step goes 1 too far.
		
		parentRef = currRef;
		currRef = this->GetTypeChild ( parentRef, boxType, 0 );
		if ( currRef == 0 ) currRef = this->AddChildBox ( parentRef, boxType, 0, 0 );
	
	}

	this->SetBox ( currRef, dataPtr, size );
	
}	// MOOV_Manager::SetBox

// =================================================================================================
// MOOV_Manager::AddChildBox
// =========================

MOOV_Manager::BoxRef MOOV_Manager::AddChildBox ( BoxRef parentRef, XMP_Uns32 childType, const void* dataPtr, XMP_Uns32 size )
{
	BoxNode * parent = (BoxNode*)parentRef;
	XMP_Assert ( parent != 0 );
	
	parent->children.push_back ( BoxNode ( 0, childType, 0, 0 ) );
	BoxNode * newNode = &parent->children.back();
	this->SetBox ( newNode, dataPtr, size );
	
	return newNode;
	
}	// MOOV_Manager::AddChildBox

// =================================================================================================
// MOOV_Manager::DeleteNthChild
// ============================

bool MOOV_Manager::DeleteNthChild ( BoxRef parentRef, size_t childIndex )
{
	BoxNode * parent = (BoxNode*)parentRef;
	
	if ( childIndex >= parent->children.size() ) return false;

	parent->children.erase ( parent->children.begin() + childIndex );
	return true;
	
}	// MOOV_Manager::DeleteNthChild

// =================================================================================================
// MOOV_Manager::DeleteTypeChild
// =============================

bool MOOV_Manager::DeleteTypeChild ( BoxRef parentRef, XMP_Uns32 childType )
{
	BoxNode * parent = (BoxNode*)parentRef;
	
	BoxListPos child = parent->children.begin();
	BoxListPos limit = parent->children.end();
	
	for ( ; child != limit; ++child ) {
		if ( child->boxType == childType ) {
			parent->children.erase ( child );
			this->moovNode.changed = true;
			return true;
		}
	}
	
	return false;
	
}	// MOOV_Manager::DeleteTypeChild

// =================================================================================================
// MOOV_Manager::NewSubtreeSize
// ============================
//
// Determine the new (changed) size of a subtree. Ignore 'free' and 'wide' boxes.

XMP_Uns32 MOOV_Manager::NewSubtreeSize ( const BoxNode & node, const std::string & parentPath )
{
	XMP_Uns32 subtreeSize = 8 + node.contentSize;	// All boxes will have 8 byte headers.

	if ( (node.boxType == ISOMedia::k_free) || (node.boxType == ISOMedia::k_wide) ) {
	}

	for ( size_t i = 0, limit = node.children.size(); i < limit; ++i ) {

		char suffix[6];
		suffix[0] = '/';
		PutUns32BE ( node.boxType, &suffix[1] );
		suffix[5] = 0;
		std::string nodePath = parentPath + suffix;
		
		subtreeSize += this->NewSubtreeSize ( node.children[i], nodePath );
		XMP_Enforce ( subtreeSize < moovBoxSizeLimit );

	}

	return subtreeSize;
	
}	// MOOV_Manager::NewSubtreeSize

// =================================================================================================
// MOOV_Manager::AppendNewSubtree
// ==============================
//
// Append this node's header, content, and children. Because the 'meta' box is a FullBox with nested
// boxes, there can be both content and children. Ignore 'free' and 'wide' boxes.

#define IncrNewPtr(count)	{ newPtr += count; XMP_Enforce ( newPtr <= newEnd ); }

#if TraceUpdateMoovTree
	static XMP_Uns8 * newOrigin;
#endif

XMP_Uns8 * MOOV_Manager::AppendNewSubtree ( const BoxNode & node, const std::string & parentPath,
											XMP_Uns8 * newPtr, XMP_Uns8 * newEnd )
{
	if ( (node.boxType == ISOMedia::k_free) || (node.boxType == ISOMedia::k_wide) ) {
	}
	
	XMP_Assert ( (node.boxType != ISOMedia::k_meta) ? (node.children.empty() || (node.contentSize == 0)) :
													  (node.children.empty() || (node.contentSize == 4)) );
	
	XMP_Enforce ( (XMP_Uns32)(newEnd - newPtr) >= (8 + node.contentSize) );

	#if TraceUpdateMoovTree
		XMP_Uns32 be32 = MakeUns32BE ( node.boxType );
		XMP_Uns32 newOffset = (XMP_Uns32) (newPtr - newOrigin);
		XMP_Uns32 addr32 = (XMP_Uns32) this->PickContentPtr ( node );
		fprintf ( stderr, "  Appending %s/%.4s @ 0x%X, size %d, content @ 0x%X\n",
				  parentPath.c_str(), &be32, newOffset, node.contentSize, addr32 );
	#endif

	// Leave the size as 0 for now, append the type and content.
	
	XMP_Uns8 * boxOrigin = newPtr;	// Save origin to fill in the final size.
	PutUns32BE ( node.boxType, (newPtr + 4) );
	IncrNewPtr ( 8 );
	
	if ( node.contentSize != 0 ) {
		const XMP_Uns8 * content = PickContentPtr( node );
		memcpy ( newPtr, content, node.contentSize );
		IncrNewPtr ( node.contentSize );
	}
	
	// Append the nested boxes.
	
	if ( ! node.children.empty() ) {

		char suffix[6];
		suffix[0] = '/';
		PutUns32BE ( node.boxType, &suffix[1] );
		suffix[5] = 0;
		std::string nodePath = parentPath + suffix;
		
		for ( size_t i = 0, limit = node.children.size(); i < limit; ++i ) {
			newPtr = this->AppendNewSubtree ( node.children[i], nodePath, newPtr, newEnd );
		}

	}
	
	// Fill in the final size.
	
	PutUns32BE ( (XMP_Uns32)(newPtr - boxOrigin), boxOrigin );
	
	return newPtr;
	
}	// MOOV_Manager::AppendNewSubtree

// =================================================================================================
// MOOV_Manager::UpdateMemoryTree
// ==============================

void MOOV_Manager::UpdateMemoryTree()
{
	if ( ! this->IsChanged() ) return;
	
	XMP_Uns32 newSize = this->NewSubtreeSize ( this->moovNode, "" );
	XMP_Enforce ( newSize < moovBoxSizeLimit );
	
	RawDataBlock newData;
	newData.assign ( newSize, 0 );	// Prefill with zeroes, can't append multiple items to a vector.
	
	XMP_Uns8 * newPtr = &newData[0];
	XMP_Uns8 * newEnd = newPtr + newSize;

	#if TraceUpdateMoovTree
		fprintf ( stderr, "Starting MOOV_Manager::UpdateMemoryTree\n" );
		newOrigin = newPtr;
	#endif
	
	XMP_Uns8 * trueEnd = this->AppendNewSubtree ( this->moovNode, "", newPtr, newEnd );
	XMP_Enforce ( trueEnd == newEnd );
	
	this->fullSubtree.swap ( newData );
	this->ParseMemoryTree ( this->fileMode );
	
}	// MOOV_Manager::UpdateMemoryTree
