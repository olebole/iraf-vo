// 2014AUG18 KJM

/**
 *  XMLPARSE.C -- Public interface procedures for the XML parser.
 *
 *  @file       xmlParse.c
 *  @author     Mike Fitzpatrick and Eric Timmermann and Ken Mighell
 *  @date       8/03/09
 *  @date       8/18/14 KJM: fixed bug in function deWS()
 *
 *  @brief      Public interface procedures for the XML parser.
 */

#include <stdio.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <expat.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>

#include <curl/curl.h>
#ifdef OLD_CURL
#include <curl/types.h>
#endif
#include <curl/easy.h>

#include "xmlParseP.h"
#include "xmlParse.h"
#ifdef HAVE_CFITSIO
#include "fitsio.h"
#endif


#define	BUFSIZE			4096


extern char *strcasestr ();


/* Private procedures
 */
static Element *xml_elementDup (handle_t element_h);
static handle_t xml_nodeCreate (int type);
static char *xml_deWS (char *in);

#ifdef USE_VALIDITY
static int xml_validParents (int type);
static int xml_validChildren (int type);
static int xml_numElements (void);
#endif

static void xml_attachToNode (handle_t parent, handle_t new);
static void xml_attachSibling (handle_t big_brother, handle_t new);
static void xml_dumpXML (Element * node, int level, int indent, FILE * fd);

static int xml_simpleGetURL (char *url, char *ofname);

#ifdef USE_DEBUG
static void xmlBob (void);
#endif


/** *************************************************************************
 *  Public Interface
 *
 *	          xml = xml_openXML (filename|str|NULL)
 *	               xml_closeXML (xml)
 *
 *            handle = xml_newNode  (parent, type)
 *                    xml_freeNode  (node)
 *                  xml_deleteNode  (node)
 *                  xml_attachNode  (parent, new)
 *        handle = xml_copyElement  (handle_t source_h, handle_t parent_h)
 *
 *             val =  xml_getValue  (handle)
 *             stat = xml_setValue  (handle, value)
 *
 *             attr =  xml_getAttr  (handle, attr)
 *              stat = xml_setAttr  (handle, attr, value)
 *
 *             len = xml_getLength  (elem_h)
 *             N = xml_getNumberOf  (elem_h, type)
 *
 *         handle = xml_findByAttr  (parent, name, value)
 *         handle *xml_findInGroup  (group, type)
 *            handle = xml_getNext  (handle)
 *         handle = xml_getSibling  (handle)
 *           handle = xml_getChild  (handle)
 *          handle = xml_getParent  (handle)
 *     handle = xml_getChildOfType  (handle, int type)
 *
 *               int = xml_valueOf  (handle)
 *               type = xml_typeOf  (handle)
 *                 xml_setWarnings  (value)
 *
 ** *************************************************************************/


Stack *element_stack = NULL;	/*  This holds a stack of elements. Should be 
				 *  empty most of the time. 
				 */

Element *xml_struct = NULL;	/*  This will hold all the VOTs.  The first 
				 *  Element in this structure is a ROOT Element.
				 */

char *xmlELevel = "";		/*  Error Message Level
				 */

int xmlWarn = 0;		/*  Warn about parsing issues.  Values:
				 *      0    No messages (lax parsing)
				 *      1    Warning messages
				 *      2    Strict parsing
				 */



/** 
 *  xml_openXML -- Parse an XML document and return a handle to it
 *
 *  @brief  Parse an XML document and return a handle to it
 *  @fn     handle_t xml_openXML (char *arg)
 *
 *  @param  arg 	The source of the table
 *  @return	 	The root node handle of the VOTable
 */
