#ifndef __XMPCore_Impl_hpp__
#define __XMPCore_Impl_hpp__ 1

// =================================================================================================
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! Must be the first #include!
#include "public/include/XMP_Const.h"
#include "build/XMP_BuildInfo.h"
#include "source/XMP_LibUtils.hpp"

// #include "XMPCore/source/XMPMeta.hpp"

#include "public/include/client-glue/WXMP_Common.hpp"

#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <cstring>
#include <cstdlib>

#if XMP_WinBuild
	#pragma warning ( disable : 4244 )	// possible loss of data (temporary for 64 bit builds)
	#pragma warning ( disable : 4267 )	// possible loss of data (temporary for 64 bit builds)
#endif

// =================================================================================================
// Primary internal types

class XMP_Node;
class XML_Node;
class XPathStepInfo;

typedef XMP_Node *	XMP_NodePtr;

typedef std::vector<XMP_Node*>		XMP_NodeOffspring;
typedef XMP_NodeOffspring::iterator	XMP_NodePtrPos;

typedef XMP_VarString::iterator			XMP_VarStringPos;
typedef XMP_VarString::const_iterator	XMP_cVarStringPos;

typedef std::vector < XPathStepInfo >		XMP_ExpandedXPath;
typedef XMP_ExpandedXPath::iterator			XMP_ExpandedXPathPos;
typedef XMP_ExpandedXPath::const_iterator	XMP_cExpandedXPathPos;

typedef std::map < XMP_VarString, XMP_ExpandedXPath > XMP_AliasMap;	// Alias name to actual path.
typedef XMP_AliasMap::iterator			XMP_AliasMapPos;
typedef XMP_AliasMap::const_iterator	XMP_cAliasMapPos;

// =================================================================================================
// General global variables and macros

extern XMP_Int32 sXMP_InitCount;

extern XMP_NamespaceTable * sRegisteredNamespaces;

extern XMP_AliasMap * sRegisteredAliasMap;

#define WtoXMPMeta_Ref(xmpRef)	(const XMPMeta &) (*((XMPMeta*)(xmpRef)))
#define WtoXMPMeta_Ptr(xmpRef)	((XMPMeta*)(xmpRef))

#define WtoXMPDocOps_Ptr(docRef)	((XMPDocOps*)(docRef))

extern void *			voidVoidPtr;	// Used to backfill null output parameters.
extern XMP_StringPtr	voidStringPtr;
extern XMP_StringLen	voidStringLen;
extern XMP_OptionBits	voidOptionBits;
extern XMP_Bool			voidByte;
extern bool				voidBool;
extern XMP_Int32		voidInt32;
extern XMP_Int64		voidInt64;
extern double			voidDouble;
extern XMP_DateTime		voidDateTime;
extern WXMP_Result		void_wResult;

#define kHexDigits "0123456789ABCDEF"

#define XMP_LitMatch(s,l)		(strcmp((s),(l)) == 0)
#define XMP_LitNMatch(s,l,n)	(strncmp((s),(l),(n)) == 0)
	// *** Use the above macros!

#if XMP_WinBuild
	#define snprintf _snprintf
#endif

// =================================================================================================
// Version info

#if XMP_DebugBuild
	#define kXMPCore_DebugFlag 1
#else
	#define kXMPCore_DebugFlag 0
#endif

#define kXMPCore_VersionNumber	( (kXMPCore_DebugFlag << 31)    |	\
                                  (XMP_API_VERSION_MAJOR << 24) |	\
						          (XMP_API_VERSION_MINOR << 16) |	\
						          (XMP_API_VERSION_MICRO << 8) )

	#define kXMPCoreName "XMP Core"
	#define kXMPCore_VersionMessage	kXMPCoreName " " XMPCORE_API_VERSION_STRING
// =================================================================================================
// Support for call tracing

#ifndef XMP_TraceCoreCalls
	#define XMP_TraceCoreCalls			0
	#define XMP_TraceCoreCallsToFile	0
