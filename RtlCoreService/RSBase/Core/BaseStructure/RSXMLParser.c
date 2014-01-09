//
//  RSXMLParser.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/8/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSXMLParser.h"
#include "RSPrivate/gumbo/gumbo.h"

#include <RSCoreFoundation/RSRuntime.h>

RSInline BOOL __RSXMLIsSubClassOfNode(RSTypeRef id);
#define RAPIDXML_PARSE_ERROR(errorPtr, msg, textPtr) do {if((errorPtr)) {*error = RSErrorWithReason(RSStringWithCString((msg), RSStringEncodingUTF8));}}while(0);
//static void _RAPIDXML_PARSE_ERROR(RSErrorRef *error, const char *msg, char **text)
//{
//    RAPIDXML_PARSE_ERROR(error, msg, text);
//    HALTWithError(RSInvalidArgumentException, msg);
//}

enum RSXMLNodeTypeId
{
    RSXMLNodeDocument,      //!< A document node. Name and value are empty.
    RSXMLNodeElement,       //!< An element node. Name contains element name. Value contains text of first data node.
    RSXMLNodeData,          //!< A data node. Name is empty. Value contains data text.
    RSXMLNodeCData,         //!< A CDATA node. Name is empty. Value contains data text.
    RSXMLNodeComment,       //!< A comment node. Name is empty. Value contains comment text.
    RSXMLNodeDeclaration,   //!< A declaration node. Name and value are empty. Declaration parameters (version, encoding and standalone) are in node attributes.
    RSXMLNodeDocType,       //!< A DOCTYPE node. Name is empty. Value contains DOCTYPE text.
    RSXMLNodePI             //!< A PI node. Name contains target. Value contains instructions.
};
typedef enum RSXMLNodeTypeId RSXMLNodeTypeId;

///////////////////////////////////////////////////////////////////////
// Parsing flags

//! Parse flag instructing the parser to not create data nodes.
//! Text of first data node will still be placed in value of parent element, unless rapidxml::parse_no_element_values flag is also specified.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_no_data_nodes = 0x1;

//! Parse flag instructing the parser to not use text of first data node as a value of parent element.
//! Can be combined with other flags by use of | operator.
//! Note that child data nodes of element node take precendence over its value when printing.
//! That is, if element has one or more child data nodes <em>and</em> a value, the value will be ignored.
//! Use rapidxml::parse_no_data_nodes flag to prevent creation of data nodes if you want to manipulate data using values of elements.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_no_element_values = 0x2;

//! Parse flag instructing the parser to not place zero terminators after strings in the source text.
//! By default zero terminators are placed, modifying source text.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_no_string_terminators = 0x4;

//! Parse flag instructing the parser to not translate entities in the source text.
//! By default entities are translated, modifying source text.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_no_entity_translation = 0x8;

//! Parse flag instructing the parser to disable UTF-8 handling and assume plain 8 bit characters.
//! By default, UTF-8 handling is enabled.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_no_utf8 = 0x10;

//! Parse flag instructing the parser to create XML declaration node.
//! By default, declaration node is not created.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_declaration_node = 0x20;

//! Parse flag instructing the parser to create comments nodes.
//! By default, comment nodes are not created.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_comment_nodes = 0x40;

//! Parse flag instructing the parser to create DOCTYPE node.
//! By default, doctype node is not created.
//! Although W3C specification allows at most one DOCTYPE node, RapidXml will silently accept documents with more than one.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_doctype_node = 0x80;

//! Parse flag instructing the parser to create PI nodes.
//! By default, PI nodes are not created.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_pi_nodes = 0x100;

//! Parse flag instructing the parser to validate closing tag names.
//! If not set, name inside closing tag is irrelevant to the parser.
//! By default, closing tags are not validated.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_validate_closing_tags = 0x200;

//! Parse flag instructing the parser to trim all leading and trailing whitespace of data nodes.
//! By default, whitespace is not trimmed.
//! This flag does not cause the parser to modify source text.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_trim_whitespace = 0x400;

//! Parse flag instructing the parser to condense all whitespace runs of data nodes to a single space character.
//! Trimming of leading and trailing whitespace of data is controlled by rapidxml::parse_trim_whitespace flag.
//! By default, whitespace is not normalized.
//! If this flag is specified, source text will be modified.
//! Can be combined with other flags by use of | operator.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_normalize_whitespace = 0x800;

// Compound flags

//! Parse flags which represent default behaviour of the parser.
//! This is always equal to 0, so that all other flags can be simply ored together.
//! Normally there is no need to inconveniently disable flags by anding with their negated (~) values.
//! This also means that meaning of each flag is a <i>negation</i> of the default setting.
//! For example, if flag name is rapidxml::parse_no_utf8, it means that utf-8 is <i>enabled</i> by default,
//! and using the flag will disable it.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_default = 0;

//! A combination of parse flags that forbids any modifications of the source text.
//! This also results in faster parsing. However, note that the following will occur:
//! <ul>
//! <li>names and values of nodes will not be zero terminated, you have to use xml_base::name_size() and xml_base::value_size() functions to determine where name and value ends</li>
//! <li>entities will not be translated</li>
//! <li>whitespace will not be normalized</li>
//! </ul>
//! See xml_document::parse() function.
static const int parse_non_destructive = parse_no_string_terminators | parse_no_entity_translation;

//! A combination of parse flags resulting in fastest possible parsing, without sacrificing important data.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_fastest = parse_non_destructive | parse_no_data_nodes;

//! A combination of parse flags resulting in largest amount of data being extracted.
//! This usually results in slowest parsing.
//! <br><br>
//! See xml_document::parse() function.
static const int parse_full = parse_declaration_node | parse_comment_nodes | parse_doctype_node | parse_pi_nodes | parse_validate_closing_tags;

// Whitespace (space \n \r \t)
static const unsigned char lookup_tables_lookup_whitespace[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  0,  0,  // 0
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 1
    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 2
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 3
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 4
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 5
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 6
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 7
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 8
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 9
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // A
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // B
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // C
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // D
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // E
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0   // F
};