handle_t xml_openXML (char *arg)
{
    FILE *fd = (FILE *) NULL;
    Element *my_element;
    char buf[BUFSIZE], *ip, urlFname[BUFSIZE];
    size_t len, nleft = 0, fsize = -1, nread = 0;
    int done, ret_handle, nerrs;
    XML_Parser parser;

    struct stat st;


    memset (buf, 0, BUFSIZE);
    memset (urlFname, 0, BUFSIZE);


    xml_newHandleTable ();	/* initialize the handle table  */
    if (element_stack == NULL)
	element_stack = xml_newStack ();

    if (xml_struct == NULL)	/* initialize the DOM and stack     */
	xml_struct = xml_newElem (TY_ROOT);
    xmlPush (element_stack, xml_struct);


    /*  Process the argument depending on it's type, i.e. a literal string,
     *  a filename, stdin, etc.
     */
    if (arg == NULL) {
	/** **********************************************
	 *  Create an empty, but valid, DOM tree.
	 */
	my_element = xml_newElem (TY_ROOT);

	if (xml_struct->child)
	    xml_struct->last_child->next = my_element;
	else
	    xml_struct->child = my_element;

	xml_struct->last_child = my_element;

	xml_clearStack (element_stack);

	my_element->parent = xml_struct;

	return (xml_setHandle (my_element));

    } else if (strncmp (arg, "http://", 7) == 0) {
	/** **********************************************
	 *   Download the URL and process the result.
	 */
	int tfd = 0;

	/*  Open a temp file for the downloaded URL.
	 */
	strcpy (urlFname, "/tmp/xmlXXXXXX");
	if ((tfd = mkstemp (urlFname) < 0))
	    strcpy (urlFname, "/tmp/xmlParser");
	close (tfd);

	nerrs = xml_simpleGetURL (arg, urlFname);
	if (!(fd = fopen (urlFname, "r"))) {
	    fprintf (stderr, "Unable to open url '%s'\n", arg);
	    return (0);		/* cannot open file error       */
	}
	fstat (fileno (fd), &st);
	fsize = (size_t) st.st_size;

    } else if (strcmp (arg, "-") == 0
	       || strncasecmp (arg, "stdin", 5) == 0) {
	/** **********************************************
	 *  Get the input from stdin.
	 */
	fd = stdin;
	fsize = -1;

    } else if (strncmp (arg, "file://", 7) == 0) {
	/** **********************************************
	 *  Process a local file URI.
	 */
	len = strlen (&arg[7]);
	if (!(fd = fopen (&arg[7], "r"))) {
	    fprintf (stderr, "Unable to open input file '%s'\n", &arg[7]);
	    return (0);		/* cannot open file error       */
	}
	fstat (fileno (fd), &st);
	fsize = (size_t) st.st_size;

    } else if (access (arg, R_OK) == 0) {	/* input from file   */
	/** **********************************************
	 *  Process a local file pathname.
	 */
	len = strlen (arg);
	if (!(fd = fopen (arg, "r"))) {
	    fprintf (stderr, "Unable to open input file '%s'\n", arg);
	    return (0);		/* cannot open file error       */
	}
	fstat (fileno (fd), &st);
	fsize = (size_t) st.st_size;

    } else if (strcasestr (arg, "<?xml ")) {
	/** **********************************************
	 *  Process an input argument is XML string
	 */
	ip = arg;
	len = strlen (arg);

    } else {
	fprintf (stderr, "openVOTable(): Invalid input arg '%s'\n", arg);
	return (-1);
    }


    /*  Create the parser and set the input handlers.
     */
    parser = XML_ParserCreate (NULL);
    XML_SetElementHandler (parser, xml_startElement, xml_endElement);
    XML_SetCdataSectionHandler (parser, xml_startCData, xml_endCData);
    XML_SetCharacterDataHandler (parser, xml_charData);

    ip = arg;			/* initialize           */
    done = 0;
    nleft = fsize;
    nread = 0;

    if (fd) {
	do {
	    memset (buf, 0, BUFSIZE);
	    len = fread (buf, 1, sizeof (buf), fd);

	    if (nread == 0) {
		/*  Check that this actually is a VOTable.
		 */
		if (strcasestr (buf, "<?xml") == (char *) NULL) {
		    if (fd != stdin)
			fclose (fd);
		    if (urlFname[0])
			unlink (urlFname);
		    return (-1);	/* not a XML document */
		}
	    }
	    nread += len;

	    if (fd != stdin) {
		nleft -= len;
		done = nread >= fsize;
	    } else
		done = (len == 0 ? feof (stdin) : 0);


	    if (done && buf[len - 1] == '\0')	/* trim trailing null       */
		while (len && !buf[len - 1])
		    len--;
	    if (done && buf[len - 1] != '\n')	/* no newline on file       */
		buf[len] = '\n';

	    if (!XML_Parse (parser, buf, len, done)) {
		fprintf (stderr, "Error: %s at line %d\n",
			 XML_ErrorString (XML_GetErrorCode (parser)),
			 (int) XML_GetCurrentLineNumber (parser));
		return (0);	/* parse error                  */
	    }
	} while (!done);

    } else {
	if (!XML_Parse (parser, ip, len, 1)) {
	    fprintf (stderr, "Error: %s at line %d\n",
		     XML_ErrorString (XML_GetErrorCode (parser)),
		     (int) XML_GetCurrentLineNumber (parser));
	    return (0);		/* parse error                  */
	}
    }
    XML_ParserFree (parser);

    if (fd && fd != stdin)
	fclose (fd);
    if (urlFname[0])
	unlink (urlFname);

    xml_clearStack (element_stack);

    ret_handle = xml_lookupHandle (xml_struct->last_child);

    return (ret_handle);
}


/** 
 *  xml_closeXML -- Destroy the root node and all of it's children.
 *
 *  @brief  Destroy the root node and all of it's children.
 *  @fn     xml_closeXML (handle_t xml)
 *
 *  @param  xml 	A handle to the Element that you want deleted
 *  @return		nothing
 *
 *  @warning Destroys the node and all of it's children.
 */