#endif

#if XMP_TraceCoreCalls

	#undef AnnounceThrow
	#undef AnnounceCatch

	#undef AnnounceEntry
	#undef AnnounceNoLock
	#undef AnnounceExit

	extern FILE * xmpCoreLog;

	#define AnnounceThrow(msg)	\
		fprintf ( xmpCoreLog, "XMP_Throw: %s\n", msg ); fflush ( xmpCoreLog )
	#define AnnounceCatch(msg)	\
		fprintf ( xmpCoreLog, "Catch in %s: %s\n", procName, msg ); fflush ( xmpCoreLog )

	#define AnnounceEntry(proc)			\
		const char * procName = proc;	\
		fprintf ( xmpCoreLog, "Entering %s\n", procName ); fflush ( xmpCoreLog )
	#define AnnounceNoLock(proc)		\
		const char * procName = proc;	\
		fprintf ( xmpCoreLog, "Entering %s (no lock)\n", procName ); fflush ( xmpCoreLog )
	#define AnnounceExit()	\
		fprintf ( xmpCoreLog, "Exiting %s\n", procName ); fflush ( xmpCoreLog )

#endif

// =================================================================================================
// ExpandXPath, FindNode, and related support

// *** Normalize the use of "const xx &" for input params

#define kXMP_ArrayItemName	"[]"

#define kXMP_CreateNodes	true
#define kXMP_ExistingOnly	false

#define FindConstSchema(t,u)	FindSchemaNode ( const_cast<XMP_Node*>(t), u, kXMP_ExistingOnly, 0 )
#define FindConstChild(p,c)		FindChildNode ( const_cast<XMP_Node*>(p), c, kXMP_ExistingOnly, 0 )
#define FindConstQualifier(p,c)	FindQualifierNode ( const_cast<XMP_Node*>(p), c, kXMP_ExistingOnly, 0 )
#define FindConstNode(t,p)		FindNode ( const_cast<XMP_Node*>(t), p, kXMP_ExistingOnly, 0 )

extern XMP_OptionBits
VerifySetOptions ( XMP_OptionBits options, XMP_StringPtr propValue );

extern void
ComposeXPath ( const XMP_ExpandedXPath & expandedXPath,
			   XMP_VarString * stringXPath );

extern void
ExpandXPath	( XMP_StringPtr			schemaNS,
			  XMP_StringPtr			propPath,
			  XMP_ExpandedXPath *	expandedXPath );

extern XMP_Node *
FindSchemaNode ( XMP_Node *		  xmpTree,
				 XMP_StringPtr	  nsURI,
				 bool			  createNodes,
				 XMP_NodePtrPos * ptrPos = 0 );

extern XMP_Node *
FindChildNode ( XMP_Node *		 parent,
				XMP_StringPtr	 childName,
				bool			 createNodes,
				XMP_NodePtrPos * ptrPos = 0 );

extern XMP_Node *
FindQualifierNode ( XMP_Node *		 parent,
					XMP_StringPtr	 qualName,
					bool			 createNodes,
					XMP_NodePtrPos * ptrPos = 0 );

extern XMP_Node *
FindNode ( XMP_Node *		xmpTree,
		   const XMP_ExpandedXPath & expandedXPath,
		   bool				createNodes,
		   XMP_OptionBits	leafOptions = 0,
		   XMP_NodePtrPos * ptrPos = 0 );

extern XMP_Index
LookupLangItem ( const XMP_Node * arrayNode, XMP_VarString & lang );	// ! Lang must be normalized!

extern XMP_Index
LookupFieldSelector ( const XMP_Node * arrayNode, XMP_StringPtr fieldName, XMP_StringPtr fieldValue );

extern void
CloneOffspring ( const XMP_Node * origParent, XMP_Node * cloneParent, bool skipEmpty = false );

