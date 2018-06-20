/**
 *  XMLELEMENT.C -- (Private) Method to manage XML elements.
 *
 *  @file       xmlElement.c
 *  @author     Mike Fitzpatrick and Eric Timmermann
 *  @date       8/03/09
 *
 *  @brief      (Private) Methods to manage XML elements.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "xmlParseP.h"
#include "xmlParse.h"

static void xml_setDefaultAttrs (AttrBlock * ablock);

#ifdef VOTABLE

struct {
    int type;		/** element type		*/
    char *name;		/** element name		*/
} elemTypes[] = {
    {
    TY_ROOT, "ROOT"}, {
    TY_VOTABLE, "VOTABLE"}, {
    TY_RESOURCE, "RESOURCE"}, {
    TY_FIELD, "FIELD"}, {
    TY_PARAM, "PARAM"}, {
    TY_INFO, "INFO"}, {
    TY_TR, "TR"}, {
    TY_TD, "TD"}, {
    TY_TABLE, "TABLE"}, {
    TY_TABLEDATA, "TABLEDATA"}, {
    TY_DATA, "DATA"}, {
    TY_STREAM, "STREAM"}, {
    TY_FITS, "FITS"}, {
    TY_GROUP, "GROUP"}, {
    TY_FIELDREF, "FIELDREF"}, {
    TY_PARAMREF, "PARAMREF"}, {
    TY_MIN, "MIN"}, {
    TY_MAX, "MAX"}, {
    TY_OPTION, "OPTION"}, {
    TY_VALUES, "VALUES"}, {
    TY_LINK, "LINK"}, {
    TY_COOSYS, "COOSYS"}, {
    TY_DESCRIPTION, "DESCRIPTION"}, {
    TY_DEFINITIONS, "DEFINITIONS"}, {
    -1, NULL}
};


/**
 *  Definition of Required and Optional attributes of VOTable elements.
 */

struct {
    int type;		/** element type		*/
    char *req;		/** required attrs		*/
    char *opt;		/** optional attrs		*/
} elemAttrs[] = {
    {
    TY_ROOT, "", ""}, {
    TY_VOTABLE, "", "ID|version|"}, {
    TY_RESOURCE, "", "ID|name|type|utype|"}, {
    TY_TABLE, "", "ID|name|ucd|utype|ref|nrows|ncols|"}, {
    TY_INFO, "name|value|", "ID|unit|ucd|utype|ref|"}, {
    TY_STREAM, "",
	    "type|href|actuate|encoding|expires|rights|serialization"}, {
    TY_FITS, "", "extnum|"}, {
    TY_TD, "", "encoding|serialization"}, {
    TY_TR, "", ""}, {
    TY_COOSYS, "", "ID|equinox|epoch|system|"}, {
    TY_DESCRIPTION, "", ""}, {
    TY_DEFINITIONS, "", ""}, {
    TY_DATA, "", ""}, {
    TY_TABLEDATA, "", "nrows|ncols"}, {
    TY_GROUP, "", "ID|name|ucd|utype|ref|"}, {
    TY_PARAM, "datatype|name|value|",
	    "ID|unit|ucd|utype|ref|precision|width|arraysize|"}, {
    TY_FIELD, "datatype|name|type|xtype|",
	    "ID|unit|ucd|utype|ref|precision|width|arraysize|"}, {
    TY_FIELDREF, "ref|", "ucd|utype"}, {
    TY_PARAMREF, "ref|", "ucd|utype"}, {
    TY_MIN, "value|", "inclusive|"}, {
    TY_MAX, "value|", "inclusive|"}, {
    TY_OPTION, "value|", "name|"}, {
    TY_VALUES, "", "ID|type|null|ref|"}, {
    TY_LINK, "action|",
	    "ID|content-role|content-type|title|value|href|"}, {
    -1, NULL, NULL}
};

typedef struct {
    char *in;				/** input element name		*/
    char *name;				/** element name		*/
    char *nspace;			/** element namespace		*/
    int type;				/** element type code		*/
} eTypes;

#else

typedef struct {
    int type;				/** element type code		*/
    char in[SZ_ELEMNAME];		/** input element name		*/
    char name[SZ_ELEMNAME];		/** element name		*/
    char nspace[SZ_ELEMNAME];		/** element namespace		*/
} eTypes;

typedef struct {
    int type;				/** element type code		*/
    char *req;				/** required attrs		*/
    char *opt;				/** optional attrs		*/
} eAttrs;