void xml_closeXML (handle_t xml)
{
    Element *elem = xml_getElement (xml);
    int my_type = xml_elemType (elem);


    if ((my_type != TY_ROOT)) {
	xmlEmsg ("closeXML() arg must be a root tag\n");
	return;
    }
    xml_deleteNode (xml);
}




/*****************************************************************************
 *  Routines to get nodes of a VOTable as a handle.
 ****************************************************************************/

/****************************************************************************/

#ifdef VOTABLE
struct {
    int type;			/** element type		*/
    int parents;		/** allowed parent types	*/
    int children;		/** allowed child types		*/
} elemParents[] = {
    {
    TY_ROOT, 0, TY_VOTABLE}, {
    TY_VOTABLE,
	    TY_ROOT,
	    TY_DESCRIPTION | TY_COOSYS | TY_INFO | TY_PARAM | TY_GROUP
	    | TY_RESOURCE}, {
    TY_RESOURCE, TY_VOTABLE | TY_RESOURCE,
	    TY_DESCRIPTION | TY_COOSYS | TY_INFO | TY_PARAM | TY_GROUP
	    | TY_RESOURCE | TY_LINK | TY_TABLE}, {
    TY_TABLE, TY_RESOURCE,
	    TY_DESCRIPTION | TY_FIELD | TY_INFO | TY_PARAM | TY_GROUP |
	    TY_LINK | TY_DATA}, {
    TY_INFO, TY_VOTABLE | TY_RESOURCE | TY_DATA | TY_TABLE,
	    TY_DESCRIPTION | TY_VALUES | TY_LINK}, {
    TY_STREAM, TY_BINARY | TY_BINARY2 | TY_FITS, 0}, {
    TY_FITS, TY_DATA, 0}, {
    TY_TD, TY_TR, 0}, {
    TY_TR, TY_TABLEDATA, TY_TD}, {
    TY_COOSYS, TY_VOTABLE | TY_RESOURCE, 0}, {
    TY_DESCRIPTION,
	    TY_VOTABLE | TY_RESOURCE | TY_TABLE | TY_FIELD | TY_PARAM |
	    TY_GROUP | TY_INFO, 0}, {
    TY_DEFINITIONS, 0, 0}, {
    TY_DATA, TY_TABLE, TY_TABLEDATA | TY_BINARY | TY_BINARY2 | TY_FITS}, {
    TY_TABLEDATA, TY_DATA, TY_TR}, {
    TY_GROUP,
	    TY_VOTABLE | TY_RESOURCE | TY_TABLE | TY_GROUP,
	    TY_DESCRIPTION | TY_FIELDREF | TY_PARAM | TY_PARAMREF |
	    TY_GROUP}, {
    TY_PARAM, TY_VOTABLE | TY_RESOURCE | TY_TABLE,
	    TY_DESCRIPTION | TY_VALUES | TY_LINK}, {
    TY_FIELD, TY_TABLE, TY_DESCRIPTION | TY_VALUES | TY_LINK}, {
    TY_FIELDREF, TY_GROUP, 0}, {
    TY_PARAMREF, TY_GROUP, 0}, {
    TY_MIN, TY_VALUES, 0}, {
    TY_MAX, TY_VALUES, 0}, {
    TY_OPTION, TY_VALUES | TY_OPTION, 0}, {
    TY_VALUES, TY_PARAM | TY_INFO, TY_MIN | TY_MAX | TY_OPTION}, {
    TY_LINK, TY_RESOURCE | TY_TABLE | TY_FIELD | TY_PARAM | TY_INFO, 0}, {
    -1, 0, 0}
};

#else

typedef struct {
    int type;			/** element type		*/
    int parents;		/** allowed parent types	*/
    int children;		/** allowed child types		*/
} elemParents[MAX_ELEMENTS];

#endif



/**
 *  xml_newNode -- Creates a new blank unlinked node.
 * 
 *  @brief  Creates a new blank unlinked node.
 *  @fn     handle_t xml_newNode (handle_t parent, int type)
 *
 *  @param   parent 	A handle to the Element that you want to add a node to
 *  @param   type 	The type of node you wish to create
 *  @return 		A handle to the created node
 */
handle_t xml_newNode (handle_t parent, int type)
{
    /*  Refer to xml_newRESOURCE for detailed comments on function workings. 
     */
    handle_t elem_h = 0;


    /*  Check that parent is proper for the type we create.
       assert ( (parent & xml_validParents (type)) );
     */

    /*  Attach the new node to the parent.
     */
    elem_h = xml_nodeCreate (type);
    xml_attachToNode (parent, elem_h);

    return (elem_h);
}


/**
 *  xml_attachNode -- Adds a node as a child of parent.
 *
 *  @brief  Adds a node as a child of parent.
 *  @fn     xml_attachNode (handle_t parent, handle_t new)
 *
 *  @param  parent 	A handle to the Element that you want to add a node to
 *  @param  new 	A handle to the Element that you want to add
 *  @return		nothing
 */