extern XMP_Node *
CloneSubtree ( const XMP_Node * origRoot, XMP_Node * cloneParent, bool skipEmpty = false );

extern bool
CompareSubtrees ( const XMP_Node & leftNode, const XMP_Node & rightNode );

extern void
DeleteSubtree ( XMP_NodePtrPos rootNodePos );

extern void
DeleteEmptySchema ( XMP_Node * schemaNode );

extern void
NormalizeLangValue ( XMP_VarString * value );

extern void
NormalizeLangArray ( XMP_Node * array );

extern void
DetectAltText ( XMP_Node * xmpParent );

extern void
SortNamedNodes ( XMP_NodeOffspring & nodeVector );

static inline bool
IsPathPrefix ( XMP_StringPtr fullPath, XMP_StringPtr prefix )
{
	bool isPrefix = false;
	XMP_StringLen prefixLen = strlen(prefix);
	if ( XMP_LitNMatch ( prefix, fullPath, prefixLen ) ) {
		char separator = fullPath[prefixLen];
		if ( (separator == 0) || (separator == '/') ||
			 (separator == '[') || (separator == '*') ) isPrefix = true;
	}
	return isPrefix;
}

// -------------------------------------------------------------------------------------------------

class XPathStepInfo {
public:
	XMP_VarString	step;
	XMP_OptionBits	options;
	XPathStepInfo ( XMP_StringPtr _step, XMP_OptionBits _options ) : step(_step), options(_options) {};
	XPathStepInfo ( XMP_VarString _step, XMP_OptionBits _options ) : step(_step), options(_options) {};
private:
	XPathStepInfo() : options(0) {};	// ! Hide the default constructor.
};

enum { kSchemaStep = 0, kRootPropStep = 1, kAliasIndexStep = 2 };

enum {	// Bits for XPathStepInfo options.	// *** Add mask check to init code.
	kXMP_StepKindMask		= 0x0F,	// ! The step kinds are mutually exclusive numbers.
	kXMP_StructFieldStep	= 0x01,	// Also for top level nodes (schema "fields").
	kXMP_QualifierStep		= 0x02,	// ! Order is significant to separate struct/qual from array kinds!
	kXMP_ArrayIndexStep		= 0x03,	// ! The kinds must not overlay array form bits!
	kXMP_ArrayLastStep		= 0x04,
	kXMP_QualSelectorStep	= 0x05,
	kXMP_FieldSelectorStep	= 0x06,
	kXMP_StepIsAlias        = 0x10
};

#define GetStepKind(f)	((f) & kXMP_StepKindMask)

#define kXMP_NewImplicitNode	kXMP_InsertAfterItem

// =================================================================================================
// XMP_Node details

#if 0	// Pattern for iterating over the children or qualifiers:
	for ( size_t xxNum = 0, xxLim = _node_->_offspring_.size(); xxNum < xxLim; ++xxNum ) {
		const XMP_Node * _curr_ = _node_->_offspring_[xxNum];
	}
#endif

class XMP_Node {
public:

	XMP_OptionBits		options;
	XMP_VarString		name, value;
	XMP_Node *			parent;
	XMP_NodeOffspring	children;
	XMP_NodeOffspring	qualifiers;
	#if XMP_DebugBuild
		// *** XMP_StringPtr	_namePtr, _valuePtr;	// *** Not working, need operator=?
	#endif

	XMP_Node ( XMP_Node * _parent, XMP_StringPtr _name, XMP_OptionBits _options )
		: options(_options), name(_name), parent(_parent)
	{
		#if XMP_DebugBuild
			XMP_Assert ( (name.find ( ':' ) != XMP_VarString::npos) || (name == kXMP_ArrayItemName) ||
			             (options & kXMP_SchemaNode) || (parent == 0) );
			// *** _namePtr  = name.c_str();
			// *** _valuePtr = value.c_str();
		#endif
	};

