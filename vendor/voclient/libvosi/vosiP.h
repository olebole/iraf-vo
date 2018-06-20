/**
 *  VOSIP.H -- Internal LIBVOSI definitions.
 *
 *  @file       vosiP.h
 *  @author     Ken Mighell and Mike Fitzpatrick
 *  @date       8/11/09
 *  @date       8/26/14 KJM: modified Capabilities structure
 *  @date       9/08/14 KJM: added UniversalWorkerServices structure
 *  @data       9/12/14 KJM: added Tables structures
 *
 *  @brief      Internal LIBVOSI definitions.
 */

#include <expat.h>
#include <stdbool.h>


#define	VOT_DOC_VERSION		"1.2"	/** VOTable document version (write)  */

#define	VOT_XSI			"http://www.w3.org/2001/XMLSchema-instance"
#define	VOT_SCHEMA_LOC		"http://www.ivoa.net/xml/VOTable/v1.1  http://www.ivoa.net/xml/VOTable/v1.1"
#define	VOT_XMLNS		"http://www.ivoa.net/xml/VOTable/v1.1"


#define SZ_ATTRNAME             32	/** size of attribute name            */
#define SZ_ATTRVAL              2048	/** size of attribute value           */
#define SZ_FNAME                255	/** size of filename+path             */
#define SZ_XMLTAG               1024	/** max length of entire XML tag      */
#define SZ_LINE                 4096	/** handy size                        */

#define MAX_ATTR                100	/** max size of an attribute/value    */
#define HANDLE_INCREMENT        1024000	/** incr size of handle table         */

#define SZ_CAPABILITIES 64

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
 *  @struct Avail
 *  @brief 		Information from an Availability interface
 *  @param available 	is service accepting requests?
 *  @param upSince  	duration of continuously uptime
 *  @param downAt 	time of next scheduled down time
 *  @param backAt	time of return to service
 *  @param note  	reason for unavailability
 */
typedef struct {
    int available;		/** is service accepting requests?	*/
    char upSince[SZ_LINE];	/** duration of continuously uptime	*/
    char downAt[SZ_LINE];	/** time of next scheduled down time	*/
    char backAt[SZ_LINE];	/** time of return to service		*/
    char note[SZ_LINE];		/** reason for unavailability		*/
} Avail, *AvailP;


/**
 *  @struct Capabilities
 *  @brief Information from a Capabilities interface
 *  @param e1a_standardID                  1st level attribute: string for standardID
 *  @param e1a_standardID_IVORN            1st level attribute: string for IVORN
 *  @param e1a_standardID_LRN              1st level attribute: string for LRN
 *  @param e1a_nstype                      1st level attribute: string for nstype (e.g., foo:type)
 *  @param e1a_nstype_namespace            1st level attribute: string for namespace of nstype
 *  @param e2a_interface_role              2nd level attribute: string for role of the interface element
 *  @param e2a_interface_nstype            2nd level attribute: string for nstype of the interface element
 *  @param e2a_interface_nstype_namespace  2nd level attribute: string for nstype namespace of the interface element
 *  @param e3v_interface_accessURL         3rd level value: string for accessURL
 *  @param e3a_interface_accessURL_use     3rd level attribute: string for use of accessURL
 *  @param ServiceType                     bitmap of ServiceType (int)
 *  @param ServiceTypeName                 string describing ServiceType
 *  @param index                           index (zero to n) of the current Capabilities structure
 *  @param next                            pointer to the next capabilities structure
 */
typedef struct {
    char e1a_standardID[SZ_LINE];
    char e1a_standardID_IVORN[SZ_LINE];
    char e1a_standardID_LRN[SZ_LINE];
    char e1a_nstype[SZ_LINE];
    char e1a_nstype_namespace[SZ_LINE];
    char e2a_interface_role[SZ_LINE];
    char e2a_interface_nstype[SZ_LINE];
    char e2a_interface_nstype_namespace[SZ_LINE];
    char e3v_interface_accessURL[SZ_LINE];
    char e3a_interface_accessURL_use[SZ_LINE];
    int ServiceType;
    char ServiceTypeName[SZ_LINE];
    int index;
    void *next;			/* linked list of capabilities          */
} Capabilities, *CapabilitiesP;


/**
 *  @struct UniversalWorkerServices
 *  @brief Information from a UniversalWorkerServices interface
 *  @param jobId              string id for job                      /{jobs}/(job-id)           
 *  @param phase              string for phase                       /{jobs}/(job-id)/phase              
 *  @param executionDuration  string for maximum execution duration  /{jobs}/(job-id)/executionduration  
 *  @param destruction        string for destruction instant         /{jobs}/(job-id)/destruction         
 *  @param error              string for error                       /{jobs}/(job-id)/error              
 *  @param quote              string for the quote                   /{jobs}/(job-id)/quote 
 *  @param ownerId            string id for owner                    /{jobs}/(job-id)/owner
 *  @param hasParameters      int any parameters?
 *  @param hasResults         int any results?
 *  @param hasErrorSummary    int any error summary?
 *  @param hasJobInfo         int any job inforation?
 */