void xml_attachNode (handle_t parent, handle_t new)
{
    Element *parent_ptr, *new_ptr;
    handle_t copy;

    if ((parent == 0) || (new == 0))
	return;

    /* Make a copy of the Element and it's children. */
    copy = xml_copyElement (new, 0);

    /* Get pointers. */
    parent_ptr = xml_getElement (parent);
    new_ptr = xml_getElement (copy);

    new_ptr->ref_count++;

    /* Make the links, the attached nodes are copies not the original. */
    if (parent_ptr->child)
	parent_ptr->last_child->next = new_ptr;
    else
	parent_ptr->child = new_ptr;

    parent_ptr->last_child = new_ptr;

    new_ptr->parent = parent_ptr;
}


/**
 *  xml_freeNode -- Destroys the node and all of it's children.
 *
 *  @brief  Destroys the node and all of it's children.
 *  @fn     xml_freeNode (handle_t node)
 *
 *  @param  node 	A handle to the Element that you want deleted
 *  @return		nothing
 */
void xml_freeNode (handle_t node)
{
    /* Recursive function to delete the Element and it's children. */
    Element *node_ptr;
    handle_t child_handle, sibling_handle;


    if (!(node_ptr = xml_getElement (node)))
	return;

    if (node_ptr->child) {
	child_handle = xml_lookupHandle (node_ptr->child);
	xml_freeNode (child_handle);
    }

    if (node_ptr->next) {
	sibling_handle = xml_lookupHandle (node_ptr->next);
	xml_freeNode (sibling_handle);
    }

    /* Clean the handle and free the memory. 
     */
    xml_freeHandle (node);
    free (node_ptr);
}


/**
 *  xml_deleteNode -- Destroys the node and all of it's children.
 *
 *  @brief  Destroys the node and all of it's children.
 *  @fn     xml_deleteNode (handle_t element)
 *
 *  @param  element 	A handle to the Element that you want deleted
 *  @return		nothing
 */
void xml_deleteNode (handle_t element)
{
    /* Delete the node but update the tree. */
    Element *element_ptr, *parent, *prev;

    element_ptr = xml_getElement (element);
    parent = element_ptr->parent;

    /* Make sure the node is not still reference. Should never be the case. */
    if (element_ptr->ref_count > 1) {
	element_ptr->ref_count--;
	return;
    }

    if (parent) {
	if (parent->child == element_ptr) {
	    parent->child = element_ptr->next;
	    element_ptr->next = NULL;
	} else {
	    for (prev = parent->child; prev->next != element_ptr;
		 prev = prev->next);
	    prev->next = element_ptr->next;
	    element_ptr->next = NULL;

	    if (parent->last_child == element_ptr)
		parent->last_child = prev;
	}
    }

    xml_freeNode (element);
}


/**
 *  xml_copyElement -- Adds a node as a child of parent.
 *
 *  @brief  Adds a node as a child of parent.
 *  @fn     handle_t xml_copyElement (handle_t src_h, handle_t parent_h)
 *
 *  @param  src_h 	A handle to the Element to copy
 *  @param  parent_h 	A handle to the Elements parent
 *  @return 		A handle_t of the copy of the structure
 */
handle_t xml_copyElement (handle_t src_h, handle_t parent_h)
{
    /* A recurseive function to copy a node and it's children. */
    Element *src_ptr, *return_ptr;
    handle_t return_handle, parent;
    handle_t src_child_h, src_next_h;


    src_ptr = xml_getElement (src_h);

    if (src_ptr == 0)
	return (0);

    return_ptr = xml_elementDup (src_h);	/* copy the source Element   */
    if (!return_ptr)
	return (0);
    return_handle = xml_lookupHandle (return_ptr);	/* get the copies handle  */

    if (src_ptr->child) {	/* process children          */
	parent = return_handle;
	src_child_h =
	    xml_copyElement (xml_lookupHandle (src_ptr->child), parent);

	/* Actually attach the node. No copy.
	 */
	xml_attachToNode (return_handle, src_child_h);
    }

    if (src_ptr->next) {	/* process siblings          */
	src_next_h =
	    xml_copyElement (xml_lookupHandle (src_ptr->next), parent);

	if (parent_h != 0)
	    return_ptr->parent = xml_getElement (parent);

	/* Attach the sibling, no copy. 
	 */
	xml_attachSibling (return_handle, src_next_h);
    }

    return (return_handle);
}


/** **************************************************************************
 *  Utility methods
 ** *************************************************************************/

/**
 *  xml_getLength -- Return the number of sibling Elements of the same type.
 * 
 *  @brief  Return the number of sibling Elements of the same type.
 *  @fn     int xml_getLength (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	The status of the set
 */
