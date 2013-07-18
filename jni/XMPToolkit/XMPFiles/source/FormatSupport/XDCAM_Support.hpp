#ifndef __XDCAM_Support_hpp__
#define __XDCAM_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"
#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/ExpatAdapter.hpp"

// =================================================================================================
/// \file XDCAM_Support.hpp
/// \brief XMPFiles support for XDCAM streams.
///
// =================================================================================================

namespace XDCAM_Support 
{

	// Read XDCAM XML metadata from MEDIAPRO.XML and translate to appropriate XMP.
	bool GetMediaProLegacyMetadata ( SXMPMeta * xmpObjPtr,
									 const std::string&	umid,
									 const std::string& mediaProPath,
									 bool digestFound);

	// Read XDCAM XML metadata and translate to appropriate XMP.
	bool GetLegacyMetadata ( SXMPMeta *		xmpObjPtr,
							 XML_NodePtr	rootElem,
							 XMP_StringPtr	legacyNS,
							 bool			digestFound,
							 std::string&	umid );

	// Write XMP metadata back to XDCAM XML.
	bool SetLegacyMetadata ( XML_Node *		clipMetadata,
							 SXMPMeta *		xmpObj,
							 XMP_StringPtr	legacyNS );


} // namespace XDCAM_Support

// =================================================================================================

#endif	// __XDCAM_Support_hpp__
