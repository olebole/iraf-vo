/*
 *  VOCLIENT.H == Global include file for the VOClient Interface.
 *
 *  M. Fitzpatrick, NOAO, June 2006
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <fcntl.h>


/* Function prototypes */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#else
char    *getenv();
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* For the signal handling. */
typedef void  (*SIGFUNC)();


/* Interface Variables.
 */



#ifdef OK
#undef OK
#endif
#define OK        	0

#ifdef ERR
#undef ERR
#endif
#define ERR        	1

#ifdef TRUE
#undef TRUE
#endif
#define TRUE        	1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE        	0

#define	DEF_SERVER		"6200:localhost"
#define	DEF_NET_SERVER		"9000:iraf-nvo.noao.edu"

#define	DEF_RUNID		"VOClient"

#define MSG_CALL        	1
#define MSG_RESULT      	2
#define MSG_MESSAGE     	3
#define MSG_END     		4
#define MSG_QUIT     		5
#define MSG_ACK     		6
#define MSG_NOACK     		7

#define	TY_INT			1	/* result data types		*/
#define	TY_FLOAT		2
#define	TY_STRING		3
#define	TY_BULK			4

#define SZ_MSGBUF       	102400
#define SZ_METHOD       	64
#define SZ_CLASS        	64
#define SZ_MSGSTR       	65535
#define SZ_PBUF         	128
#define SZ_FNAME        	256
#define SZ_URL	        	2048
#define MAX_VALUES      	64

#define DAL_CONN   		1	/* DAL Connection Types		*/
#define CONE_CONN  		2	/* Simple Cone Search		*/
#define SIAP_CONN  		3	/* Simple Image Access		*/
#define SSAP_CONN  		4	/* Simple Spectral Access	*/
#define SLAP_CONN  		5	/* Simple Line Access		*/
#define STAP_CONN  		6	/* Synch TAP Access		*/

#define CONE_SERVICE  		1
#define SIAP_SERVICE  		2
#define SSAP_SERVICE  		3
#define TAP_SERVICE  		4
#define REG_SERVICE  		5
#define SKYNODE_SERVICE  	6
#define WEB_SERVICE  		7
#define REST_SERVICE  		8

#define VOC_RAW			0
#define VOC_CSV			1
#define VOC_TSV			2
#define VOC_ASCII		3
#define VOC_VOTABLE		4

#define VOC_NULL		0	/* integer NULL			*/


typedef int   ObjectID;			/* DAL type aliases		*/
typedef int   DAL;
typedef int   Query;
typedef int   QResponse;
typedef int   QRecord;
typedef int   QRAttribute;

typedef int   Sesame;			/* Name Resolver aliases	*/

typedef int   Skybot;			/* SkyBoT Service aliases	*/

typedef int   RegQuery;			/* Registry Query object	*/
typedef int   RegResult;		/* Query Reuslt object		*/

#ifdef _VOCLIENT_LIB_

typedef struct vocMsg {
    int         type;                   /* message type                 */

    ObjectID    objId;                  /* for CALL messages            */
    char        method[SZ_METHOD];
    int         nparams;                

    int         status;                 /* for RESULT messages          */
    int         restype;                
    int         nitems;         

    char        msgclass[SZ_CLASS];     /* for MESSAGE messages         */
    char        msgstr[SZ_MSGSTR];

    char        message[SZ_MSGBUF];     /* fully formed message         */
} vocMsg_t;



typedef struct vocRes {
    int         status;                 /* result status                */
    int         type;                   /* type of result value         */
    int         nitems;                 /* no. of returned items        */      
    char        value[MAX_VALUES][SZ_MSGSTR];   /* value strings        */

    void	*buf;			/* bulk data buffer		*/
    int		buflen;			/* length of buffer		*/
} vocRes_t;


typedef struct VOClient {
    char   *server_host;                /* socket to DALServer          */
    char   *runid;                      /* RUNID logging string	        */
    int     server_port;
    int     io_chan;

    int     msg_port;                   /* asynch message socket        */
    int     msg_chan;

    int     onetrip;			/* private invocation flag	*/
    int     debug;			/* debug flag			*/
    int     quiet;			/* suppress output?		*/
    int     use_cache;			/* use cached results?		*/
    int     use_runid;			/* use RUNID parameter?		*/
    int     use_mini_d;			/* use mini-daemon 		*/
    int     handle_context;		/* mini-daemon handle context	*/

} VOClient, *VOClientPtr;


