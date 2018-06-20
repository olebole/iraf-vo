/*
 *  DALCLIENT.H -- Global include file for the DALClient Interface.
 *
 *  The DALClient library is usable directly, or as a component of VOClient.
 *  It requires the VOClient VOTable library (hence also expat for XML
 *  processing), and libCurl for URL processing.
 *
 *  D.Tody, NRAO/VAO, January 2014
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>


/*
 * Function prototypes
 * -----------------------------
 */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#else
char    *getenv();
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Interface Definitions.
 * -----------------------------
 */
#ifdef OK
#undef OK
#endif
#define OK	0

#ifdef ERR
#undef ERR
#endif
#define ERR	1

#ifdef TRUE
#undef TRUE
#endif
#define TRUE	1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE	0

#define DAL_INFO		1	/* VOTable object types	*/
#define DAL_PARAM		2
#define DAL_FIELD		3

#define DAL_ID			1	/* Field attributes */
#define DAL_NAME		2
#define DAL_UTYPE		3
#define DAL_UCD			4
#define DAL_UNIT		5
#define DAL_XTYPE		6
#define DAL_DATATYPE		7
#define DAL_ARRAYSIZE		8
#define DAL_WIDTH		9
#define DAL_PRECISION		10
#define DAL_VALUE		11

#ifndef _VOCLIENT_LIB_
typedef int	DAL;			/* DAL function types */
typedef int	POS;
typedef int	Query;
typedef int	QResponse;
typedef int	QRecord;
typedef int	handle_t;
#endif


/*
 * DAL error status and error codes.
 * -----------------------------
 */
#define	DAL_OK	   		0
#define	DAL_ERROR  		-9999999		

#define DAL_BADHANDLE           100     /* Invalid object handle */
#define DAL_MEMALLOCFAIL        101     /* Cannot allocate memory */
#define DAL_VLLINIT             102     /* Cannot initialize List object */
#define DAL_VHTINIT             103     /* Cannot init Hash Table object */
#define DAL_NOBASEURL           104     /* Service baseurl not specified */
#define DAL_NOPROTOCOL          105     /* Service protocol not specified */
#define DAL_NOVERSION           106     /* Service version not specified */
#define DAL_PARAMNOTFOUND       107     /* Query parameter not found */
#define DAL_PARAMAPPEND         108     /* Cannot add param */
#define DAL_PARAMHASH           109     /* Hash entry for param not found */

#define DAL_FORMQUERYURL        120     /* Error composing query URL */
#define DAL_EXECUTEQUERY        121     /* Error executing query to service
*/
#define DAL_INITQUERYRESPONSE   122     /* Error compiling query response */
#define DAL_QUERYNOTEXEC        123     /* No query response */
#define DAL_VOTABLEPARSE        124     /* Error parsing votable */
#define DAL_VOQUERYERROR        125     /* Service returned STATUS=ERROR */
#define DAL_VOTABLENORESOURCE   126     /* VOTable resource elem not found */
#define DAL_VOTABLENOTABLE      127     /* VOTable table elem not found */
#define DAL_VOTABLENODATA       128     /* VOTable data elem not found */
#define DAL_VOTABLENOTDATA      129     /* VOTable tdata elem not found */
#define DAL_CMEMINIT            130     /* CMEM init failure */
#define DAL_QRINFOERR           131     /* Error accessing INFO */
#define DAL_QRFIELDERR          132     /* Error accessing FIELD */
#define DAL_ATTRNOTFOUND        133     /* Attribute not found */
#define DAL_FIELDNOTFOUND       134     /* Field not found */
#define DAL_RECORDNOTFOUND      135     /* Record not found */
#define DAL_PROPNOTFOUND        136     /* Property not found */
#define DAL_INVALIDINTPROP      137     /* Cannot convert property to int */
#define DAL_INVALIDFLOATPROP    138     /* Cannot convert property to float
*/
#define DAL_INDEXOUTOFBOUNDS    139     /* Field index out of bounds */
#define DAL_INVALIDINTFIELD     140     /* Cannot convert field to int */
#define DAL_INVALIDFLOATFIELD   141     /* Cannot convert field to float */
#define DAL_INVALIDURL          142     /* Invalid or NULL URL */
#define DAL_INVALIDFNAME        143     /* Invalid or NULL filename */
#define DAL_CANNOTOPENFILE      144     /* Cannot open or create file */

