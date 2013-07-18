// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/Trivial_Handler.hpp"

using namespace std;

// =================================================================================================
/// \file Trivial_Handler.cpp
/// \brief Base class for trivial handlers that only process in-place XMP.
///
/// This header ...
///
// =================================================================================================

// =================================================================================================
// Trivial_MetaHandler::~Trivial_MetaHandler
// =========================================

Trivial_MetaHandler::~Trivial_MetaHandler()
{
	// Nothing to do.

}	// Trivial_MetaHandler::~Trivial_MetaHandler

// =================================================================================================
// Trivial_MetaHandler::UpdateFile
// ===============================

void Trivial_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	IgnoreParam ( doSafeUpdate );
	XMP_Assert ( ! doSafeUpdate );	// Not supported at this level.
	if ( ! this->needsUpdate ) return;

	XMP_IO*      fileRef    = this->parent->ioRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;
	std::string &    xmpPacket  = this->xmpPacket;

	fileRef->Seek ( packetInfo.offset, kXMP_SeekFromStart );
	fileRef->Write ( xmpPacket.c_str(), packetInfo.length );
	XMP_Assert ( xmpPacket.size() == (size_t)packetInfo.length );

	this->needsUpdate = false;

}	// Trivial_MetaHandler::UpdateFile

// =================================================================================================
// Trivial_MetaHandler::WriteTempFile
// ==================================

void Trivial_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	IgnoreParam ( tempRef );

	XMP_Throw ( "Trivial_MetaHandler::WriteTempFile: Not supported", kXMPErr_Unavailable );

}	// Trivial_MetaHandler::WriteTempFile

// =================================================================================================
