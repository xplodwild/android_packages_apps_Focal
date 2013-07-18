// =================================================================================================
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include!
#include "XMPCore/source/XMPCore_Impl.hpp"
#include "XMPCore/source/XMPMeta.hpp"
#include "source/ExpatAdapter.hpp"

#include <cstring>

#if DEBUG
	#include <iostream>
#endif

using namespace std;

#if XMP_WinBuild
	#pragma warning ( disable : 4189 )	// local variable is initialized but not referenced
	#pragma warning ( disable : 4505 )	// unreferenced local function has been removed
#endif

// =================================================================================================

// *** This might be faster and use less memory as a state machine. A big advantage of building an
// *** XML tree though is easy lookahead during the recursive descent processing.

// *** It would be nice to give a line number or byte offset in the exception messages.


// 7 RDF/XML Grammar (from http://www.w3.org/TR/rdf-syntax-grammar/#section-Infoset-Grammar)
//
// 7.1 Grammar summary
//
// 7.2.2 coreSyntaxTerms
//		rdf:RDF | rdf:ID | rdf:about | rdf:parseType | rdf:resource | rdf:nodeID | rdf:datatype
//
// 7.2.3 syntaxTerms
//		coreSyntaxTerms | rdf:Description | rdf:li
//
// 7.2.4 oldTerms
//		rdf:aboutEach | rdf:aboutEachPrefix | rdf:bagID
//
// 7.2.5 nodeElementURIs
//		anyURI - ( coreSyntaxTerms | rdf:li | oldTerms )
//
// 7.2.6 propertyElementURIs
//		anyURI - ( coreSyntaxTerms | rdf:Description | oldTerms )
//
// 7.2.7 propertyAttributeURIs
//		anyURI - ( coreSyntaxTerms | rdf:Description | rdf:li | oldTerms )
//
// 7.2.8 doc
//		root ( document-element == RDF, children == list ( RDF ) )
//
// 7.2.9 RDF
//		start-element ( URI == rdf:RDF, attributes == set() )
//		nodeElementList
//		end-element()
//
// 7.2.10 nodeElementList
//		ws* ( nodeElement ws* )*
//
// 7.2.11 nodeElement
//		start-element ( URI == nodeElementURIs,
//						attributes == set ( ( idAttr | nodeIdAttr | aboutAttr )?, propertyAttr* ) )
//		propertyEltList
//		end-element()
//
// 7.2.12 ws
//		A text event matching white space defined by [XML] definition White Space Rule [3] S in section Common Syntactic Constructs.
//
// 7.2.13 propertyEltList
//		ws* ( propertyElt ws* )*
//
// 7.2.14 propertyElt
//		resourcePropertyElt | literalPropertyElt | parseTypeLiteralPropertyElt |
//		parseTypeResourcePropertyElt | parseTypeCollectionPropertyElt | parseTypeOtherPropertyElt | emptyPropertyElt
//
// 7.2.15 resourcePropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr? ) )
//		ws* nodeElement ws*
//		end-element()
//
// 7.2.16 literalPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, datatypeAttr?) )
//		text()
//		end-element()
//
// 7.2.17 parseTypeLiteralPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseLiteral ) )
//		literal
//		end-element()
//
// 7.2.18 parseTypeResourcePropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseResource ) )
//		propertyEltList
//		end-element()
//
// 7.2.19 parseTypeCollectionPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseCollection ) )
//		nodeElementList
//		end-element()
//
// 7.2.20 parseTypeOtherPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseOther ) )
//		propertyEltList
//		end-element()
//
// 7.2.21 emptyPropertyElt
//		start-element ( URI == propertyElementURIs,
//						attributes == set ( idAttr?, ( resourceAttr | nodeIdAttr )?, propertyAttr* ) )
//		end-element()
//
// 7.2.22 idAttr
//		attribute ( URI == rdf:ID, string-value == rdf-id )
//
// 7.2.23 nodeIdAttr
//		attribute ( URI == rdf:nodeID, string-value == rdf-id )
//
// 7.2.24 aboutAttr
//		attribute ( URI == rdf:about, string-value == URI-reference )
//
// 7.2.25 propertyAttr
//		attribute ( URI == propertyAttributeURIs, string-value == anyString )
//
// 7.2.26 resourceAttr
//		attribute ( URI == rdf:resource, string-value == URI-reference )
//
// 7.2.27 datatypeAttr
//		attribute ( URI == rdf:datatype, string-value == URI-reference )
//
// 7.2.28 parseLiteral
//		attribute ( URI == rdf:parseType, string-value == "Literal")
//
// 7.2.29 parseResource
//		attribute ( URI == rdf:parseType, string-value == "Resource")
//
// 7.2.30 parseCollection
//		attribute ( URI == rdf:parseType, string-value == "Collection")
//
// 7.2.31 parseOther
//		attribute ( URI == rdf:parseType, string-value == anyString - ("Resource" | "Literal" | "Collection") )
//
// 7.2.32 URI-reference
//		An RDF URI Reference.
//
// 7.2.33 literal
//		Any XML element content that is allowed according to [XML] definition Content of Elements Rule [43] content
//		in section 3.1 Start-Tags, End-Tags, and Empty-Element Tags.
//
// 7.2.34 rdf-id
//		An attribute string-value matching any legal [XML-NS] token NCName.


// =================================================================================================
// Primary Parsing Functions
// =========================
//
// Each of these is responsible for recognizing an RDF syntax production and adding the appropriate
// structure to the XMP tree. They simply return for success, failures will throw an exception. The
// class exists only to provide access to the error notification object.

class RDF_Parser {
public:

	void RDF ( XMP_Node * xmpTree, const XML_Node & xmlNode );

	void NodeElementList ( XMP_Node * xmpParent, const XML_Node & xmlParent, bool isTopLevel );

	void NodeElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void NodeElementAttrs ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void PropertyElementList ( XMP_Node * xmpParent, const XML_Node & xmlParent, bool isTopLevel );

	void PropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void ResourcePropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void LiteralPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void ParseTypeLiteralPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void ParseTypeResourcePropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void ParseTypeCollectionPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void ParseTypeOtherPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	void EmptyPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel );

	RDF_Parser ( XMPMeta::ErrorCallbackInfo * ec ) : errorCallback(ec) {};

private:

	RDF_Parser() {};	// Hidden on purpose.
	
	XMPMeta::ErrorCallbackInfo * errorCallback;

	XMP_Node * AddChildNode ( XMP_Node * xmpParent, const XML_Node & xmlNode, const XMP_StringPtr value, bool isTopLevel );

	XMP_Node * AddQualifierNode ( XMP_Node * xmpParent, const XMP_VarString & name, const XMP_VarString & value );

	XMP_Node * AddQualifierNode ( XMP_Node * xmpParent, const XML_Node & attr );
	
	void FixupQualifiedNode ( XMP_Node * xmpParent );

};

