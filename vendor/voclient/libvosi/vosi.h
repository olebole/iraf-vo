/**
 *  VOSI.H -- Public functions of the LIBVOSI interface.
 *
 *  @file       vosi.h
 *  @author     Ken Mighell and Mike Fitzpatrick
 *  @date       8/11/14
 *  @date       9/11/14 KJM
 *
 *  @brief      Public functions of the LIBVOSI interface.
 */


/**
 *  VOSI element types
 */
#define	NUM_ELEMENTS	25

#define	TY_ROOT		000000000	/* Element Type Definitions     */
#define	TY_VOTABLE	000000001
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

/* service types
 */
#define ST_ROOT              	00000
#define ST_NONE              	00000
#define ST_UNKNOWN           	00001
#define ST_VOSIAVAILABILITY  	00002
#define ST_VOSICAPABILITIES  	00004
#define ST_VOSITABLES        	00010
#define ST_WEBBROWSER        	00020
#define ST_CONESEARCH        	00040
#define ST_SIMPLEIMAGEACCESS 	00100
#define ST_TABLEACCESS       	00200

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

/*  #availability interface
 */
handle_t vosi_getAvailability (char *arg);
int vosi_availAvailable (handle_t avail);
char *vosi_availUpSince (handle_t avail);
char *vosi_availDownAt (handle_t avail);
char *vosi_availBackAt (handle_t avail);
char *vosi_availNote (handle_t avail);


/*  #capability interface
 */
handle_t vosi_getCapabilities (char *arg);
handle_t vosi_nextCapability (handle_t cap);
handle_t vosi_capabilityByIndex (int index);
int vosi_numCapabilities (void);
char *vosi_capStandardID (handle_t cap);
char *vosi_capIVORN (handle_t cap);
char *vosi_capLRN (handle_t cap);
char *vosi_capNStype (handle_t cap);
char *vosi_capAccessURL (handle_t cap);
int vosi_capIndex (handle_t cap);
int vosi_capServiceType (handle_t cap);
char *vosi_capServiceTypeNames (int);


/*  #tables interface
 */
handle_t vosi_getTables (char *arg);

handle_t vosi_schemaByIndex (handle_t tableset, int index);
handle_t vosi_getSchema (handle_t tableset);
int vosi_numSchema (handle_t tableset);
handle_t vosi_nextSchema (handle_t schema);
int vosi_schemaIndex (handle_t schema);
char *vosi_schemaName (handle_t schema);
char *vosi_schemaTitle (handle_t schema);
char *vosi_schemaDescription (handle_t schema);
char *vosi_schemaUType (handle_t schema);

int vosi_numTable (handle_t schema);
handle_t vosi_getTable (handle_t schema);
handle_t vosi_tableByIndex (handle_t schema, int index);
handle_t vosi_nextTable (handle_t table);
char *vosi_tableName (handle_t table);
char *vosi_tableTitle (handle_t table);
char *vosi_tableDescription (handle_t table);
char *vosi_tableUType (handle_t table);
int vosi_tableIndex (handle_t table);

int vosi_numCols (handle_t table);
handle_t vosi_getColumn (handle_t table);
handle_t vosi_colByIndex (handle_t table, int index);
handle_t vosi_nextColumn (handle_t column);
int vosi_colIndex (handle_t column);
char *vosi_colName (handle_t column);
char *vosi_colDescription (handle_t column);
char *vosi_colUnit (handle_t column);
char *vosi_colUCD (handle_t column);
char *vosi_colUType (handle_t column);
char *vosi_colDatatype (handle_t column);

int vosi_numKeys (handle_t column);
handle_t vosi_getKey (handle_t column);
handle_t vosi_keyByIndex (handle_t column, int index);
handle_t vosi_nextKey (handle_t key);
int vosi_keyIndex (handle_t key);
char *vosi_keyTargetTable (handle_t key);
char *vosi_keyFromColumn (handle_t key);
char *vosi_keyTargetColumn (handle_t key);
char *vosi_keyDescription (handle_t key);
char *vosi_keyUType (handle_t key);


/*****************************************************************************
 *  Utility methods
 ****************************************************************************/