int xml_getLength (handle_t elem_h)
{
    Element *elem;
    int type, total = 0;


    if ((elem = xml_getElement (elem_h)))
	type = elem->type;
    else
	return (0);

    while (elem) {
	if (elem->type == type)
	    total++;
	elem = elem->next;
    }

    return (total);
}


/**
 *  xml_getNumberOf -- Return the number of sibling Elements of the type.
 *
 *  @brief  Return the number of sibling Elements of the type.
 *  @fn     int xml_getNumberOf (handle_t elem_h, int type)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @param  type 	An int of the type of element you wish to count
 *  @return	 	The status of the set
 */
int xml_getNumberOf (handle_t elem_h, int type)
{
    Element *elem = xml_getElement (elem_h);
    int total = 0;


    if (elem == NULL)
	return (0);

    while (elem) {
	if (elem->type == type)
	    total++;
	elem = elem->next;
    }

    return (total);
}


/**
 *  xml_findByAttr -- Get a handle to an Element with the requested attribute.
 * 
 *  @brief  Get a handle to an Element with the requested attribute.
 *  @fn     handle_t xml_findByAttr (handle_t parent, char *name, char *value)
 *
 *  @param  parent 	A handle_t the parent Element
 *  @param  name 	A string holding the Value type
 *  @param  value 	A string holding the Value value
 *  @return	 	The handle to the element
 */
handle_t xml_findByAttr (handle_t parent, char *name, char *value)
{
    Element *elem, *my_parent;
    char *elem_value;
    handle_t return_h = 0;


    my_parent = xml_getElement (parent);
    elem = my_parent->child;

    if ((elem == NULL) || (name == NULL) || (value == NULL))
	return (0);

    while (elem) {
	elem_value = xml_attrGet (elem->attr, name);

	if ((elem_value != NULL) && (strcasecmp (elem_value, value) == 0)) {
	    return_h = xml_lookupHandle (elem);
	    break;
	}

	elem = elem->next;
    }

    return (return_h);
}


/**
 *  xml_getNext -- Return a handle_t of the next Element of the same type.
 *  
 *  @brief  Return a handle_t of the next Element of the same type.
 *  @fn     handle_t xml_getNext (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	A handle of the next Element of the same type
 */
handle_t xml_getNext (handle_t elem_h)
{
    Element *elem = xml_getElement (elem_h);
    int type;

    type = xml_elemType (elem);
    for (elem = elem->next; elem; elem = elem->next) {
	if (xml_elemType (elem) == type)
	    break;
    }

    return (xml_lookupHandle (elem));
}


/**
 *  xml_getSibling -- Return a handle_t of the next signling Element.
 * 
 *  @brief  Return a handle_t of the next Element.
 *  @fn     handle_t xml_getSibling (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	A handle of the next Element
 */
handle_t xml_getSibling (handle_t elem_h)
{
    Element *elem = xml_getElement (elem_h);

    return (xml_lookupHandle (elem->next));
}


/**
 *  xml_getChild -- Return a handle_t of the child Element.
 *
 *  @brief  Return a handle_t of the child Element.
 *  @fn     handle_t xml_getChild (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	A handle of the child Element
 */
handle_t xml_getChild (handle_t elem_h)
{
    Element *elem = xml_getElement (elem_h);

    return (xml_lookupHandle (elem->child));
}


/**
 *  xml_getParent -- Return the handle of the parent Element.
 *
 *  @brief  Return the handle of the parent Element.
 *  @fn     handle_t xml_getParent (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	A handle of the paretn Element
 */
handle_t xml_getParent (handle_t elem_h)
{
    Element *elem = xml_getElement (elem_h);

    return (xml_lookupHandle (elem->parent));
}


/**
 *  xml_getChildOfType -- Get the handle of the next Element of the same type.
 *
 *  @brief  Get the handle of the next Element of the same type.
 *  @fn     handle_t xml_getChildOfType (handle_t elem_h, int type)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @param  type 	An integer of the Element type for find
 *  @return	 	A handle of the Element
 */
handle_t xml_getChildOfType (handle_t elem_h, int type)
{
    Element *elem;

    elem = xml_getElement (elem_h);
    type = xml_elemType (elem);

    for (elem = elem->child; elem; elem = elem->next) {
	if (xml_elemType (elem) == type)
	    break;
    }

    return (xml_lookupHandle (elem));
}


/**
 *  xml_valueOf -- Return type of the Element.
 *
 *  @brief   Return type of the Element.
 *  @fn      int xml_valueOf (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	An integer of the type
 */
int xml_valueOf (handle_t elem_h)
{
    Element *elem = xml_getElement (elem_h);

    return (xml_elemType (elem));	/* ???? FIXME   */
}


/**
 *  xml_typeOf -- Return type of the Element.
 *
 *  @brief  Return type of the Element.
 *  @fn     int xml_typeOf (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	An integer of the type
 */
