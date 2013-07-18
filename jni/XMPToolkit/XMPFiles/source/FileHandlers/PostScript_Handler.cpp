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
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/PostScript_Handler.hpp"

#include "XMPFiles/source/FormatSupport/XMPScanner.hpp"
#include "XMPFiles/source/FileHandlers/Scanner_Handler.hpp"

#include <algorithm>

using namespace std;
// =================================================================================================
/// \file PostScript_Handler.cpp
/// \brief File format handler for PostScript and EPS files.
///
// =================================================================================================

// =================================================================================================
// PostScript_MetaHandlerCTor
// ==========================

XMPFileHandler * PostScript_MetaHandlerCTor ( XMPFiles * parent )
{
	XMPFileHandler * newHandler = new PostScript_MetaHandler ( parent );

	return newHandler;

}	// PostScript_MetaHandlerCTor

// =================================================================================================
// PostScript_CheckFormat
// ======================

bool PostScript_CheckFormat ( XMP_FileFormat format,
	                          XMP_StringPtr  filePath,
	                          XMP_IO*    fileRef,
	                          XMPFiles *     parent )
{
	IgnoreParam(filePath); IgnoreParam(parent);
	XMP_Assert ( (format == kXMP_EPSFile) || (format == kXMP_PostScriptFile) );

	return PostScript_Support::IsValidPSFile(fileRef,format) ;

}	// PostScript_CheckFormat

// =================================================================================================
// PostScript_MetaHandler::PostScript_MetaHandler
// ==============================================

PostScript_MetaHandler::PostScript_MetaHandler ( XMPFiles * _parent ):dscFlags(0),docInfoFlags(0)
	,containsXMPHint(false),fileformat(kXMP_UnknownFile)
{
	this->parent = _parent;
	this->handlerFlags = kPostScript_HandlerFlags;
	this->stdCharForm = kXMP_Char8Bit;
	this->psHint = kPSHint_NoMarker;

}	// PostScript_MetaHandler::PostScript_MetaHandler

// =================================================================================================
// PostScript_MetaHandler::~PostScript_MetaHandler
// ===============================================

PostScript_MetaHandler::~PostScript_MetaHandler()
{
	// ! Inherit the base cleanup.

}	// PostScript_MetaHandler::~PostScript_MetaHandler

// =================================================================================================
// PostScript_MetaHandler::FindPostScriptHint
// ==========================================
//
// Search for "%ADO_ContainsXMP:" at the beginning of a line, it must be before "%%EndComments". If
// the XMP marker is found, look for the MainFirst/MainLast/NoMain options.

int PostScript_MetaHandler::FindPostScriptHint()
{
	bool     found = false;
	IOBuffer ioBuf;
	XMP_Uns8 ch;

	XMP_IO* fileRef = this->parent->ioRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	// Check for the binary EPSF preview header.

	fileRef->Rewind();
	if ( ! CheckFileSpace ( fileRef, &ioBuf, 4 ) ) return false;
	XMP_Uns32 fileheader = GetUns32BE ( ioBuf.ptr );

	if ( fileheader == 0xC5D0D3C6 ) {

		if ( ! CheckFileSpace ( fileRef, &ioBuf, 30 ) ) return false;

		XMP_Uns32 psOffset = GetUns32LE ( ioBuf.ptr+4 );	// PostScript offset.
		XMP_Uns32 psLength = GetUns32LE ( ioBuf.ptr+8 );	// PostScript length.

		MoveToOffset ( fileRef, psOffset, &ioBuf );

	}

	// Look for the ContainsXMP comment.

	while ( true ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "PostScript_MetaHandler::FindPostScriptHint - User abort", kXMPErr_UserAbort );
		}

		if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsXMPString.length() ) ) return kPSHint_NoMarker;

		if ( CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSEndCommentString.c_str()), kPSEndCommentString.length() ) ) {

			// Found "%%EndComments", don't look any further.
			return kPSHint_NoMarker;

		} else if ( ! CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsXMPString.c_str()), kPSContainsXMPString.length() ) ) {

			// Not "%%EndComments" or "%ADO_ContainsXMP:", skip past the end of this line.
			do {
				if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return kPSHint_NoMarker;
				ch = *ioBuf.ptr;
				++ioBuf.ptr;
			} while ( ! IsNewline ( ch ) );

		} else {

			// Found "%ADO_ContainsXMP:", look for the main packet location option.

			ioBuf.ptr += kPSContainsXMPString.length();
			int xmpHint = kPSHint_NoMain;	// ! From here on, a failure means "no main", not "no marker".
			if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return kPSHint_NoMain;
			if ( ! IsSpaceOrTab ( *ioBuf.ptr ) ) return kPSHint_NoMain;

			while ( true ) {

				while ( true ) {	// Skip leading spaces and tabs.
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return kPSHint_NoMain;
					if ( ! IsSpaceOrTab ( *ioBuf.ptr ) ) break;
					++ioBuf.ptr;
				}
				if ( IsNewline ( *ioBuf.ptr ) ) return kPSHint_NoMain;	// Reached the end of the ContainsXMP comment.

				if ( ! CheckFileSpace ( fileRef, &ioBuf, 6 ) ) return kPSHint_NoMain;

				if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("NoMain"), 6 ) ) {

					ioBuf.ptr += 6;
					xmpHint = kPSHint_NoMain;
					break;

				} else if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("MainFi"), 6 ) ) {

					ioBuf.ptr += 6;
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 3 ) ) return kPSHint_NoMain;
					if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("rst"), 3 ) ) {
						ioBuf.ptr += 3;
						xmpHint = kPSHint_MainFirst;
					}
					break;

				} else if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("MainLa"), 6 ) ) {

					ioBuf.ptr += 6;
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 2 ) ) return kPSHint_NoMain;
					if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("st"), 2 ) ) {
						ioBuf.ptr += 2;
						xmpHint = kPSHint_MainLast;
					}
					break;

				} else {

					while ( true ) {	// Skip until whitespace.
						if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return kPSHint_NoMain;
						if ( IsWhitespace ( *ioBuf.ptr ) ) break;
						++ioBuf.ptr;
					}

				}

			}	// Look for the main packet location option.

			// Make sure we found exactly a known option.
			if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return kPSHint_NoMain;
			if ( ! IsWhitespace ( *ioBuf.ptr ) ) return kPSHint_NoMain;
			return xmpHint;

		}	// Found "%ADO_ContainsXMP:".

	}	// Outer marker loop.

	return kPSHint_NoMarker;	// Should never reach here.

}	// PostScript_MetaHandler::FindPostScriptHint


// =================================================================================================
// PostScript_MetaHandler::FindFirstPacket
// =======================================
//
// Run the packet scanner until we find a valid packet. The first one is the main. For simplicity,
// the state of all snips is checked after each buffer is read. In theory only the last of the
// previous snips might change from partial to valid, but then we would have to special case the
// first pass when there is no previous set of snips. Since we have to get a full report to look at
// the last snip anyway, it costs virtually nothing extra to recheck all of the snips.

bool PostScript_MetaHandler::FindFirstPacket()
{
	int		snipCount;
	bool 	found	= false;
	size_t	bufPos, bufLen;

	XMP_IO* fileRef = this->parent->ioRef;
	XMP_Int64   fileLen = fileRef->Length();
	XMP_PacketInfo & packetInfo = this->packetInfo;

	XMPScanner scanner ( fileLen );
	XMPScanner::SnipInfoVector snips;

	enum { kBufferSize = 64*1024 };
	XMP_Uns8	buffer [kBufferSize];

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	bufPos = 0;
	bufLen = 0;

	fileRef->Rewind();	// Seek back to the beginning of the file.
	bool firstfound=false;

	while ( true ) 
	{

		if ( checkAbort && abortProc(abortArg) ) 
		{
			XMP_Throw ( "PostScript_MetaHandler::FindFirstPacket - User abort", kXMPErr_UserAbort );
		}

		bufPos += bufLen;
		bufLen = fileRef->Read ( buffer, kBufferSize );
		if ( bufLen == 0 ) return firstfound;	// Must be at EoF, no packets found.

		scanner.Scan ( buffer, bufPos, bufLen );
		snipCount = scanner.GetSnipCount();
		scanner.Report ( snips );
		for ( int i = 0; i < snipCount; ++i ) 
		{
			if ( snips[i].fState == XMPScanner::eValidPacketSnip ) 
			{
				if (!firstfound)
				{
					if ( snips[i].fLength > 0x7FFFFFFF ) XMP_Throw ( "PostScript_MetaHandler::FindFirstPacket: Oversize packet", kXMPErr_BadXMP );
					packetInfo.offset = snips[i].fOffset;
					packetInfo.length = (XMP_Int32)snips[i].fLength;
					packetInfo.charForm  = snips[i].fCharForm;
					packetInfo.writeable = (snips[i].fAccess == 'w');
					firstPacketInfo=packetInfo;
					lastPacketInfo=packetInfo;
					firstfound=true;
				}
				else
				{					
					lastPacketInfo.offset = snips[i].fOffset;
					lastPacketInfo.length = (XMP_Int32)snips[i].fLength;
					lastPacketInfo.charForm  = snips[i].fCharForm;
					lastPacketInfo.writeable = (snips[i].fAccess == 'w');
				}
			}
		}

	}
	
	return firstfound;

}	// FindFirstPacket