// Node name (anything but space \n \r \t / > ? \0)
static const unsigned char lookup_tables_lookup_node_name[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  0,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Text (i.e. PCDATA) (anything but < \0)
static const unsigned char lookup_tables_lookup_text[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Text (i.e. PCDATA) that does not require processing when ws normalization is disabled
// (anything but < \0 &)
static const unsigned char lookup_tables_lookup_text_pure_no_ws[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Text (i.e. PCDATA) that does not require processing when ws normalizationis is enabled
// (anything but < \0 & space \n \r \t)
static const unsigned char lookup_tables_lookup_text_pure_with_ws[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  0,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    0,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Attribute name (anything but space \n \r \t / < > = ? ! \0)
static const unsigned char lookup_tables_lookup_attribute_name[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  0,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Attribute data with single quote (anything but ' \0)
static const unsigned char lookup_tables_lookup_attribute_data_1[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Attribute data with single quote that does not require processing (anything but ' \0 &)
static const unsigned char lookup_tables_lookup_attribute_data_1_pure[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Attribute data with double quote (anything but " \0)
static const unsigned char lookup_tables_lookup_attribute_data_2[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Attribute data with double quote that does not require processing (anything but " \0 &)
static const unsigned char lookup_tables_lookup_attribute_data_2_pure[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 1
    1,  1,  0,  1,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 2
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 3
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 4
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 5
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 6
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 7
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 8
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 9
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // A
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // B
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // C
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // D
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // E
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1   // F
};

// Digits (dec and hex, 255 denotes end of numeric character reference)
static const unsigned char lookup_tables_lookup_digits[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 0
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 1
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 2
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  255,255,255,255,255,255,  // 3
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,  // 4
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 5
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,  // 6
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 7
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 8
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // 9
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // A
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // B
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // C
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // D
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  // E
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255   // F
};

// Upper case conversion
static const unsigned char lookup_tables_lookup_upcase[256] =
{
    // 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  A   B   C   D   E   F
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,   // 0
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,   // 1
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,   // 2
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,   // 3
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,   // 4
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,   // 5
    96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,   // 6
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 123,124,125,126,127,  // 7
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,  // 8
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,  // 9
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,  // A
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,  // B
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,  // C
    208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,  // D
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,  // E
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255   // F
};

// Detect whitespace character
typedef unsigned char (*__internal_lookup_tables) (char ch);
typedef unsigned char (*__internal_lookup_tables_quote) (unsigned char Quote, char ch);


RSInline unsigned char __internal_lookup_tables_lookup_whitespace(char ch)   // whitespace_pred
{
    return lookup_tables_lookup_whitespace[(unsigned char)(ch)];
}

// Detect node name character
RSInline unsigned char __internal_lookup_tables_lookup_node_name(char ch)    // node_name_pred
{
    return lookup_tables_lookup_node_name[(unsigned char)(ch)];
}

// Detect attribute name character
RSInline unsigned char __internal_lookup_tables_lookup_attribute_name(char ch)   // attribute_name_pred
{
    return lookup_tables_lookup_attribute_name[(unsigned char)(ch)];
}

// Detect text character (PCDATA)
RSInline unsigned char __internal_lookup_tables_lookup_text(char ch)       // text_pred
{
    return lookup_tables_lookup_text[(unsigned char)(ch)];
}

// Detect text character (PCDATA) that does not require processing
RSInline unsigned char __internal_lookup_tables_lookup_text_pure_no_ws(char ch)  // text_pure_no_ws_pred
{
    return lookup_tables_lookup_text_pure_no_ws[(unsigned char)(ch)];
}

// Detect text character (PCDATA) that does not require processing
RSInline unsigned char __internal_lookup_tables_lookup_text_pure_with_ws(char ch)    // text_pure_with_ws_pred
{
    return lookup_tables_lookup_text_pure_with_ws[(unsigned char)(ch)];
}

// Detect attribute value character
RSInline unsigned char __internal_lookup_tables_lookup_attribute_value_pred(unsigned char Quote, char ch)      // attribute_value_pred
{
    if (Quote == (unsigned char)('\''))
        return lookup_tables_lookup_attribute_data_1[(unsigned char)(ch)];
    if (Quote == (unsigned char)('\"'))
        return lookup_tables_lookup_attribute_data_2[(unsigned char)(ch)];
    return NO;       // Should never be executed, to avoid warnings on Comeau
}

// Detect attribute value character
RSInline unsigned char __internal_lookup_tables_lookup_attribute_value_pure_pred(unsigned char Quote, char ch)   // attribute_value_pure_pred
{
    if (Quote == (unsigned char)('\''))
        return lookup_tables_lookup_attribute_data_1_pure[(unsigned char)(ch)];
    if (Quote == (unsigned char)('\"'))
        return lookup_tables_lookup_attribute_data_2_pure[(unsigned char)(ch)];
    return NO;       // Should never be executed, to avoid warnings on Comeau
}

// Insert coded character, using UTF8 or 8-bit ASCII
static void __internal_insert_coded_character(int Flags, char **text, unsigned long code, RSErrorRef *error)  // insert_coded_character
{
    if (Flags & parse_no_utf8)
    {
        // Insert 8-bit ASCII character
        // Todo: possibly verify that code is less than 256 and use replacement char otherwise?
        (*text)[0] = (unsigned char)(code);
        (*text) += 1;
    }
    else
    {
        // Insert UTF8 sequence
        if (code < 0x80)    // 1 byte sequence
        {
            (*text)[0] = (unsigned char)(code);
            (*text) += 1;
        }
        else if (code < 0x800)  // 2 byte sequence
        {
            (*text)[1] = (unsigned char)((code | 0x80) & 0xBF); code >>= 6;
            (*text)[0] = (unsigned char)(code | 0xC0);
            (*text) += 2;
        }
        else if (code < 0x10000)    // 3 byte sequence
        {
            (*text)[2] = (unsigned char)((code | 0x80) & 0xBF); code >>= 6;
            (*text)[1] = (unsigned char)((code | 0x80) & 0xBF); code >>= 6;
            (*text)[0] = (unsigned char)(code | 0xE0);
            (*text) += 3;
        }
        else if (code < 0x110000)   // 4 byte sequence
        {
            (*text)[3] = (unsigned char)((code | 0x80) & 0xBF); code >>= 6;
            (*text)[2] = (unsigned char)((code | 0x80) & 0xBF); code >>= 6;
            (*text)[1] = (unsigned char)((code | 0x80) & 0xBF); code >>= 6;
            (*text)[0] = (unsigned char)(code | 0xF0);
            (*text) += 4;
        }
        else    // Invalid, only codes up to 0x10FFFF are allowed in Unicode
        {
            RAPIDXML_PARSE_ERROR(error, "invalid numeric character entity", text);
            return;
        }
    }
}

RSInline size_t __internal_measure(const char *p)
{
    const char *tmp = p;
    while (*tmp)
        ++tmp;
    return tmp - p;
}

RSInline BOOL __internal_compare(const char *p1, size_t size1, const char *p2, size_t size2, bool case_sensitive)
{
    if (size1 != size2)
        return NO;
    if (case_sensitive)
    {
        for (const char *end = p1 + size1; p1 < end; ++p1, ++p2)
            if (*p1 != *p2)
                return NO;
    }
    else
    {
        for (const char *end = p1 + size1; p1 < end; ++p1, ++p2)
            if (lookup_tables_lookup_upcase[(unsigned char)(*p1)] !=
                lookup_tables_lookup_upcase[(unsigned char)(*p2)])
                return NO;
    }
    return YES;
}

static void skip(__internal_lookup_tables stop, int Flags, char **text)
{
    char *tmp = *text;
    while (stop(*tmp))
        ++tmp;
    *text = tmp;
}

static void skip_quote(__internal_lookup_tables_quote stop, char q, int Flags, char **text)
{
    char *tmp = *text;
    while (stop(q, *tmp))
        ++tmp;
    *text = tmp;
}

static char *skip_and_expand_character_refs(__internal_lookup_tables StopPred, __internal_lookup_tables StopPredPure, int Flags, char **text, RSErrorRef *error)
{
    // If entity translation, whitespace condense and whitespace trimming is disabled, use plain skip
    if (Flags & parse_no_entity_translation &&
        !(Flags & parse_normalize_whitespace) &&
        !(Flags & parse_trim_whitespace))
    {
        skip(StopPred, Flags, text);
        return *text;
    }
    
    // Use simple skip until first modification is detected
    skip(StopPredPure, Flags, text);
    
    // Use translation skip
//    Ch *src = text;
//    Ch *dest = src;
    char *src = *text;
    char *dest = src;
    while (StopPred(*src))
    {
        // If entity translation is enabled
        if (!(Flags & parse_no_entity_translation))
        {
            // Test if replacement is needed
            if (src[0] == ('&'))
            {
                switch (src[1])
                {
                        
                        // &amp; &apos;
                    case ('a'):
                        if (src[2] == ('m') && src[3] == ('p') && src[4] == (';'))
                        {
                            *dest = ('&');
                            ++dest;
                            src += 5;
                            continue;
                        }
                        if (src[2] == ('p') && src[3] == ('o') && src[4] == ('s') && src[5] == (';'))
                        {
                            *dest = ('\'');
                            ++dest;
                            src += 6;
                            continue;
                        }
                        break;
                        
                        // &quot;
                    case ('q'):
                        if (src[2] == ('u') && src[3] == ('o') && src[4] == ('t') && src[5] == (';'))
                        {
                            *dest = ('"');
                            ++dest;
                            src += 6;
                            continue;
                        }
                        break;
                        
                        // &gt;
                    case ('g'):
                        if (src[2] == ('t') && src[3] == (';'))
                        {
                            *dest = ('>');
                            ++dest;
                            src += 4;
                            continue;
                        }
                        break;
                        
                        // &lt;
                    case ('l'):
                        if (src[2] == ('t') && src[3] == (';'))
                        {
                            *dest = ('<');
                            ++dest;
                            src += 4;
                            continue;
                        }
                        break;
                        
                        // &#...; - assumes ASCII
                    case ('#'):
                        if (src[2] == ('x'))
                        {
                            unsigned long code = 0;
                            src += 3;   // Skip &#x
                            while (1)
                            {
                                unsigned char digit = lookup_tables_lookup_digits[(unsigned char)(*src)];
                                if (digit == 0xFF)
                                    break;
                                code = code * 16 + digit;
                                ++src;
                            }
                            __internal_insert_coded_character(Flags, &dest, code, nil);    // Put character in output
                        }
                        else
                        {
                            unsigned long code = 0;
                            src += 2;   // Skip &#
                            while (1)
                            {
                                unsigned char digit = lookup_tables_lookup_digits[(unsigned char)(*src)];
                                if (digit == 0xFF)
                                    break;
                                code = code * 10 + digit;
                                ++src;
                            }
                            __internal_insert_coded_character(Flags, &dest, code, error);    // Put character in output
                            if (error && *error){
                                return nil;
                            }
                        }
                        if (*src == (';'))
                            ++src;
                        else
                        {
                            RAPIDXML_PARSE_ERROR(error, "expected ;", &src);
                            return *text;
                        }
                        continue;
                        
                        // Something else
                    default:
                        // Ignore, just copy '&' verbatim
                        break;
                        
                }
            }
        }
        
        // If whitespace condensing is enabled
        if (Flags & parse_normalize_whitespace)
        {
            // Test if condensing is needed
            if (__internal_lookup_tables_lookup_whitespace(*src))
            {
                *dest = (' '); ++dest;    // Put single space in dest
                ++src;                      // Skip first whitespace char
                // Skip remaining whitespace chars
                while (__internal_lookup_tables_lookup_whitespace(*src))
                    ++src;
                continue;
            }
        }
        
        // No replacement, only copy character
        *dest++ = *src++;
        
    }
    
    // Return new end
    *text = src;
    return dest;
}

static char *skip_and_expand_character_refs_quote(__internal_lookup_tables_quote StopPred, char sp1, __internal_lookup_tables_quote StopPredPure, char sp2, int Flags, char **text, RSErrorRef *error)
{
    // If entity translation, whitespace condense and whitespace trimming is disabled, use plain skip
    if (Flags & parse_no_entity_translation &&
        !(Flags & parse_normalize_whitespace) &&
        !(Flags & parse_trim_whitespace))
    {
        skip_quote(StopPred, sp1, Flags, text);
        return *text;
    }
    
    // Use simple skip until first modification is detected
    skip_quote(StopPredPure, sp2, Flags, text);
    
    // Use translation skip
    char *src = *text;
    char *dest = src;
    while (StopPred(sp1, *src))
    {
        // If entity translation is enabled
        if (!(Flags & parse_no_entity_translation))
        {
            // Test if replacement is needed
            if (src[0] == ('&'))
            {
                switch (src[1])
                {
                        
                        // &amp; &apos;
                    case ('a'):
                        if (src[2] == ('m') && src[3] == ('p') && src[4] == (';'))
                        {
                            *dest = ('&');
                            ++dest;
                            src += 5;
                            continue;
                        }
                        if (src[2] == ('p') && src[3] == ('o') && src[4] == ('s') && src[5] == (';'))
                        {
                            *dest = ('\'');
                            ++dest;
                            src += 6;
                            continue;
                        }
                        break;
                        
                        // &quot;
                    case ('q'):
                        if (src[2] == ('u') && src[3] == ('o') && src[4] == ('t') && src[5] == (';'))
                        {
                            *dest = ('"');
                            ++dest;
                            src += 6;
                            continue;
                        }
                        break;
                        
                        // &gt;
                    case ('g'):
                        if (src[2] == ('t') && src[3] == (';'))
                        {
                            *dest = ('>');
                            ++dest;
                            src += 4;
                            continue;
                        }
                        break;
                        
                        // &lt;
                    case ('l'):
                        if (src[2] == ('t') && src[3] == (';'))
                        {
                            *dest = ('<');
                            ++dest;
                            src += 4;
                            continue;
                        }
                        break;
                        
                        // &#...; - assumes ASCII
                    case ('#'):
                        if (src[2] == ('x'))
                        {
                            unsigned long code = 0;
                            src += 3;   // Skip &#x
                            while (1)
                            {
                                unsigned char digit = lookup_tables_lookup_digits[(unsigned char)(*src)];
                                if (digit == 0xFF)
                                    break;
                                code = code * 16 + digit;
                                ++src;
                            }
                            __internal_insert_coded_character(Flags, &dest, code, nil);    // Put character in output
                        }
                        else
                        {
                            unsigned long code = 0;
                            src += 2;   // Skip &#
                            while (1)
                            {
                                unsigned char digit = lookup_tables_lookup_digits[(unsigned char)(*src)];
                                if (digit == 0xFF)
                                    break;
                                code = code * 10 + digit;
                                ++src;
                            }
                            __internal_insert_coded_character(Flags, &dest, code, nil);    // Put character in output
                        }
                        if (*src == (';'))
                            ++src;
                        else
                        {
                            RAPIDXML_PARSE_ERROR(error, "expected ;", text);
                            return *text;
                        }
                        continue;
                        
                        // Something else
                    default:
                        // Ignore, just copy '&' verbatim
                        break;
                        
                }
            }
        }
        
        // If whitespace condensing is enabled
        if (Flags & parse_normalize_whitespace)
        {
            // Test if condensing is needed
            if (__internal_lookup_tables_lookup_whitespace(*src))
            {
                *dest = (' '); ++dest;    // Put single space in dest
                ++src;                      // Skip first whitespace char
                // Skip remaining whitespace chars
                while (__internal_lookup_tables_lookup_whitespace(*src))
                    ++src;
                continue;
            }
        }
        
        // No replacement, only copy character
        *dest++ = *src++;
        
    }
    
    // Return new end
    *text = src;
    return dest;
}

#pragma mark -
#pragma mark RSXMLNode

struct __RSXMLNode
{
    RSRuntimeBase _base;
    RSUInteger _nodeType;
    RSUInteger _namespace;
    
    RSStringRef _name;
    RSTypeRef _objectValue;
    
    struct __RSXMLNode *_parent;
};

RSInline RSUInteger __RSXMLNodeGetNodeTypeId(RSXMLNodeRef node) {
    return node->_nodeType;
}

RSInline void __RSXMLNodeSetNodeTypeId(RSXMLNodeRef node, RSUInteger nodeType) {
    ((struct __RSXMLNode *)node)->_nodeType = nodeType;
}

RSInline RSUInteger __RSXMLNodeGetNodeNamespace(RSXMLNodeRef node) {
    return node->_namespace;
}

RSInline void __RSXMLNodeSetNodeNamespace(RSXMLNodeRef node, RSUInteger namespace) {
    ((struct __RSXMLNode *)node)->_namespace = namespace;
}

RSInline RSStringRef __RSXMLNodeGetName(RSXMLNodeRef node)
{
    return node->_name;
}

RSInline void __RSXMLNodeSetName(RSXMLNodeRef node, RSStringRef name) {
    if (node->_name) RSRelease(node->_name);
    if (name) ((struct __RSXMLNode *)node)->_name = RSRetain(name);
}

RSInline RSTypeRef __RSXMLNodeGetValue(RSXMLNodeRef node)
{
    return node->_objectValue;
}

RSInline void __RSXMLNodeSetObjectValue(RSXMLNodeRef node, RSTypeRef objectValue)
{
    if (node->_objectValue) RSRelease(node->_objectValue);
    if (objectValue) ((struct __RSXMLNode *)node)->_objectValue = RSRetain(objectValue);
}


RSInline RSXMLNodeRef __RSXMLNodeGetParent(RSXMLNodeRef node) {
    return node->_parent;
}

RSInline void __RSXMLNodeSetParent(RSXMLNodeRef node, RSXMLNodeRef parent) {
    // FIXME: weak or strong?
    ((struct __RSXMLNode *)node)->_parent = (struct __RSXMLNode *)parent;
}

static void __RSXMLNodeClassInit(RSTypeRef rs)
{
    
}

static RSTypeRef __RSXMLNodeClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}


static void __RSXMLNodeClassDeallocate(RSTypeRef rs)
{
    RSXMLNodeRef node = (RSXMLNodeRef)rs;
    if (node->_name) RSRelease(node->_name);
    ((struct __RSXMLNode *)node)->_name = nil;
    if (node->_objectValue) RSRelease(node->_objectValue);
    ((struct __RSXMLNode *)node)->_objectValue = nil;
    ((struct __RSXMLNode *)node)->_parent = nil;    // weak ? strong
}

static BOOL __RSXMLNodeClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSXMLNodeRef RSXMLNode1 = (RSXMLNodeRef)rs1;
    RSXMLNodeRef RSXMLNode2 = (RSXMLNodeRef)rs2;
    BOOL result = NO;
    if (RSXMLNode1->_nodeType == RSXMLNode2->_nodeType &&
        RSEqual(RSXMLNode1->_name, RSXMLNode2->_name) &&
        RSEqual(RSXMLNode1->_objectValue, RSXMLNode2->_objectValue))
        result = YES;
    return result;
}

static RSHashCode __RSXMLNodeClassHash(RSTypeRef rs)
{
    RSXMLNodeRef node = (RSXMLNodeRef)rs;
    return RSHash(node->_name) ^ RSHash(node->_objectValue);
}

static RSStringRef __RSXMLNodeClassDescription(RSTypeRef rs)
{
    RSXMLNodeRef node = (RSXMLNodeRef)rs;
    RSStringRef description = nil;
    if(__RSXMLNodeGetName(node) && __RSXMLNodeGetValue(node)) {
        RSStringRef format = nil;
        if (GUMBO_TAG_HTML == __RSXMLNodeGetNodeTypeId(node)) {
            format = RSSTR("%r = %r");
        } else {
            format = RSSTR("%r%r");
        }
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, format, __RSXMLNodeGetName(node), __RSXMLNodeGetValue(node));
    }
    else if (__RSXMLNodeGetName(node)) description = RSCopy(RSAllocatorSystemDefault, __RSXMLNodeGetName(node));
    else if (__RSXMLNodeGetValue(node)) description = RSCopy(RSAllocatorSystemDefault, __RSXMLNodeGetValue(node));
    else description = RSStringGetEmptyString();
    return description;
}

static RSRuntimeClass __RSXMLNodeClass =
{
    _RSRuntimeScannedObject,
    "RSXMLNode",
    __RSXMLNodeClassInit,
    __RSXMLNodeClassCopy,
    __RSXMLNodeClassDeallocate,
    __RSXMLNodeClassEqual,
    __RSXMLNodeClassHash,
    __RSXMLNodeClassDescription,
    nil,
    nil
};

static RSTypeID _RSXMLNodeTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSXMLNodeGetTypeID()
{
    return _RSXMLNodeTypeID;
}

RSPrivate void __RSXMLNodeInitialize()
{
    _RSXMLNodeTypeID = __RSRuntimeRegisterClass(&__RSXMLNodeClass);
    __RSRuntimeSetClassTypeID(&__RSXMLNodeClass, _RSXMLNodeTypeID);
}

RSPrivate void __RSXMLNodeDeallocate()
{
    //    <#do your finalize operation#>
}

static RSXMLNodeRef __RSXMLNodeCreateInstance(RSAllocatorRef allocator, RSXMLNodeTypeId typeId, RSStringRef name, RSTypeRef value)
{
    struct __RSXMLNode * instance = (struct __RSXMLNode *)__RSRuntimeCreateInstance(allocator, _RSXMLNodeTypeID, sizeof(struct __RSXMLNode) - sizeof(RSRuntimeBase));
    __RSXMLNodeSetNodeTypeId(instance, typeId);
    if (name) instance->_name = RSRetain(name);
    if (value) instance->_objectValue = RSRetain(value);
    
    return instance;
}

static void __RSXMLNodeGenericValidInstance(RSTypeRef obj)
{
    __RSRuntimeCheckInstanceAvailable(obj);
    if (!__RSXMLIsSubClassOfNode(obj))
    {
        HALTWithError(RSGenericException, function);
    }
    return;
}

#pragma mark -
#pragma mark RSXMLNode Public

RSExport RSStringRef RSXMLNodeGetName(RSXMLNodeRef node)
{
    if (!node) return nil;
    __RSXMLNodeGenericValidInstance(node);
    return __RSXMLNodeGetName(node);
}

RSExport void RSXMLNodeSetName(RSXMLNodeRef node, RSStringRef name)
{
    if (!node) return;
    __RSXMLNodeGenericValidInstance(node);
    __RSXMLNodeSetName(node, name);
}

RSExport RSTypeRef RSXMLNodeGetValue(RSXMLNodeRef node)
{
    if (!node) return nil;
    __RSXMLNodeGenericValidInstance(node);
    return __RSXMLNodeGetValue(node);
}

RSExport void RSXMLNodeSetObjectValue(RSXMLNodeRef node, RSTypeRef value)
{
    if (!node) return ;
    __RSXMLNodeGenericValidInstance(node);
    __RSXMLNodeSetObjectValue(node, value);
}

RSExport RSXMLNodeRef RSXMLNodeGetParent(RSXMLNodeRef node)
{
    if (!node) return nil;
    __RSXMLNodeGenericValidInstance(node);
    return __RSXMLNodeGetParent(node);
}

RSExport RSUInteger RSXMLNodeGetNodeTypeId(RSXMLNodeRef node)
{
    if (!node) return 0;
    __RSXMLNodeGenericValidInstance(node);
    return __RSXMLNodeGetNodeTypeId(node);
}

RSExport RSStringRef RSXMLNodeGetStringValue(RSXMLNodeRef node)
{
    if (!node) return nil;
    __RSXMLNodeGenericValidInstance(node);
    return RSAutorelease(RSDescription(__RSXMLNodeGetValue(node)));
}



#pragma mark -
#pragma mark RSXMLElement

struct __RSXMLElement
{
    struct __RSXMLNode _node;
    RSMutableArrayRef _children;
    RSMutableDictionaryRef _attributes;
};

static void __RSXMLElementRemoveAllAttribtues(RSXMLElementRef element)
{
    RSRelease(element->_attributes);
    ((struct __RSXMLElement *)element)->_attributes = nil;
}

RSInline RSDictionaryRef __RSXMLElementGetAttributes(RSXMLElementRef element) {
    return element->_attributes;
}

RSInline void __RSXMLElementAddAttribute(RSXMLElementRef element, RSXMLNodeRef attribute) {
    if (!__RSXMLElementGetAttributes(element)) {
        ((struct __RSXMLElement *)element)->_attributes = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    }
    RSDictionarySetValue((RSMutableDictionaryRef)__RSXMLElementGetAttributes(element), __RSXMLNodeGetName(attribute), attribute);
}

static void __RSXMLElementRemoveAttribtueForName(RSXMLElementRef element, RSStringRef name) {
    RSMutableDictionaryRef attributes = (RSMutableDictionaryRef)__RSXMLElementGetAttributes(element);
    RSDictionaryRemoveValue(attributes, name);
}

static void __RSXMLElementClassInit(RSTypeRef rs)
{
    // call super
    __RSXMLNodeClassInit(rs);
}

static RSTypeRef __RSXMLElementClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSXMLElementClassDeallocate(RSTypeRef rs)
{
    RSXMLElementRef element = (RSXMLElementRef)rs;
    if (element->_attributes) __RSXMLElementRemoveAllAttribtues(element);
    if (element->_children) RSRelease(element->_children);
    __RSXMLNodeClassDeallocate(rs); // call super
}

static BOOL __RSXMLElementClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSXMLElementRef RSXMLElement1 = (RSXMLElementRef)rs1;
    RSXMLElementRef RSXMLElement2 = (RSXMLElementRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSXMLElement1->_attributes, RSXMLElement2->_attributes);
    
    return result;
}

static RSHashCode __RSXMLElementClassHash(RSTypeRef rs)
{
    RSXMLElementRef element = (RSXMLElementRef)rs;
    return RSHash(element->_attributes);
}

//static void __RSXMLElementClassDescriptionForAttributes(const void *key, const void *value, void *context)
//{
//    RSMutableStringRef ms = (RSMutableStringRef)context;
//    RSStringRef description = RSDescription(value);
//    RSStringAppendStringWithFormat(ms, RSSTR("\t%r,\n"), description);
//    RSRelease(description);
//}
//        RSDictionaryApplyFunction(element->_attributes, __RSXMLElementClassDescriptionForAttributes, attributes);

static RSStringRef __RSXMLElementClassDescription(RSTypeRef rs)
{
    RSXMLElementRef element = (RSXMLElementRef)rs;
    RSMutableStringRef attributes = nil;
    if (element->_attributes) {
        attributes = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
        __block RSUInteger idx = 0;
        RSUInteger cnt = RSDictionaryGetCount(element->_attributes);
        RSStringRef formatNormal = RSSTR(" %r=\"%r\""), format0 = RSSTR("%r=\"%r\"");
        RSDictionaryApplyBlock(element->_attributes, ^(const void *key, const void *value, BOOL *stop) {
            RSStringRef format = nil;
            if (idx && idx < cnt) {
                format = formatNormal;
            } else if (idx == 0) {
                format = format0;
            }
            idx++;
            RSStringRef description = RSDescription(__RSXMLNodeGetValue((RSXMLNodeRef)value));
            RSStringAppendStringWithFormat(attributes, format, key, description);
//            RSShow(RSStringWithFormat(RSSTR("[attributes] = %r"), description));
            RSRelease(description);
        });
    }
    RSMutableStringRef children = nil;
    if (element->_children && RSArrayGetCount(element->_children)) {
        children = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
        RSArrayApplyBlock(element->_children, RSMakeRange(0, RSArrayGetCount(element->_children)), ^(RSTypeRef value, RSUInteger idx, BOOL *isStop) {
            RSXMLElementRef element = (RSXMLElementRef)value;
            RSStringRef description = RSDescription(element);
            if (description) {
                RSStringAppendStringWithFormat(children, RSSTR("%r"), description);
//                RSShow(RSStringWithFormat(RSSTR("[children] = %r"), description));
                RSRelease(description);
            }
        });
    }
    RSStringRef description = nil;
    if (element->_attributes && (element->_children && RSArrayGetCount(element->_children))) {
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%r %r>%r</%r>"), __RSXMLNodeGetName((RSXMLNodeRef)element), attributes, children, __RSXMLNodeGetName((RSXMLNodeRef)element));
    } else if (element->_attributes && element->_node._objectValue) {
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%r %r>%r</%r>"), __RSXMLNodeGetName((RSXMLNodeRef)element), attributes, element->_node._objectValue, __RSXMLNodeGetName((RSXMLNodeRef)element));
    } else if (element->_attributes) {
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%r %r></%r>"), __RSXMLNodeGetName((RSXMLNodeRef)element), attributes, __RSXMLNodeGetName((RSXMLNodeRef)element));
    } else if ((element->_children && RSArrayGetCount(element->_children))) {
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%r>%r</%r>"), __RSXMLNodeGetName((RSXMLNodeRef)element), children, __RSXMLNodeGetName((RSXMLNodeRef)element));
    } else if (element->_node._objectValue) {
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%r>%r</%r>"), __RSXMLNodeGetName((RSXMLNodeRef)element), element->_node._objectValue, __RSXMLNodeGetName((RSXMLNodeRef)element));
    }
    if (attributes) RSRelease(attributes);
    if (children) RSRelease(children);
    return description;
}

static RSRuntimeClass __RSXMLElementClass =
{
    _RSRuntimeScannedObject,
    "RSXMLElement",
    __RSXMLElementClassInit,
    __RSXMLElementClassCopy,
    __RSXMLElementClassDeallocate,
    __RSXMLElementClassEqual,
    __RSXMLElementClassHash,
    __RSXMLElementClassDescription,
    nil,
    nil
};

static RSTypeID _RSXMLElementTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSXMLElementGetTypeID()
{
    return _RSXMLElementTypeID;
}

RSPrivate void __RSXMLElementInitialize()
{
    _RSXMLElementTypeID = __RSRuntimeRegisterClass(&__RSXMLElementClass);
    __RSRuntimeSetClassTypeID(&__RSXMLElementClass, _RSXMLElementTypeID);
}

RSPrivate void __RSXMLElementDeallocate()
{
    //    <#do your finalize operation#>

}

static RSXMLElementRef __RSXMLElementCreateInstance(RSAllocatorRef allocator)
{
    RSXMLElementRef instance = (RSXMLElementRef)__RSRuntimeCreateInstance(allocator, _RSXMLElementTypeID, sizeof(struct __RSXMLElement) - sizeof(RSRuntimeBase));
    __RSXMLNodeSetNodeTypeId((RSXMLNodeRef)instance, RSXMLNodeElement);
    
    return instance;
}

#pragma mark -
#pragma mark RSXMLElement Attributes

RSExport void RSXMLElementAddAttribute(RSXMLElementRef element, RSXMLNodeRef attribute)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    __RSXMLElementAddAttribute(element, attribute);
}

RSExport void RSXMLElementRemoveAttributeForName(RSXMLElementRef element, RSStringRef name)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    __RSXMLElementRemoveAttribtueForName(element, name);
}

RSExport RSArrayRef RSXMLElementGetAttributes(RSXMLElementRef element)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    return RSAutorelease(RSDictionaryCopyAllValues(__RSXMLElementGetAttributes(element)));
}

RSInline RSXMLNodeRef __RSXMLElementGetAttributeForName(RSXMLElementRef element, RSStringRef name)
{
    return (RSXMLNodeRef)RSDictionaryGetValue(__RSXMLElementGetAttributes(element), name);
}

RSExport RSXMLNodeRef RSXMLElementGetAttributeForName(RSXMLElementRef element, RSStringRef name)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    return  __RSXMLElementGetAttributeForName(element, name);
}

RSExport void RSXMLElementSetAttributes(RSXMLElementRef element, RSArrayRef attributes)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (attributes)
    {
        RSUInteger cnt = RSArrayGetCount(attributes);
        for (RSUInteger idx = 0; idx < cnt; ++idx)
        {
            RSXMLNodeRef obj = nil;
            RSTypeID id = RSGetTypeID(obj = (RSXMLNodeRef)RSArrayObjectAtIndex(attributes, idx));
            if (id == _RSXMLElementTypeID ||
                id == _RSXMLNodeTypeID) {
                __RSXMLElementAddAttribute(element, obj);
            }
        }
    }
}

#pragma mark -
#pragma mark RSXMLElement Elements

static RSMutableArrayRef __RSXMLElementGetElementsForName(RSXMLElementRef element, RSMutableArrayRef stores, RSStringRef name)
{
    if (!element->_children) return stores;
    RSUInteger cnt = RSArrayGetCount(element->_children);
    
    RSArrayApplyBlock(element->_children, RSMakeRange(0, cnt), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        if (RSEqual(__RSXMLNodeGetName((RSXMLNodeRef)value), name)) {
            RSArrayAddObject(stores, value);
        }
    });
    return stores;
}

RSExport RSArrayRef RSXMLElementGetElementsForName(RSXMLElementRef element, RSStringRef name)
{
    if (!element) return nil;
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    return RSAutorelease(__RSXMLElementGetElementsForName(element, array, name));
}

#pragma mark -
#pragma mark RSXMLElement Child

RSExport void RSXMLElementInsertChild(RSXMLElementRef element, RSXMLNodeRef child, RSIndex atIndex)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (!child) return;
    if (!element->_children) ((struct __RSXMLElement *)element)->_children = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    if (element->_children) {
        RSArrayInsertObjectAtIndex(element->_children, atIndex, child);
    }
}

RSExport void RSXMLElementInsertChildren(RSXMLElementRef element, RSArrayRef children, RSIndex atIndex)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (!children || !RSArrayGetCount(children)) return;
    if (!element->_children) ((struct __RSXMLElement *)element)->_children = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    if (element->_children) {
        RSUInteger cnt = RSArrayGetCount(children);
        for (RSUInteger idx = 0; idx < cnt; idx++) {
            RSArrayInsertObjectAtIndex(element->_children, atIndex + idx, RSArrayObjectAtIndex(children, idx));
        }
    }
}