static eTypes elemTypes[MAX_ELEMENTS];
static eAttrs elemAttrs[MAX_ELEMENTS];

#endif

static int numElems = 0;



/** 
 *  xml_elemType -- Get the integer value (ID) of the Element (private method)
 *
 *  @brief  Get the integer value (ID) of the Element (private method)
 *  @fn     int xml_elemType (Element *e)
 *
 *  @param  e 		A pointer to the Element that you want the type of
 *  @return 		An integer corresponding to the type of the element
 */
int xml_elemType (Element * e)
{
    return ((e == NULL ? -1 : e->type));
}


/** 
 *  xml_elemName -- Get the name of the Element (private method).
 *
 *  @brief  Get the name of the Element (private method)
 *  @fn     char *xml_elemName  (Element *e)
 *
 *  @param  *e 		A pointer to the Element that you want the name of
 *  @return 		A string pointer to the name of the element
 */
char *xml_elemName (Element * e)
{
    register int i;

    for (i = 1; elemTypes[i].type > 0; i++) {
	if (e->type == elemTypes[i].type)
	    return ((elemTypes[i].name ? elemTypes[i].name : ""));
    }
    return (NULL);
}


/** 
 *  xml_eType -- Get the integer value (ID) of the name (private method).
 *
 *  @brief  Get the integer value (ID) of the name (private method)
 *  @fn     int xml_eType (char *name)
 *
 *  @param  name 	Name of the desired type
 *  @return 		An integer corresponding to the type of the element
 */
int xml_eType (char *name)
{
    register int i;
    char *n = NULL;


    if (numElems == 0) {	/* initialize           */
	/* Note that elemTypes[] here is 1-indexed.
	 */
	memset (&elemTypes[0], 0, (MAX_ELEMENTS * sizeof (eTypes)));
	memset (&elemAttrs[0], 0, (MAX_ELEMENTS * sizeof (eAttrs)));
	numElems = 1;
    }

    /*  Search for an existing element name.
     */
    for (i = 1; i < numElems; i++) {
	if (strcasecmp (name, elemTypes[i].in) == 0)
	    return (elemTypes[i].type);
    }

    /*  Have a new element name, push it on the list and save.
     */

    if ((n = strchr (name, (int) ':'))) {	/* parse the name       */
	char tmp[SZ_LINE];

	memset (tmp, 0, SZ_LINE);
	strcpy (tmp, name);
	n = strchr (tmp, (int) ':');
	*n = '\0';

	strcpy (elemTypes[numElems].name, ++n);
	strcpy (elemTypes[numElems].nspace, tmp);
    } else
	strcpy (elemTypes[numElems].name, name);

    strcpy (elemTypes[numElems].in, name);
    elemTypes[numElems].type = numElems;
    elemAttrs[numElems].type = numElems;

    numElems++;

    return (numElems - 1);
}


/** 
 *  xml_elemXMLEnd -- Build a string of the ending XML Tag (private method)
 *
 *  @brief  Build a string of the ending XML Tag (private method)
 *  @fn     char *xml_elemXMLEnd (Element *e)
 *
 *  @param  *e 		A pointer to an Element
 *  @return 		A string that contains the ending XML tag for e
 */
char *xml_elemXMLEnd (Element * e)
{
    char *XML_out = (char *) calloc (SZ_XMLTAG, sizeof (char));

    sprintf (XML_out, "</%s>", xml_elemName (e));
    return (XML_out);
}


/** 
 *  xml_elemXML -- Builds a string of the opening XML Tag (private method)
 *
 *  @brief  Builds a string of the opening XML Tag (private method)
 *  @fn     char *xml_elemXML (Element *e)
 *
 *  @param  *e 		A pointer to an Element
 *  @return 		A string that contains the opening XML tag for e
 */

#define outstr(s)	strcat(XML_out,s);
#define outattr(a,s)	{outstr(a);outstr(s);outstr("\"");}

char *xml_elemXML (Element * e)
{
    char *XML_out = (char *) calloc (SZ_XMLTAG, sizeof (char));
    char *name = xml_elemName (e);


    outstr ("<");
    outstr (name);
    if (strcasecmp (name, "VOTABLE") == 0) {
	outattr (" version=\"", VOT_DOC_VERSION);
	outattr (" xmlns:xsi=\"", VOT_XSI);
	outattr (" xsi:schemaLocation=\"", VOT_SCHEMA_LOC);
	outattr (" xmlns=\"", VOT_XMLNS);
    } else
	outstr (xml_attrXML (e->attr));
    outstr (">");

    return (XML_out);
}