// =================================================================================================
// PostScript_MetaHandler::FindLastPacket
// ======================================
//
// Run the packet scanner backwards until we find the start of a packet, or a valid packet. If we
// found a packet start, resume forward scanning to see if it is a valid packet. For simplicity, all
// of the snips are checked on each pass, for much the same reasons as in FindFirstPacket.

bool PostScript_MetaHandler::FindLastPacket()
{
	size_t	bufPos, bufLen;

	XMP_IO* fileRef = this->parent->ioRef;
	XMP_Int64   fileLen = fileRef->Length();
	XMP_PacketInfo & packetInfo = this->packetInfo;

	// ------------------------------------------------------
	// Scan the entire file to find all of the valid packets.

	XMPScanner	scanner ( fileLen );

	enum { kBufferSize = 64*1024 };
	XMP_Uns8	buffer [kBufferSize];

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	fileRef->Rewind();	// Seek back to the beginning of the file.

	for ( bufPos = 0; bufPos < (size_t)fileLen; bufPos += bufLen )
	{
		if ( checkAbort && abortProc(abortArg) ) 
		{
			XMP_Throw ( "PostScript_MetaHandler::FindLastPacket - User abort", kXMPErr_UserAbort );
		}
		bufLen = fileRef->Read ( buffer, kBufferSize );
		if ( bufLen == 0 ) XMP_Throw ( "PostScript_MetaHandler::FindLastPacket: Read failure", kXMPErr_ExternalFailure );
		scanner.Scan ( buffer, bufPos, bufLen );
	}

	// -------------------------------
	// Pick the last the valid packet.

	int snipCount = scanner.GetSnipCount();

	XMPScanner::SnipInfoVector snips ( snipCount );
	scanner.Report ( snips );

	bool lastfound=false;
	for ( int i = 0; i < snipCount; ++i ) 
	{
		if ( snips[i].fState == XMPScanner::eValidPacketSnip ) 
		{
			if (!lastfound)
			{
				if ( snips[i].fLength > 0x7FFFFFFF ) XMP_Throw ( "PostScript_MetaHandler::FindLastPacket: Oversize packet", kXMPErr_BadXMP );
				packetInfo.offset = snips[i].fOffset;
				packetInfo.length = (XMP_Int32)snips[i].fLength;
				packetInfo.charForm  = snips[i].fCharForm;
				packetInfo.writeable = (snips[i].fAccess == 'w');
				firstPacketInfo=packetInfo;
				lastPacketInfo=packetInfo;
				lastfound=true;
			}
			else
			{					
				lastPacketInfo.offset = snips[i].fOffset;
				lastPacketInfo.length = (XMP_Int32)snips[i].fLength;
				lastPacketInfo.charForm  = snips[i].fCharForm;
				lastPacketInfo.writeable = (snips[i].fAccess == 'w');
				packetInfo=lastPacketInfo;
			}
		}
	}
	return lastfound;

}	// PostScript_MetaHandler::FindLastPacket

// =================================================================================================
// PostScript_MetaHandler::setTokenInfo
// ====================================
//
// Function Sets the docInfo flag for tokens(PostScript DSC comments/ Docinfo Dictionary values)
//	when parsing the file stream.Also takes note of the token offset from the start of the file
//	and the length of the token
void PostScript_MetaHandler::setTokenInfo(TokenFlag tFlag,XMP_Int64 offset,XMP_Int64 length)
{
	if (!(docInfoFlags&tFlag)&&tFlag>=kPS_ADOContainsXMP && tFlag<=kPS_EndPostScript)
	{
		size_t index=0;
		XMP_Uns64 flag=tFlag;
		while(flag>>=1) index++;
		fileTokenInfo[index].offsetStart=offset;
		fileTokenInfo[index].tokenlen=length;
		docInfoFlags|=tFlag;
	}
}

// =================================================================================================
// PostScript_MetaHandler::getTokenInfo
// ====================================
//
// Function returns the token info for the flag, which was collected in parsing the input file
//	
PostScript_MetaHandler::TokenLocation& PostScript_MetaHandler::getTokenInfo(TokenFlag tFlag)
{
	if ((docInfoFlags&tFlag)&&tFlag>=kPS_ADOContainsXMP && tFlag<=kPS_EndPostScript)
	{
		size_t index=0;
		XMP_Uns64 flag=tFlag;
		while(flag>>=1) index++;
		return fileTokenInfo[index];
	}
	return fileTokenInfo[kPS_NoData];
}

// =================================================================================================
// PostScript_MetaHandler::ExtractDSCCommentValue
// ==============================================
//
// Function extracts the DSC comment value when parsing the file.This may be later used to reconcile
//	
bool PostScript_MetaHandler::ExtractDSCCommentValue(IOBuffer &ioBuf,NativeMetadataIndex index)
{
	
	XMP_IO* fileRef = this->parent->ioRef;
	if ( ! PostScript_Support::SkipTabsAndSpaces(fileRef,ioBuf) ) return false;
	if ( !IsNewline ( *ioBuf.ptr ) )
	{
		do 
		{
			if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			nativeMeta[index] += *ioBuf.ptr;
			++ioBuf.ptr;
		} while ( ! IsNewline ( *ioBuf.ptr) );
		if (!PostScript_Support::HasCodesGT127(nativeMeta[index]))
		{
			dscFlags|=nativeIndextoFlag[index];
		}
		else
			nativeMeta[index].clear();
	}
	return true;
}


// =================================================================================================
// PostScript_MetaHandler::ExtractContainsXMPHint
// ==============================================
//
// Function extracts the the value of "ADOContainsXMP:" DSC comment's value
//	
bool PostScript_MetaHandler::ExtractContainsXMPHint(IOBuffer &ioBuf,XMP_Int64 containsXMPStartpos)
{
	XMP_IO* fileRef = this->parent->ioRef;
	int xmpHint = kPSHint_NoMain;	// ! From here on, a failure means "no main", not "no marker".
	//checkfor atleast one whitespace
	if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
	if ( ! IsSpaceOrTab ( *ioBuf.ptr ) ) return false;
	//skip extra ones	
	if ( ! PostScript_Support::SkipTabsAndSpaces(fileRef,ioBuf) ) return false;
	if ( IsNewline ( *ioBuf.ptr ) ) return false;	// Reached the end of the ContainsXMP comment.

	if ( ! CheckFileSpace ( fileRef, &ioBuf, 6 ) ) return false ;

	if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("NoMain"), 6 ) ) 
	{
		ioBuf.ptr += 6;
		if ( ! PostScript_Support::SkipTabsAndSpaces(fileRef,ioBuf) ) return false;
		if ( ! IsNewline( *ioBuf.ptr) ) return false;
		this->psHint = kPSHint_NoMain;
		setTokenInfo(kPS_ADOContainsXMP,containsXMPStartpos,ioBuf.filePos+ioBuf.ptr-ioBuf.data-containsXMPStartpos);

	} 
	else if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("MainFi"), 6 ) ) 
	{
		ioBuf.ptr += 6;
		if ( ! CheckFileSpace ( fileRef, &ioBuf, 3 ) ) return false;
		if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("rst"), 3 ) ) 
		{
			ioBuf.ptr += 3;
			if ( ! PostScript_Support::SkipTabsAndSpaces(fileRef,ioBuf) ) return false;
			if ( ! IsNewline( *ioBuf.ptr) ) return false;
			this->psHint = kPSHint_MainFirst;
			setTokenInfo(kPS_ADOContainsXMP,containsXMPStartpos,ioBuf.filePos+ioBuf.ptr-ioBuf.data-containsXMPStartpos);
			containsXMPHint=true;
		}
	} 
	else if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("MainLa"), 6 ) ) 
	{
		ioBuf.ptr += 6;
		if ( ! CheckFileSpace ( fileRef, &ioBuf, 2 ) ) return false;
		if ( CheckBytes ( ioBuf.ptr, Uns8Ptr("st"), 2 ) ) {
			ioBuf.ptr += 2;
			if ( ! PostScript_Support::SkipTabsAndSpaces(fileRef,ioBuf) ) return false;
			if ( ! IsNewline( *ioBuf.ptr) ) return false;
			this->psHint = kPSHint_MainLast;
			setTokenInfo(kPS_ADOContainsXMP,containsXMPStartpos,ioBuf.filePos+ioBuf.ptr-ioBuf.data-containsXMPStartpos);
			containsXMPHint=true;
		}
	} 
	else 
	{
		if ( ! PostScript_Support::SkipUntilNewline(fileRef,ioBuf) ) return false;
	}
	return true;
}