enum { kIsTopLevel = true, kNotTopLevel = false };

// =================================================================================================

typedef XMP_Uns8 RDFTermKind;

// *** Logic might be safer with just masks.

enum {
	kRDFTerm_Other				=  0,
	kRDFTerm_RDF				=  1,	// Start of coreSyntaxTerms.
	kRDFTerm_ID					=  2,
	kRDFTerm_about				=  3,
	kRDFTerm_parseType			=  4,
	kRDFTerm_resource			=  5,
	kRDFTerm_nodeID				=  6,
	kRDFTerm_datatype			=  7,	// End of coreSyntaxTerms.
	kRDFTerm_Description		=  8,	// Start of additions for syntaxTerms.
	kRDFTerm_li					=  9,	// End of of additions for syntaxTerms.
	kRDFTerm_aboutEach			= 10,	// Start of oldTerms.
	kRDFTerm_aboutEachPrefix	= 11,
	kRDFTerm_bagID				= 12,	// End of oldTerms.
	
	kRDFTerm_FirstCore          = kRDFTerm_RDF,
	kRDFTerm_LastCore           = kRDFTerm_datatype,
	kRDFTerm_FirstSyntax        = kRDFTerm_FirstCore,	// ! Yes, the syntax terms include the core terms.
	kRDFTerm_LastSyntax         = kRDFTerm_li,
	kRDFTerm_FirstOld           = kRDFTerm_aboutEach,
	kRDFTerm_LastOld            = kRDFTerm_bagID
};

enum {
	kRDFMask_Other				= 1 << kRDFTerm_Other,
	kRDFMask_RDF				= 1 << kRDFTerm_RDF,
	kRDFMask_ID					= 1 << kRDFTerm_ID,
	kRDFMask_about				= 1 << kRDFTerm_about,
	kRDFMask_parseType			= 1 << kRDFTerm_parseType,
	kRDFMask_resource			= 1 << kRDFTerm_resource,
	kRDFMask_nodeID				= 1 << kRDFTerm_nodeID,
	kRDFMask_datatype			= 1 << kRDFTerm_datatype,
	kRDFMask_Description		= 1 << kRDFTerm_Description,
	kRDFMask_li					= 1 << kRDFTerm_li,
	kRDFMask_aboutEach			= 1 << kRDFTerm_aboutEach,
	kRDFMask_aboutEachPrefix	= 1 << kRDFTerm_aboutEachPrefix,
	kRDFMask_bagID				= 1 << kRDFTerm_bagID
};

enum {
	kRDF_HasValueElem = 0x10000000UL	// ! Contains rdf:value child. Must fit within kXMP_ImplReservedMask!
};

// -------------------------------------------------------------------------------------------------
// GetRDFTermKind
// --------------

static RDFTermKind
GetRDFTermKind ( const XMP_VarString & name )
{
	RDFTermKind term = kRDFTerm_Other;

	// Arranged to hopefully minimize the parse time for large XMP.

	if ( (name.size() > 4) && (strncmp ( name.c_str(), "rdf:", 4 ) == 0) ) {

		if ( name == "rdf:li" ) {
			term = kRDFTerm_li;
		} else if ( name == "rdf:parseType" ) {
			term = kRDFTerm_parseType;
		} else if ( name == "rdf:Description" ) {
			term = kRDFTerm_Description;
		} else if ( name == "rdf:about" ) {
			term = kRDFTerm_about;
		} else if ( name == "rdf:resource" ) {
			term = kRDFTerm_resource;
		} else if ( name == "rdf:RDF" ) {
			term = kRDFTerm_RDF;
		} else if ( name == "rdf:ID" ) {
			term = kRDFTerm_ID;
		} else if ( name == "rdf:nodeID" ) {
			term = kRDFTerm_nodeID;
		} else if ( name == "rdf:datatype" ) {
			term = kRDFTerm_datatype;
		} else if ( name == "rdf:aboutEach" ) {
			term = kRDFTerm_aboutEach;
		} else if ( name == "rdf:aboutEachPrefix" ) {
			term = kRDFTerm_aboutEachPrefix;
		} else if ( name == "rdf:bagID" ) {
			term = kRDFTerm_bagID;
		}

	}

	return term;

}	// GetRDFTermKind

// =================================================================================================

static void
RemoveChild ( XMP_Node * xmpParent, size_t index )
{
	XMP_Node * child = xmpParent->children[index];
	xmpParent->children.erase ( xmpParent->children.begin() + index );
	delete child;
}

// -------------------------------------------------------------------------------------------------

static void
RemoveQualifier ( XMP_Node * xmpParent, size_t index )
{
	XMP_Node * qualifier = xmpParent->qualifiers[index];
	xmpParent->qualifiers.erase ( xmpParent->qualifiers.begin() + index );
	delete qualifier;
}

// -------------------------------------------------------------------------------------------------

