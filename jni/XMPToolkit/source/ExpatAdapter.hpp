#ifndef __ExpatAdapter_hpp__
#define __ExpatAdapter_hpp__

// =================================================================================================
// Copyright 2005 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! Must be the first #include!
#include "source/XMLParserAdapter.hpp"

// =================================================================================================
// Derived XML parser adapter for Expat.
// =================================================================================================

#ifndef BanAllEntityUsage
	#define BanAllEntityUsage	0
#endif

struct XML_ParserStruct;	// ! Hack to avoid exposing expat.h to all clients.
typedef struct XML_ParserStruct *XML_Parser;

class ExpatAdapter : public XMLParserAdapter {
public:

	XML_Parser parser;
	XMP_NamespaceTable * registeredNamespaces;
	
	#if BanAllEntityUsage
		bool isAborted;
	#endif
	
	#if XMP_DebugBuild
		size_t elemNesting;
	#endif
	
	static const bool kUseGlobalNamespaces = true;
	static const bool kUseLocalNamespaces  = false;
	
	ExpatAdapter ( bool useGlobalNamespaces );
	virtual ~ExpatAdapter();
	
	void ParseBuffer ( const void * buffer, size_t length, bool last = true );

private:

	ExpatAdapter() : registeredNamespaces(0) {};	// ! Force use of constructor with namespace parameter.

};

extern "C" ExpatAdapter * XMP_NewExpatAdapter ( bool useGlobalNamespaces );

// =================================================================================================

#endif	// __ExpatAdapter_hpp__