#define	VOC_DEBUG	(vo->debug > 0)
#define MSG_DEBUG	(vo->debug > 1)

#endif





/* Prototype Declarations.
 */

/*  DAL Interface procedures.
 */
char       *voc_coneCaller (char *url, double ra, double dec, double sr, 
		int otype);
int         voc_coneCallerToFile (char *url, double ra, double dec, double sr, 
            	int otype, char *file);

char       *voc_siapCaller (char *url, double ra, double dec, double rsize, 
            	double dsize, char *fmt, int otype);
int         voc_siapCallerToFile (char *url, double ra, double dec, 
            	double rsize, double dsize, char *fmt, int otype, char *file);
char       *voc_ssapCaller (char *url, double ra, double dec,
            	double size, char *band, char *time, char *fmt, int otype);
int         voc_ssapCallerToFile (char *url, double ra, double dec, 
            	double size, char *band, char *time, char *fmt, int otype, 
		char *file);
char       *voc_getRawURL (char *url, int *nbytes);
int         voc_validateObject (int hcode);
void        voc_freePointer (char *ptr);


int 	    voc_initVOClient (char *opts);
void	    voc_closeVOClient (int shutdown);
void	    voc_abortVOClient (int code, char *msg);

DAL         voc_openConnection (char *service_url, int type, ...);
DAL         voc_openConeConnection (char *service_url, ...);
DAL         voc_openSiapConnection (char *service_url, ...);
DAL         voc_openSsapConnection (char *service_url, ...);
void	    voc_closeConnection (DAL dal);

int	    voc_getServiceCount (DAL dal);
void	    voc_addServiceURL (DAL dal, char *service_url);
char 	   *voc_getServiceURL (DAL dal, int index);

Query	    voc_getQuery (DAL dal, int type);
Query	    voc_getConeQuery (DAL dal, double ra, double dec, double sr);
Query	    voc_getSiapQuery (DAL dal, double ra, double dec, 
			double ra_size, double dec_size, char *format);
Query	    voc_getSsapQuery (DAL dal, double ra, double dec, 
			double size, char *band, char *time, char *format);

int         voc_addIntParam (Query query, char *name, int value);
int         voc_addFloatParam (Query query, char *name, double value);
int         voc_addStringParam (Query query, char *name, char *value);

char	   *voc_getQueryString (Query query, int type, int index);

QResponse   voc_executeQuery (Query query);
QResponse   voc_getQueryResponse (Query query);
char       *voc_executeCSV (Query query);
char       *voc_executeTSV (Query query);
char       *voc_executeASCII (Query query);
char       *voc_executeVOTable (Query query);
int	    voc_executeQueryAs (Query query, char *fname, int type);
int	    voc_getRecordCount (QResponse qr);

QRecord     voc_getRecord (QResponse qr, int recnum);
char       *voc_getFieldAttr (QResponse qr, int fieldnum, char *attr);
               
QRAttribute voc_getAttribute (QRecord rec, char *attrname);

int         voc_intValue (QRAttribute v);
double      voc_floatValue (QRAttribute v);
char       *voc_stringValue (QRAttribute v);

int         voc_getIntAttr (QRecord rec, char *attr_name);
double      voc_getFloatAttr (QRecord rec, char *attr_name);
char       *voc_getStringAttr (QRecord rec, char *attr_name);

char       *voc_getAttrList (QRecord rec);
int 	    voc_getAttrCount (QRecord rec);

void	    voc_setIntAttr (QRecord rec, char *attrname, int ival);
void	    voc_setFloatAttr (QRecord rec, char *attrname, double dval) ;
void	    voc_setStringAttr (QRecord rec, char *attrname, char *str);

int 	    voc_getDataset (QRecord rec, char *acref, char *fname);



/*  Registry Interface procedures.
 */
RegResult   voc_regSearch (char *term1,  char *term2, int orValues);
RegResult   voc_regSearchByService (char *svc,  char *term, int orValues);
RegQuery    voc_regQuery (char *term, int orValues);
void	    voc_regConstSvcType (RegQuery query, char *svc);
void	    voc_regConstWaveband (RegQuery query, char *bpass);
void	    voc_regDALOnly (RegQuery query, int value);
void	    voc_regSortRes (RegQuery query, int value);
void	    voc_regAddSearchTerm (RegQuery query, char *term, int orValue);
void	    voc_regRemoveSearchTerm (RegQuery query, char *term);
int	    voc_regGetSTCount (RegQuery query);
char	   *voc_regGetQueryString (RegQuery query);
RegResult   voc_regExecute (RegQuery query);
char	   *voc_regExecuteRaw (RegQuery query);

