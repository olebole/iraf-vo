/**
 *  XMLPARSE.H -- Public procedure declarations for the XML parser interface.
 *
 *  @file       xmlParse.h
 *  @author     Mike Fitzpatrick and Eric Timmermann and Ken Mighell
 *  @date       8/03/09
 *  @date       8/18/14
 *
 *  @brief      Public procedure declarations for the XML parser interface.
 */


/**
 *  VOTable element types
 */
#define	NUM_ELEMENTS	25

#define	TY_ROOT		000000000	/* Element Type Definitions     */
#define	TY_TOPLEVEL	000000001

#define	TY_RESOURCE	000000002
#define	TY_FIELD	000000004
#define	TY_PARAM	000000010
#define	TY_INFO		000000020
#define	TY_TR		000000040
#define	TY_TD		000000100
#define TY_TABLE    	000000200
#define TY_STREAM   	000000400
#define TY_FITS    	000001000
#define TY_GROUP   	000002000
#define TY_FIELDREF 	000004000
#define TY_PARAMREF 	000010000
#define TY_MIN      	000020000
#define TY_MAX      	000040000
#define TY_OPTION   	000100000
#define TY_VALUES   	000200000
#define TY_LINK     	000400000
#define TY_DATA     	001000000
#define TY_DESCRIPTION 	002000000
#define TY_TABLEDATA    004000000
#define TY_BINARY     	010000000
#define TY_BINARY2     	020000000

#define TY_COOSYS     	100000000	/* deprecated elements          */
#define TY_DEFINITIONS  200000000


#ifndef	OK			/* Utility values               */
#define	OK		0
#endif
#ifndef	ERR
#define	ERR		1
#endif

#ifndef TRUE
#define TRUE    	1
#endif
#ifndef FALSE
#define FALSE   	0
#endif


#ifndef  handle_t
#define  handle_t	int
#endif


/** *************************************************************************
 *  Public LIBVOSI interface.
 ** ************************************************************************/

handle_t xml_openXML (char *arg);
void xml_closeXML (handle_t vot);

handle_t xml_newNode (handle_t parent, int type);
void xml_freeNode (handle_t delete_me);
void xml_attachNode (handle_t parent, handle_t new);
void xml_deleteNode (handle_t element);
handle_t xml_copyElement (handle_t src_h, handle_t parent_h);


/*****************************************************************************
 *  Utility methods
 ****************************************************************************/

handle_t xml_findByAttr (handle_t parent, char *name, char *value);
handle_t *xml_findInGroup (handle_t group, int type);
handle_t xml_getNext (handle_t elem_h);
handle_t xml_getSibling (handle_t elem_h);
handle_t xml_getChild (handle_t elem_h);
handle_t xml_getParent (handle_t elem_h);
handle_t xml_getChildOfType (handle_t elem_h, int type);
handle_t xml_getToplevel (handle_t root);
int xml_valueOf (handle_t elem_h);
int xml_typeOf (handle_t elem_h);
int xml_handleCount ();

char *xml_elemNameByHandle (handle_t elem);
char *xml_getToplevelName (handle_t root);
char *xml_getToplevelNSpace (handle_t root);

int xml_setValue (handle_t elem_h, char *value);
char *xml_getValue (handle_t elem_h);
int xml_setAttr (handle_t elem_h, char *attr, char *value);
char *xml_getAttr (handle_t elem_h, char *attr);
char *xml_dumpAllAttr (handle_t elem_h);
char *xml_dumpNamesAttr (handle_t elem_h);

void xml_setWarnings (int value);
void xmlEmsg (char *msg);

///// KJM:EXPERIMENTAL /////
void xml_dumpXMLpublic (handle_t elem_h, int level, int indent, FILE * fd);

//EOF