#define DAL_CURLINITERR         151     /* Cannot initialize Curl subsystem
*/
#define DAL_CURLEXECERR         152     /* Error executing Curl */
#define DAL_HOSTNOCONNECT       153     /* Couldn't connect to host */
#define DAL_HTTPERROR           154     /* HTTP error from service */
#define DAL_HTTPTIMEOUT         155     /* HTTP request timedout */
#define DAL_HTTPREDIRLOOP       156     /* HTTP redirect loop */
#define DAL_FILETOOLARGE        157     /* file too large */
#define DAL_CURLERRBASE         1000    /* For pass-through of curl stat */


/*
 * DALClient Private Definitions
 * -----------------------------
 */
#ifdef _DALCLIENT_LIB_

#include "vocList.h"
#include "vocHash.h"

#define	DEF_RUNID		"DALClient"

#define SZ_FNAME        	256
#define SZ_URL        		2048
#define SZ_ERRMSG               80
#define SZ_NAME 	       	64	/* Generic name of something	*/
#define SZ_KEY	 	       	256	/* Max size of a hash key	*/
#define SZ_PNAMEHASH		64	/* Parameter name hash table	*/
#define SZ_FNAMEHASH		256	/* Field name hash table	*/
#define SZ_VALSTR		1024	/* Parameter/property value	*/
#define MAX_PROP		64	/* Max Properties		*/
#define MAX_COLS		200	/* Max table columns		*/

#define DAL_CONN   		1	/* DAL Connection Types		*/
#define CONE_CONN  		2	/* Simple Cone Search		*/
#define SIAP_CONN  		3	/* Simple Image Access		*/
#define SSAP_CONN  		4	/* Simple Spectral Access	*/
#define SLAP_CONN  		5	/* Simple Line Access		*/
#define STAP_CONN  		6	/* Synch TAP Access		*/

#define DAL_QueryData           1       /* QueryData request            */
#define DAL_AccessData          2       /* AccessData request           */
#define DAL_GetMetadata         3       /* GetMetadata request          */

/* Service context.
 */
typedef struct dalConn_t {
    DAL		dal_h;			/* Connection handle */
    int		type;			/* Service type code */
    char	protocol[SZ_NAME];	/* Service protocol name */
    char	version[SZ_NAME];	/* Service version */
    char	baseurl[SZ_URL];	/* BaseURL for service */
    int		context;		/* Handle context */
    int		status;			/* Error status */
    int		debug;			/* Debug level (0=off) */
    int		errcode;		/* Error code */
    char	errmsg[SZ_ERRMSG];	/* Error message */
} dalConn_t;


/* ParameterSet used to build a query against a service.
 */
typedef struct dalQuery_t {
    Query	query_h;		/* Query object (max 1) */
    QResponse	qr_h;			/* Last QResponse object */
    dalConn_t	*dal;			/* Service context */
    int		request;		/* Type of request */
    vocList_t	*params;		/* Linked list of parameters. */
    vocHash_t	*pnames;		/* Hashed by param name */
} dalQuery_t;


/* Parameter object.
 */
typedef struct dalParam_t {
    char	name[SZ_NAME];		/* Parameter name. */
    char	value[SZ_VALSTR];	/* Parameter value. */
    long	index;			/* List index back-reference */
    struct	dalParam_t *next;	/* Next instance of this param. */
} dalParam_t;