int	    voc_resGetCount (RegResult res);
char	   *voc_resGetStr (RegResult res, char *attribute, int index);
double	    voc_resGetFloat (RegResult res, char *attribute, int index);
int	    voc_resGetInt (RegResult res, char *attribute, int index);



/*  SESAME interface procedures.
 */
Sesame      voc_nameResolver (char *target);
char       *voc_resolverPos (Sesame sr);
double      voc_resolverRA (Sesame sr);
double      voc_resolverDEC (Sesame sr);
double      voc_resolverRAErr (Sesame sr);
double      voc_resolverDECErr (Sesame sr);
char       *voc_resolverOtype (Sesame sr);



/*  SkyBoT interface procedures.
 */
Skybot      voc_skybot (double ra, double dec, double rsz, double dsz, 
		double epoch);
int         voc_skybotNObjs (Skybot sb);
char       *voc_skybotStrAttr (Skybot sb, char *attr, int index);
double      voc_skybotDblAttr (Skybot sb, char *attr, int index);



/***************************************************************************
 *   VOCMSG.C Prototypes
 */
#ifdef _VOCLIENT_LIB_

vocMsg_t *msg_newCallMsg (ObjectID objid, char *method, int nparams);
vocMsg_t *msg_newResultMsg (int status, int type, int nitems);
vocMsg_t *msg_newMsg (char *msgclass, char *str);

vocMsg_t *msg_shutdownMsg ();
vocMsg_t *msg_quitMsg ();
vocMsg_t *msg_ackMsg ();

vocRes_t *msg_sendMsg (int fd, vocMsg_t *msg);
int       msg_sendRawMsg (int fd, vocMsg_t *msg);
int       msg_setMiniDaemon (int val);

vocRes_t *msg_getResult (int fd);
vocRes_t *msg_getResultToFile (int fd, char *fname, int overwrite);

void      msg_addIntParam (vocMsg_t *msg, int ival);
void      msg_addFloatParam (vocMsg_t *msg, double dval);
void      msg_addStringParam (vocMsg_t *msg, char *str);
void      msg_addIntResult (vocMsg_t *msg, int ival);
void      msg_addFloatResult (vocMsg_t *msg, double dval);
void      msg_addStringResult (vocMsg_t *msg, char *str);

int       msg_resultStatus (vocRes_t *res);
int       msg_resultType (vocRes_t *res);
int       msg_resultLength (vocRes_t *res);

int       msg_getIntResult (vocRes_t *res, int index);
double    msg_getFloatResult (vocRes_t *res, int index);
char     *msg_getStringResult (vocRes_t *res, int index);
void     *msg_getBuffer (vocRes_t *res);
char     *msg_getFilename (vocRes_t *res);

#endif



/***************************************************************************
 *   C-Daemon Implementation Prototypes
 */

int   cvo_initVOClient (char *opts);


/*  DAL Services interface.
*/
char *cvo_coneCaller (char *url, double ra, double dec, double sr, int otype);
int   cvo_coneCallerToFile (char *url, double ra, double dec, double sr, 
	int otype, char *file);
char *cvo_siapCaller (char *url, double ra, double dec, double rsize, 
	double dsize, char *fmt, int otype);
int   cvo_siapCallerToFile (char *url, double ra, double dec, double rsize, 
	double dsize, char *fmt, int otype, char *file);
char *cvo_ssapCaller (char *url, double ra, double dec, double size, 
	char *band, char *time, char *fmt, int otype);
int   cvo_ssapCallerToFile (char *url, double ra, double dec, double size, 
	char *band, char *time, char *fmt, int otype, char *file);

char *cvo_getRawURL (char *url, int *nbytes);

DAL   cvo_openConnection (char *service_url, int type, ...);
DAL   cvo_openConeConnection (char *service_url, ...);
DAL   cvo_openSiapConnection (char *service_url, ...);
DAL   cvo_openSsapConnection (char *service_url, ...);
void  cvo_closeConnection (DAL dal);