int xml_typeOf (handle_t elem_h)
{
    return (xml_elemType (xml_getElement (elem_h)));
}


/****************************************************************************
 *
 ***************************************************************************/


/**
 *  xml_setValue -- Set the Value for the ELEMENT.
 *
 *  @brief  Set the Value for the ELEMENT.
 *  @fn     int xml_setValue (handle_t elem_h, char *value)
 *
 *  @param  elem_h 	A handle_t the ELEMENT
 *  @param  value 	A string holding the value
 *  @return 		The status of the set
 */
int xml_setValue (handle_t elem_h, char *value)
{
    Element *cur = xml_getElement (elem_h);
    int len = strlen (value) + 1;


    if (value) {
	if (cur->content != NULL)
	    free (cur->content);

	cur->content = (char *) calloc (len, sizeof (char));

	if (cur->content == NULL) {
	    fprintf (stderr, "ERROR:  CALLOC failed for xml_setValue.\n");
	    return (0);
	}

	strncat (cur->content, value, len);
	return (1);

    } else
	return (0);
}


/**
 *  xml_getValue -- Get the Value for the ELEMENT.
 *
 *  @brief  Get the Value for the ELEMENT.
 *  @fn     char *xml_getValue (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the ELEMENT
 *  @return 		A string of the value or the Value
 */
char *xml_getValue (handle_t elem_h)
{
    Element *elem = xml_getElement (elem_h);

#ifdef USE_NULL_VALUE
    return ((elem ? elem->content : NULL));
#else
    return ((elem ? xml_deWS (elem->content) : ""));
#endif
}


/**
 *  xml_setAttr -- Set the attribute for the Element.
 * 
 *  @brief  Set the attribute for the Element.
 *  @fn     int xml_setAttr (handle_t elem_h, char *attr, char *value)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @param  attr 	A string holding the attribute name
 *  @param  value 	A string holding the attribute value
 *  @return	 	The status of the set
 */
int xml_setAttr (handle_t elem_h, char *attr, char *value)
{
    Element *elem = xml_getElement (elem_h);

    return (xml_attrSet (elem->attr, attr, value));
}


/**
 *  xml_getAttr -- Return the attribute for the Element.
 * 
 *  @brief  Return the attribute for the Element.
 *  @fn     char * xml_getAttr (handle_t elem_h, char *attr)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @param  attr 	A string holding the attribute name
 *  @return	 	A string of the value or the attr
 */
char *xml_getAttr (handle_t elem_h, char *attr)
{
    Element *elem = xml_getElement (elem_h);

    return (xml_attrGet (elem->attr, attr));
}


/**
 *  xml_dumpAllAttr -- Dump all information about the attributes of the Element
 * 
 *  @brief  Dump all information about the attributes of the Element
 *  @fn     char * xml_dumpAllAttr (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	A string with names and value pairs of all the attributes of the Element
 */
char *xml_dumpAllAttr (handle_t elem_h)
{				// KJM
    Element *elem = xml_getElement (elem_h);

    return (xml_attrXML (elem->attr));
}


/**
 *  xml_dumpNamesAttr -- Dump all names of the attributes of the Element
 * 
 *  @brief  Dump all names of the attributes of the Element
 *  @fn     char * xml_dumpNamesAttr (handle_t elem_h)
 *
 *  @param  elem_h 	A handle_t the Element
 *  @return	 	A space-separated string with names of all the attributes of the Element
 */
char *xml_dumpNamesAttr (handle_t elem_h)
{				// KJM
    Element *elem = xml_getElement (elem_h);

    return (xml_attrNames (elem->attr));
}


/** 
 *  VOT_SIMPLEGETURL -- Utility routine to do a simple URL download to the file.
 */
static int xml_simpleGetURL (char *url, char *ofname)
{
    int stat = 0;
    char lockfile[SZ_FNAME], errBuf[CURL_ERROR_SIZE], fname[SZ_FNAME];
    FILE *fd;
    CURL *curl_handle;



    /*  For the CURL operation to download the file.
     */
    curl_global_init (CURL_GLOBAL_ALL);	/* init curl session    */
    curl_handle = curl_easy_init ();

    if ((fd = fopen (ofname, "wb")) == NULL) {	/* open the output file */
	fprintf (stderr, "Error: cannot open output file '%s'\n", ofname);
	curl_easy_cleanup (curl_handle);
	return (1);
    }

    /*  Set cURL options
     */
    curl_easy_setopt (curl_handle, CURLOPT_URL, url);
    curl_easy_setopt (curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, fd);
    curl_easy_setopt (curl_handle, CURLOPT_ERRORBUFFER, errBuf);
    curl_easy_setopt (curl_handle, CURLOPT_FOLLOWLOCATION, 1);

    /*  Do the download.
     */
    if ((stat = curl_easy_perform (curl_handle)) != 0) {
	/*  Error in download, clean up.
	 */
	unlink (fname);
	fclose (fd);		/* close the file       */
	curl_easy_cleanup (curl_handle);	/* cleanup curl stuff   */
	return (1);
    }

    fflush (fd);
    fclose (fd);		/* close the file       */
    curl_easy_cleanup (curl_handle);	/* cleanup curl stuff   */


    /*  Remove the lock file to indicate we are done.
     */
    unlink (lockfile);

    return (0);
}


