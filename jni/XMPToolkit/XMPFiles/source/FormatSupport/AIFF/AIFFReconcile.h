// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _AIFFReconcile_h_
#define _AIFFReconcile_h_

#include "XMPFiles/source/NativeMetadataSupport/IReconcile.h"

namespace IFF_RIFF
{

class AIFFReconcile : public IReconcile
{
public:
	~AIFFReconcile() {};

	/** 
	* @see IReconcile::importToXMP
	* Legacy values are always imported.
	* If the values are not UTF-8 they will be converted to UTF-8 except in ServerMode
	*/
	XMP_Bool importToXMP( SXMPMeta& outXMP, const MetadataSet& inMetaData );

	/** 
	* @see IReconcile::exportFromXMP
	* XMP values are always exported to Legacy as UTF-8 encoded
	*/
	XMP_Bool exportFromXMP( MetadataSet& outMetaData, SXMPMeta& inXMP );

};

}

#endif