RSExport void RSXMLElementRemoveChild(RSXMLElementRef element, RSXMLNodeRef child, RSIndex atIndex)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (!child) return;
    if (!element->_children) ((struct __RSXMLElement *)element)->_children = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    if (element->_children) {
        RSArrayRemoveObjectAtIndex(element->_children, atIndex);
    }
}

RSExport void RSXMLElementSetChildren(RSXMLElementRef element, RSArrayRef children)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (element->_children) RSRelease(element->_children);
    ((struct __RSXMLElement *)element)->_children = nil;
    if (children) ((struct __RSXMLElement *)element)->_children = RSMutableCopy(RSAllocatorSystemDefault, children);
}

RSExport void RSXMLElementAddChild(RSXMLElementRef element, RSXMLNodeRef child)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (!child) return;
    if (!element->_children) ((struct __RSXMLElement *)element)->_children = RSArrayCreateMutable(RSAllocatorSystemDefault, 1);
    RSArrayAddObject(element->_children, child);
}

RSExport void RSXMLElementReplaceChild(RSXMLElementRef element, RSXMLNodeRef child, RSIndex atIndex)
{
    __RSGenericValidInstance(element, _RSXMLElementTypeID);
    if (!child) return;
    if (!element->_children || !RSArrayGetCount(element->_children)) return;
    RSArrayReplaceObject(element->_children, RSMakeRange(atIndex, 1), (RSTypeRef *)&child, 1);
}