/* Spatial position or region.  This is generic and is translated
 * into whatever is required to query an individual service.
 */
typedef struct dalPos_t {
    char	objname[SZ_NAME];	/* Object name, if any */
    char	frame[SZ_NAME];		/* Coordinate system or frame */
    double	c1, c2;			/* e.g., ra,dec or long,lat */
    double	size1, size2;		/* width of region along either axis */
} dalPos_t;


/* Query response object.  This is a generic table query result set object,
 * optimized for efficient runtime access.  All table metadata is cached in
 * the descriptor and hashed for efficient access.  Table data (VOTable FIELD
 * elements) remains in the VOTable and is accessed via the VOTable API.
 * Data model properties are computed for each row of the table and stored
 * in a hash table, one per table row.
 */
typedef struct dalQR_t {
    QResponse	qr_h;			/* Query response handle */
    dalQuery_t	*query;			/* Query context */
    char	*votable;		/* Cached votable QR text */
    char	*delimited;		/* Cached delimited QR text */
    int		nrows;			/* Number of table rows */
    int		ncols;			/* Number of table columns */
    vocList_t	*infos;			/* List of INFO elements */
    vocList_t	*fields;		/* List of PARAMs and FIELDs */
    vocList_t	*properties;		/* List of object properties */
    vocHash_t	*hash_prop;		/* Hash by UCD attribute */
    vocHash_t	*hash_id;		/* Hash by ID attribute */
    vocHash_t	*hash_name;		/* Hash by NAME attribute */
    vocHash_t	*hash_utype;		/* Hash by UTYPE attribute */
    vocHash_t	*hash_ucd;		/* Hash by UCD attribute */
    struct	dalRecord_t *rows;	/* Record storage */
    void 	*cmem;			/* Temp storage for string data */
} dalQR_t;


/* Table Record object.
 */
typedef struct dalRecord_t {
    dalQR_t	*qr;			/* Query response */
    int		recnum;			/* Record number  */
    char	*pdata[MAX_PROP];	/* property values */
    char	*cdata[MAX_COLS];	/* column data */
} dalRecord_t;


/* Table Property object.  Properties are abstract properties of the object
 * stored in a table or table row.  Properties provide a level of abstraction
 * one level higher than the underlying VO data model, allowing the major
 * properties of the object to be accessed by simple fixed names, hiding
 * differences that may occur in versions of the underlying data model
 * (Utypes for example may vary with different VO data model versions).
 */
typedef struct dalProp_t {
    int		type;			/* PARAM (fixed) or FIELD (per row) */
    long	index;			/* List index back-reference */
    long	colnum;			/* Column number if per-row value */
    char	name[SZ_NAME];		/* NAME attribute */
    char	value[SZ_VALSTR];	/* Value for PARAM or INFO */
} dalProp_t;


/* Table Field object (VOTable FIELD or PARAM).  INFO elements may also
 * be stored as a dalField.  The value is stored for a PARAM (or INFO),
 * but not for a FIELD.
 */
typedef struct dalField_t {
    int		type;			/* FIELD, PARAM, INFO */
    long	index;			/* List index back-reference */
    int		colnum;			/* Table column, if FIELD */
    char	id[SZ_NAME];		/* ID attribute */
    char	name[SZ_NAME];		/* NAME attribute */
    char	utype[SZ_KEY];		/* UTYPE attribute */
    char	ucd[SZ_NAME];		/* UCD attribute */
    char	unit[SZ_NAME];		/* Unit attribute */
    char	xtype[SZ_NAME];		/* Xtype attribute */
    char	datatype[SZ_NAME];	/* Datatype attribute */
    char	arraysize[SZ_NAME];	/* Arraysize attribute */
    char	width[SZ_NAME];		/* Width attribute */
    char	precision[SZ_NAME];	/* Precision attribute */
    char	value[SZ_VALSTR];	/* Value for PARAM or INFO */
} dalField_t;


