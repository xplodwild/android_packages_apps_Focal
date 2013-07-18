// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/AIFF/AIFFMetadata.h"

using namespace IFF_RIFF;

//-----------------------------------------------------------------------------
// 
// AIFFMetadata::AIFFMetadata(...)
// 
// Purpose: ctor/dtor
// 
//-----------------------------------------------------------------------------

AIFFMetadata::AIFFMetadata()
{
}

AIFFMetadata::~AIFFMetadata()
{
}

//-----------------------------------------------------------------------------
// 
// AIFFMetadata::isEmptyValue(...)
// 
// Purpose: Is the value of the passed ValueObject and its id "empty"?
// 
//-----------------------------------------------------------------------------

bool AIFFMetadata::isEmptyValue( XMP_Uns32 id, ValueObject& valueObj )
{
	TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);

	return ( strObj == NULL || ( strObj != NULL && strObj->getValue().empty() ) );
}