#pragma mark -
#pragma mark RSXMLDocument

struct __RSXMLDocument
{
    struct __RSXMLNode _node;
    RSMutableArrayRef _children;
    RSXMLElementRef _rootElement;
};

RSInline RSXMLElementRef __RSXMLDocumentGetRootElement(RSXMLDocumentRef document)
{
    return document->_rootElement;
}

RSInline void __RSXMLDocumentSetRootElement(RSXMLDocumentRef document, RSXMLElementRef rootElement)
{
    if (document->_rootElement) RSRelease(document->_rootElement);
    ((struct __RSXMLDocument *)document)->_rootElement = nil;
    if (rootElement) ((struct __RSXMLDocument *)document)->_rootElement = (RSXMLElementRef)RSRetain(rootElement);
}

static void __RSXMLDocumentInsertChild(RSXMLDocumentRef document, RSXMLNodeRef child, RSIndex atIndex)
{
    RSArrayInsertObjectAtIndex(document->_children, atIndex, child);
}

static void __RSXMLDocumentInsertChildren(RSXMLDocumentRef document, RSArrayRef children, RSIndex atIndex)
{
    RSUInteger cnt = RSArrayGetCount(children);
    for (RSUInteger idx = 0; idx < cnt; ++idx) {
        RSXMLNodeRef child = (RSXMLNodeRef)RSArrayObjectAtIndex(children, idx);
        __RSXMLDocumentInsertChild(document, child, atIndex + idx);
    }
}