/**
 *  xml_setWarnings --  Set the warning level.
 *
 *  @brief  Set the warning level.     
 *  @fn     xml_setWarnings (int value)
 *
 *  @param  value 	Warning level
 *  @return		nothing
 */
void xml_setWarnings (int value)
{
    switch ((xmlWarn = value)) {
    case 0:
	xmlELevel = "";
	break;
    case 1:
	xmlELevel = "Warning: ";
	break;
    case 2:
	xmlELevel = "Error: ";
	break;
    }
}


/**
 *  xmlEmsg -- Error message print utility.
 */
void xmlEmsg (char *msg)
{
    if (xmlWarn)
	fprintf (stderr, "%s%s", xmlELevel, msg);
}



/****************************************************************************
 *  Private procedures.
 ****************************************************************************/

/**
 *  xml_cleanUp
 *
 *  @brief	Free all the handles and Element nodes.
static void
xml_cleanUp (void)
{
    xml_handleCleanup ();
        
    if (xml_struct == NULL)
        xml_struct = xml_newElem (TY_ROOT);
    
    xml_struct->parent     = NULL;
    xml_struct->child      = NULL;
    xml_struct->last_child = NULL;
    xml_struct->next       = NULL;
}
 */


/**
 *  xml_elementDup -- Duplicate the input Element.
 *
 *  @brief  Duplicate the input Element.
 *  @fn     Element * xml_elementDup (handle_t element_h)
 *
 *  @param  element_h 	A handle_t to the ELEMENT you want to copy
 *  @return 		An ELEMENT type
 */
static Element *xml_elementDup (handle_t element_h)
{
    Element *new, *src;
    handle_t new_h;
    int type;
    AttrList *attr;


    src = xml_getElement (element_h);	/* get the element to copy      */
    type = xml_typeOf (element_h);	/* get the type                 */

    if (type >= MAX_ELEMENTS)
	return ((void *) NULL);

    new_h = xml_nodeCreate (type);	/* make a blank Node            */
    new = xml_getElement (new_h);	/* get the pointer              */


    /* Copy the attributes. 
     */
    for (attr = src->attr->attributes; attr; attr = attr->next)
	xml_attrSet (new->attr, attr->name, attr->value);

    /* Copy the content. 
     */
    if (src->content) {
	int len = strlen (src->content);

	new->content = (char *) calloc ((len + 2), sizeof (char));
	new->isCData = src->isCData;
	strncpy (new->content, src->content, len);
    }

    return (new);		/* return the copy      */
}


/**
 *  xml_nodeCreate -- Create a new blank unlinked node.
 * 
 *  @brief  Create a new blank unlinked node.
 *  @fn     handle_t xml_nodeCreate (int type)
 *
 *  @param  type 	The type of node you wish to create
 *  @return 		A handle to the created node
 */
static handle_t xml_nodeCreate (int type)
{
    /* Make a new blank node and give it a handle. */
    Element *elem = xml_newElem (type);

    return (xml_setHandle (elem));
}


/**
 *  xml_attachToNode -- Adds a node as a child of parent.
 *
 *  @brief  Adds a node as a child of parent.
 *  @fn     xml_attachToNode (handle_t parent, handle_t new)
 *
 *  @param  parent 	A handle to the Element that you want to add a node to
 *  @param  new 	A handle to the Element that you want to add
 *  @return		nothing
 */
static void xml_attachToNode (handle_t parent, handle_t new)
{
    Element *parent_ptr, *new_ptr;

    /* Sanity check. 
       assert ( (new & xml_validChildren (parent)) );
     */
    if ((parent == 0) || (new == 0))
	return;

    parent_ptr = xml_getElement (parent);
    new_ptr = xml_getElement (new);

    new_ptr->ref_count++;

    /* Make links. */
    if (parent_ptr->child)
	parent_ptr->last_child->next = new_ptr;
    else
	parent_ptr->child = new_ptr;

    parent_ptr->last_child = new_ptr;

    new_ptr->parent = parent_ptr;
}


/**
 *  xml_attachSibling -- Adds a node as a sibling of big_brother.
 *
 *  @brief  Adds a node as a sibling of big_brother.
 *  @fn     xml_attachSibling (handle_t big_brother, handle_t new)
 *
 *  @param  big_brother Handle to the Element you want to add a node to
 *  @param  new 	A handle to the Element that you want to add
 *  @return		nothing
 */