// =================================================================================================
// PostScript_MetaHandler::ExtractDocInfoDict
// ==============================================
//
// Function extracts the the value of DocInfo Dictionary.The keys that are looked in the dictionary
//	are Creator, CreationDate, ModDate, Author, Title, Subject and Keywords.Other keys for the 
//	Dictionary are ignored
bool PostScript_MetaHandler::ExtractDocInfoDict(IOBuffer &ioBuf)
{
	XMP_Uns8 ch;
	XMP_IO* fileRef = this->parent->ioRef;
	XMP_Int64 endofDocInfo=(ioBuf.ptr-ioBuf.data)+ioBuf.filePos;
	if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
	if ( IsWhitespace (*ioBuf.ptr))
	{
		//skip the whitespaces
		if ( ! ( PostScript_Support::SkipTabsAndSpaces(fileRef, ioBuf))) return false;
		//check the pdfmark
		if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsPdfmarkString.length() ) ) return false;
		if ( ! CheckBytes ( ioBuf.ptr,  Uns8Ptr(kPSContainsPdfmarkString.c_str()), kPSContainsPdfmarkString.length() ) ) return false;
		//reverse the direction to collect data
		while(true)
		{
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			ch=*ioBuf.ptr;
			--ioBuf.ptr;
			if (ch=='/') break;//slash of /DOCINFO
		} 
		//skip white spaces
		while(true)
		{
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if (!IsWhitespace(*ioBuf.ptr)) break;
			--ioBuf.ptr;
		} 
						
		bool findingkey=false;
		std::string key, value;
		while(true)
		{
			XMP_Uns32 noOfMarks=0;
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if (*ioBuf.ptr==')')
			{
				--ioBuf.ptr;
				while(true)
				{
					//get the string till '('
					if (*ioBuf.ptr=='(')
					{
						if(findingkey)
						{
							reverse(key.begin(), key.end());
							reverse(value.begin(), value.end());
							RegisterKeyValue(key,value);
						}
						if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
						--ioBuf.ptr;
						findingkey=!findingkey;
						break;
					}
					else
					{
						if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
						if (findingkey)
							key+=*ioBuf.ptr;
						else
							value+=*ioBuf.ptr;
						--ioBuf.ptr;
					}
				}
			}
			else if(*ioBuf.ptr=='[')
			{
				//end of Doc Info parsing
				//return;
				break;
			}
			else
			{
				while(true)
				{
					if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
					if (findingkey)
						key+=*ioBuf.ptr;
					else
						value+=*ioBuf.ptr;
					--ioBuf.ptr;
					if (*ioBuf.ptr=='/')
					{
						if(findingkey)
						{
							reverse(key.begin(), key.end());
							reverse(value.begin(), value.end());
							RegisterKeyValue(key,value);
						}
						if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
						--ioBuf.ptr;
						findingkey=!findingkey;
						break;
					}
					else if(IsWhitespace(*ioBuf.ptr))
					{
						//something not expected in Doc Info
						break;
					}
				}
			}
			while(true)
			{
				if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
				if (!IsWhitespace(*ioBuf.ptr)) break;
				--ioBuf.ptr;
			} 
						
		}
						
		fileRef->Rewind();	
		FillBuffer (fileRef, endofDocInfo, &ioBuf );
		return true;
	}//white space not after DOCINFO
	return false;
}

