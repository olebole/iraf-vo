/**
 *  XMLEXPATCB.C -- (Private) Expat XML Parser callback methods.
 *
 *  @file       xmlExpatCB.c
 *  @author     Mike Fitzpatrick and Eric Timmermann and Ken Mighell
 *  @date       8/03/09
 *  @date       8/18/14 KJM: modified xml_startElement
 *
 *  @brief      (Private) Expat parser XML callback methods
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>
#include <unistd.h>
#include <ctype.h>

#include "xmlParseP.h"
#include "xmlParse.h"


extern Stack *element_stack;



/** 
 *  xml_startElement -- CB whenever a start tag is seen (private method)
 *  
 *  @brief  CB whenever a start tag is seen (private method)
 *  @fn     xml_startElement (void *user, const char *name, const char **atts)
 *
 *  @param  user 	User data (not used)
 *  @param  name 	The name in the XML tag.
 *  @param  atts 	An array of attributes.
 *  @return 		nothing
 */

void xml_startElement (void *user, const char *name, const char **atts)
{
    Element *me, *cur;
    int att, type;
    char name_str[SZ_ATTRNAME];


    memset (name_str, 0, SZ_ATTRNAME);
    strncpy (name_str, name, (SZ_ATTRNAME - 1));

    type = xml_eType (name_str);

    if (type != -1) {
	if ((me = xml_newElem (type)) == (Element *) NULL)
	    fprintf (stderr, "Cannot create new element for <%s>\n",
		     name_str);

	if (!xml_isEmpty (element_stack)) {
	    cur = xmlPeek (element_stack);

	    if (cur->child)
		cur->last_child->next = me;
	    else
		cur->child = me;

	    cur->last_child = me;

	    xml_setHandle (me);

	    /* Gets the attributes. 
	     */

#undef ORIGINAL_CODE
#ifdef ORIGINAL_CODE
	    for (att = 0; atts[att]; att += 2)
		xml_attrSet (me->attr, (char *) atts[att],
			     (char *) atts[att + 1]);
#endif
	    {			// KJM 
		for (att = 0; atts[att]; att += 2) {
		    xml_attrSet (me->attr, (char *) atts[att],
				 (char *) atts[att + 1]);
		    //printf( "xml_startElement: attribute_name is %s\n", (char *)atts[att] ); 
		    //printf( "xml_startElement: attribute_value is \"%s\"\n", (char *)atts[att+1] ); 
		}
	    }

	    me->parent = cur;

	    xmlPush (element_stack, me);

	} else
	    fprintf (stderr, "ERROR: No Root node!\n");
    }
}


/** 
 *  xml_endElement -- CB whenever an end tag is seen (private method)
 *
 *  @brief  CB whenever an end tag is seen (private method)
 *  @fn     xml_endElement (void *user, const char *name)
 *
 *  @param  user	User data (not used)
 *  @param  name 	The name in the XML tag
 *  @return 		nothing
 */
void xml_endElement (void *user, const char *name)
{
#ifdef VOTABLE
    static int cols = 0, rows = 0;
#endif
    Element *cur, *parent;
    int type;
    char name_str[SZ_ATTRNAME];


    memset (name_str, 0, SZ_ATTRNAME);
    strncpy (name_str, name, (SZ_ATTRNAME - 1));

    if ((type = xml_eType (name_str)) != -1) {
	/* BUILD TYPE */
	if (element_stack->head) {
	    cur = xmlPop (element_stack);

	    if (!xml_isEmpty (element_stack)) {
		parent = element_stack->head->element;
	    }

	    if (cur->type != type)
		fprintf (stderr,
			 "ERROR: Malformed XML!!!!!\n%s not matched.",
			 xml_elemName (cur));

	} else
	    fprintf (stderr, "ERROR: No Root node!\n");
    }
}


/**
 *  xml_charData -- Handle non-element character strings (private method)
 *
 *  @brief  Handle non-element character strings (private method)
 *  @fn     xml_charData (void *user, const XML_Char *s, int len) 
 *
 *  @param  user	User data (not used)
 *  @param  s 		content string
 *  @param  len 	length of string
 *  @return 		nothing
 */
void xml_charData (void *user, const XML_Char * s, int len)
{
    Element *cur;
    char *ip = (char *) s;
    char *rstr;
    int clen = 0;


    cur = xmlPeek (element_stack);
    clen = (cur->content ? strlen (cur->content) : 0);

#ifdef STRIP_NL
    while (len && isspace (*ip))	/*  Strip newlines from content.  */
	ip++, len--;
#endif

    if (len > 0 && ip && *ip) {
	if (cur->content == NULL) {
	    cur->content = (char *) calloc ((len + 2), sizeof (char));
	} else {
	    if ((rstr = (char *) realloc (cur->content, (clen + len + 2))))
		cur->content = rstr;
	    else
		fprintf (stderr,
			 "ERROR: Could not realloc charData space.\n");
	}
	strncat (cur->content, ip, len);
    }
}


/**
 *  xml_startCData -- Handle the start of CDATA strings (private method)
 *
 *  @brief  Handle the start of CDATA strings (private method)
 *  @fn     xml_startCData (void *user)
 *
 *  @param  user	User data (not used)
 *  @return 		nothing
 */
void xml_startCData (void *user)
{
    Element *cur = xmlPeek (element_stack);

    cur->isCData = 1;
}


/**
 *  xml_endCData -- Handle the end of CDATA strings (private method)
 *
 *  @brief  Handle the end of CDATA strings (private method)
 *  @fn     xml_endCData (void *user)
 *
 *  @param  user	User data (not used)
 *  @return 		nothing
 */
void xml_endCData (void *user)
{
    ;
}


/****************************************************************************
 *  Private procedures.
 ****************************************************************************/