#define	DAL_DEBUG	(dal->debug > 0)
#endif


/*
 * Public DAL Interface functions.
 * -----------------------------
 */

/* dalQuery.c */
DAL   dal_openConnection (char *baseurl, char *protocol, char *version);
DAL   dal_openConeConnection (char *baseurl, char *version);
DAL   dal_openSiapConnection (char *baseurl, char *version);
DAL   dal_openSsapConnection (char *baseurl, char *version);
void  dal_closeConnection (DAL dal_h);

void  dal_setBaseUrl (DAL dal_h, char *base_url);
char *dal_getBaseUrl (DAL dal_h);

Query dal_getQuery (DAL dal_h);
void  dal_closeQuery (Query query_h);
Query dal_getConeQuery (DAL dal_h, double ra, double dec, double sr);
Query dal_getSiapQuery (DAL dal_h, double ra, double dec, double ra_size,
			double dec_size, char *format);
Query dal_getSsapQuery (DAL dal_h, double ra, double dec, double size,
			char *band, char *time, char *format);
int   dal_getParamCount (Query query_h);
char *dal_getParamName (Query query_h, int index);
int   dal_addIntParam (Query query_h, char *pname, long value);
int   dal_addFloatParam (Query query_h, char *pname, double value);
int   dal_addStringParam (Query query_h, char *pname, char *value);
int   dal_setParam (Query query_h, char *pname, char *value);
char *dal_getParam (Query query_h, char *pname, int index);
int   dal_delParam (Query query_h, char *pname);
char *dal_getQueryURL (Query query_h);
int   dal_getError (DAL dal_h);
char *dal_getErrorMsg (DAL dal_h);
void  dal_clearError (DAL dal_h);

/* dalQExec.c */
QResponse dal_executeQuery (Query query_h);
QResponse dal_getQueryResponse (Query query_h);
QResponse dal_initQueryResponse (DAL dal_h, char *votable, int resource,
		int table);
void    dal_closeQueryResponse (Query query_h);
int 	dal_accessData (Query query_h, char *fname);
char   *dal_executeVOTable (Query query_h);
char   *dal_executeCSV (Query query_h);
char   *dal_executeTSV (Query query_h);
char   *dal_executeASCII (Query query_h);
char   *dal_executeDelimited (Query query_h, char delim);

/* dalQResp.c */
int    dal_getRecordCount (QResponse qr_h);
int    dal_getColumnCount (QResponse qr_h);
int    dal_getInfoCount (QResponse qr_h);
int    dal_getFieldCount (QResponse qr_h);
int    dal_getPropCount (QResponse qr_h);
char  *dal_getPropName (QResponse qr_h, int index);
char  *dal_getInfoAttr (QResponse qr_h, int index, int attr);
char  *dal_getFieldAttr (QResponse qr_h, int index, int attr);
int    dal_getFieldIndex (QResponse qr_h, char *key, int attr);
QRecord dal_getRecord (QResponse qr_h, int recnum);
void   dal_releaseRecord  (QRecord rec_h);
int    dal_getIntProperty (QRecord rec_h, char *property);
double dal_getFloatProperty (QRecord rec_h, char *property);
char  *dal_getStringProperty (QRecord rec_h, char *property);
int    dal_getIntField (QRecord rec_h, int index);
double dal_getFloatField (QRecord rec_h, int index);
char  *dal_getStringField (QRecord rec_h, int index);
int    dal_getIntColumn (QResponse qr_h, int index, long *iptr);
int    dal_getFloatColumn (QResponse qr_h, int index, double *dptr);
int    dal_getStringColumn (QResponse qr_h, int index, char **sptr);
int    dal_getDataset (QRecord rec_h, char *acref, char *fname);

/* dalPOS.c */
#ifndef _VOCLIENT_LIB_
POS    dal_setPos  (DAL dal_h, double c1, double c2, double size, char *frame);
#else
int    dal_setPos  (DAL dal_h, double c1, double c2, double size, char *frame);
#endif


