/**
 *  XMLATTR.C -- (Private) Methods to manage XML attributes.
 *
 *  @file       xmlAttr.c
 *  @author     Mike Fitzpatrick and Eric Timmermann and Ken Mighell
 *  @date       8/03/09
 *  @date       8/18/14 KJM: added function xml_attrNames
 *
 *  @brief  	(Private) Methods to manage XML attributes.
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "xmlParseP.h"

extern char *strcasestr ();


/** 
 *  xml_attrSet -- Set/Create an attributes (private method).
 *
 *  @brief  Set/Create an attributes (private method)
 *  @fn	    status = xml_attrSet (AttrBlock *ablock, char *name, char *value)
 *
 *  @param  ablock 	An AttrBlock to insert these attributes.
 *  @param  name 	A string that hold the name of an attribute.
 *  @param  value 	A string that hold the value of an attribute.
 *  @return 		The status of the request.  1 Success, 0=FAIL.
 *
 *  @warning If an attribute has no name/value, this will not create it.
 */
int xml_attrSet (AttrBlock * ablock, char *name, char *value)
{
    char *name_m = NULL;
    int value_found = 0, value_existing = 0;
    AttrList *attr = (ablock ? ablock->attributes : (AttrList *) NULL);


    if (name == NULL)
	return (0);
    name_m = strdup (name);

    //printf( "xml_attrSet: [1] name_m is \"%s\"\n", name_m ); // KJM

    /* KJM: original code is now way too conservative as it rejects
     * valid XML attribute names.  
     */
#undef ORIGINAL_CODE
#ifdef ORIGINAL_CODE
    /* Check for namespace qualifiers on the attribute.
     */
    if ((name_m[0] && strchr (name_m, (int) ':'))
	|| strcmp ("xmlns", name_m) == 0)
	value_found = 1;

    /* Check for an 'xtype' attribute in the v1.1+ spec.
     */
    if (name_m[0] && strcmp ("xtype", name_m) == 0)
	value_found = 1;

    if (ablock->req && strcasestr (ablock->req, name_m) != NULL)
	value_found = 1;
    else if (ablock->opt && strcasestr (ablock->opt, name_m) != NULL)
	value_found = 1;
#endif

    {				// KJM
	value_found = 0;	// NUM, ME VEXO?
	if (ablock->req != NULL) {	// there *is* a list of required attribute names
	    if (strcasestr (ablock->req, name_m) != NULL)
		value_found = 1;
	}
	if (!value_found)
	    if (ablock->opt != NULL) {	// there *is* a list of optional attribute names
		if (strcasestr (ablock->req, name_m) != NULL)
		    value_found = 1;
	    }
	if (!value_found) {	// ensure that there are no illegal characters
	    if ((NULL == strchr (name_m, '>'))
		&& (NULL == strchr (name_m, '<'))
		&& (NULL == strchr (name_m, '&'))) {
		value_found = 1;
		/* ===== CAVEAT EMPTOR =====
		 * Valid multi-character characters like
		 * &amp [ampersand],
		 * &lt; [less than sign],
		 * &gt; [greater than sign],
		 * etc., will get rejected with this test
		 */
	    }
	}
    }				// KJM

    //printf( "xml_attrSet: [2] %d =value_found\n", value_found ); // KJM

    if (!value_found) {
#ifdef USE_STRICT
	fprintf (stderr, "Error: '%s' not a valid Attribute.\n", name);
	return (0);
#else
	return (1);
#endif

    } else {
	while (attr != NULL) {
	    if (name_m[0] && strcasecmp (attr->name, name_m) == 0) {
		strncpy (attr->value, value,
			 min (strlen (value), SZ_ATTRVAL));
		value_existing = 1;
	    }
	    attr = attr->next;
	}

	if (!value_existing) {
	    attr = (AttrList *) calloc (1, sizeof (AttrList));
	    if (ablock->attributes == NULL) {
		attr->next = NULL;
		strncpy (attr->value, value,
			 min (strlen (value), SZ_ATTRVAL));
		strcpy (attr->name, name_m);
	    } else {
		attr = (AttrList *) calloc (1, sizeof (AttrList));
		attr->next = ablock->attributes;
		strncpy (attr->value, value,
			 min (strlen (value), SZ_ATTRVAL));
		strcpy (attr->name, name_m);
	    }
	    ablock->attributes = attr;
	}
    }

    if (name_m != NULL)
	free (name_m);

    return (1);
}


/** 
 *  xml_attrGet -- Get an attribute's value (private method).
 *
 *  @brief  Get an attribute's value (private method)
 *  @fn	    char *xml_attrGet (AttrBlock *ablock, char *name)
 *
 *  @param  *ablock 	An AttrBlock to insert these attributes
 *  @param  *name 	A string that hold the name of an attribute
 *  @return 		Value of the attribute or NULL
 */
char *xml_attrGet (AttrBlock * ablock, char *name)
{
    char *value;
    AttrList *attr = (ablock ? ablock->attributes : (AttrList *) NULL);

    while (attr != NULL) {
	if (strcasecmp (attr->name, name) == 0) {
	    value =
		(char *) calloc (SZ_ATTRNAME, strlen (attr->value) + 1);

	    strncpy (value, attr->value, strlen (attr->value));
	    if (value && value[0])
		return (value);
	    else
		return (NULL);
	}
	attr = attr->next;
    }

    return (NULL);
}


/** 
 *  xml_attrXML -- Get the attributes for an XML tag (private method).
 *
 *  @brief  Get the attributes for an XML tag (private method)
 *  @fn	    char *xml_attrXML (AttrBlock *ablock)
 *
 *  @param *ablock 	An AttrBlock to insert these attributes
 *  @return 		A string containing the attributes for an XML tag
 */
char *xml_attrXML (AttrBlock * ablock)
{
    char *out = (char *) calloc (SZ_XMLTAG, sizeof (char));
    AttrList *attr = (ablock ? ablock->attributes : (AttrList *) NULL);

    while (attr != NULL) {
	if ((attr->value && attr->value[0]) ||
	    (strcasecmp (attr->name, "value") == 0)) {
	    strcat (out, " ");
	    strcat (out, attr->name);
	    strcat (out, "=\"");
	    strcat (out, attr->value);
	    strcat (out, "\"");
	}
	attr = attr->next;
    }

    return (out);
}

/** 
 *  xml_attrNames -- Get the names of the attributes for an XML tag (private method).
 *
 *  @brief  Get names of the attributes for an XML tag (private method)
 *  @fn	    char *xml_attrXML (AttrBlock *ablock)
 *
 *  @param *ablock 	An AttrBlock to insert these attributes
 *  @return 		A space-separated string containing the names of the attributes for an XML tag 
 */
char *xml_attrNames (AttrBlock * ablock)
{				// KJM
    char *out = (char *) calloc (SZ_XMLTAG, sizeof (char));
    AttrList *attr = (ablock ? ablock->attributes : (AttrList *) NULL);

    while (attr != NULL) {
	if ((attr->value && attr->value[0]) ||
	    (strcasecmp (attr->name, "value") == 0)) {
	    strcat (out, attr->name);
	    strcat (out, " ");	// a space character
	}
	attr = attr->next;
    }

    return (out);
}