static void __RSXMLDocumentRemoveChild(RSXMLDocumentRef document, RSIndex atIndex)
{
    RSArrayRemoveObjectAtIndex(document->_children, atIndex);
}

static void __RSXMLDocumentSetChildren(RSXMLDocumentRef document, RSArrayRef children)
{
    if (document->_children) RSRelease(document->_children);
    ((struct __RSXMLDocument *)document)->_children = nil;
    if (children) ((struct __RSXMLDocument *)document)->_children = RSMutableCopy(RSAllocatorSystemDefault, children);
    else ((struct __RSXMLDocument *)document)->_children = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
}

static void __RSXMLDocumentAddChild(RSXMLDocumentRef document, RSXMLNodeRef child)
{
    RSArrayAddObject(document->_children, child);
}

static void __RSXMLDocumentReplaceChild(RSXMLDocumentRef document, RSXMLNodeRef child, RSIndex atIndex)
{
    RSArrayReplaceObject(document->_children, RSMakeRange(atIndex, 1), (RSTypeRef *)&child, 1);
}

static void __RSXMLDocumentClassInit(RSTypeRef rs)
{
    // call super
    __RSXMLNodeClassInit(rs);
    RSXMLDocumentRef document = (RSXMLDocumentRef)rs;
    ((struct __RSXMLDocument *)document)->_children = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
}

static RSTypeRef __RSXMLDocumentClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSXMLDocumentClassDeallocate(RSTypeRef rs)
{
    RSXMLDocumentRef document = (RSXMLDocumentRef)rs;
    if (document->_children) RSRelease(document->_children);
    if (document->_rootElement) RSRelease(document->_rootElement);
    ((struct __RSXMLDocument *)document)->_children = nil;
    ((struct __RSXMLDocument *)document)->_rootElement = nil;
}

static BOOL __RSXMLDocumentClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSXMLDocumentRef RSXMLDocument1 = (RSXMLDocumentRef)rs1;
    RSXMLDocumentRef RSXMLDocument2 = (RSXMLDocumentRef)rs2;
    BOOL result = NO;
    
    result =  RSEqual(RSXMLDocument1->_rootElement, RSXMLDocument2->_rootElement);
    if (result == YES) result = RSEqual(RSXMLDocument1->_children, RSXMLDocument2->_children);
    return result;
}

static RSHashCode __RSXMLDocumentClassHash(RSTypeRef rs)
{
    RSXMLDocumentRef document = (RSXMLDocumentRef)rs;
    return RSHash(document->_rootElement);
}

static RSStringRef __RSXMLDocumentClassDescription(RSTypeRef rs)
{
    RSXMLDocumentRef document = (RSXMLDocumentRef)rs;
    
    RSStringRef description = nil;
    RSUInteger namespace = __RSXMLNodeGetNodeNamespace((RSXMLNodeRef)document);
    if (namespace == GUMBO_NAMESPACE_HTML) {
        RSStringRef prefix = RSSTR("<!DOCTYPE HTML>\n");
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%r"), prefix, document->_rootElement);
    } else {
        description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r"), document->_rootElement);
    }
    return description;
}

static RSRuntimeClass __RSXMLDocumentClass =
{
    _RSRuntimeScannedObject,
    "RSXMLDocument",
    __RSXMLDocumentClassInit,
    __RSXMLDocumentClassCopy,
    __RSXMLDocumentClassDeallocate,
    __RSXMLDocumentClassEqual,
    __RSXMLDocumentClassHash,
    __RSXMLDocumentClassDescription,
    nil,
    nil
};

static RSTypeID _RSXMLDocumentTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSXMLDocumentGetTypeID()
{
    return _RSXMLDocumentTypeID;
}

RSPrivate void __RSXMLDocumentInitialize()
{
    _RSXMLDocumentTypeID = __RSRuntimeRegisterClass(&__RSXMLDocumentClass);
    __RSRuntimeSetClassTypeID(&__RSXMLDocumentClass, _RSXMLDocumentTypeID);
}

RSPrivate void __RSXMLDocumentDeallocate()
{
    //    <#do your finalize operation#>
}

static RSXMLDocumentRef __RSXMLDocumentCreateInstance(RSAllocatorRef allocator, RSXMLElementRef root)
{
    struct __RSXMLDocument *instance = (struct __RSXMLDocument *)__RSRuntimeCreateInstance(allocator, _RSXMLDocumentTypeID, sizeof(struct __RSXMLDocument) - sizeof(RSRuntimeBase));
    
    __RSXMLNodeSetNodeTypeId((RSXMLNodeRef)instance, RSXMLNodeDocument);
    if (root) instance->_rootElement = (RSXMLElementRef)RSRetain(root);
    
    return instance;
}

RSExport void RSXMLDocumentInsertChild(RSXMLDocumentRef document, RSXMLNodeRef child, RSIndex atIndex)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    if (!child)
    {
        HALTWithError(RSInvalidArgumentException, "node is nil");
        return;
    }
    if (!__RSXMLIsSubClassOfNode(child))
    {
        HALTWithError(RSInvalidArgumentException, "node is not a RSXMLNode");
        return ;
    }
    return __RSXMLDocumentInsertChild(document, child, atIndex);
}

static void __RSXMLNodeCheckArrayApplierFunction (const void *value, void *context)
{
    if (!__RSXMLIsSubClassOfNode((RSXMLNodeRef)value))
    {
        HALTWithError(RSInvalidArgumentException, "node is not a RSXMLNode");
        return ;
    }
}

RSExport RSXMLElementRef RSXMLDocumentGetRootElement(RSXMLDocumentRef document)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    return __RSXMLDocumentGetRootElement(document);
}

RSExport void RSXMLDocumentSetRootElement(RSXMLDocumentRef document, RSXMLElementRef rootElement)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    return __RSXMLDocumentSetRootElement(document, rootElement);
}

RSExport void RSXMLDocumentInsertChildren(RSXMLDocumentRef document, RSArrayRef children, RSIndex atIndex)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    if (!children)
    {
        HALTWithError(RSInvalidArgumentException, "node is nil");
        return;
    }
    RSUInteger cnt = RSArrayGetCount(children);
    if (cnt) {
        RSArrayApplyFunction(children, RSMakeRange(0, cnt), __RSXMLNodeCheckArrayApplierFunction, nil);
        return __RSXMLDocumentInsertChildren(document, children, atIndex);
    }
}

RSExport void RSXMLDocumentRemoveChild(RSXMLDocumentRef document, RSIndex atIndex)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    return __RSXMLDocumentRemoveChild(document, atIndex);
}

RSExport void RSXMLDocumentSetChildren(RSXMLDocumentRef document, RSArrayRef children)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    if (children)
        RSArrayApplyFunction(children, RSMakeRange(0, RSArrayGetCount(children)), __RSXMLNodeCheckArrayApplierFunction, nil);
    return __RSXMLDocumentSetChildren(document, children);
}

RSExport void RSXMLDocumentAddChild(RSXMLDocumentRef document, RSXMLNodeRef child)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    if (!__RSXMLIsSubClassOfNode(child))
    {
        HALTWithError(RSInvalidArgumentException, "node is not a RSXMLNode");
        return;
    }
    return __RSXMLDocumentAddChild(document, child);
}

RSExport void RSXMLDocumentReplaceChild(RSXMLDocumentRef document, RSXMLNodeRef child, RSIndex atIndex)
{
    __RSGenericValidInstance(document, _RSXMLDocumentTypeID);
    if (!__RSXMLIsSubClassOfNode(child))
    {
        HALTWithError(RSInvalidArgumentException, "node is not a RSXMLNode");
        return;
    }
    return __RSXMLDocumentReplaceChild(document, child, atIndex);
}

RSExport RSDataRef RSXMLDocumentGetXMLData(RSXMLDocumentRef document)
{
    return RSDataWithString(RSAutorelease(RSDescription(document)), RSStringEncodingUTF8);
}

#pragma mark -
#pragma mark RSXMLParser


RSPrivate void __RSXMLParserInitialize()
{
    __RSXMLNodeInitialize();
    __RSXMLElementInitialize();
    __RSXMLDocumentInitialize();
}


static void __xml_parse_bom(char **text)
{
    // UTF-8?
    if ((unsigned char)((*text)[0]) == 0xEF &&
        (unsigned char)((*text)[1]) == 0xBB &&
        (unsigned char)((*text)[2]) == 0xBF)
    {
        *text += 3;      // Skup utf-8 bom
    }
}

static char __xml_parse_and_append_data(int Flags, RSXMLElementRef node, char **text, char *contents_start, RSErrorRef *error);
static RSXMLNodeRef __xml_parse_node(int Flags, char **text, RSErrorRef *error);