// =================================================================================================
// PostScript_MetaHandler::ParsePSFile
// ===================================
//
// Main parser for the Post script file.This is where all the DSC comments , Docinfo key value pairs 
// and other insertion related Data is looked for and stored
void PostScript_MetaHandler::ParsePSFile()
{
	bool     found = false;
	IOBuffer ioBuf;

	XMP_IO* fileRef = this->parent->ioRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	//Determine the file type PS or EPS
	if ( ! PostScript_Support::IsValidPSFile(fileRef,this->fileformat) ) return ; 
	// Check for the binary EPSF preview header.

	fileRef->Rewind();
	if ( ! CheckFileSpace ( fileRef, &ioBuf, 4 ) ) return ;
	XMP_Uns32 fileheader = GetUns32BE ( ioBuf.ptr );

	if ( fileheader == 0xC5D0D3C6 ) 
	{

		if ( ! CheckFileSpace ( fileRef, &ioBuf, 30 ) ) return ;

		XMP_Uns32 psOffset = GetUns32LE ( ioBuf.ptr+4 );	// PostScript offset.
		XMP_Uns32 psLength = GetUns32LE ( ioBuf.ptr+8 );	// PostScript length.

		setTokenInfo(kPS_EndPostScript,psOffset+psLength,0);
		MoveToOffset ( fileRef, psOffset, &ioBuf );

	}

	while ( true ) 
	{
		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "PostScript_MetaHandler::FindPostScriptHint - User abort", kXMPErr_UserAbort );
		}

		if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsForString.length() ) ) return ;

		if ( (CheckFileSpace ( fileRef, &ioBuf, kPSEndCommentString.length() )&& 
				CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSEndCommentString.c_str()), kPSEndCommentString.length() )
				)|| *ioBuf.ptr!='%' || !(*(ioBuf.ptr+1)>32 && *(ioBuf.ptr+1)<=126 )) // implicit endcomment check
		{
			if (CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSEndCommentString.c_str()), kPSEndCommentString.length() ))
			{
				setTokenInfo(kPS_EndComments,ioBuf.filePos+ioBuf.ptr-ioBuf.data,kPSEndCommentString.length());
				ioBuf.ptr+=kPSEndCommentString.length();
			}
			else
			{
				setTokenInfo(kPS_EndComments,ioBuf.filePos+ioBuf.ptr-ioBuf.data,0);
			}
			// Found "%%EndComments", look for docInfo Dictionary
			// skip past the end of this line.
			while(true)
			{
				if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return ;
				if (! IsWhitespace (*ioBuf.ptr)) break;
				++ioBuf.ptr;
			} 
			// search for /DOCINFO
			while(true)
			{
				if ( ! CheckFileSpace ( fileRef, &ioBuf, 5 ) ) return ;
				if (CheckBytes ( ioBuf.ptr, Uns8Ptr("/DOCI"), 5 ) 
					&& CheckFileSpace ( fileRef, &ioBuf, kPSContainsDocInfoString.length() )
					&&CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsDocInfoString.c_str()), kPSContainsDocInfoString.length() ))
					
				{
					
					ioBuf.ptr+=kPSContainsDocInfoString.length();
					ExtractDocInfoDict(ioBuf);
				}// DOCINFO Not found in document
				else if(CheckBytes ( ioBuf.ptr, Uns8Ptr("%%Beg"), 5 ))
				{//possibly one of %%BeginProlog %%BeginSetup %%BeginBinary %%BeginData 
					// %%BeginDocument %%BeginPageSetup
					XMP_Int64 begStartpos=ioBuf.filePos+ioBuf.ptr-ioBuf.data;
					ioBuf.ptr+=5;
					if (!CheckFileSpace ( fileRef, &ioBuf, 6 )) return;
					if(CheckBytes ( ioBuf.ptr, Uns8Ptr("inProl"), 6 ))
					{//%%BeginProlog
						ioBuf.ptr+=6;
						if (!CheckFileSpace ( fileRef, &ioBuf, 2 ))return;
						if(CheckBytes ( ioBuf.ptr, Uns8Ptr("og"), 2 ))
						{
							ioBuf.ptr+=2;
							setTokenInfo(kPS_BeginProlog,begStartpos,13);
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("inSetu"), 6 ))
					{//%%BeginSetup 
						ioBuf.ptr+=6;
						if (!CheckFileSpace ( fileRef, &ioBuf, 1 ))return;
						if(CheckBytes ( ioBuf.ptr, Uns8Ptr("p"), 1 ))
						{
							ioBuf.ptr+=1;
							setTokenInfo(kPS_BeginSetup,begStartpos,12);
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("inBina"), 6 ))
					{//%%BeginBinary
						ioBuf.ptr+=6;
						if (!CheckFileSpace ( fileRef, &ioBuf, 3 ))return;
						if(CheckBytes ( ioBuf.ptr, Uns8Ptr("ry"), 3 ))
						{
							ioBuf.ptr+=3;
							//ignore till %%EndBinary
							while(true)
							{
								if (!CheckFileSpace ( fileRef, &ioBuf, 12 ))return;
								if (CheckBytes ( ioBuf.ptr, Uns8Ptr("%%EndBinary"), 11 ))
								{
									ioBuf.ptr+=11;
									if (IsWhitespace(*ioBuf.ptr))
									{
										ioBuf.ptr++;
										break;
									}
								}
								++ioBuf.ptr;
							}
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("inData"), 6 ))
					{//%%BeginData
						ioBuf.ptr+=6;
						if (!CheckFileSpace ( fileRef, &ioBuf, 1 ))return;
						if(CheckBytes ( ioBuf.ptr, Uns8Ptr(":"), 1 ))
						{
							//ignore till %%EndData
							while(true)
							{
								if (!CheckFileSpace ( fileRef, &ioBuf, 10 ))return;
								if (CheckBytes ( ioBuf.ptr, Uns8Ptr("%%EndData"), 9 ))
								{
									ioBuf.ptr+=9;
									if (IsWhitespace(*ioBuf.ptr))
									{
										ioBuf.ptr++;
										break;
									}
								}
								++ioBuf.ptr;
							}
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("inDocu"), 6 ))
					{// %%BeginDocument
						ioBuf.ptr+=6;
						if (!CheckFileSpace ( fileRef, &ioBuf, 5 ))return;
						if(CheckBytes ( ioBuf.ptr, Uns8Ptr("ment:"), 5 ))
						{
							ioBuf.ptr+=5;
							//ignore till %%EndDocument
							while(true)
							{
								if (!CheckFileSpace ( fileRef, &ioBuf, 14 ))return;
								if (CheckBytes ( ioBuf.ptr, Uns8Ptr("%%EndDocument"), 13 ))
								{
									ioBuf.ptr+=13;
									if (IsWhitespace(*ioBuf.ptr))
									{
										ioBuf.ptr++;
										break;
									}
								}
								++ioBuf.ptr;
							}
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("inPage"), 6 ))
					{// %%BeginPageSetup
						ioBuf.ptr+=6;
						if (!CheckFileSpace ( fileRef, &ioBuf, 5 ))return;
						if(CheckBytes ( ioBuf.ptr, Uns8Ptr("Setup"), 5 ))
						{
							ioBuf.ptr+=5;
							setTokenInfo(kPS_BeginPageSetup,begStartpos,16);
						}
					}
				}
				else if(CheckBytes ( ioBuf.ptr, Uns8Ptr("%%End"), 5 ))
				{//possibly %%EndProlog %%EndSetup %%EndPageSetup %%EndPageComments
					XMP_Int64 begStartpos=ioBuf.filePos+ioBuf.ptr-ioBuf.data;
					ioBuf.ptr+=5;
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 5 ) ) return ;
					if (CheckBytes ( ioBuf.ptr, Uns8Ptr("Prolo"), 5 ))
					{// %%EndProlog
						ioBuf.ptr+=5;
						if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return ;
						if (CheckBytes ( ioBuf.ptr, Uns8Ptr("g"), 1 ))
						{
							ioBuf.ptr+=1;
							setTokenInfo(kPS_EndProlog,begStartpos,11);
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("Setup"), 5 ))
					{//%%EndSetup
						ioBuf.ptr+=5;
						setTokenInfo(kPS_EndSetup,begStartpos,10);
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("PageS"), 5 ))
					{//%%EndPageSetup
						ioBuf.ptr+=5;
						if ( ! CheckFileSpace ( fileRef, &ioBuf, 4 ) ) return ;
						if (CheckBytes ( ioBuf.ptr, Uns8Ptr("etup"), 4 ))
						{
							ioBuf.ptr+=4;
							setTokenInfo(kPS_EndPageSetup,begStartpos,14);
						}
					}
					else if (CheckBytes ( ioBuf.ptr, Uns8Ptr("PageC"), 5 ))
					{//%%EndPageComments
						ioBuf.ptr+=5;
						if ( ! CheckFileSpace ( fileRef, &ioBuf, 7 ) ) return ;
						if (CheckBytes ( ioBuf.ptr, Uns8Ptr("omments"), 7 ))
						{
							ioBuf.ptr+=7;
							setTokenInfo(kPS_EndPageComments,begStartpos,17);
						}
					}
				}
				else if(CheckBytes ( ioBuf.ptr, Uns8Ptr("%%Pag"), 5 ))
				{
					XMP_Int64 begStartpos=ioBuf.filePos+ioBuf.ptr-ioBuf.data;
					ioBuf.ptr+=5;
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 2 ) ) return ;
					if (CheckBytes ( ioBuf.ptr, Uns8Ptr(":"), 2 ))
					{
						ioBuf.ptr+=2;
						while(!IsNewline(*ioBuf.ptr))
						{
							if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return ;
							++ioBuf.ptr;
						}
						setTokenInfo(kPS_Page,begStartpos,ioBuf.filePos+ioBuf.ptr-ioBuf.data-begStartpos);
					}

				}
				else if(CheckBytes ( ioBuf.ptr, Uns8Ptr("%%Tra"), 5 ))
				{
					XMP_Int64 begStartpos=ioBuf.filePos+ioBuf.ptr-ioBuf.data;
					ioBuf.ptr+=5;
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 4 ) ) return ;
					if (CheckBytes ( ioBuf.ptr, Uns8Ptr("iler"), 4 ))
					{
						ioBuf.ptr+=4;
						while(!IsNewline(*ioBuf.ptr)) ++ioBuf.ptr;
						setTokenInfo(kPS_Trailer,begStartpos,ioBuf.filePos+ioBuf.ptr-ioBuf.data-begStartpos);
					}
				}
				else if(CheckBytes ( ioBuf.ptr, Uns8Ptr("%%EOF"), 5 ))
				{
					ioBuf.ptr+=5;
					setTokenInfo(kPS_EOF,ioBuf.filePos+ioBuf.ptr-ioBuf.data,5);
				}
				if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return ;
				++ioBuf.ptr;
			}
			//dont have to search after this DOCINFO last thing 
			return;

		}else if (!(kPS_Creator & dscFlags) && 
			 CheckFileSpace ( fileRef, &ioBuf, kPSContainsForString.length() )&&
			CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsForString.c_str()), kPSContainsForString.length() ))
		{
			ioBuf.ptr+=kPSContainsForString.length();
			if ( ! ExtractDSCCommentValue(ioBuf,kPS_dscFor) ) return ;
		}
		else if (!(kPS_CreatorTool & dscFlags) &&
			 CheckFileSpace ( fileRef, &ioBuf, kPSContainsCreatorString.length() )&&
			 CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsCreatorString.c_str()), kPSContainsCreatorString.length() ))
		{
			ioBuf.ptr+=kPSContainsCreatorString.length();
			if ( ! ExtractDSCCommentValue(ioBuf,kPS_dscCreator) ) return ;
		}
		else if (!(kPS_CreateDate & dscFlags) &&
			 CheckFileSpace ( fileRef, &ioBuf, kPSContainsCreateDateString.length() )&&
			CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsCreateDateString.c_str()), kPSContainsCreateDateString.length() ))
		{
			
			ioBuf.ptr+=kPSContainsCreateDateString.length();
			if ( ! ExtractDSCCommentValue(ioBuf,kPS_dscCreateDate) ) return ;
		}
		else if (!(kPS_Title & dscFlags) &&
			 CheckFileSpace ( fileRef, &ioBuf, kPSContainsTitleString.length() )&&
			CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsTitleString.c_str()), kPSContainsTitleString.length() ))
		{
						
			ioBuf.ptr+=kPSContainsTitleString.length();
			if ( ! ExtractDSCCommentValue(ioBuf,kPS_dscTitle) ) return ;
		}
		else if( CheckFileSpace ( fileRef, &ioBuf, kPSContainsXMPString.length() )&&
			 (  CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsXMPString.c_str()), kPSContainsXMPString.length()    ) )) {

			// Found "%ADO_ContainsXMP:", look for the main packet location option.
			
			XMP_Int64 containsXMPStartpos=ioBuf.filePos+ioBuf.ptr-ioBuf.data;
			ioBuf.ptr += kPSContainsXMPString.length();
			ExtractContainsXMPHint(ioBuf,containsXMPStartpos);

		}	// Found "%ADO_ContainsXMP:".
		//Some other DSC comments skip past the end of this line.
		if ( ! PostScript_Support::SkipUntilNewline(fileRef,ioBuf) ) return ;

	}	// Outer marker loop.

	return ;	// Should never reach here.

}	