typedef struct {
    char jobId[SZ_LINE];
    char phase[SZ_LINE];
    char executionDuration[SZ_LINE];
    char destruction[SZ_LINE];
    char error[SZ_LINE];
    char quote[SZ_LINE];
    char ownerId[SZ_LINE];
    char startTime[SZ_LINE];
    char endTime[SZ_LINE];
    int hasParameters;
    int hasResults;
    int hasErrorSummary;
    int hasJobInfo;
} UniversalWorkerServices, *UniversalWorkerServicesP;


// ******************************
// ***** #tables structures *****
// ******************************


/**
 *  @struct vosiForeignKey
 *  @brief 		 Information about a vosiForeignKey
 *  @param targetTable 
 *  @param targetTable   string  FQ name of table joinable by key
 *  @param fromColumn    string  column from current table
 *  @param targetColumn  string  column from target table 	    
 *  @param description   string  description of key and meaning    
 *  @param utype         string  DM concept of key
 *  @param index         int     index of structure
 *  @param next          void *	 next pointer in linked list
 */
typedef struct {
    char *targetTable;		/* FQ name of table joinable by key  */
    char *fromColumn;		/* column from current table         */
    char *targetColumn;		/* column from target table          */
    char *description;		/* description of key and meaning    */
    char *utype;		/* DM concept of key                 */
    int index;			/* index of structure (0 to n)       */
    void *next;			/* next pointer in linked list       */
} vosiForeignKey, *vosiForeignKeyP;


#define  COL_INDEXED		0	// column flag options
#define  COL_PRIMARY		1
#define  COL_NULLABLE		2


/**
 *  @struct vosiColumn
 *  @brief 		Information about a vosiColumn
 *  @param name         string            name of the column
 *  @param description  string            description of contents	     
 *  @param unit         string            unit associated w/ col values
 *  @param ucd          string            UCD describing content
 *  @param utype        string            DM concept of column
 *  @param datatype     string            type of data in column
 *  @param flag         int               traits of column (bitmap)
 *  @param std          int               true if defined by std model
 *  @param index        int               index of structure (0 to n)
 *  @param nkeys        int               number of foreign keys
 *  @param keys         vosiForeignKey *  pointer to foreignKeys
 *  @param next         void *            next pointer in linked list
 */
typedef struct {
    char *name;			// name of the column                
    char *description;		// description of contents           
    char *unit;			// unit associated w/ col values     
    char *ucd;			// UCD describing content            
    char *utype;		// DM concept of column              
    char *datatype;		// type of data in column            
    int flag;			// traits of column                  
    int std;			// true if defined by std model 
    int index;			// index of structure (0 to n)     
    int nkeys;			// number of foreign keys            
    vosiForeignKey *keys;	// pointer to foreign keys               
    void *next;			// next pointer in linked list       
} vosiColumn, *vosiColumnP;


/**
 *  @struct vosiTable
 *  @brief 		Information about a vosiTable
 *  @param name         string        unique table name in set          
 *  @param title        string        human readable name of table      
 *  @param description  string        description of how table relation 
 *  @param utype        string        DM concept of table		     
 *  @param index        int           index of structure (0 to n)
 *  @param ncols        int           number of columns in table        
 *  @param cols         vosiColumn *  pointer to column linked list  
 *  @param next         void *        next point in linked list               
 */
typedef struct {
    char *type;			// type of table 
    char *name;			// unique table name in set          
    char *title;		// human readable name of table      
    char *description;		// description of how table relation 
    char *utype;		// DM concept of table                   
    int index;			// index of the structure (0 to n)
    int ncols;			// number of columns in table        
    vosiColumn *cols;		// pointer to column linked list
    void *next;			// next point in linked list
} vosiTable, *vosiTableP;


/**
 *  @struct vosiSchema
 *  @brief 		Information about a vosiSchema
 *  @param name         string       unique table name in set          
 *  @param title        string       human readable name of table      
 *  @param description  string 	     description of how table relation 
 *  @param utype        string 	     DM concept of table		     
 *  @param index        int          index of structure (0 to n)
 *  @param table        vosiTable *  pointer to table
 *  @param next         void *       next pointer in linked list       
 */
typedef struct {
    char *name;			// unique table name in set          
    char *title;		// human readable name of table      
    char *description;		// description of how table relation 
    char *utype;		// DM concept of table                    
    int index;			// index of structure (0 to n)
    vosiTable *table;		// ptr to table                      
    void *next;			// next pointer in linked list       
} vosiSchema, *vosiSchemaP;


/**
 *  @struct vosiTableSet
 *  @brief 	    Information about a vosiTableSet
 *  @param nschema  int           number of table schema in response
 *  @param schema   vosiSchema *  pointer to schema linked list
 */
typedef struct {
    int nschema;		// number of table schema in response
    vosiSchema *schema;		// pointer to schema linked list
} vosiTableSet, *vosiTableSetP;


/** ***************************************************************************
 *
 *  Public Internal Methods.  The procedures are used to implement the
 *  library, however are not part of the public interface.
 *
 ** **************************************************************************/

void vosi_initHandles (void);
int vosi_newHandleContext (char *name, int size);
void vosi_closeHandleContext (int context);
handle_t vosi_newHandle (int context, void *ptr);
void vosi_freeHandle (handle_t handle);
void *vosi_H2P (handle_t handle);