static void __xml_parse_node_contents(int Flags, char **text, RSXMLElementRef node, RSErrorRef *error)
{
    // For all children and text
    while (1)
    {
        // Skip whitespace between > and node contents
        char *contents_start = *text;      // Store start of node contents before whitespace is skipped
        
//        skip<whitespace_pred, Flags>(text);
        skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
        char next_char = *(*text);
        
        // After data nodes, instead of continuing the loop, control jumps here.
        // This is because zero termination inside parse_and_append_data() function
        // would wreak havoc with the above code.
        // Also, skipping whitespace after data nodes is unnecessary.
    after_data_node:
        
        // Determine what comes next: node closing, child node, data node, or 0?
        switch (next_char)
        {
                
                // Node closing or child node
            case ('<'):
                if ((*text)[1] == ('/'))
                {
                    // Node closing
                    (*text) += 2;      // Skip '</'
                    if (Flags & parse_validate_closing_tags)
                    {
                        // Skip and validate closing tag name
                        char *closing_name = *text;
//                        skip<node_name_pred, Flags>(text);
                        skip(__internal_lookup_tables_lookup_node_name, Flags, text);
                        
//                        if (!internal::compare(node->name(), node->name_size(), closing_name, text - closing_name, YES))
//                            RAPIDXML_PARSE_ERROR(nil, "invalid closing tag name", text);
                        if (__internal_compare(RSStringGetCStringPtr(RSXMLNodeGetName((RSXMLNodeRef)node), RSStringEncodingASCII), RSStringGetLength(RSXMLNodeGetName((RSXMLNodeRef)node)), closing_name, (*text) - closing_name, YES))
                        {
                            RAPIDXML_PARSE_ERROR(error, "invalid closing tag name", text);
                            return;
                        }
                    }
                    else
                    {
                        // No validation, just skip name
//                        skip<node_name_pred, Flags>(text);
                        skip(__internal_lookup_tables_lookup_node_name, Flags, text);
                    }
                    // Skip remaining whitespace after node name
//                    skip<whitespace_pred, Flags>(text);
                    skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
                    if (*(*text) != ('>'))
                    {
                        RAPIDXML_PARSE_ERROR(error, "expected >", text);
                        return;
                    }
                    ++(*text);     // Skip '>'
                    return;     // Node closed, finished parsing contents
                }
                else
                {
                    // Child node
                    ++(*text);     // Skip '<'
                    RSXMLNodeRef child = __xml_parse_node(Flags, text, error);
                    if (child) {
                        RSXMLElementAddChild(node, child);
                        RSRelease(child);
                    }
//                    if (xml_node<Ch> *child = parse_node<Flags>(text))
//                        node->append_node(child);
                }
                break;
                
                // End of data - error
            case ('\0'):
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                return;
            }
                
                // Data node
            default:
                next_char = __xml_parse_and_append_data(Flags, node, text, contents_start, error);
                goto after_data_node;   // Bypass regular processing after data nodes
                
        }
    }
}

static void __xml_parse_node_attributes(int Flags, char **text, RSXMLElementRef element, RSErrorRef *error)
{
    // For all attributes
    
    while (__internal_lookup_tables_lookup_attribute_name(**text))
    {
        // Extract attribute name
        char *name = *text;
        ++(*text);     // Skip first character of attribute name
        skip(__internal_lookup_tables_lookup_attribute_name, Flags, text);
        if ((*text) == name)
        {
            RAPIDXML_PARSE_ERROR(error, "expected attribute name", &name);
            return;
        }
        
        // Create new attribute
//        xml_attribute<Ch> *attribute = this->allocate_attribute();
//        attribute->name(name, text - name);
//        node->append_attribute(attribute);
        
        RSXMLNodeRef attribute = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, RSXMLNodeElement, nil, nil);
        RSStringRef attributeName = RSStringCreateWithBytes(RSAllocatorSystemDefault, (UTF8Char *)name, (*text) - name, RSStringEncodingUTF8, NO);
        RSXMLNodeSetName(attribute, attributeName);
        RSRelease(attributeName);
        
        // Skip whitespace after attribute name
        skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
        
        // Skip =
        if (*(*text) != ('='))
        {
            RAPIDXML_PARSE_ERROR(error, "expected =", text);
            return;
        }
        ++(*text);
        
        // Add terminating zero after name
//        if (!(Flags & parse_no_string_terminators))
//            attribute->name()[attribute->name_size()] = 0;
        
        // Skip whitespace after =
//        skip<whitespace_pred, Flags>(text);
        skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
        
        // Skip quote and remember if it was ' or "
        char quote = **text;
        if (quote != ('\'') && quote != ('"'))
        {
            RAPIDXML_PARSE_ERROR(error, "expected ' or \"", text);
            return;
        }
        ++(*text);
        
        // Extract attribute value and expand char refs in it
        char *value = *text, *end;
        const int AttFlags = Flags & ~parse_normalize_whitespace;   // No whitespace normalization in attributes
        if (quote == ('\''))
//            end = skip_and_expand_character_refs<attribute_value_pred<Ch('\'')>, attribute_value_pure_pred<Ch('\'')>, AttFlags>(text);
            end = skip_and_expand_character_refs_quote(__internal_lookup_tables_lookup_attribute_value_pred, '\'', __internal_lookup_tables_lookup_attribute_value_pure_pred, '\'', AttFlags, text, error);
        
        else
//            end = skip_and_expand_character_refs<attribute_value_pred<Ch('"')>, attribute_value_pure_pred<Ch('"')>, AttFlags>(text);
            end = skip_and_expand_character_refs_quote(__internal_lookup_tables_lookup_attribute_value_pred, '"', __internal_lookup_tables_lookup_attribute_value_pure_pred, '"', AttFlags, text, error);
        
        // Set attribute value
        RSStringRef attributeValue = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)value, end - value, RSStringEncodingUTF8, NO);
//        attribute->value(value, end - value);
        RSXMLNodeSetObjectValue(attribute, attributeValue);
        RSRelease(attributeValue);
        
        RSXMLElementAddAttribute(element, attribute);
        RSRelease(attribute);
        
        // Make sure that end quote is present
        if (*(*text) != quote)
        {
            RAPIDXML_PARSE_ERROR(error, "expected ' or \"", text);
            return;
        }
        ++(*text);     // Skip quote
        
        // Add terminating zero after value
//        if (!(Flags & parse_no_string_terminators))
//            attribute->value()[attribute->value_size()] = 0;
        
        // Skip whitespace after attribute value
//        skip<whitespace_pred, Flags>(text);
        skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
    }
}

static RSXMLNodeRef __xml_parse_xml_declaration(int Flags, char **text, RSErrorRef *error)
{
    // If parsing of declaration is disabled
    if (!(Flags & parse_declaration_node))
    {
        // Skip until end of declaration
        while ((*text)[0] != ('?') || (*text)[1] != '>')
        {
            if (!(*text)[0])
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                return nil;
            }
            ++(*text);
        }
        (*text) += 2;    // Skip '?>'
        return nil;
    }
    
    // Create declaration
//    RSXMLNodeRef declaration = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, node_declaration, nil, nil);//this->allocate_node(node_declaration);
    RSXMLElementRef declaration = __RSXMLElementCreateInstance(RSAllocatorSystemDefault);
    
    // Skip whitespace before attributes or ?>
    skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
    
    // Parse declaration attributes
    __xml_parse_node_attributes(Flags ,text, declaration, error);
    if (error && *error) {
        RSRelease(declaration);
        return nil;
    }
    // Skip ?>
    if ((*text)[0] != ('?') || (*text)[1] != ('>'))
    {
        RAPIDXML_PARSE_ERROR(error, "expected ?>", text);
        RSRelease(declaration);
        return nil;
    }
    (*text) += 2;
    
    return (RSXMLNodeRef)declaration;
}

static RSXMLNodeRef __xml_parse_comment(int Flags, char **text, RSErrorRef *error)
{
    // If parsing of comments is disabled
    if (!(Flags & parse_comment_nodes))
    {
        // Skip until end of comment
        while ((*text)[0] != ('-') || (*text)[1] != ('-') || (*text)[2] != ('>'))
        {
            if (!(*text)[0])
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                return nil;
            }
            ++(*text);
        }
        (*text) += 3;     // Skip '-->'
        return nil;      // Do not produce comment node
    }
    
    // Remember value start
    char *value = *text;
    
    // Skip until end of comment
    while ((*text)[0] != ('-') || (*text)[1] != ('-') || (*text)[2] != ('>'))
    {
        if (!(*text)[0])
        {
            RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
            return nil;
        }
        ++(*text);
    }
    
    // Create comment node
//    xml_node<Ch> *comment = this->allocate_node(node_comment);
//    comment->value(value, text - value);
    RSXMLNodeRef comment = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, RSXMLNodeComment, nil, nil);
    RSStringRef attributeValue = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)value, *text - value, RSStringEncodingUTF8, NO);
    RSXMLNodeSetObjectValue(comment, attributeValue);
    RSRelease(attributeValue);
    
    // Place zero terminator after comment value
    if (!(Flags & parse_no_string_terminators))
        *(*text) = ('\0');
    
    (*text) += 3;     // Skip '-->'
    return comment;
}

static RSXMLNodeRef __xml_parse_doctype(int Flags, char **text, RSErrorRef *error)
{
    // Remember value start
    char *value = *text;
    
    // Skip to >
    while (*(*text) != ('>'))
    {
        // Determine character type
        switch (*(*text))
        {
                
                // If '[' encountered, scan for matching ending ']' using naive algorithm with depth
                // This works for all W3C test files except for 2 most wicked
            case ('['):
            {
                ++(*text);     // Skip '['
                int depth = 1;
                while (depth > 0)
                {
                    switch (*(*text))
                    {
                        case ('['): ++depth; break;
                        case (']'): --depth; break;
                        case 0:
                        {
                            RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                            return nil;
                        }
                    }
                    ++(*text);
                }
                break;
            }
                
                // Error on end of text
            case ('\0'):
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                return nil;
            }
                
                // Other character, skip it
            default:
                ++(*text);
                
        }
    }
    
    // If DOCTYPE nodes enabled
    if (Flags & parse_doctype_node)
    {
        // Create a new doctype node
        RSXMLNodeRef doctype = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, RSXMLNodeDocType, nil, nil);
//        xml_node<Ch> *doctype = this->allocate_node(node_doctype);
//        doctype->value(value, text - value);
        RSStringRef docValue = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)value, (*text) - value, RSStringEncodingUTF8, NO);
        RSXMLNodeSetObjectValue(doctype, docValue);
        RSRelease(docValue);
        
        
        // Place zero terminator after value
        if (!(Flags & parse_no_string_terminators))
            *(*text) = ('\0');
        
        (*text) += 1;      // skip '>'
        return doctype;
    }
    else
    {
        (*text) += 1;      // skip '>'
        return nil;
    }
    
}