static void xml_attachSibling (handle_t big_brother, handle_t new)
{
    Element *big_brother_ptr, *new_ptr;

    /* Sanity check. */
    if ((big_brother == 0) || (new == 0))
	return;

    /* Get relevant pointers. 
     */
    big_brother_ptr = xml_getElement (big_brother);
    new_ptr = xml_getElement (new);

    new_ptr->ref_count++;	/* Up reference count. DEFUNCT. */

    /* Make the links. 
     */
    if (big_brother_ptr->next)
	big_brother_ptr->last_child->next = new_ptr;
    else
	big_brother_ptr->next = new_ptr;

    big_brother_ptr->last_child = new_ptr;

    new_ptr->parent = big_brother_ptr;
}


/**
 *  xml_dumpXML -- Prints the document tree as readable XML.
 *
 *  @brief  Prints the document tree as readable XML.
 *  @fn     xml_dumpXML (Element *node, int level, int indent, FILE *fd)
 *
 *  @param  node 	A pointer to the Element that you want to print from.
 *  @param  level 	The number of tabs to format the output.
 *  @param  indent 	Number of spaces to indent at each level.
 *  @param  fd 		The file descriptor to send the output to.
 *  @return		nothing
 */
static void xml_dumpXML (Element * node, int level, int indent, FILE * fd)
{
    register int i, space = indent;


    /* If the element is NULL, there is nothing to print.
     */
    if (node == NULL)
	return;

    /* Make spaces based on how deep we are and print the formatted Element. 
     */
    for (i = 0; space && i < (space * level); i++)
	fprintf (fd, " ");
    fprintf (fd, "%s", xml_elemXML (node));

    /* If there are children, recurse to them, print function returns. 
     */
    if (node->child) {
	if (indent)
	    fprintf (fd, "\n");
	xml_dumpXML (node->child, (level + 1), indent, fd);

	/* Print the content between the tags.  */
	if (node->content) {
	    if (node->isCData)
		fprintf (fd, "<![CDATA[%s]]>", node->content);
	    else
		fprintf (fd, "%s", xml_deWS (node->content));
	}

	/*  Make space and print the closing XML tag.
	 */
	for (i = 0; space && i < (space * level); i++)
	    fprintf (fd, " ");
	fprintf (fd, "%s", xml_elemXMLEnd (node));

    } else {			/* This node has no children, beginning of base case. */
	if (node->content) {
	    if (node->isCData)
		fprintf (fd, "<![CDATA[%s]]>", node->content);
	    else
		fprintf (fd, "%s", xml_deWS (node->content));
	}

	/* Print the closing XML tag. 
	 */
	fprintf (fd, "%s", xml_elemXMLEnd (node));
    }

    if (indent)
	fprintf (fd, "\n");

    /* If there are siblings, recurse through them. 
     */
    if (node->next)
	xml_dumpXML (node->next, level, indent, fd);

    /* At this point there should be no more children or sibling on this node.
     */
    fflush (fd);
}


/**
 *  xml_deWS -- Determine whether the input string is nothing but whitespace.
 */
static char *xml_deWS (char *in)
{
    /* KJM: the original code fails when input parameter in is a NULL pointer 
     */
#undef ORIGINAL_CODE
#ifdef ORIGINAL_CODE
    char *ip = in;

    for (ip = in; *ip && isspace (*ip); ip++);
    return ((*ip ? in : ""));
#endif
    {				// KJM
	char *ip = in;

	if (NULL == ip)
	    return ("");	// a NULL pointer now returns an empty string
	for (ip = in; *ip && isspace (*ip); ip++);
	return ((*ip ? in : ""));
    }
}


#ifdef USE_VALIDITY
/**
 *  xml_numElements -- Return the number of unique element names in the doc.
 */
static int xml_numElements (void)
{
    return (MAX_ELEMENTS);	/* FIXME        */
}


/**
 *  xml_validParents -- Return the mask of valid parents for the type.
 */
static int xml_validParents (int type)
{
    int i;

    for (i = 0; i < xml_numElements (); i++) {
	if (elemParents[i].type >= 0 && elemParents[i].type)
	    return (elemParents[i].parents);
    }

    return (0);
}


/**
 *  xml_validChildren -- Return the mask of valid children for the type.
 */
static int xml_validChildren (int type)
{
    int i;

    for (i = 0; i < xml_numElements (); i++) {
	if (elemParents[i].type >= 0 && elemParents[i].type)
	    return (elemParents[i].children);
    }

    return (0);
}
#endif


/*  Debug utility
 */
#ifdef USE_DEBUG
static void xmlBob (void)
{
}
#endif

///// KJM:EXPERIMENTAL /////

void xml_dumpXMLpublic (	// an equivalent public function of the private function xml_dumpXML
			   handle_t elem_h,
			   int level, int indent, FILE * fd) {
    Element *elem = xml_getElement (elem_h);

    xml_dumpXML (elem, level, indent, stdout);
    return;
}