	XMP_Node ( XMP_Node * _parent, const XMP_VarString & _name, XMP_OptionBits _options )
		: options(_options), name(_name), parent(_parent)
	{
		#if XMP_DebugBuild
			XMP_Assert ( (name.find ( ':' ) != XMP_VarString::npos) || (name == kXMP_ArrayItemName) ||
			             (options & kXMP_SchemaNode) || (parent == 0) );
			// *** _namePtr  = name.c_str();
			// *** _valuePtr = value.c_str();
		#endif
	};

	XMP_Node ( XMP_Node * _parent, XMP_StringPtr _name, XMP_StringPtr _value, XMP_OptionBits _options )
		: options(_options), name(_name), value(_value), parent(_parent)
	{
		#if XMP_DebugBuild
			XMP_Assert ( (name.find ( ':' ) != XMP_VarString::npos) || (name == kXMP_ArrayItemName) ||
			             (options & kXMP_SchemaNode) || (parent == 0) );
			// *** _namePtr  = name.c_str();
			// *** _valuePtr = value.c_str();
		#endif
	};

	XMP_Node ( XMP_Node * _parent, const XMP_VarString & _name, const XMP_VarString & _value, XMP_OptionBits _options )
		: options(_options), name(_name), value(_value), parent(_parent)
	{
		#if XMP_DebugBuild
			XMP_Assert ( (name.find ( ':' ) != XMP_VarString::npos) || (name == kXMP_ArrayItemName) ||
			             (options & kXMP_SchemaNode) || (parent == 0) );
			// *** _namePtr  = name.c_str();
			// *** _valuePtr = value.c_str();
		#endif
	};

	void GetLocalURI ( XMP_StringPtr * uriStr, XMP_StringLen * uriSize ) const;

	void RemoveChildren()
	{
		for ( size_t i = 0, vLim = children.size(); i < vLim; ++i ) {
			if ( children[i] != 0 ) delete children[i];
		}
		children.clear();
	}

	void RemoveQualifiers()
	{
		for ( size_t i = 0, vLim = qualifiers.size(); i < vLim; ++i ) {
			if ( qualifiers[i] != 0 ) delete qualifiers[i];
		}
		qualifiers.clear();
	}

	void ClearNode()
	{
		options = 0;
		name.erase();
		value.erase();
		this->RemoveChildren();
		this->RemoveQualifiers();
	}

	virtual ~XMP_Node() { RemoveChildren(); RemoveQualifiers(); };

private:
	XMP_Node() : options(0), parent(0)	// ! Make sure parent pointer is always set.
	{
		#if XMP_DebugBuild
			// *** _namePtr  = name.c_str();
			// *** _valuePtr = value.c_str();
		#endif
	};

};

class XMP_AutoNode {	// Used to hold a child during subtree construction.
public:
	XMP_Node * nodePtr;
	XMP_AutoNode() : nodePtr(0) {};
	~XMP_AutoNode() { if ( nodePtr != 0 ) delete ( nodePtr ); nodePtr = 0; };
	XMP_AutoNode ( XMP_Node * _parent, XMP_StringPtr _name, XMP_OptionBits _options )
		: nodePtr ( new XMP_Node ( _parent, _name, _options ) ) {};
	XMP_AutoNode ( XMP_Node * _parent, const XMP_VarString & _name, XMP_OptionBits _options )
		: nodePtr ( new XMP_Node ( _parent, _name, _options ) ) {};
	XMP_AutoNode ( XMP_Node * _parent, XMP_StringPtr _name, XMP_StringPtr _value, XMP_OptionBits _options )
		: nodePtr ( new XMP_Node ( _parent, _name, _value, _options ) ) {};
	XMP_AutoNode ( XMP_Node * _parent, const XMP_VarString & _name, const XMP_VarString & _value, XMP_OptionBits _options )
		: nodePtr ( new XMP_Node ( _parent, _name, _value, _options ) ) {};
};

// =================================================================================================

#endif	// __XMPCore_Impl_hpp__