// =================================================================================================
// PostScript_MetaHandler::ReadXMPPacket
// =====================================
//
// Helper method read the raw xmp into a string from a file
void PostScript_MetaHandler::ReadXMPPacket (std::string & xmpPacket )
{
	if ( packetInfo.length == 0 ) XMP_Throw ( "ReadXMPPacket - No XMP packet", kXMPErr_BadXMP );

	xmpPacket.erase();
	xmpPacket.reserve ( packetInfo.length );
	xmpPacket.append ( packetInfo.length, ' ' );

	XMP_StringPtr packetStr = XMP_StringPtr ( xmpPacket.c_str() );	// Don't set until after reserving the space!

	this->parent->ioRef->Seek ( packetInfo.offset, kXMP_SeekFromStart );
	this->parent->ioRef->ReadAll ( (char*)packetStr, packetInfo.length );

}	// ReadXMPPacket


// =================================================================================================
// PostScript_MetaHandler::RegisterKeyValue
// =========================================
//
// Helper method registers the Key value pairs for the DocInfo dictionary and sets the Appriopriate 
// DocInfo flags
void PostScript_MetaHandler::RegisterKeyValue(std::string& key, std::string& value)
{
	size_t vallen=value.length();
	if (key.length()==0||vallen==0)
	{
		key.clear();
		value.clear();
		return;
	}
	for (size_t index=0;index<vallen;index++)
	{
		if ((unsigned char)value[index]>127)
		{
			key.clear();
			value.clear();
			return;
		}
	}
	switch (key[0])
	{	
		case 'A': // probably Author
			{
				if (!key.compare("Author"))
				{
					nativeMeta[kPS_docInfoAuthor]=value;
					docInfoFlags|=kPS_Creator;
				}
			break;
			}
		case 'C': //probably Creator or CreationDate
			{
				if (!key.compare("Creator"))
				{
					nativeMeta[kPS_docInfoCreator]=value;
					docInfoFlags|=kPS_CreatorTool;
				}
				else if (!key.compare("CreationDate"))
				{
					nativeMeta[kPS_docInfoCreateDate]=value;
					docInfoFlags|=kPS_CreateDate;
				}
				break;
			}
		case 'T': // probably Title
			{				
				if (!key.compare("Title"))
				{
					nativeMeta[kPS_docInfoTitle]=value;
					docInfoFlags|=kPS_Title;
				}
				break;
			}
		case 'S':// probably Subject
			{
				if (!key.compare("Subject"))
				{
					nativeMeta[kPS_docInfoSubject]=value;
					docInfoFlags|=kPS_Description;
				}
				break;
			}
		case 'K':// probably Keywords
			{
				if (!key.compare("Keywords"))
				{
					nativeMeta[kPS_docInfoKeywords]=value;
					docInfoFlags|=kPS_Subject;
				}
				break;
			}
		case 'M': // probably ModDate 
			{
				if (!key.compare("ModDate"))
				{
					nativeMeta[kPS_docInfoModDate]=value;
					docInfoFlags|=kPS_ModifyDate;
				}
				break;
			}
		default: //ignore everything else
			{
				;
			}
	}
	key.clear();
	value.clear();
}


// =================================================================================================
// PostScript_MetaHandler::ReconcileXMP
// =========================================
//
// Helper method that facilitates the read time reconcilliation of native metadata

void PostScript_MetaHandler::ReconcileXMP( const std::string &xmpStr, std::string *outStr ) 
{
	SXMPMeta xmp;
	xmp.ParseFromBuffer( xmpStr.c_str(), xmpStr.length() );
	// Adding creator Toll if any 
	if (!xmp.DoesPropertyExist ( kXMP_NS_XMP,"CreatorTool" ))
	{
		if(docInfoFlags&kPS_CreatorTool)
		{
			xmp.SetProperty( kXMP_NS_XMP, "CreatorTool", nativeMeta[kPS_docInfoCreator] );
		}
		else if (dscFlags&kPS_CreatorTool)
		{
			xmp.SetProperty( kXMP_NS_XMP, "CreatorTool", nativeMeta[kPS_dscCreator] );
		}
	}
	if (!xmp.DoesPropertyExist ( kXMP_NS_XMP,"CreateDate" ))
	{
		if(docInfoFlags&kPS_CreateDate && nativeMeta[kPS_docInfoCreateDate].length()>0)
		{
			std::string xmpdate=PostScript_Support::ConvertToDate(nativeMeta[kPS_docInfoCreateDate].c_str());
			if (xmpdate.length()>0)
			{
				xmp.SetProperty( kXMP_NS_XMP, "CreateDate", xmpdate );
			}
		}
		else if (dscFlags&kPS_CreateDate&& nativeMeta[kPS_dscCreateDate].length()>0)
		{
			std::string xmpdate=PostScript_Support::ConvertToDate(nativeMeta[kPS_dscCreateDate].c_str());
			xmp.SetProperty( kXMP_NS_XMP, "CreateDate", xmpdate );
		}
	}
	if (!xmp.DoesPropertyExist ( kXMP_NS_XMP,"ModifyDate" ))
	{
		if(docInfoFlags&kPS_ModifyDate && nativeMeta[kPS_docInfoModDate].length()>0)
		{
			std::string xmpdate=PostScript_Support::ConvertToDate(nativeMeta[kPS_docInfoModDate].c_str());
			if (xmpdate.length()>0)
			{
				xmp.SetProperty( kXMP_NS_XMP, "ModifyDate", xmpdate );
			}
		}
	}
	if (!xmp.DoesPropertyExist ( kXMP_NS_DC,"creator" ))
	{
		if(docInfoFlags&kPS_Creator)
		{
			xmp.AppendArrayItem ( kXMP_NS_DC, "creator", kXMP_PropArrayIsOrdered,
										   nativeMeta[kPS_docInfoAuthor] );
		}
		else if ( dscFlags&kPS_Creator)
		{
			xmp.AppendArrayItem ( kXMP_NS_DC, "creator", kXMP_PropArrayIsOrdered,
										   nativeMeta[kPS_dscFor] );
		}
	}
	if (!xmp.DoesPropertyExist ( kXMP_NS_DC,"title" ))
	{
		if(docInfoFlags&kPS_Title)
		{
			xmp.SetLocalizedText( kXMP_NS_DC, "title",NULL,"x-default", nativeMeta[kPS_docInfoTitle] );
		}
		else if ( dscFlags&kPS_Title)
		{
			xmp.SetLocalizedText( kXMP_NS_DC, "title",NULL,"x-default", nativeMeta[kPS_dscTitle] );
		}
	}
	if (!xmp.DoesPropertyExist ( kXMP_NS_DC,"description" ))
	{
		if(docInfoFlags&kPS_Description)
		{
			xmp.SetLocalizedText( kXMP_NS_DC, "description",NULL,"x-default", nativeMeta[kPS_docInfoSubject] );
		}
	}
	if (!xmp.DoesPropertyExist ( kXMP_NS_DC,"subject" ))
	{
		if(docInfoFlags&kPS_Subject)
		{
			xmp.AppendArrayItem( kXMP_NS_DC, "subject", kXMP_PropArrayIsUnordered,
								nativeMeta[kPS_docInfoKeywords], kXMP_NoOptions );
		}
	}

	if (packetInfo.length>0)
	{
		try
		{
			xmp.SerializeToBuffer( outStr, kXMP_ExactPacketLength|kXMP_UseCompactFormat,packetInfo.length);
		}
		catch(...)
		{
			xmp.SerializeToBuffer( outStr, kXMP_UseCompactFormat,0);
		}
	}
	else
	{
		xmp.SerializeToBuffer( outStr, kXMP_UseCompactFormat,0);
	}
}


// =================================================================================================
// PostScript_MetaHandler::CacheFileData
// =====================================
void PostScript_MetaHandler::CacheFileData()
{
	this->containsXMP = false;
	this->psHint = kPSHint_NoMarker;
	ParsePSFile();

	if ( this->psHint == kPSHint_MainFirst ) {
		this->containsXMP = FindFirstPacket();
	} else if ( this->psHint == kPSHint_MainLast ) {
		this->containsXMP = FindLastPacket();
	}else
	{
		//find first packet in case of NoMain or absence of hint
		//When inserting new packet should be inserted in front 
		//any other existing packet
		FindFirstPacket();
	}
	if ( this->containsXMP )
	{
		ReadXMPPacket ( xmpPacket );
	}
}	// PostScript_MetaHandler::CacheFileData