int   cvo_getServiceCount (DAL dal);
void  cvo_addServiceURL (DAL dal, char *service_url);
char *cvo_getServiceURL (DAL dal, int index);

Query cvo_getQuery (DAL dal, int type);
Query cvo_getConeQuery (DAL dal, double ra, double dec, double sr);
Query cvo_getSiapQuery (DAL dal, double ra, double dec, double ra_size, 
	double dec_size, char *format);
Query cvo_getSsapQuery (DAL dal, double ra, double dec, double size, 
	char *band, char *tim, char *format);

int   cvo_addIntParam (Query query, char *name, int ival);
int   cvo_addFloatParam (Query query, char *name, double dval);
int   cvo_addStringParam (Query query, char *name, char *str);

char *cvo_getQueryString (Query query, int type, int index);

QResponse cvo_executeQuery (Query query);
QResponse cvo_getQueryResponse (Query query);

char *cvo_executeCSV (Query query);
char *cvo_executeTSV (Query query);
char *cvo_executeASCII (Query query);
char *cvo_executeVOTable (Query query);
int   cvo_executeQueryAs (Query query, char *fname, int type);

char *cvo_getErrMsg ();

QRecord cvo_getRecord (QResponse qr, int recnum);
int     cvo_getRecordCount (QResponse qr);
int     cvo_getAttrCount (QRecord rec);
char   *cvo_getFieldAttr (QResponse qr, int index, char *attr);
char   *cvo_getAttrList (QRecord rec);

QRAttribute cvo_getAttribute (QRecord rec, char *attrname);
void   cvo_setIntAttr (QRecord rec, char *attrname, int ival);
void   cvo_setFloatAttr (QRecord rec, char *attrname, double dval);
void   cvo_setStringAttr (QRecord rec, char *attrname, char *str);
int    cvo_getIntAttr (QRecord rec, char *attrname);
double cvo_getFloatAttr (QRecord rec, char *attrname);
char  *cvo_getStringAttr (QRecord rec, char *attrname);
int    cvo_intValue (QRAttribute v);
double cvo_floatValue (QRAttribute v);
char  *cvo_stringValue (QRAttribute v);

int    cvo_getDataset (QRecord rec, char *acref, char *fname);



/*  Sesame Name Resolver 
*/
Sesame   cvo_nameResolver (char *target);	
 
char    *cvo_resolverPos (Sesame sr);
char    *cvo_resolverOtype (Sesame sr);
double   cvo_resolverRA (Sesame sr);
double   cvo_resolverRAErr (Sesame sr);
double   cvo_resolverDEC (Sesame sr);
double   cvo_resolverDECErr (Sesame sr);


/*  Registry Interface procedures.
 */
RegResult   cvo_regSearch (char *term1,  char *term2, int orValues);
RegResult   cvo_regSearchByService (char *svc,  char *term, int orValues);
RegQuery    cvo_regQuery (char *term, int orValues);
void	    cvo_regConstSvcType (RegQuery query, char *svc);
void	    cvo_regConstWaveband (RegQuery query, char *bpass);
void	    cvo_regDALOnly (RegQuery query, int value);
void	    cvo_regSortRes (RegQuery query, int value);
void	    cvo_regAddSearchTerm (RegQuery query, char *term, int orValue);
void	    cvo_regRemoveSearchTerm (RegQuery query, char *term);
int	    cvo_regGetSTCount (RegQuery query);
char	   *cvo_regGetQueryString (RegQuery query);
RegResult   cvo_regExecute (RegQuery query);
char	   *cvo_regExecuteRaw (RegQuery query);

int	    cvo_resGetCount (RegResult res);
char	   *cvo_resGetStr (RegResult res, char *attribute, int index);
double	    cvo_resGetFloat (RegResult res, char *attribute, int index);
int	    cvo_resGetInt (RegResult res, char *attribute, int index);



#ifdef __cplusplus
}
#endif


#ifdef VOC_DIRECT

#define	  voc_initVOClient		cvo_initVOClient

/**
 *  DAL Services Interface
 */
#define	  voc_coneCaller		cvo_coneCaller
#define	  voc_coneCallerToFile		cvo_coneCallerToFile
#define	  voc_siapCaller		cvo_siapCaller
#define	  voc_siapCallerToFile		cvo_siapCallerToFile
#define	  voc_ssapCaller		cvo_ssapCaller
#define	  voc_ssapCallerToFile		cvo_ssapCallerToFile
	
