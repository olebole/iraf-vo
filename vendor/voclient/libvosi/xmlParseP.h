/**
 *  XMLPARSEP.H -- Internal XML parser definitions.
 *
 *  @file       xmlParseP.h
 *  @author     Mike Fitzpatrick and Eric Timmermann
 *  @date       8/03/09
 *
 *  @brief      Internal XML parser definitions.
 */


#include <expat.h>

#define	VOT_DOC_VERSION		"1.2"	/** VOTable document version (write)  */

#define	VOT_XSI			"http://www.w3.org/2001/XMLSchema-instance"
#define	VOT_SCHEMA_LOC		"http://www.ivoa.net/xml/VOTable/v1.1  http://www.ivoa.net/xml/VOTable/v1.1"
#define	VOT_XMLNS		"http://www.ivoa.net/xml/VOTable/v1.1"


#define SZ_ELEMNAME             32	/** size of element name              */
#define SZ_ATTRNAME             32	/** size of attribute name            */
#define SZ_ATTRVAL              2048	/** size of attribute value           */
#define SZ_FNAME                255	/** size of filename+path             */
#define SZ_XMLTAG               1024	/** max length of entire XML tag      */
#define SZ_LINE                 4096	/** handy size                        */

#define MAX_ATTR                100	/** max size of an attribute/value    */
#define MAX_ELEMENTS            1024	/** max number of elements            */
#define HANDLE_INCREMENT        1024000	/** incr size of handle table         */


#ifdef  min
#undef  min
#endif
#define min(a,b)	((a<b)?a:b)

#ifdef  max
#undef  max
#endif
#define max(a,b)	((a>b)?a:b)




/**
 *  @brief  Handle type definition
 */
#ifndef  handle_t
#define  handle_t       int
#endif



/** 
 *  @struct AttrList.
 *  @brief 		Information for an attribute.
 *  @param name 	A string of the attributes name.
 *  @param value 	A string of the attributes value.
 *  @param *next 	A pointer to the next element.
 */
typedef struct {
    char name[SZ_ATTRNAME];
    char value[SZ_ATTRVAL];
    void *next;
} AttrList;


/**
 *  @struct AttrBlock
 *  @brief 		Information for a block of attributes.
 *  @param req 		A '|' delimited string of required attribute names.
 *  @param opt 		A '|' delimited string of optional attribute names.
 *  @param attributes 	A pointer to an AttrList structure.
 */
typedef struct {
    char *req;
    char *opt;
    void *attributes;
} AttrBlock;


/**
 *  @struct Element
 *
 *  @brief This is a structure that hold the information for an XML element.
 */
typedef struct elem_t {
    char ns[SZ_ELEMNAME];     /** @brief   Element namespace		      */
    char name[SZ_ELEMNAME];   /** @brief   Element name			      */
    unsigned int type;	      /** @brief   Type of element this is            */

    AttrBlock *attr;	      /** @brief   A pointer to a block of attributes */
    char *content;	      /** @brief   Content of the tag elements        */
    int isCData;	      /** @brief   Is value a CDATA string?           */
    int handle;		      /** @brief   handle value			      */

    struct elem_t *next;      /** @brief   Ptr to the next element (sibling)  */
    struct elem_t *last_child;/** @brief   Ptr the the last child element     */
    struct elem_t *child;     /** @brief   Ptr the the child element          */
    struct elem_t *parent;    /** @brief   Ptr to the parent element          */

    unsigned char ref_count;  /** @brief   No. refrences to this Element      */
} Element;


/**
 *  @struct 	Node
 *  @brief 	Struct that holds a stack Node containing an Element
 */
typedef struct node {
    Element *element;
    void *next;
} Node;


/**
 *  @struct 	Stack
 *  @brief 	This is a structure that holds the information for a stack
 */
typedef struct {
    Node *head;
    int level;
} Stack;



/** ***************************************************************************
 *
 *  Public Internal Methods.  The procedures are used to implement the
 *  library, however are not part of the public interface.
 *
 ** **************************************************************************/

/*  xmlAttribute.c
 */
int xml_attrSet (AttrBlock * ablock, char *name, char *value);
char *xml_attrGet (AttrBlock * ablock, char *name);
char *xml_attrXML (AttrBlock * ablock);
char *xml_attrNames (AttrBlock * ablock);

/*  xmlElement.c
 */
int xml_eType (char *name);
char *xml_elemName (Element * e);
int xml_elemType (Element * e);
char *xml_elemXML (Element * e);
char *xml_elemXMLEnd (Element * e);
Element *xml_newElem (unsigned int type);

/*  xmlHandle.c
 */
handle_t xml_setHandle (Element * elem);
handle_t xml_lookupHandle (Element * elem);
void xml_freeHandle (handle_t handle);
Element *xml_getElement (handle_t handle);
void xml_newHandleTable (void);
int xml_handleCount (void);
void xml_handleCleanup (void);
void xml_handleError (char *msg);

/*  xmlParseCB.c
 */
void xml_endElement (void *userData, const char *name);
void xml_startElement (void *userData, const char *name,
		       const char **atts);
void xml_charData (void *userData, const XML_Char * s, int len);
void xml_startCData (void *userData);
void xml_endCData (void *userData);

/*  xmlStack.c
 */
void xmlPush (Stack * st, Element * elem);
Element *xmlPop (Stack * st);
Element *xmlPeek (Stack * st);

Stack *xml_newStack (void);
int xml_isEmpty (Stack * st);
void xml_clearStack (Stack * st);
void xml_printStack (Stack * st);