static void
RemoveQualifier ( XMP_Node * xmpParent, XMP_NodePtrPos pos )
{
	XMP_Node * qualifier = *pos;
	xmpParent->qualifiers.erase ( pos );
	delete qualifier;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------
// IsCoreSyntaxTerm
// ----------------
//
// 7.2.2 coreSyntaxTerms
//		rdf:RDF | rdf:ID | rdf:about | rdf:parseType | rdf:resource | rdf:nodeID | rdf:datatype

static bool
IsCoreSyntaxTerm ( RDFTermKind term )
{
	if 	( (kRDFTerm_FirstCore <= term) && (term <= kRDFTerm_LastCore) ) return true;
	return false;
}

// -------------------------------------------------------------------------------------------------
// IsSyntaxTerm
// ------------
//
// 7.2.3 syntaxTerms
//		coreSyntaxTerms | rdf:Description | rdf:li

static bool
IsSyntaxTerm ( RDFTermKind term )
{
	if 	( (kRDFTerm_FirstSyntax <= term) && (term <= kRDFTerm_LastSyntax) ) return true;
	return false;
}

// -------------------------------------------------------------------------------------------------
// IsOldTerm
// ---------
//
// 7.2.4 oldTerms
//		rdf:aboutEach | rdf:aboutEachPrefix | rdf:bagID

static bool
IsOldTerm ( RDFTermKind term )
{
	if 	( (kRDFTerm_FirstOld <= term) && (term <= kRDFTerm_LastOld) ) return true;
	return false;
}

// -------------------------------------------------------------------------------------------------
// IsNodeElementName
// -----------------
//
// 7.2.5 nodeElementURIs
//		anyURI - ( coreSyntaxTerms | rdf:li | oldTerms )

static bool
IsNodeElementName ( RDFTermKind term )
{
	if 	( (term == kRDFTerm_li) || IsOldTerm ( term ) ) return false;
	return (! IsCoreSyntaxTerm ( term ));
}

// -------------------------------------------------------------------------------------------------
// IsPropertyElementName
// ---------------------
//
// 7.2.6 propertyElementURIs
//		anyURI - ( coreSyntaxTerms | rdf:Description | oldTerms )

static bool
IsPropertyElementName ( RDFTermKind term )
{
	if 	( (term == kRDFTerm_Description) || IsOldTerm ( term ) ) return false;
	return (! IsCoreSyntaxTerm ( term ));
}

// -------------------------------------------------------------------------------------------------
// IsPropertyAttributeName
// -----------------------
//
// 7.2.7 propertyAttributeURIs
//		anyURI - ( coreSyntaxTerms | rdf:Description | rdf:li | oldTerms )

static bool
IsPropertyAttributeName ( RDFTermKind term )
{
	if 	( (term == kRDFTerm_Description) || (term == kRDFTerm_li) || IsOldTerm ( term ) ) return false;
	return (! IsCoreSyntaxTerm ( term ));
}

// -------------------------------------------------------------------------------------------------
// IsNumberedArrayItemName
// -----------------------
//
// Return true for a name of the form "rdf:_n", where n is a decimal integer. We're not strict about
// the integer part, it just has to be characters in the range '0'..'9'.

static bool
IsNumberedArrayItemName ( const std::string & name )
{
	if ( name.size() <= 5 ) return false;
	if ( strncmp ( name.c_str(), "rdf:_", 5 ) != 0 ) return false;
	for ( size_t i = 5; i < name.size(); ++i ) {
		if ( (name[i] < '0') | (name[i] > '9') ) return false;
	}
	return true;
}

// =================================================================================================
// RDF_Parser::AddChildNode
// ========================

XMP_Node * RDF_Parser::AddChildNode ( XMP_Node * xmpParent, const XML_Node & xmlNode, const XMP_StringPtr value, bool isTopLevel )
{
	
	if ( xmlNode.ns.empty() ) {
		XMP_Error error ( kXMPErr_BadRDF, "XML namespace required for all elements and attributes" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		return 0;
	}
		
	bool isArrayParent = (xmpParent->options & kXMP_PropValueIsArray);
	bool isArrayItem   = (xmlNode.name == "rdf:li");
	bool isValueNode   = (xmlNode.name == "rdf:value");
	XMP_OptionBits childOptions = 0;
	XMP_StringPtr  childName = xmlNode.name.c_str();

	if ( isTopLevel ) {

		// Lookup the schema node, adjust the XMP parent pointer.
		XMP_Assert ( xmpParent->parent == 0 );	// Incoming parent must be the tree root.
		XMP_Node * schemaNode = FindSchemaNode ( xmpParent, xmlNode.ns.c_str(), kXMP_CreateNodes );
		if ( schemaNode->options & kXMP_NewImplicitNode ) schemaNode->options ^= kXMP_NewImplicitNode;	// Clear the implicit node bit.
			// *** Should use "opt &= ~flag" (no conditional), need runtime check for proper 32 bit code.
		xmpParent = schemaNode;
		
		// If this is an alias set the isAlias flag in the node and the hasAliases flag in the tree.
		if ( sRegisteredAliasMap->find ( xmlNode.name ) != sRegisteredAliasMap->end() ) {
			childOptions |= kXMP_PropIsAlias;
			schemaNode->parent->options |= kXMP_PropHasAliases;
		}
		
	}
	
	// Check use of rdf:li and rdf:_n names. Must be done before calling FindChildNode!
	if ( isArrayItem ) {

		// rdf:li can only be used for array children.
		if ( ! isArrayParent ) {
			XMP_Error error ( kXMPErr_BadRDF, "Misplaced rdf:li element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			return 0;
		}
		childName = kXMP_ArrayItemName;

	} else if ( isArrayParent ) {

		// Tolerate use of rdf:_n, don't verify order.
		if ( IsNumberedArrayItemName ( xmlNode.name ) ) {
			childName = kXMP_ArrayItemName;
			isArrayItem = true;
		} else {
			XMP_Error error ( kXMPErr_BadRDF, "Array items cannot have arbitrary child names" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			return 0;
		}

	}

	// Make sure that this is not a duplicate of a named node.
	if ( ! (isArrayItem | isValueNode) ) {
		if ( FindChildNode ( xmpParent, childName, kXMP_ExistingOnly ) != 0 ) {
			XMP_Error error ( kXMPErr_BadXMP, "Duplicate property or field node" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			return 0;
		}
	}

	// Make sure an rdf:value node is used properly.
	if ( isValueNode ) {
		if ( isTopLevel || (! (xmpParent->options & kXMP_PropValueIsStruct)) ) {
			XMP_Error error ( kXMPErr_BadRDF, "Misplaced rdf:value element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			return 0;
		}
		xmpParent->options |= kRDF_HasValueElem;
	}
	
	// Add the new child to the XMP parent node.
	XMP_Node * newChild = new XMP_Node ( xmpParent, childName, value, childOptions );
	if ( (! isValueNode) || xmpParent->children.empty() ) {
		 xmpParent->children.push_back ( newChild );
	} else {
		 xmpParent->children.insert ( xmpParent->children.begin(), newChild );
	}
	
	return newChild;

}	// RDF_Parser::AddChildNode

// =================================================================================================
// RDF_Parser::AddQualifierNode
// ============================

XMP_Node * RDF_Parser::AddQualifierNode ( XMP_Node * xmpParent, const XMP_VarString & name, const XMP_VarString & value )
{
	
	const bool isLang = (name == "xml:lang");
	const bool isType = (name == "rdf:type");

	XMP_Node * newQual = 0;

	newQual = new XMP_Node ( xmpParent, name, value, kXMP_PropIsQualifier );

	if ( ! (isLang | isType) ) {
		xmpParent->qualifiers.push_back ( newQual );
	} else if ( isLang ) {
		if ( xmpParent->qualifiers.empty() ) {
			xmpParent->qualifiers.push_back ( newQual );
		} else {
			xmpParent->qualifiers.insert ( xmpParent->qualifiers.begin(), newQual );
		}
		xmpParent->options |= kXMP_PropHasLang;
	} else {
		XMP_Assert ( isType );
		if ( xmpParent->qualifiers.empty() ) {
			xmpParent->qualifiers.push_back ( newQual );
		} else {
			size_t offset = 0;
			if ( XMP_PropHasLang ( xmpParent->options ) ) offset = 1;
			xmpParent->qualifiers.insert ( xmpParent->qualifiers.begin()+offset, newQual );
		}
		xmpParent->options |= kXMP_PropHasType;
	}

	xmpParent->options |= kXMP_PropHasQualifiers;

	return newQual;

}	// RDF_Parser::AddQualifierNode

// =================================================================================================
// RDF_Parser::AddQualifierNode
// ============================

XMP_Node * RDF_Parser::AddQualifierNode ( XMP_Node * xmpParent, const XML_Node & attr )
{
	if ( attr.ns.empty() ) {
		XMP_Error error ( kXMPErr_BadRDF, "XML namespace required for all elements and attributes" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		return 0;
	}
	
	return this->AddQualifierNode ( xmpParent, attr.name, attr.value );

}	// RDF_Parser::AddQualifierNode

// =================================================================================================
// RDF_Parser::FixupQualifiedNode
// ==============================
//
// The parent is an RDF pseudo-struct containing an rdf:value field. Fix the XMP data model. The
// rdf:value node must be the first child, the other children are qualifiers. The form, value, and
// children of the rdf:value node are the real ones. The rdf:value node's qualifiers must be added
// to the others.
	
void RDF_Parser::FixupQualifiedNode ( XMP_Node * xmpParent )
{
	size_t qualNum, qualLim;
	size_t childNum, childLim;

	XMP_Enforce ( (xmpParent->options & kXMP_PropValueIsStruct) && (! xmpParent->children.empty()) );

	XMP_Node * valueNode = xmpParent->children[0];
	XMP_Enforce ( valueNode->name == "rdf:value" );
	
	xmpParent->qualifiers.reserve ( xmpParent->qualifiers.size() + xmpParent->children.size() + valueNode->qualifiers.size() );
	
	// Move the qualifiers on the value node to the parent. Make sure an xml:lang qualifier stays at
	// the front.
	
	qualNum = 0;
	qualLim = valueNode->qualifiers.size();
	
	if ( valueNode->options & kXMP_PropHasLang ) {

		if ( xmpParent->options & kXMP_PropHasLang ) {
			XMP_Error error ( kXMPErr_BadXMP, "Duplicate xml:lang for rdf:value element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			XMP_Assert ( xmpParent->qualifiers[0]->name == "xml:lang" );
			RemoveQualifier ( xmpParent, 0 );	// Use the rdf:value node's language.
		}

		XMP_Node * langQual = valueNode->qualifiers[0];
		
		XMP_Assert ( langQual->name == "xml:lang" );
		langQual->parent = xmpParent;
		xmpParent->options |= kXMP_PropHasLang;
		XMP_ClearOption ( valueNode->options, kXMP_PropHasLang );

		if ( xmpParent->qualifiers.empty() ) {
			xmpParent->qualifiers.push_back ( langQual );	// *** Should use utilities to add qual & set parent.
		} else {
			xmpParent->qualifiers.insert ( xmpParent->qualifiers.begin(), langQual );
		}
		valueNode->qualifiers[0] = 0;	// We just moved it to the parent.

		qualNum = 1;	// Start the remaining copy after the xml:lang qualifier.

	}
	
	for ( ; qualNum != qualLim; ++qualNum ) {

		XMP_Node * currQual = valueNode->qualifiers[qualNum];
		XMP_NodePtrPos existingPos;
		XMP_Node * existingQual = FindQualifierNode ( xmpParent, currQual->name.c_str(), kXMP_ExistingOnly, &existingPos );

		if ( existingQual != 0 ) {
			XMP_Error error ( kXMPErr_BadXMP, "Duplicate qualifier node" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			RemoveQualifier ( xmpParent, existingPos );	// Use the rdf:value node's qualifier.
		}

		currQual->parent = xmpParent;
		xmpParent->qualifiers.push_back ( currQual );
		valueNode->qualifiers[qualNum] = 0;	// We just moved it to the parent.

	}
	
	valueNode->qualifiers.clear();	// ! There should be nothing but null pointers.
	
	// Change the parent's other children into qualifiers. This loop starts at 1, child 0 is the
	// rdf:value node. Put xml:lang at the front, append all others.
	
	for ( childNum = 1, childLim = xmpParent->children.size(); childNum != childLim; ++childNum ) {

		XMP_Node * currQual = xmpParent->children[childNum];
		bool isLang = (currQual->name == "xml:lang");
		
		if ( FindQualifierNode ( xmpParent, currQual->name.c_str(), kXMP_ExistingOnly ) != 0 ) {
			XMP_Error error ( kXMPErr_BadXMP, "Duplicate qualifier" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			delete currQual;

		} else {
		
			currQual->options |= kXMP_PropIsQualifier;
			currQual->parent = xmpParent;
	
			if ( isLang ) {
				xmpParent->options |= kXMP_PropHasLang;
			} else if ( currQual->name == "rdf:type" ) {
				xmpParent->options |= kXMP_PropHasType;
			}
	
			if ( (! isLang) || xmpParent->qualifiers.empty() ) {
				xmpParent->qualifiers.push_back ( currQual );
			} else {
				xmpParent->qualifiers.insert ( xmpParent->qualifiers.begin(), currQual );
			}
			
		}
		
		xmpParent->children[childNum] = 0;	// We just moved it to the qualifers, or ignored it.

	}
	
	if ( ! xmpParent->qualifiers.empty() ) xmpParent->options |= kXMP_PropHasQualifiers;
	
	// Move the options and value last, other checks need the parent's original options. Move the
	// value node's children to be the parent's children. Delete the now useless value node.
	
	XMP_Assert ( xmpParent->options & (kXMP_PropValueIsStruct | kRDF_HasValueElem) );
	xmpParent->options &= ~ (kXMP_PropValueIsStruct | kRDF_HasValueElem);
	xmpParent->options |= valueNode->options;
	
	xmpParent->value.swap ( valueNode->value );

	xmpParent->children[0] = 0;	// ! Remove the value node itself before the swap.
	xmpParent->children.swap ( valueNode->children );
	
	for ( childNum = 0, childLim = xmpParent->children.size(); childNum != childLim; ++childNum ) {
		XMP_Node * currChild = xmpParent->children[childNum];
		currChild->parent = xmpParent;
	}

	delete valueNode;
	
}	// RDF_Parser::FixupQualifiedNode

// =================================================================================================
// RDF_Parser::RDF
// ===============
//
// 7.2.9 RDF
//		start-element ( URI == rdf:RDF, attributes == set() )
//		nodeElementList
//		end-element()
//
// The top level rdf:RDF node. It can only have xmlns attributes, which have already been removed
// during construction of the XML tree.

void RDF_Parser::RDF ( XMP_Node * xmpTree, const XML_Node & xmlNode )
{

	if ( ! xmlNode.attrs.empty() ) {
		XMP_Error error ( kXMPErr_BadRDF, "Invalid attributes of rdf:RDF element" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
	}
	this->NodeElementList ( xmpTree, xmlNode, kIsTopLevel );	// ! Attributes are ignored.

}	// RDF_Parser::RDF

// =================================================================================================
// RDF_Parser::NodeElementList
// ===========================
//
// 7.2.10 nodeElementList
//		ws* ( nodeElement ws* )*

void RDF_Parser::NodeElementList ( XMP_Node * xmpParent, const XML_Node & xmlParent, bool isTopLevel )
{
	XMP_Assert ( isTopLevel );
	
	XML_cNodePos currChild = xmlParent.content.begin();	// *** Change these loops to the indexed pattern.
	XML_cNodePos endChild  = xmlParent.content.end();

	for ( ; currChild != endChild; ++currChild ) {
		if ( (*currChild)->IsWhitespaceNode() ) continue;
		this->NodeElement ( xmpParent, **currChild, isTopLevel );
	}

}	// RDF_Parser::NodeElementList

// =================================================================================================
// RDF_Parser::NodeElement
// =======================
//
// 7.2.5 nodeElementURIs
//		anyURI - ( coreSyntaxTerms | rdf:li | oldTerms )
//
// 7.2.11 nodeElement
//		start-element ( URI == nodeElementURIs,
//						attributes == set ( ( idAttr | nodeIdAttr | aboutAttr )?, propertyAttr* ) )
//		propertyEltList
//		end-element()
//
// A node element URI is rdf:Description or anything else that is not an RDF term.

void RDF_Parser::NodeElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	RDFTermKind nodeTerm = GetRDFTermKind ( xmlNode.name );
	if ( (nodeTerm != kRDFTerm_Description) && (nodeTerm != kRDFTerm_Other) ) {
		XMP_Error error ( kXMPErr_BadRDF, "Node element must be rdf:Description or typedNode" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
	} else if ( isTopLevel && (nodeTerm == kRDFTerm_Other) ) {
		XMP_Error error ( kXMPErr_BadXMP, "Top level typedNode not allowed" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
	} else {
		this->NodeElementAttrs ( xmpParent, xmlNode, isTopLevel );
		this->PropertyElementList ( xmpParent, xmlNode, isTopLevel );
	}

}	// RDF_Parser::NodeElement

// =================================================================================================
// RDF_Parser::NodeElementAttrs
// ============================
//
// 7.2.7 propertyAttributeURIs
//		anyURI - ( coreSyntaxTerms | rdf:Description | rdf:li | oldTerms )
//
// 7.2.11 nodeElement
//		start-element ( URI == nodeElementURIs,
//						attributes == set ( ( idAttr | nodeIdAttr | aboutAttr )?, propertyAttr* ) )
//		propertyEltList
//		end-element()
//
// Process the attribute list for an RDF node element. A property attribute URI is anything other
// than an RDF term. The rdf:ID and rdf:nodeID attributes are simply ignored, as are rdf:about
// attributes on inner nodes.

static const XMP_OptionBits kExclusiveAttrMask = (kRDFMask_ID | kRDFMask_nodeID | kRDFMask_about);

void RDF_Parser::NodeElementAttrs ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	XMP_OptionBits exclusiveAttrs = 0;	// Used to detect attributes that are mutually exclusive.

	XML_cNodePos currAttr = xmlNode.attrs.begin();
	XML_cNodePos endAttr  = xmlNode.attrs.end();

	for ( ; currAttr != endAttr; ++currAttr ) {

		RDFTermKind attrTerm = GetRDFTermKind ( (*currAttr)->name );

		switch ( attrTerm ) {

			case kRDFTerm_ID     :
			case kRDFTerm_nodeID :
			case kRDFTerm_about  :

				if ( exclusiveAttrs & kExclusiveAttrMask ) {
					XMP_Error error ( kXMPErr_BadRDF, "Mutally exclusive about, ID, nodeID attributes" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
					continue;	// Skip the later mutually exclusive attributes.
				}
				exclusiveAttrs |= (1 << attrTerm);

				if ( isTopLevel && (attrTerm == kRDFTerm_about) ) {
					// This is the rdf:about attribute on a top level node. Set the XMP tree name if
					// it doesn't have a name yet. Make sure this name matches the XMP tree name.
					XMP_Assert ( xmpParent->parent == 0 );	// Must be the tree root node.
					if ( xmpParent->name.empty() ) {
						xmpParent->name = (*currAttr)->value;
					} else if ( ! (*currAttr)->value.empty() ) {
						if ( xmpParent->name != (*currAttr)->value ) {
							XMP_Error error ( kXMPErr_BadXMP, "Mismatched top level rdf:about values" );
							this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
						}
					}
				}

				break;

			case kRDFTerm_Other :
				this->AddChildNode ( xmpParent, **currAttr, (*currAttr)->value.c_str(), isTopLevel );
				break;

			default :
				{
					XMP_Error error ( kXMPErr_BadRDF, "Invalid nodeElement attribute" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
				}
				continue;

		}

	}

}	// RDF_Parser::NodeElementAttrs

// =================================================================================================
// RDF_Parser::PropertyElementList
// ===============================
//
// 7.2.13 propertyEltList
//		ws* ( propertyElt ws* )*

void RDF_Parser::PropertyElementList ( XMP_Node * xmpParent, const XML_Node & xmlParent, bool isTopLevel )
{
	XML_cNodePos currChild = xmlParent.content.begin();
	XML_cNodePos endChild  = xmlParent.content.end();

	for ( ; currChild != endChild; ++currChild ) {
		if ( (*currChild)->IsWhitespaceNode() ) continue;
		if ( (*currChild)->kind != kElemNode ) {
			XMP_Error error ( kXMPErr_BadRDF, "Expected property element node not found" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			continue;
		}
		this->PropertyElement ( xmpParent, **currChild, isTopLevel );
	}

}	// RDF_Parser::PropertyElementList

// =================================================================================================
// RDF_Parser::PropertyElement
// ===========================
//
// 7.2.14 propertyElt
//		resourcePropertyElt | literalPropertyElt | parseTypeLiteralPropertyElt |
//		parseTypeResourcePropertyElt | parseTypeCollectionPropertyElt | parseTypeOtherPropertyElt | emptyPropertyElt
//
// 7.2.15 resourcePropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr? ) )
//		ws* nodeElement ws*
//		end-element()
//
// 7.2.16 literalPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, datatypeAttr?) )
//		text()
//		end-element()
//
// 7.2.17 parseTypeLiteralPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseLiteral ) )
//		literal
//		end-element()
//
// 7.2.18 parseTypeResourcePropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseResource ) )
//		propertyEltList
//		end-element()
//
// 7.2.19 parseTypeCollectionPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseCollection ) )
//		nodeElementList
//		end-element()
//
// 7.2.20 parseTypeOtherPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseOther ) )
//		propertyEltList
//		end-element()
//
// 7.2.21 emptyPropertyElt
//		start-element ( URI == propertyElementURIs,
//						attributes == set ( idAttr?, ( resourceAttr | nodeIdAttr )?, propertyAttr* ) )
//		end-element()
//
// The various property element forms are not distinguished by the XML element name, but by their
// attributes for the most part. The exceptions are resourcePropertyElt and literalPropertyElt. They
// are distinguished by their XML element content.
//
// NOTE: The RDF syntax does not explicitly include the xml:lang attribute although it can appear in
// many of these. We have to allow for it in the attibute counts below.

void RDF_Parser::PropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	RDFTermKind nodeTerm = GetRDFTermKind ( xmlNode.name );
	if ( ! IsPropertyElementName ( nodeTerm ) ) {
		XMP_Error error ( kXMPErr_BadRDF, "Invalid property element name" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		return;
	}
	
	if ( xmlNode.attrs.size() > 3 ) {

		// Only an emptyPropertyElt can have more than 3 attributes.
		this->EmptyPropertyElement ( xmpParent, xmlNode, isTopLevel );

	} else {

		// Look through the attributes for one that isn't rdf:ID or xml:lang, it will usually tell
		// what we should be dealing with. The called routines must verify their specific syntax!

		XML_cNodePos currAttr = xmlNode.attrs.begin();
		XML_cNodePos endAttr  = xmlNode.attrs.end();
		XMP_VarString * attrName = 0;

		for ( ; currAttr != endAttr; ++currAttr ) {
			attrName = &((*currAttr)->name);
			if ( (*attrName != "xml:lang") && (*attrName != "rdf:ID") ) break;
		}

		if ( currAttr != endAttr ) {

			XMP_Assert ( attrName != 0 );
			XMP_VarString& attrValue = (*currAttr)->value;

			if ( *attrName == "rdf:datatype" ) {
				this->LiteralPropertyElement ( xmpParent, xmlNode, isTopLevel );
			} else if ( *attrName != "rdf:parseType" ) {
				this->EmptyPropertyElement ( xmpParent, xmlNode, isTopLevel );
			} else if ( attrValue == "Literal" ) {
				this->ParseTypeLiteralPropertyElement ( xmpParent, xmlNode, isTopLevel );
			} else if ( attrValue == "Resource" ) {
				this->ParseTypeResourcePropertyElement ( xmpParent, xmlNode, isTopLevel );
			} else if ( attrValue == "Collection" ) {
				this->ParseTypeCollectionPropertyElement ( xmpParent, xmlNode, isTopLevel );
			} else {
				this->ParseTypeOtherPropertyElement ( xmpParent, xmlNode, isTopLevel );
			}

		} else {

			// Only rdf:ID and xml:lang, could be a resourcePropertyElt, a literalPropertyElt, or an.
			// emptyPropertyElt. Look at the child XML nodes to decide which.

			if ( xmlNode.content.empty() ) {

				this->EmptyPropertyElement ( xmpParent, xmlNode, isTopLevel );

			} else {
			
				XML_cNodePos currChild = xmlNode.content.begin();
				XML_cNodePos endChild  = xmlNode.content.end();

				for ( ; currChild != endChild; ++currChild ) {
					if ( (*currChild)->kind != kCDataNode ) break;
				}
				
				if ( currChild == endChild ) {
					this->LiteralPropertyElement ( xmpParent, xmlNode, isTopLevel );
				} else {
					this->ResourcePropertyElement ( xmpParent, xmlNode, isTopLevel );
				}
			
			}

		}
		
	}

}	// RDF_Parser::PropertyElement

// =================================================================================================
// RDF_Parser::ResourcePropertyElement
// ===================================
//
// 7.2.15 resourcePropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr? ) )
//		ws* nodeElement ws*
//		end-element()
//
// This handles structs using an rdf:Description node, arrays using rdf:Bag/Seq/Alt, and Typed Nodes.
// It also catches and cleans up qualified properties written with rdf:Description and rdf:value.

void RDF_Parser::ResourcePropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	if ( isTopLevel && (xmlNode.name == "iX:changes") ) return;	// Strip old "punchcard" chaff.
	
	XMP_Node * newCompound = this->AddChildNode ( xmpParent, xmlNode, "", isTopLevel );
	if ( newCompound == 0 ) return;	// Ignore lower level errors.
	
	XML_cNodePos currAttr = xmlNode.attrs.begin();
	XML_cNodePos endAttr  = xmlNode.attrs.end();

	for ( ; currAttr != endAttr; ++currAttr ) {
		XMP_VarString & attrName = (*currAttr)->name;
		if ( attrName == "xml:lang" ) {
			this->AddQualifierNode ( newCompound, **currAttr );
		} else if ( attrName == "rdf:ID" ) {
			continue;	// Ignore all rdf:ID attributes.
		} else {
			XMP_Error error ( kXMPErr_BadRDF, "Invalid attribute for resource property element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			continue;
		}
	}
	
	XML_cNodePos currChild = xmlNode.content.begin();
	XML_cNodePos endChild  = xmlNode.content.end();

	for ( ; currChild != endChild; ++currChild ) {
		if ( ! (*currChild)->IsWhitespaceNode() ) break;
	}
	if ( currChild == endChild ) {
		XMP_Error error ( kXMPErr_BadRDF, "Missing child of resource property element" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		return;
	}
	if ( (*currChild)->kind != kElemNode ) {
		XMP_Error error ( kXMPErr_BadRDF, "Children of resource property element must be XML elements" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		return;
	}

	if ( (*currChild)->name == "rdf:Bag" ) {
		newCompound->options |= kXMP_PropValueIsArray;
	} else if ( (*currChild)->name == "rdf:Seq" ) {
		newCompound->options |= kXMP_PropValueIsArray | kXMP_PropArrayIsOrdered;
	} else if ( (*currChild)->name == "rdf:Alt" ) {
		newCompound->options |= kXMP_PropValueIsArray | kXMP_PropArrayIsOrdered | kXMP_PropArrayIsAlternate;
	} else {
		// This is the Typed Node case. Add an rdf:type qualifier with a URI value.
		if ( (*currChild)->name != "rdf:Description" ) {
			XMP_VarString typeName ( (*currChild)->ns );
			size_t colonPos = (*currChild)->name.find_first_of(':');
			if ( colonPos == XMP_VarString::npos ) {
				XMP_Error error ( kXMPErr_BadXMP, "All XML elements must be in a namespace" );
				this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
				return;
			}
			typeName.append ( (*currChild)->name, colonPos+1, XMP_VarString::npos );	// Append just the local name.
			XMP_Node * typeQual = this->AddQualifierNode ( newCompound, XMP_VarString("rdf:type"), typeName );
			if ( typeQual != 0 ) typeQual->options |= kXMP_PropValueIsURI;
		}
		newCompound->options |= kXMP_PropValueIsStruct;
	}

	this->NodeElement ( newCompound, **currChild, kNotTopLevel );
	if ( newCompound->options & kRDF_HasValueElem ) {
		this->FixupQualifiedNode ( newCompound );
	} else if ( newCompound->options & kXMP_PropArrayIsAlternate ) {
		DetectAltText ( newCompound );
	}

	for ( ++currChild; currChild != endChild; ++currChild ) {
		if ( ! (*currChild)->IsWhitespaceNode() ) {
			XMP_Error error ( kXMPErr_BadRDF, "Invalid child of resource property element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			break;	// Don't bother looking for more trailing errors.
		}
	}

}	// RDF_Parser::ResourcePropertyElement

// =================================================================================================
// RDF_Parser::LiteralPropertyElement
// ==================================
//
// 7.2.16 literalPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, datatypeAttr?) )
//		text()
//		end-element()
//
// Add a leaf node with the text value and qualifiers for the attributes.

void RDF_Parser::LiteralPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	XMP_Node * newChild = this->AddChildNode ( xmpParent, xmlNode, "", isTopLevel );
	if ( newChild == 0 ) return;	// Ignore lower level errors.
	
	XML_cNodePos currAttr = xmlNode.attrs.begin();
	XML_cNodePos endAttr  = xmlNode.attrs.end();

	for ( ; currAttr != endAttr; ++currAttr ) {
		XMP_VarString & attrName = (*currAttr)->name;
		if ( attrName == "xml:lang" ) {
			this->AddQualifierNode ( newChild, **currAttr );
		} else if ( (attrName == "rdf:ID") || (attrName == "rdf:datatype") ) {
			continue; 	// Ignore all rdf:ID and rdf:datatype attributes.
		} else {
			XMP_Error error ( kXMPErr_BadRDF, "Invalid attribute for literal property element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			continue;
		}
	}
	
	XML_cNodePos currChild = xmlNode.content.begin();
	XML_cNodePos endChild  = xmlNode.content.end();
	size_t textSize = 0;

	for ( ; currChild != endChild; ++currChild ) {
		if ( (*currChild)->kind == kCDataNode ) {
			textSize += (*currChild)->value.size();
		} else {
			XMP_Error error ( kXMPErr_BadRDF, "Invalid child of literal property element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		}
	}
	
	newChild->value.reserve ( textSize );

	for ( currChild = xmlNode.content.begin(); currChild != endChild; ++currChild ) {
		newChild->value += (*currChild)->value;
	}

}	// RDF_Parser::LiteralPropertyElement

// =================================================================================================
// RDF_Parser::ParseTypeLiteralPropertyElement
// ===========================================
//
// 7.2.17 parseTypeLiteralPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseLiteral ) )
//		literal
//		end-element()

void RDF_Parser::ParseTypeLiteralPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	IgnoreParam(xmpParent); IgnoreParam(xmlNode); IgnoreParam(isTopLevel); 
	XMP_Error error ( kXMPErr_BadXMP, "ParseTypeLiteral property element not allowed" );
	this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );

}	// RDF_Parser::ParseTypeLiteralPropertyElement

// =================================================================================================
// RDF_Parser::ParseTypeResourcePropertyElement
// ============================================
//
// 7.2.18 parseTypeResourcePropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseResource ) )
//		propertyEltList
//		end-element()
//
// Add a new struct node with a qualifier for the possible rdf:ID attribute. Then process the XML
// child nodes to get the struct fields.

void RDF_Parser::ParseTypeResourcePropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	XMP_Node * newStruct = this->AddChildNode ( xmpParent, xmlNode, "", isTopLevel );
	if ( newStruct == 0 ) return;	// Ignore lower level errors.
	newStruct->options  |= kXMP_PropValueIsStruct;
	
	XML_cNodePos currAttr = xmlNode.attrs.begin();
	XML_cNodePos endAttr  = xmlNode.attrs.end();

	for ( ; currAttr != endAttr; ++currAttr ) {
		XMP_VarString & attrName = (*currAttr)->name;
		if ( attrName == "rdf:parseType" ) {
			continue;	// ! The caller ensured the value is "Resource".
		} else if ( attrName == "xml:lang" ) {
			this->AddQualifierNode ( newStruct, **currAttr );
		} else if ( attrName == "rdf:ID" ) {
			continue;	// Ignore all rdf:ID attributes.
		} else {
			XMP_Error error ( kXMPErr_BadRDF, "Invalid attribute for ParseTypeResource property element" );
			this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
			continue;
		}
	}

	this->PropertyElementList ( newStruct, xmlNode, kNotTopLevel );

	if ( newStruct->options & kRDF_HasValueElem ) this->FixupQualifiedNode ( newStruct );
	
	// *** Need to look for arrays using rdf:Description and rdf:type.

}	// RDF_Parser::ParseTypeResourcePropertyElement

// =================================================================================================
// RDF_Parser::ParseTypeCollectionPropertyElement
// ==============================================
//
// 7.2.19 parseTypeCollectionPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseCollection ) )
//		nodeElementList
//		end-element()

void RDF_Parser::ParseTypeCollectionPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	IgnoreParam(xmpParent); IgnoreParam(xmlNode); IgnoreParam(isTopLevel); 
	XMP_Error error ( kXMPErr_BadXMP, "ParseTypeCollection property element not allowed" );
	this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );

}	// RDF_Parser::ParseTypeCollectionPropertyElement

// =================================================================================================
// RDF_Parser::ParseTypeOtherPropertyElement
// =========================================
//
// 7.2.20 parseTypeOtherPropertyElt
//		start-element ( URI == propertyElementURIs, attributes == set ( idAttr?, parseOther ) )
//		propertyEltList
//		end-element()

void RDF_Parser::ParseTypeOtherPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	IgnoreParam(xmpParent); IgnoreParam(xmlNode); IgnoreParam(isTopLevel); 
	XMP_Error error ( kXMPErr_BadXMP, "ParseTypeOther property element not allowed" );
	this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );

}	// RDF_Parser::ParseTypeOtherPropertyElement

// =================================================================================================
// RDF_Parser::EmptyPropertyElement
// ================================
//
// 7.2.21 emptyPropertyElt
//		start-element ( URI == propertyElementURIs,
//						attributes == set ( idAttr?, ( resourceAttr | nodeIdAttr )?, propertyAttr* ) )
//		end-element()
//
//	<ns:Prop1/>  <!-- a simple property with an empty value --> 
//	<ns:Prop2 rdf:resource="http://www.adobe.com/"/> <!-- a URI value --> 
//	<ns:Prop3 rdf:value="..." ns:Qual="..."/> <!-- a simple qualified property --> 
//	<ns:Prop4 ns:Field1="..." ns:Field2="..."/> <!-- a struct with simple fields -->
//
// An emptyPropertyElt is an element with no contained content, just a possibly empty set of
// attributes. An emptyPropertyElt can represent three special cases of simple XMP properties: a
// simple property with an empty value (ns:Prop1), a simple property whose value is a URI
// (ns:Prop2), or a simple property with simple qualifiers (ns:Prop3). An emptyPropertyElt can also
// represent an XMP struct whose fields are all simple and unqualified (ns:Prop4).
//
// It is an error to use both rdf:value and rdf:resource - that can lead to invalid  RDF in the
// verbose form written using a literalPropertyElt.
//
// The XMP mapping for an emptyPropertyElt is a bit different from generic RDF, partly for 
// design reasons and partly for historical reasons. The XMP mapping rules are: 
//	1. If there is an rdf:value attribute then this is a simple property with a text value.
//		All other attributes are qualifiers.
//	2. If there is an rdf:resource attribute then this is a simple property with a URI value. 
//		All other attributes are qualifiers.
//	3. If there are no attributes other than xml:lang, rdf:ID, or rdf:nodeID then this is a simple 
//		property with an empty value. 
//	4. Otherwise this is a struct, the attributes other than xml:lang, rdf:ID, or rdf:nodeID are fields. 

void RDF_Parser::EmptyPropertyElement ( XMP_Node * xmpParent, const XML_Node & xmlNode, bool isTopLevel )
{
	bool hasPropertyAttrs = false;
	bool hasResourceAttr  = false;
	bool hasNodeIDAttr    = false;
	bool hasValueAttr     = false;
	
	const XML_Node * valueNode = 0;	// ! Can come from rdf:value or rdf:resource.
	
	if ( ! xmlNode.content.empty() ) {
		XMP_Error error ( kXMPErr_BadRDF, "Nested content not allowed with rdf:resource or property attributes" );
		this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
		return;
	}
	
	// First figure out what XMP this maps to and remember the XML node for a simple value.
	
	XML_cNodePos currAttr = xmlNode.attrs.begin();
	XML_cNodePos endAttr  = xmlNode.attrs.end();

	for ( ; currAttr != endAttr; ++currAttr ) {

		RDFTermKind attrTerm = GetRDFTermKind ( (*currAttr)->name );

		switch ( attrTerm ) {

			case kRDFTerm_ID :
				// Nothing to do.
				break;

			case kRDFTerm_resource :
				if ( hasNodeIDAttr ) {
					XMP_Error error ( kXMPErr_BadRDF, "Empty property element can't have both rdf:resource and rdf:nodeID" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
					return;
				}
				if ( hasValueAttr ) {
					XMP_Error error ( kXMPErr_BadXMP, "Empty property element can't have both rdf:value and rdf:resource" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
					return;
				}
				hasResourceAttr = true;
				if ( ! hasValueAttr ) valueNode = *currAttr;
				break;

			case kRDFTerm_nodeID :
				if ( hasResourceAttr ) {
					XMP_Error error ( kXMPErr_BadRDF, "Empty property element can't have both rdf:resource and rdf:nodeID" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
					return;
				}
				hasNodeIDAttr = true;
				break;

			case kRDFTerm_Other :
				if ( (*currAttr)->name == "rdf:value" ) {
					if ( hasResourceAttr ) {
						XMP_Error error ( kXMPErr_BadXMP, "Empty property element can't have both rdf:value and rdf:resource" );
						this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
						return;
					}
					hasValueAttr = true;
					valueNode = *currAttr;
				} else if ( (*currAttr)->name != "xml:lang" ) {
					hasPropertyAttrs = true;
				}
				break;

			default :
				{
					XMP_Error error ( kXMPErr_BadRDF, "Unrecognized attribute of empty property element" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
				}
				
				return;

		}

	}
	
	// Create the right kind of child node and visit the attributes again to add the fields or qualifiers.
	// ! Because of implementation vagaries, the xmpParent is the tree root for top level properties.
	// ! The schema is found, created if necessary, by AddChildNode.
	
	XMP_Node * childNode = this->AddChildNode ( xmpParent, xmlNode, "", isTopLevel );
	if ( childNode == 0 ) return;	// Ignore lower level errors.
	bool childIsStruct = false;
	
	if ( hasValueAttr | hasResourceAttr ) {
		childNode->value = valueNode->value;
		if ( ! hasValueAttr ) childNode->options |= kXMP_PropValueIsURI;	// ! Might have both rdf:value and rdf:resource.
	} else if ( hasPropertyAttrs ) {
		childNode->options |= kXMP_PropValueIsStruct;
		childIsStruct = true;
	}
		
	currAttr = xmlNode.attrs.begin();
	endAttr  = xmlNode.attrs.end();

	for ( ; currAttr != endAttr; ++currAttr ) {

		if ( *currAttr == valueNode ) continue;	// Skip the rdf:value or rdf:resource attribute holding the value.
		RDFTermKind attrTerm = GetRDFTermKind ( (*currAttr)->name );

		switch ( attrTerm ) {

			case kRDFTerm_ID       :
			case kRDFTerm_nodeID   :
				break;	// Ignore all rdf:ID and rdf:nodeID attributes.
				
			case kRDFTerm_resource :
				this->AddQualifierNode ( childNode, **currAttr );
				break;

			case kRDFTerm_Other :
				if ( (! childIsStruct) || (*currAttr)->name == "xml:lang" ) {
					this->AddQualifierNode ( childNode, **currAttr );
				} else {
					this->AddChildNode ( childNode, **currAttr, (*currAttr)->value.c_str(), false );
				}
				break;

			default :
				{
					XMP_Error error ( kXMPErr_BadRDF, "Unrecognized attribute of empty property element" );
					this->errorCallback->NotifyClient ( kXMPErrSev_Recoverable, error );
				}
				continue;

		}

	}

}	// RDF_Parser::EmptyPropertyElement

// =================================================================================================
// XMPMeta::ProcessRDF
// ===================
//
// Parse the XML tree of the RDF and build the corresponding XMP tree.

void XMPMeta::ProcessRDF ( const XML_Node & rdfNode, XMP_OptionBits options )
{
	IgnoreParam(options);
	
	RDF_Parser parser ( &this->errorCallback );
	
	parser.RDF ( &this->tree, rdfNode );

}	// XMPMeta::ProcessRDF

// =================================================================================================