// =================================================================================================
// PostScript_MetaHandler::ProcessXMP
// ==================================
void PostScript_MetaHandler::ProcessXMP()
{
	
	XMP_Assert ( ! this->processedXMP );
	this->processedXMP = true;	// Make sure we only come through here once.

	std::string xmptempStr=xmpPacket;

	//Read time reconciliation with native metadata
	try
	{
		ReconcileXMP(xmptempStr, &xmpPacket);
	}
	catch(...)
	{
	}
	if ( ! this->xmpPacket.empty() ) 
	{
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = (XMP_StringLen)this->xmpPacket.size();
		this->xmpObj.ParseFromBuffer ( packetStr, packetLen );
	}
	if (xmpPacket.length()>0)
	{
		this->containsXMP = true;	// Assume we had something for the XMP.
	}
}


// =================================================================================================
// PostScript_MetaHandler::modifyHeader
// =====================================
//
// Method modifies the header (if one is present) for the postscript offset, tiff offset etc. 
//	when an XMP update resulted in increase in the file size(non-inplace updates)
void PostScript_MetaHandler::modifyHeader(XMP_IO* fileRef,XMP_Int64 extrabytes,XMP_Int64 offset )
{
	//change the header
	IOBuffer temp;
	fileRef->Rewind();
	if ( ! CheckFileSpace ( fileRef, &temp, 4 ) ) return ;
	XMP_Uns32 fileheader = GetUns32BE ( temp.ptr );

	if ( fileheader == 0xC5D0D3C6 ) 
	{
		XMP_Uns8 buffLE[4];
		if ( ! CheckFileSpace ( fileRef, &temp, 32 ) ) return ;
		XMP_Uns32 psLength = GetUns32LE ( temp.ptr+8 );	// PostScript length.
		if (psLength>0)
		{
			psLength+=extrabytes;
			PutUns32LE ( psLength, buffLE);
			fileRef->Seek ( 8, kXMP_SeekFromStart );
			fileRef->Write(buffLE,4);
		}
		XMP_Uns32 wmfOffset = GetUns32LE ( temp.ptr+12 );	// WMF offset.
		if (wmfOffset>0 && wmfOffset>offset)
		{
			wmfOffset+=extrabytes;
			PutUns32LE ( wmfOffset, buffLE);
			fileRef->Seek ( 12, kXMP_SeekFromStart );
			fileRef->Write(buffLE,4);
		}
						
		XMP_Uns32 tiffOffset = GetUns32LE ( temp.ptr+20 );	// Tiff offset.
		if (tiffOffset>0 && tiffOffset>offset)
		{
			tiffOffset+=extrabytes;
			PutUns32LE ( tiffOffset, buffLE);
			fileRef->Seek ( 20, kXMP_SeekFromStart );
			fileRef->Write(buffLE,4);
		}
		//set the checksum to 0xFFFFFFFF
		XMP_Uns16 checksum=0xFFFF;
		PutUns16LE ( checksum, buffLE);
		fileRef->Seek ( 28, kXMP_SeekFromStart );
		fileRef->Write(buffLE,2);
	}
}

// =================================================================================================
// PostScript_MetaHandler::DetermineUpdateMethod
// =============================================
//
// The policy followed to update a Postscript file is
//	a) if the update can fit into the existing xpacket size, go for inplace update.
//	b) If sub file decode filter is used to embed the metadata expand the metadata 
//		and the move the rest contents(after the xpacket) by some calc offset.
//  c) If some other method is used to embed the xpacket readstring or readline
//		insert a new metadata xpacket before the existing xpacket.
//	The preference to use these methods is in the same order a , b and then c
//  DetermineUpdateMethod helps to decide which method be used for the given
//	input file
//
UpdateMethod PostScript_MetaHandler::DetermineUpdateMethod(std::string & outStr)
{
	SXMPMeta xmp;
	std::string &    xmpPacket  = this->xmpPacket;
	XMP_PacketInfo & packetInfo = this->packetInfo;
	xmp.ParseFromBuffer( xmpPacket.c_str(), xmpPacket.length() );
	if (packetInfo.length>0)
	{
		try
		{
			//First try to fit the modified XMP data into existing XMP packet length
			//prefers Inplace
			xmp.SerializeToBuffer( &outStr, kXMP_ExactPacketLength|kXMP_UseCompactFormat,packetInfo.length);
		}
		catch(...)
		{
			// if it doesnt fit :(
			xmp.SerializeToBuffer( &outStr, kXMP_UseCompactFormat,0);
		}
	}
	else
	{
		// this will be the case for Injecting new metadata
		xmp.SerializeToBuffer( &outStr, kXMP_UseCompactFormat,0);
	}
	if ( this->containsXMPHint && (outStr.size() == (size_t)packetInfo.length) )
	{
		return kPS_Inplace;
	}
	else if (this->containsXMPHint && PostScript_Support::IsSFDFilterUsed(this->parent->ioRef,packetInfo.offset))
	{
		return kPS_ExpandSFDFilter;
	}
	else
	{
		return kPS_InjectNew;
	}

}

// =================================================================================================
// PostScript_MetaHandler::InplaceUpdate
// =====================================
//
// Method does the inplace update of the metadata
void PostScript_MetaHandler::InplaceUpdate (std::string &outStr,XMP_IO* &tempRef ,bool doSafeUpdate)
{
	
	XMP_IO*      fileRef		= this->parent->ioRef;
	XMP_Int64	 pos			= 0;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
	//Inplace update of metadata
	if (!doSafeUpdate)
	{
		if ( progressTracker != 0 )  progressTracker->AddTotalWork ((float) outStr.length() );
		fileRef->Seek(packetInfo.offset,kXMP_SeekFromStart);
		fileRef->Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));
	}
	else
	{
		if ( ! tempRef ) tempRef=fileRef->DeriveTemp();
		pos=fileRef->Length();
		if ( progressTracker != 0 )  progressTracker->AddTotalWork ((float) pos );
		//copy data till xmp Packet
		fileRef->Seek(0,kXMP_SeekFromStart);
		XIO::Copy ( fileRef, tempRef, packetInfo.offset, this->parent->abortProc, this->parent->abortArg );

		//insert the new XMP packet
		fileRef->Seek(packetInfo.offset+packetInfo.length,kXMP_SeekFromStart);
		tempRef->Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));

		//copy the rest of data
		XIO::Copy ( fileRef, tempRef,pos-packetInfo.offset-packetInfo.length, this->parent->abortProc, this->parent->abortArg );
	}
}


// =================================================================================================
// PostScript_MetaHandler::ExpandingSFDFilterUpdate
// ================================================
//
// Method updates the metadata by expanding it
void PostScript_MetaHandler::ExpandingSFDFilterUpdate (std::string &outStr,XMP_IO* &tempRef ,bool doSafeUpdate)
{
	//If SubFileDecode Filter is present expanding the
	//existing metadata is easy 

	XMP_IO*      fileRef		= this->parent->ioRef;
	XMP_Int64	 pos			= 0;
	XMP_Int32 extrapacketlength=outStr.length()-packetInfo.length;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;	
	if ( progressTracker != 0 )  progressTracker->AddTotalWork ((float) (extrapacketlength + fileRef->Length() -packetInfo.offset+14) );
	if (!doSafeUpdate)
	{
		size_t bufSize=extrapacketlength/(kIOBufferSize) +1*(extrapacketlength!=(kIOBufferSize));
		std::vector<IOBuffer> tempfilebuffer1(bufSize);
		IOBuffer temp;
		XMP_Int64 readpoint=packetInfo.offset+packetInfo.length,writepoint=packetInfo.offset;
		fileRef->Seek ( readpoint, kXMP_SeekFromStart );
					
		for(size_t x=0;x<bufSize;x++)
		{
			tempfilebuffer1[x].len=fileRef->Read(tempfilebuffer1[x].data,kIOBufferSize,false);
			readpoint+=tempfilebuffer1[x].len;
		}

		fileRef->Seek ( writepoint, kXMP_SeekFromStart );
		fileRef->Write( (void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));
		writepoint+=static_cast<XMP_Uns32>(outStr.length());
		size_t y=0;
		bool continueread=(tempfilebuffer1[bufSize-1].len==kIOBufferSize);
		size_t loop=bufSize;
		while(loop)
		{
			if(continueread)
			{
				fileRef->Seek ( readpoint, kXMP_SeekFromStart );
				temp.len=fileRef->Read(temp.data,kIOBufferSize,false);
				readpoint+=temp.len;
			}
			fileRef->Seek ( writepoint, kXMP_SeekFromStart );
			fileRef->Write(tempfilebuffer1[y].data,tempfilebuffer1[y].len);
			writepoint+=tempfilebuffer1[y].len;
			if (continueread)
				tempfilebuffer1[y]=temp;
			else
				--loop;
			if (temp.len<kIOBufferSize)continueread=false;
			y=(y+1)%bufSize;
		}

		modifyHeader(fileRef,extrapacketlength,packetInfo.offset );
	}
	else
	{
		if ( progressTracker != 0 )  progressTracker->AddTotalWork ((float) (packetInfo.offset) );
		if ( ! tempRef ) tempRef=fileRef->DeriveTemp();
		//copy data till xmp Packet
		fileRef->Seek(0,kXMP_SeekFromStart);
		XIO::Copy ( fileRef, tempRef, packetInfo.offset, this->parent->abortProc, this->parent->abortArg );

		//insert the new XMP packet
		fileRef->Seek(packetInfo.offset+packetInfo.length,kXMP_SeekFromStart);
		tempRef->Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));

		//copy the rest of data
		pos=fileRef->Length();
		XIO::Copy ( fileRef, tempRef,pos-packetInfo.offset-packetInfo.length,  this->parent->abortProc, this->parent->abortArg );
		modifyHeader(tempRef,extrapacketlength,packetInfo.offset );
	}
}