/** 
 *  xml_newElem -- Allocate a new structure of the given type (private method)
 *
 *  @brief  Allocate a new structure of the given type (private method)
 *  @fn     Element *xml_newElem (unsigned int type)
 *
 *  @param  type 	An integer that defines the type of Element
 *  @return 		An new Element structure
 */

Element *xml_newElem (unsigned int type)
{
    register int i;
    Element *new;


    new = (Element *) calloc (1, sizeof (Element));
    new->attr = (AttrBlock *) calloc (1, sizeof (AttrBlock));
    new->type = type;

    for (i = 1; elemTypes[i].type > 0; i++) {
	if (type == elemAttrs[i].type) {
	    strcpy (new->name, elemTypes[i].name);
	    if (elemTypes[i].nspace[0])
		strcpy (new->ns, elemTypes[i].nspace);

	    new->type = type;
	    new->attr->req = elemAttrs[i].req;
	    new->attr->opt = elemAttrs[i].opt;
	    xml_setDefaultAttrs (new->attr);
	    new->handle = -1;
	    return (new);
	}
    }

    if (type == TY_ROOT) {
	strcpy (new->name, elemTypes[1].name);
	if (elemTypes[1].nspace[0])
	    strcpy (new->ns, elemTypes[1].nspace);
	new->attr->req = NULL;
	new->attr->opt = NULL;
	xml_setDefaultAttrs (new->attr);
	new->handle = -1;
	return (new);
    }

    free ((void *) new->attr);
    free ((void *) new);

    return ((Element *) NULL);
}


/**
 *  xml_getToplevel -- Get the handle to the toplevel element.
 *
 *  @brief   Get the handle to the toplevel element.
 *  @fn      handle_t xml_getToplevel (handle_t root)
 *
 *  @param   root 	document root handle
 *  @returns		handle to toplevel element
 */
handle_t xml_getToplevel (handle_t root)
{
    return (2);			/* 1 == root, 2 == toplevel     */
}


/**
 *  xml_getToplevelName -- Get the name of toplevel element.
 *
 *  @brief   Get the name of toplevel element.
 *  @fn      handle_t xml_getToplevelName (handle_t root)
 *
 *  @param   root 	document root handle
 *  @returns		name of toplevel element
 */
char *xml_getToplevelName (handle_t root)
{
    Element *e = xml_getElement (root);

    return (e->name);
}


/**
 *  xml_getToplevelNSpace -- Get the name of toplevel element namespace.
 *
 *  @brief   Get the name of toplevel element namespace.
 *  @fn      handle_t xml_getToplevelNSpace (handle_t root)
 *
 *  @param   root 	document root handle
 *  @returns		toplevel element namespace
 */
char *xml_getToplevelNSpace (handle_t root)
{
    Element *e = xml_getElement (root);

    return (e->ns);
}

/**
 *  xml_elemNameByHandle -- Get the name of requested element.
 *
 *  @brief   Get the name of requested element.
 *  @fn      char *xml_elemNameByHandle (handle_t elem)
 *
 *  @param   elem 	element handle
 *  @returns		name of element
 */
char *xml_elemNameByHandle (handle_t elem)
{
    Element *e = xml_getElement (elem);

    return (e->name);
}


/**
 *  xml_setDefaultAttrs -- Create all required attributes
 *
 *  @brief   Create all required attributes (static private method)
 *  @fn      xml_setDefaultAttrs (AttrBlock *ablock)
 *
 *  @param   attr 	An AttrBlock to insert these attributes.
 *  @returns		Nothing
 */
static void xml_setDefaultAttrs (AttrBlock * ablock)
{
    char req_attr[MAX_ATTR], *tok = req_attr, *name;

    if (ablock->req) {
	memset (req_attr, 0, MAX_ATTR);
	strcpy (req_attr, ablock->req);

	while ((name = strtok (tok, "|")) != NULL) {
	    tok = NULL;
	    if (strcasecmp ("datatype", name) == 0) {
		/*
		   xml_attrSet (ablock, "arraysize", "*");
		 */
		xml_attrSet (ablock, name, "char");
	    } else
		xml_attrSet (ablock, name, "");
	}
    }
}