static RSXMLNodeRef __xml_parse_pi(int Flags, char **text, RSErrorRef *error)
{
    // If creation of PI nodes is enabled
    if (Flags & parse_pi_nodes)
    {
        // Create pi node
        RSXMLNodeRef pi = nil;
        // Extract PI target name
        char *name = (*text);
//        skip<node_name_pred, Flags>(text);
        skip(__internal_lookup_tables_lookup_node_name, Flags, text);
        
        if (*text == name)
        {
            RAPIDXML_PARSE_ERROR(error, "expected PI target", text);
            return nil;
        }
        pi = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, RSXMLNodePI, nil, nil);
        RSStringRef piName = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)name, (*text) - name, RSStringEncodingUTF8, NO);
        RSXMLNodeSetName(pi, piName);
        RSRelease(piName);
        
//        pi->name(name, text - name);
        
        // Skip whitespace between pi target and pi
        skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
//        skip<whitespace_pred, Flags>(text);
        
        // Remember start of pi
        char *value = *text;
        
        // Skip to '?>'
        while ((*text)[0] != ('?') || (*text)[1] != ('>'))
        {
            if (*(*text) == ('\0'))
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                if (pi) RSRelease(pi);
                return nil;
            }
            ++(*text);
        }
        
        // Set pi value (verbatim, no entity expansion or whitespace normalization)
        RSStringRef piValue = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)value, *text - value, RSStringEncodingUTF8, NO);
        RSXMLNodeSetObjectValue(pi, piValue);
        RSRelease(piValue);
//        pi->value(value, text - value);
        
        // Place zero terminator after name and value
//        if (!(Flags & parse_no_string_terminators))
//        {
//            pi->name()[pi->name_size()] = Ch('\0');
//            pi->value()[pi->value_size()] = Ch('\0');
//        }
        
        (*text) += 2;                          // Skip '?>'
        return pi;
    }
    else
    {
        // Skip to '?>'
        while ((*text)[0] != ('?') || (*text)[1] != ('>'))
        {
            if (*(*text) == ('\0'))
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                return nil;
            }
            ++(*text);
        }
        (*text) += 2;    // Skip '?>'
        return 0;
    }
}

static char __xml_parse_and_append_data(int Flags, RSXMLElementRef node, char **text, char *contents_start, RSErrorRef *error)
{
    // Backup to contents start if whitespace trimming is disabled
    if (!(Flags & parse_trim_whitespace))
        *text = contents_start;
    
    // Skip until end of data
    char *value = *text, *end;
    if (Flags & parse_normalize_whitespace)
        end = skip_and_expand_character_refs(__internal_lookup_tables_lookup_text, __internal_lookup_tables_lookup_text_pure_with_ws, Flags, text, error);
//        end = skip_and_expand_character_refs<text_pred, text_pure_with_ws_pred, Flags>(text);
    else
        end = skip_and_expand_character_refs(__internal_lookup_tables_lookup_text, __internal_lookup_tables_lookup_text_pure_no_ws, Flags, text, error);
//        end = skip_and_expand_character_refs<text_pred, text_pure_no_ws_pred, Flags>(text);
    
    // Trim trailing whitespace if flag is set; leading was already trimmed by whitespace skip after >
    if (Flags & parse_trim_whitespace)
    {
        if (Flags & parse_normalize_whitespace)
        {
            // Whitespace is already condensed to single space characters by skipping function, so just trim 1 char off the end
            if (*(end - 1) == (' '))
                --end;
        }
        else
        {
            // Backup until non-whitespace character is found
            while (__internal_lookup_tables_lookup_whitespace(*(end - 1)))
                --end;
        }
    }
    
    // If characters are still left between end and value (this test is only necessary if normalization is enabled)
    // Create new data node
    if (!(Flags & parse_no_data_nodes))
    {
//        RSXMLNodeRef data = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, node_data, nil, nil);
        RSStringRef dataValue = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)value, end - value, RSStringEncodingUTF8, NO);
        RSXMLNodeSetObjectValue((RSXMLNodeRef)node, dataValue);
        RSRelease(dataValue);
//        RSXMLElementAddChild(node, data);
//        RSRelease(data);
//        xml_node<Ch> *data = this->allocate_node(node_data);
//        data->value(value, end - value);
//        node->append_node(data);
    }
    
    // Add data to parent node if no data exists yet
    if (!(Flags & parse_no_element_values))
    {
        if (nil == RSXMLNodeGetValue((RSXMLNodeRef)node))
            RSXMLNodeSetObjectValue((RSXMLNodeRef)node, RSStringGetEmptyString());
    }
//        if (*node->value() == Ch('\0'))
//            node->value(value, end - value);
    
    // Place zero terminator after value
    if (!(Flags & parse_no_string_terminators))
    {
        char ch = **text;
        *end = ('\0');
        return ch;      // Return character that ends data; this is required because zero terminator overwritten it
    }
    
    // Return character that ends data
    return *(*text);
}

static RSXMLNodeRef __xml_parse_cdata(int Flags, char **text, RSErrorRef *error)
{
    // If CDATA is disabled
    if (Flags & parse_no_data_nodes)
    {
        // Skip until end of cdata
        while ((*text)[0] != (']') || (*text)[1] != (']') || (*text)[2] != ('>'))
        {
            if (!(*text)[0])
            {
                RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                return nil;
            }
            ++(*text);
        }
        (*text) += 3;      // Skip ]]>
        return 0;       // Do not produce CDATA node
    }
    
    // Skip until end of cdata
    char *value = (*text);
    while ((*text)[0] != (']') || (*text)[1] != (']') || (*text)[2] != ('>'))
    {
        if (!(*text)[0])
        {
            RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
            return nil;
        }
        ++*text;
    }
    
    // Create new cdata node
//    xml_node<Ch> *cdata = this->allocate_node(node_cdata);
//    cdata->value(value, text - value);
    RSXMLNodeRef cdata = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, RSXMLNodeCData, nil, nil);
    RSStringRef cdataValue = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)value, *text - value, RSStringEncodingUTF8, NO);
    RSXMLNodeSetObjectValue(cdata, cdataValue);
    RSRelease(cdataValue);
    
    // Place zero terminator after value
    if (!(Flags & parse_no_string_terminators))
        **text = ('\0');
    
    *text += 3;      // Skip ]]>
    return cdata;
}

static RSXMLNodeRef __xml_parse_element(int Flags, char **text, RSErrorRef *error)
{
    // Create element node
//    xml_node<Ch> *element = this->allocate_node(node_element);
    // Extract element name
    char *name = (*text);
    skip(__internal_lookup_tables_lookup_node_name, Flags, text);
//    skip<node_name_pred, Flags>(text);
    if ((*text) == name)
    {
        RAPIDXML_PARSE_ERROR(error, "expected element name", text);
        return nil;
    }
    RSXMLElementRef element = __RSXMLElementCreateInstance(RSAllocatorSystemDefault);
//    element->name(name, text - name);
    RSStringRef elementName = RSStringCreateWithBytes(RSAllocatorSystemDefault, (RSCUBuffer)name, *text - name, RSStringEncodingUTF8, NO);
    RSXMLNodeSetName((RSXMLNodeRef)element, elementName);
    RSRelease(elementName);
    
    // Skip whitespace between element name and attributes or >
//    skip<whitespace_pred, Flags>(text);
    skip(__internal_lookup_tables_lookup_whitespace, Flags, text);
    // Parse attributes, if any
    __xml_parse_node_attributes(Flags, text, element, error);

    
    // Determine ending type
    if (*(*text) == ('>'))
    {
        ++(*text);
        __xml_parse_node_contents(Flags, text, element, error);
    }
    else if (*(*text) == ('/'))
    {
        ++(*text);
        if (*(*text) != ('>'))
        {
            RAPIDXML_PARSE_ERROR(error, "expected >", text);
            if (element) RSRelease(element);
            return nil;
        }
        ++(*text);
    }
    else
    {
        RAPIDXML_PARSE_ERROR(error, "expected >", text);
        if (element) RSRelease(element);
        return nil;
    }
    
    // Place zero terminator after name
//    if (!(Flags & parse_no_string_terminators))
//        element->name()[element->name_size()] = Ch('\0');
    
    if (error && *error)
    {
        if (element) RSRelease(element);
        element = nil;
    }
    // Return parsed element
    return (RSXMLNodeRef)element;
}

static RSXMLNodeRef __xml_parse_node(int Flags, char **text, RSErrorRef *error)
{
    // Parse proper node type
    switch (*text[0])
    {
            
            // <...
        default:
            // Parse and append element node
            return __xml_parse_element(Flags, text, error);
            
            // <?...
        case ('?'):
            ++*text;     // Skip ?
            if (((*text)[0] == ('x') || (*text)[0] == ('X')) &&
                ((*text)[1] == ('m') || (*text)[1] == ('M')) &&
                ((*text)[2] == ('l') || (*text)[2] == ('L')) &&
                __internal_lookup_tables_lookup_whitespace((*text)[3]))
            {
                // '<?xml ' - xml declaration
                *text += 4;      // Skip 'xml '
                return __xml_parse_xml_declaration(Flags, text, error);
            }
            else
            {
                // Parse PI
                return __xml_parse_pi(Flags, text, error);
            }
            
            // <!...
        case ('!'):
            
            // Parse proper subset of <! node
            switch ((*text)[1])
        {
                
                // <!-
            case ('-'):
                if ((*text)[2] == ('-'))
                {
                    // '<!--' - xml comment
                    (*text) += 3;     // Skip '!--'
                    return __xml_parse_comment(Flags, text, error);
                }
                break;
                
                // <![
            case ('['):
                if ((*text)[2] == ('C') && (*text)[3] == ('D') && (*text)[4] == ('A') &&
                    (*text)[5] == ('T') && (*text)[6] == ('A') && (*text)[7] == ('['))
                {
                    // '<![CDATA[' - cdata
                    (*text) += 8;     // Skip '![CDATA['
                    return __xml_parse_cdata(Flags, text, error);
                }
                break;
                
                // <!D
            case ('D'):
                if ((*text)[2] == ('O') && (*text)[3] == ('C') && (*text)[4] == ('T') &&
                    (*text)[5] == ('Y') && (*text)[6] == ('P') && (*text)[7] == ('E') &&
                    __internal_lookup_tables_lookup_whitespace((*text)[8]))
                {
                    // '<!DOCTYPE ' - doctype
                    (*text) += 9;      // skip '!DOCTYPE '
                    return __xml_parse_doctype(Flags, text, error);
                }
                
        }   // switch
            
            // Attempt to skip other, unrecognized node types starting with <!
            ++(*text);     // Skip !
            while (*(*text) != ('>'))
            {
                if (*(*text) == 0)
                {
                    RAPIDXML_PARSE_ERROR(error, "unexpected end of data", text);
                    return nil;
                }
                ++(*text);
            }
            ++(*text);     // Skip '>'
            return 0;   // No node recognized
            
    }
}