/*
 *  Private routines.
 * -----------------------------
 */
#ifdef _DALCLIENT_LIB_

/* dalQuery.c */
int   dal_error (dalConn_t *dal, int errcode);
void *dal_nError (dalConn_t *dal, int errcode);
int   dal_qError (dalQuery_t *query, int errcode);
int   dal_qrError (dalQR_t *qr, int errcode);

/* dalQResp.c */
dalProp_t *vut_addProperty  (dalQR_t *qr, char *propname, char *value, int type);
dalProp_t *vut_setProperty  (QRecord rec_h, char *propname, char *value);

/* dalSCSP.c */
char *scsp_getQueryURL  (dalQuery_t *query);
void  scsp_initProperties  (dalQR_t *qr);

/* dalSIAP.c */
char *siap_getQueryURL  (dalQuery_t *query);
void  siap_initProperties  (dalQR_t *qr);

/* dalSSAP.c */
char *ssap_getQueryURL  (dalQuery_t *query);
void  ssap_initProperties  (dalQR_t *qr);

/* svrHandle.c */
void  svr_initHandles (void);
void  svr_resetHandles (void);
int   svr_newHandleContext (char *name, int size);
void  svr_closeHandleContext (int context);
handle_t svr_newHandle (int context, void *ptr);
void  svr_freeHandle (handle_t handle);
void *svr_H2P (handle_t handle);

/* vocHash.c */
vocHash_t *vht_init (size_t size, int  (*hash_func) (void *, size_t, size_t),
		char *flag);
void  *vht_search (vocHash_t *hashtable, void *key, size_t keylen);
void  *vht_findKey (vocHash_t *hashtable, char *key);
void  *vht_insert (vocHash_t *hashtable, void *key, size_t keylen, void *value, size_t vallen);
void  *vht_insertKey (vocHash_t *hashtable, char *key, void *value);
void   vht_remove (vocHash_t *hashtable, void *key, size_t keylen);
void   vht_removeKey (vocHash_t *hashtable, char *key);
void  *vht_grow (vocHash_t *old_ht, size_t new_size);
void   vht_destroy (vocHash_t *hashtable);
void   vht_iter_init (vocHash_t *hashtable, vocHash_iter_t *ii);
void   vht_iter_inc (vocHash_iter_t *ii);
int    vht_hash (void *key, size_t keylen, size_t hashtab_size);

/* vocList.c */
vocList_t *vll_init (long req_size);
long   vll_count (vocList_t *list);
void  *vll_seek (vocList_t *list, long offset, int whence);
void  *vll_find (vocList_t *list, void *obj, long *index);
void  *vll_prev (vocList_t *list);
void  *vll_next (vocList_t *list);
void  *vll_insert (vocList_t *list, void *obj, long *index);
void  *vll_append (vocList_t *list, void *obj, long *index);
void  *vll_remove (vocList_t *list, long req_pos);
void   vll_destroy (vocList_t *list, void  (*func) (void *ptr));
int    vll_rebuild_index (vocList_t *list, int req_size);

/* vutUtil.c */
void  *vut_urlOpen (char *baseURL, char *resource);
void   vut_urlAddParam (void *urlBuilder, char *param, char *valstr);
char  *vut_urlGetString (void *urlBuilder);
void   vut_urlClose (void *urlBuilder);
int    vut_getURLtoFile (dalConn_t *dal, char *url, char *fname);
char  *vut_getURL (dalConn_t *dal, char *url);
char  *vut_setString (char *out, char *in, int size);
void  *vut_cmemInit (void);
void   vut_cmemDestroy (void *cmem);
char  *vut_cmemCopy (void *cmem, char *str);
char  *vut_votableToDelimited (char *votable, char delim);

#endif


#ifdef __cplusplus
}
#endif