// =================================================================================================
// PostScript_MetaHandler::DetermineInsertionOffsets
// =============================================
//
// Method determines the offsets at which the new xpacket and other postscript code be inserted
void PostScript_MetaHandler::DetermineInsertionOffsets(XMP_Int64& ADOhintOffset,XMP_Int64& InjectData1Offset,
							XMP_Int64& InjectData3Offset)
{
	//find the position to place ADOContainsXMP hint
	if(psHint!=kPSHint_MainFirst && (fileformat==kXMP_EPSFile||kXMPFiles_UnknownLength==packetInfo.offset))
	{
		TokenLocation& tokenLoc= getTokenInfo(kPS_ADOContainsXMP);
		if(tokenLoc.offsetStart==-1)
		{
			TokenLocation& tokenLoc1= getTokenInfo(kPS_EndComments);
			if(tokenLoc1.offsetStart==-1)
			{
				//should never reach here
				throw XMP_Error(kXMPErr_BadFileFormat,"%%EndComment Missing");
			}
			ADOhintOffset=tokenLoc1.offsetStart;
		}
		else
		{
			ADOhintOffset= tokenLoc.offsetStart;
		}
	}
	else if (psHint!=kPSHint_MainLast &&fileformat==kXMP_PostScriptFile)
	{
		TokenLocation& tokenLoc= getTokenInfo(kPS_ADOContainsXMP);
		if(tokenLoc.offsetStart==-1)
		{
			TokenLocation& tokenLoc1= getTokenInfo(kPS_EndComments);
			if(tokenLoc1.offsetStart==-1)
			{
				//should never reach here
				throw XMP_Error(kXMPErr_BadFileFormat,"%%EndComment Missing");
			}
			ADOhintOffset=tokenLoc1.offsetStart;
		}
		else
		{
			ADOhintOffset= tokenLoc.offsetStart;
		}
	}
	//Find the location to insert kEPS_Injectdata1
	XMP_Uns64 xpacketLoc;
	if ( (fileformat == kXMP_PostScriptFile) && (kXMPFiles_UnknownLength != packetInfo.offset) )
	{
		xpacketLoc = (XMP_Uns64)lastPacketInfo.offset;
		TokenLocation& endPagsetuploc = getTokenInfo(kPS_EndPageSetup);
		if ( (endPagsetuploc.offsetStart > -1) && (xpacketLoc < (XMP_Uns64)endPagsetuploc.offsetStart) )
		{
			InjectData1Offset=endPagsetuploc.offsetStart;
		}
		else
		{
			TokenLocation& trailerloc= getTokenInfo(kPS_Trailer);
			if ( (trailerloc.offsetStart > -1) && (xpacketLoc < (XMP_Uns64)trailerloc.offsetStart) )
			{
				InjectData1Offset=trailerloc.offsetStart;
			}
			else
			{
				TokenLocation& eofloc= getTokenInfo(kPS_EOF);
				if ( (eofloc.offsetStart > -1) && (xpacketLoc < (XMP_Uns64)eofloc.offsetStart) )
				{
					InjectData1Offset=eofloc.offsetStart;
				}
				else
				{
					TokenLocation& endPostScriptloc= getTokenInfo(kPS_EndPostScript);
					if ( (endPostScriptloc.offsetStart > -1) && (xpacketLoc < (XMP_Uns64)endPostScriptloc.offsetStart) )
					{
						InjectData1Offset=endPostScriptloc.offsetStart;
					}
				}
			}
		}
	}
	else
	{
		xpacketLoc = (XMP_Uns64)firstPacketInfo.offset;
		TokenLocation& endPagsetuploc = getTokenInfo(kPS_EndPageSetup);
		if ( (endPagsetuploc.offsetStart > -1) && (xpacketLoc > (XMP_Uns64)endPagsetuploc.offsetStart) )
		{
			InjectData1Offset=endPagsetuploc.offsetStart;
		}
		else 
		{
			TokenLocation& beginPagsetuploc= getTokenInfo(kPS_BeginPageSetup);
			if ( (beginPagsetuploc.offsetStart > -1) &&
				 (xpacketLoc > (XMP_Uns64)(beginPagsetuploc.offsetStart + beginPagsetuploc.tokenlen)) )
			{
				InjectData1Offset=beginPagsetuploc.offsetStart+beginPagsetuploc.tokenlen;
			}
			else
			{
				TokenLocation& endPageCommentsloc= getTokenInfo(kPS_EndPageComments);
				if ( (endPageCommentsloc.offsetStart > -1) &&
					 (xpacketLoc > (XMP_Uns64)(endPageCommentsloc.offsetStart + endPageCommentsloc.tokenlen)) )
				{
					InjectData1Offset=endPageCommentsloc.offsetStart+endPageCommentsloc.tokenlen;
				}
				else
				{
					TokenLocation& pageLoc= getTokenInfo(kPS_Page);
					if ( (pageLoc.offsetStart > -1) &&
						 (xpacketLoc > (XMP_Uns64)(pageLoc.offsetStart + pageLoc.tokenlen)) )
					{
						InjectData1Offset=pageLoc.offsetStart+pageLoc.tokenlen;
					}
					else
					{
						TokenLocation& endSetupLoc= getTokenInfo(kPS_EndSetup);
						if ( (endSetupLoc.offsetStart > -1) && (xpacketLoc > (XMP_Uns64)endSetupLoc.offsetStart) )
						{
							InjectData1Offset=endSetupLoc.offsetStart;
						}
						else
						{
							TokenLocation& beginSetupLoc= getTokenInfo(kPS_BeginSetup);
							if ( (beginSetupLoc.offsetStart > -1) &&
								 (xpacketLoc > (XMP_Uns64)(beginSetupLoc.offsetStart + beginSetupLoc.tokenlen)) )
							{
								InjectData1Offset=beginSetupLoc.offsetStart+beginSetupLoc.tokenlen;
							}
							else
							{
								TokenLocation& endPrologLoc= getTokenInfo(kPS_EndProlog);
								if ( (endPrologLoc.offsetStart > -1) &&
									 (xpacketLoc > (XMP_Uns64)(endPrologLoc.offsetStart + endPrologLoc.tokenlen)) )
								{
									InjectData1Offset=endPrologLoc.offsetStart+endPrologLoc.tokenlen;
								}
								else
								{
									TokenLocation& endCommentsLoc= getTokenInfo(kPS_EndComments);
									if ( (endCommentsLoc.offsetStart > -1) &&
										 (xpacketLoc > (XMP_Uns64)(endCommentsLoc.offsetStart + endCommentsLoc.tokenlen)) )
									{
										InjectData1Offset=endCommentsLoc.offsetStart+endCommentsLoc.tokenlen;
									}
									else
									{
										//should never reach here
										throw XMP_Error(kXMPErr_BadFileFormat,"%%EndComment Missing");
									}
								}

							}
						}
					}
				}
			}
		}
	}
	
					
	//Find the location to insert kEPS_Injectdata3
	TokenLocation& trailerloc= getTokenInfo(kPS_Trailer);
	if(trailerloc.offsetStart>-1 )
	{
		InjectData3Offset=trailerloc.offsetStart+trailerloc.tokenlen;
	}
	else
	{
		TokenLocation& eofloc= getTokenInfo(kPS_EOF);
		if(eofloc.offsetStart>-1 )
		{
			InjectData3Offset=eofloc.offsetStart;
		}
		else
		{
			TokenLocation& endPostScriptloc= getTokenInfo(kPS_EndPostScript);
			if(endPostScriptloc.offsetStart>-1 )
			{
				InjectData3Offset=endPostScriptloc.offsetStart;
			}
		}
	}
}