static void __xml_parse_entry(RSXMLDocumentRef document, const char *text, RSErrorRef *error, int Flags)
{
    assert(document);
    assert(text);
    char *_text = (char *)text;
    // Remove current contents
//    this->remove_all_nodes();
//    this->remove_all_attributes();
    RSXMLDocumentSetRootElement(document, nil);
    RSXMLDocumentSetChildren(document, nil);

    // Parse BOM, if any
    __xml_parse_bom(&_text);
    RSXMLElementRef rootElement = nil;
    // Parse children
    while (1 && !(*error))
    {
        // Skip whitespace before node
//        skip<whitespace_pred, Flags>(text);
        skip(__internal_lookup_tables_lookup_whitespace, Flags, &_text);
        if (*_text == 0)
            break;
        
        // Parse and append new child
        if (*_text == (char)('<'))
        {
            ++_text;     // Skip '<'
            RSXMLNodeRef child = nil;
            if ((child = __xml_parse_node(Flags, &_text, error)) && !(*error))
            {
                if (rootElement == nil) {
                    rootElement = (RSXMLElementRef)child;
                    RSXMLDocumentSetRootElement(document, rootElement);
                }
                else RSXMLDocumentAddChild(document, child);
                RSRelease(child);
            }
//            if (xml_node<Ch> *node = parse_node<Flags>(text))
//                this->append_node(node);
        }
        else
        {
            RAPIDXML_PARSE_ERROR(error, "expected <", &_text);
            return;
        }
    }
}

typedef enum {
    RS_HTML_ELEMENT_START,
    RS_HTML_ELEMENT_END,
    RS_HTML_TEXT
} __RSHtmlGumboType;

static RSArrayRef __RSHTMLCreateArrayOfNodesFromGumboVector(GumboVector *childrenVector, RSXMLNodeRef parent);

static RSXMLNodeRef __RSHTMLCreateNodeFromGumboNode(GumboNode *gumboNode) {
    RSXMLNodeRef node = nil;
    if (gumboNode->type == GUMBO_NODE_DOCUMENT) {
        RSXMLNodeRef document = (RSXMLNodeRef)__RSXMLDocumentCreateInstance(RSAllocatorSystemDefault, nil);
        const char * cName = gumboNode->v.document.name;
        const char * cSystemIdentifier __unused = gumboNode->v.document.system_identifier;
        const char * cPublicIdentifier __unused = gumboNode->v.document.public_identifier;
        RSStringRef name = RSStringCreateWithCString(RSAllocatorSystemDefault, cName, RSStringEncodingUTF8);
        __RSXMLNodeSetName(document, name);
        RSRelease(name);
        GumboVector * vector = &gumboNode->v.document.children;
        RSArrayRef nodes = __RSHTMLCreateArrayOfNodesFromGumboVector(vector, document);
        __RSXMLDocumentSetChildren((RSXMLDocumentRef)document, nodes);
        RSRelease(nodes);
        node = document;
    } else if (gumboNode->type == GUMBO_NODE_ELEMENT) {
        RSXMLElementRef element = __RSXMLElementCreateInstance(RSAllocatorSystemDefault);
        __RSXMLNodeSetNodeTypeId((RSXMLNodeRef)element, gumboNode->v.element.tag);
        __RSXMLNodeSetNodeNamespace((RSXMLNodeRef)element, gumboNode->v.element.tag_namespace);
        RSStringRef elementTagName = __RSStringMakeConstantString(gumbo_normalized_tagname(gumboNode->v.element.tag));
        __RSXMLNodeSetName((RSXMLNodeRef)element, elementTagName);
        RSRelease(elementTagName);
        GumboVector * cAttributes = &gumboNode->v.element.attributes;
        for (RSUInteger idx = 0; idx < cAttributes->length; idx++) {
            GumboAttribute * cAttribute = (GumboAttribute *)cAttributes->data[idx];
            const char *cName = cAttribute->name;
            const char *cValue = cAttribute->value;
            RSStringRef name = RSStringCreateWithCString(RSAllocatorSystemDefault, cName, RSStringEncodingUTF8);
            RSStringRef value = RSStringCreateWithCString(RSAllocatorSystemDefault, cValue, RSStringEncodingUTF8);
            RSXMLNodeRef node = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, 0, name, value);
            __RSXMLNodeSetNodeNamespace(node, cAttribute->attr_namespace);
            RSRelease(name);
            RSRelease(value);
            __RSXMLElementAddAttribute((RSXMLElementRef)element, node);
            RSRelease(node);
        }
        GumboVector * cChildren = &gumboNode->v.element.children;
        ((struct __RSXMLElement*)element)->_children = (RSMutableArrayRef)__RSHTMLCreateArrayOfNodesFromGumboVector(cChildren, (RSXMLNodeRef)element);
        node  = (RSXMLNodeRef)element;
    } else {
        if (gumboNode->type != GUMBO_NODE_WHITESPACE) {
            const char *cText = gumboNode->v.text.text;
            RSStringRef text = RSStringCreateWithCString(RSAllocatorSystemDefault, cText, RSStringEncodingUTF8);
            node = __RSXMLNodeCreateInstance(RSAllocatorSystemDefault, gumboNode->type, text, RSStringGetEmptyString());
            RSRelease(text);
        }
    }
    return node;
}

static RSArrayRef __RSHTMLCreateArrayOfNodesFromGumboVector(GumboVector *childrenVector, RSXMLNodeRef parent) {
    RSMutableArrayRef children = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    for (RSUInteger idx = 0; idx < childrenVector->length; idx++) {
        RSXMLNodeRef node = __RSHTMLCreateNodeFromGumboNode(childrenVector->data[idx]);
        if (!node) continue;
        else if (RSGetTypeID(node) == _RSXMLNodeTypeID && node->_name && nil == parent->_objectValue) {
            __RSXMLNodeSetObjectValue(parent, __RSXMLNodeGetName(node));
            RSRelease(node);
        } else {
            __RSXMLNodeSetParent(node, parent);
            RSArrayAddObject(children, node);
            RSRelease(node);
        }
    }
    return children;
}

RSExport RSXMLDocumentRef __RSHTML5Parser(RSDataRef xmlData, RSErrorRef *error) {
    if (!xmlData) return nil;
    __block RSXMLDocumentRef document = __RSXMLDocumentCreateInstance(RSAllocatorSystemDefault, nil);
    RSStringRef xmlStr = RSStringCreateWithData(RSAllocatorSystemDefault, xmlData, RSStringEncodingUTF8);
    RSAutoreleaseBlock(^{
        RSErrorRef parseError = nil;
        GumboOutput* output = gumbo_parse(RSStringGetUTF8String(xmlStr));
        RSTypeRef out = nil;
        out = __RSHTMLCreateNodeFromGumboNode(output->root);
        RSXMLDocumentSetRootElement(document, (RSXMLElementRef)out);
        RSXMLDocumentInsertChild(document, (RSXMLNodeRef)out, 0);
        RSRelease(out);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        if (error) *error = (RSErrorRef)RSRetain(parseError);
    });
    RSRelease(xmlStr);
    if (error && *error) RSAutorelease(*error);
    return document;
}

static RSXMLDocumentRef __RSXMLParser(RSDataRef xmlData, RSErrorRef *error)
{
    if (!xmlData) return nil;
    RSXMLDocumentRef doc = __RSXMLDocumentCreateInstance(RSAllocatorSystemDefault, nil);
    RSStringRef xmlStr = RSStringCreateWithData(RSAllocatorSystemDefault, xmlData, RSStringEncodingUTF8);
    RSAutoreleaseBlock(^{
        RSErrorRef parseError = nil;
        __xml_parse_entry(doc, RSStringGetUTF8String(xmlStr), &parseError, 0/*flags*/);
        if (error) *error = (RSErrorRef)RSRetain(parseError);
    });
    RSRelease(xmlStr);
    if (error && *error) RSAutorelease(*error);
    return doc;
}

RSExport RSXMLDocumentRef RSXMLDocumentCreateWithXMLData(RSAllocatorRef allocator, RSDataRef xmlData, RSXMLDocumentType documentType)
{
    if (xmlData)
    {
        RSErrorRef error = nil;
        RSXMLDocumentRef doc = nil;
        switch (documentType) {
            case RSXMLDocumentTidyHTML:
                doc = __RSHTML5Parser(xmlData, &error);
                break;
            case RSXMLDocumentTidyXML:
            default:
                doc = __RSXMLParser(xmlData, &error);
                break;
        }
        if (error) {
            RSRelease(doc);
            doc = nil;
            RSShow(error);
        }
        return doc;
    }
    return nil;
}

RSExport RSXMLDocumentRef RSXMLDocumentCreateWithContentOfFile(RSAllocatorRef allocator, RSStringRef path, RSXMLDocumentType documentType)
{
    RSDataRef xmlData = RSDataCreateWithContentOfPath(allocator, path);
    RSXMLDocumentRef doc = RSXMLDocumentCreateWithXMLData(allocator, xmlData, documentType);
    RSRelease(xmlData);
    return doc;
}

RSExport RSXMLDocumentRef RSXMLDocumentWithXMLData(RSDataRef xmlData, RSXMLDocumentType documentType)
{
    return RSAutorelease(RSXMLDocumentCreateWithXMLData(RSAllocatorSystemDefault, xmlData, documentType));
}

RSExport RSXMLDocumentRef RSXMLDocumentWithContentOfFile(RSStringRef path, RSXMLDocumentType documentType)
{
    return RSAutorelease(RSXMLDocumentCreateWithContentOfFile(RSAllocatorSystemDefault, path, documentType));
}

RSInline BOOL __RSXMLIsSubClassOfNode(RSTypeRef id)
{
    RSTypeID tid = RSGetTypeID(id);
    if (tid == _RSXMLNodeTypeID || tid == _RSXMLElementTypeID || tid == _RSXMLDocumentTypeID) return YES;
    return NO;
}