#define	  voc_getRawURL			cvo_getRawURL
	
#define	  voc_openConnection		cvo_openConnection
#define	  voc_openConeConnection	cvo_openConeConnection
#define	  voc_openSiapConnection	cvo_openSiapConnection
#define	  voc_openSsapConnection	cvo_openSsapConnection
	
#define	  voc_closeConnection		cvo_closeConnection
#define	  voc_getServiceCount		cvo_getServiceCount
#define	  voc_addServiceURL		cvo_addServiceURL
#define	  voc_getServiceURL		cvo_getServiceURL
	
#define	  voc_getQuery			cvo_getQuery
#define	  voc_getConeQuery		cvo_getConeQuery
#define	  voc_getSiapQuery		cvo_getSiapQuery
#define	  voc_getSsapQuery		cvo_getSsapQuery

#define	  voc_addIntParam		cvo_addIntParam
#define	  voc_addFloatParam		cvo_addFloatParam
#define	  voc_addStringParam		cvo_addStringParam
	
#define	  voc_executeQuery		cvo_executeQuery
#define	  voc_getQueryResponse		cvo_getQueryResponse
#define	  voc_getQueryString		cvo_getQueryString
	
#define	  voc_executeCSV		cvo_executeCSV
#define	  voc_executeTSV		cvo_executeTSV
#define	  voc_executeASCII		cvo_executeASCII
#define	  voc_executeVOTable		cvo_executeVOTable
#define	  voc_executeQueryAs		cvo_executeQueryAs
	
#define	  voc_getErrMsg			cvo_getErrMsg
	
#define	  voc_getRecordCount		cvo_getRecordCount
#define	  voc_getRecord			cvo_getRecord
#define	  voc_getAttrCount		cvo_getAttrCount
#define	  voc_getFieldAttr		cvo_getFieldAttr
#define	  voc_getAttrList		cvo_getAttrList
	
#define	  voc_getAttribute		cvo_getAttribute
	
#define	  voc_setIntAttr		cvo_setIntAttr
#define	  voc_setFloatAttr		cvo_setFloatAttr
#define	  voc_setStringAttr		cvo_setStringAttr
#define	  voc_getIntAttr		cvo_getIntAttr
#define	  voc_getFloatAttr		cvo_getFloatAttr
#define	  voc_getStringAttr		cvo_getStringAttr
#define	  voc_intValue			cvo_intValue
#define	  voc_floatValue		cvo_floatValue
#define	  voc_stringValue		cvo_stringValue
	
#define	  voc_getDataset		cvo_getDataset


/**
 *  VO Registry Interface
 */
#define   voc_regSearch	      		cvo_regSearch
#define   voc_regSearchByService	cvo_regSearchByService
#define   voc_regQuery	      		cvo_regQuery
#define   voc_regConstSvcType	      	cvo_regConstSvcType
#define   voc_regConstWaveband	      	cvo_regConstWaveband
#define   voc_regDALOnly	      	cvo_regDALOnly
#define   voc_regSortRes	      	cvo_regSortRes
#define   voc_regAddSearchTerm	      	cvo_regAddSearchTerm
#define   voc_regRemoveSearchTerm	cvo_regRemoveSearchTerm
#define   voc_regGetSTCount	      	cvo_regGetSTCount
#define   voc_regGetQueryString	      	cvo_regGetQueryString
#define   voc_regExecute	      	cvo_regExecute
#define   voc_regExecuteRaw	      	cvo_regExecuteRaw

#define   voc_resGetCount	      	cvo_resGetCount
#define   voc_resGetStr	      		cvo_resGetStr
#define   voc_resGetFloat	      	cvo_resGetFloat
#define   voc_resGetInt	      		cvo_resGetInt


/**
 *  Sesame Service Interface
 */
#define   voc_nameResolver	      	cvo_nameResolver
#define   voc_resolverPos	      	cvo_resolverPos
#define   voc_resolverRA	      	cvo_resolverRA
#define   voc_resolverDEC	      	cvo_resolverDEC
#define   voc_resolverRAErr	      	cvo_resolverRAErr
#define   voc_resolverDECErr	      	cvo_resolverDECErr
#define   voc_resolverOtype	      	cvo_resolverOtype

#endif