// =================================================================================================
// PostScript_MetaHandler::InsertNewUpdate
// =======================================
//
// Method inserts a new Xpacket in the postscript file.This will be called in two cases
//	a) If there is no xpacket in the PS file
//	b) If the existing xpacket is embedded using readstring or readline method
void PostScript_MetaHandler::InsertNewUpdate (std::string &outStr,XMP_IO* &tempRef,bool doSafeUpdate )
{
	// In this case it is better to have safe update 
	// as non-safe update implementation is going to be complex 
	// and more time consuming 
	// ignoring doSafeUpdate for this update method

	//No SubFileDecode Filter 
	// Need to insert new Metadata before existing metadata
	// with SubFileDecode Filter
	
	XMP_IO*      fileRef		= this->parent->ioRef;	
	if ( ! tempRef ) tempRef=fileRef->DeriveTemp();
	//inject metadata at the right place
	XMP_Int64 ADOhintOffset=-1,InjectData1Offset=-1,InjectData3Offset=-1;
	DetermineInsertionOffsets(ADOhintOffset,InjectData1Offset,InjectData3Offset);
	XMP_Int64 tempInjectData1Offset=InjectData1Offset;
	fileRef->Rewind();
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;	
	if ( progressTracker != 0 )  
	{
		progressTracker->AddTotalWork ((float) ( fileRef->Length() +outStr.length() + 14) );
		if (fileformat==kXMP_EPSFile)
		{
			progressTracker->AddTotalWork ((float) ( kEPS_Injectdata1.length() + kEPS_Injectdata2.length() + kEPS_Injectdata3.length()) );
		}
		else
		{
			progressTracker->AddTotalWork ((float) ( kPS_Injectdata1.length() + kPS_Injectdata2.length()) );
		}
	}
	XMP_Int64 totalReadLength=0;
	//copy contents from orignal file to Temp File
	if(ADOhintOffset!=-1)
	{
		XIO::Copy(fileRef,tempRef,ADOhintOffset,this->parent->abortProc,this->parent->abortArg);
		totalReadLength+=ADOhintOffset;
		if (fileformat==kXMP_EPSFile || kXMPFiles_UnknownLength==packetInfo.offset)
		{
			if ( progressTracker != 0 )  progressTracker->AddTotalWork ((float) ( kPS_XMPHintMainFirst.length()) );
			tempRef->Write(kPS_XMPHintMainFirst.c_str(),kPS_XMPHintMainFirst.length());
		}
		else
		{
			if ( progressTracker != 0 )  progressTracker->AddTotalWork ((float) ( kPS_XMPHintMainLast.length()) );
			tempRef->Write(kPS_XMPHintMainLast.c_str(),kPS_XMPHintMainLast.length());
		}
	}
	InjectData1Offset-=totalReadLength;
	XIO::Copy(fileRef,tempRef,InjectData1Offset,this->parent->abortProc,this->parent->abortArg);
	totalReadLength+=InjectData1Offset;
	if (fileformat==kXMP_EPSFile)
	{
		tempRef->Write(kEPS_Injectdata1.c_str(),kEPS_Injectdata1.length());
		tempRef->Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));
		tempRef->Write(kEPS_Injectdata2.c_str(),kEPS_Injectdata2.length());
	}
	else
	{
		tempRef->Write(kPS_Injectdata1.c_str(),kPS_Injectdata1.length());
		tempRef->Write((void *)outStr.c_str(), static_cast<XMP_Uns32>(outStr.length()));
		tempRef->Write(kPS_Injectdata2.c_str(),kPS_Injectdata2.length());
	}
	if (InjectData3Offset!=-1)
	{
		InjectData3Offset-=totalReadLength;
		XIO::Copy(fileRef,tempRef,InjectData3Offset,this->parent->abortProc,this->parent->abortArg);
		totalReadLength+=InjectData3Offset;
		if (fileformat==kXMP_EPSFile)
		{
			tempRef->Write(kEPS_Injectdata3.c_str(),kEPS_Injectdata3.length());
		}
		XMP_Int64 remlength=fileRef->Length()-totalReadLength;
		XIO::Copy(fileRef,tempRef,remlength,this->parent->abortProc,this->parent->abortArg);
		totalReadLength+=remlength;
	}
	else
	{
		XMP_Int64 remlength=fileRef->Length()-totalReadLength;
		XIO::Copy(fileRef,tempRef,remlength,this->parent->abortProc,this->parent->abortArg);
		totalReadLength+=remlength;
		if (fileformat==kXMP_EPSFile)
		{
			tempRef->Write(kEPS_Injectdata3.c_str(),kEPS_Injectdata3.length());
		}
	}
	XMP_Int64 extraBytes;
	if (fileformat==kXMP_EPSFile )
	{
		extraBytes=((ADOhintOffset!=-1)?kPS_XMPHintMainFirst.length():0)+kEPS_Injectdata3.length()+kEPS_Injectdata2.length()+
			kEPS_Injectdata1.length()+outStr.length();
	}
	else
	{
		extraBytes=((ADOhintOffset!=-1)?(kXMPFiles_UnknownLength!=packetInfo.offset?kPS_XMPHintMainLast.length():kPS_XMPHintMainFirst.length()):0)+kPS_Injectdata2.length()+kPS_Injectdata1.length()+outStr.length();
	}
	modifyHeader(tempRef,extraBytes,tempInjectData1Offset );
}

// =================================================================================================
// PostScript_MetaHandler::UpdateFile
// ==================================
//
// Virtual Method implementation to update XMP metadata in a PS file 
void PostScript_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	IgnoreParam ( doSafeUpdate );
	if ( ! this->needsUpdate ) return;

	XMP_IO *	tempRef			= 0;
	XMP_IO*      fileRef		= this->parent->ioRef;
	std::string &    xmpPacket  = this->xmpPacket;
	std::string outStr;
	
	if (!fileRef )
	{
		XMP_Throw ( "Invalid File Refernce Cannot update XMP", kXMPErr_BadOptions );
	}
	bool localProgressTracking = false;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;	
	if ( progressTracker != 0 ) 
	{
		if ( ! progressTracker->WorkInProgress() ) 
		{
			localProgressTracking = true;
			progressTracker->BeginWork ();
		}
	}
	try
	{	switch(DetermineUpdateMethod(outStr))
		{
			case kPS_Inplace:
			{
				InplaceUpdate ( outStr, tempRef, doSafeUpdate );
				break;
			}
			case kPS_ExpandSFDFilter:
			{
				ExpandingSFDFilterUpdate ( outStr, tempRef, doSafeUpdate );
				break;
			}
			case kPS_InjectNew:
			{
				InsertNewUpdate ( outStr, tempRef, doSafeUpdate );
				break;
			}
			case kPS_None:
			default:
			{
				XMP_Throw ( "XMP Write Failed ", kXMPErr_BadOptions );
			}
		}
	}
	catch(...)
	{
		if( tempRef ) fileRef->DeleteTemp();
		throw;
	}
	// rename the modified temp file and then delete the temp file
	if ( tempRef ) fileRef->AbsorbTemp();
	if ( localProgressTracking ) progressTracker->WorkComplete();
	this->needsUpdate = false;

}	// PostScript_MetaHandler::UpdateFile


// =================================================================================================
// PostScript_MetaHandler::UpdateFile
// ==================================
//
//  Method to write the file with updated XMP metadata to the passed temp file reference
void PostScript_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_IO* origRef = this->parent->ioRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;

	XMP_Int64 fileLen = origRef->Length();
	
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
	if ( progressTracker != 0 )  progressTracker->BeginWork ((float) fileLen );
	origRef->Rewind ( );
	tempRef->Truncate ( 0 );
	XIO::Copy ( origRef, tempRef, fileLen, abortProc, abortArg );

	try 
	{
		this->parent->ioRef = tempRef;	// ! Make UpdateFile update the temp.
		this->UpdateFile ( false );
		this->parent->ioRef = origRef;
	} 
	catch ( ... ) 
	{
		this->parent->ioRef = origRef;
		throw;
	}
	
	if ( progressTracker != 0 ) progressTracker->WorkComplete();
}
// =================================================================================================
