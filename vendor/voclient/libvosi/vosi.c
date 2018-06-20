/**
 *  VOSI.C -- Public interface procedures for the VO Support Interfaces (VOSI)
 *
 *  @file       vosi.c
 *  @author     Ken Mighell and Mike Fitzpatrick
 *  @date       8/11/14
 *  @date       9/11/14 KJM 
 *  @date       9/16/14 KJM 
 *  @date       9/17/14 KJM 
 *  @date       9/18/14 KJM 
 *
 *  @brief      Public interface procedures for the VO Support Interfaces (VOSI)
 */

#include <stdbool.h>
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
#include "vosiP.h"
#include "vosi.h"

#define	BUFSIZE		4096
#define VOSI_DEBUG  	(getenv("VOSI_DBG")||access("/tmp/VOSI_DBG",F_OK)==0)



extern char *strcasestr ();
extern int strcasecmp ();

/** *************************************************************************
 *  Public Interface
 *
 *    Availability Metadata
 *    ---------------------
 *
 *	 avail = vosi_getAvailability  (filename|str|url|NULL)
 *	    int = vosi_availAvailable  (avail)
 *	      str = vosi_availUpSince  (avail)
 *	       str = vosi_availDownAt  (avail)
 *	       str = vosi_availBackAt  (avail)
 *	         str = vosi_availNote  (avail)
 *
 *
 *    Capability Metadata
 *    -------------------
 *
 *   	   cap = vosi_getCapabilities  (filename|str|url|NULL)
 *     next_cap = vosi_nextCapability  (cap)
 *   new_cap = vosi_capabilityByIndex  (index)
 *         int = vosi_numCapabilities  (void)
 *           str = vosi_capStandardID  (cap)
 *                str = vosi_capIVORN  (cap)
 *                  str = vosi_capLRN  (cap)
 *               str = vosi_capNStype  (cap)
 *            str = vosi_capAccessURL  (cap)
 *                int = vosi_capIndex  (cap)
 *         int = vosi_cap_ServiceType  (cap)
 *     str = vosi_capServiceTypeNames  (int)
 *
 *
 *    Table Metadata
 *    --------------
 *
 *              tset = vosi_getTables  (filename|str|url|NULL)
 *            schema = vosi_getSchema  (tableset)
 *        schema = vosi_schemaByIndex  (tableset, index)
 *          schema =  vosi_nextSchema  (schema)
 *               int = vosi_numSchema  (tableset)
 *
 *              int= vosi_schemaIndex  (schema)			// Schemas
 *              str = vosi_schemaName  (schema)
 *             str = vosi_schemaTitle  (schema)
 *       str = vosi_schemaDescription  (schema)
 *             str = vosi_schemaUType  (schema)
 *                int = vosi_numTable  (schema)
 *
 *              table = vosi_getTable  (schema)			// Tables
 *          table = vosi_tableByIndex  (schema, index)
 *             table = vosi_nextTable  (table)
 *               str = vosi_tableName  (table)
 *              str = vosi_tableTitle  (table)
 *        str = vosi_tableDescription  (table)
 *              str = vosi_tableUType  (table)
 *              int = vosi_tableIndex  (table)
 *                 int = vosi_numCols  (table)
 *
 *            column = vosi_getColumn  (table)			// Columns
 *           column = vosi_colByIndex  (table, index)
 *           column = vosi_nextColumn  (column)
 *                 str = vosi_colName  (column)
 *          str = vosi_colDescription  (column)
 *                 str = vosi_colUnit  (column)
 *                  str = vosi_colUCD  (column)
 *                str = vosi_colUType  (column)
 *             str = vosi_colDatatype  (column)
 *                int = vosi_colIndex  (column)
 *                 int = vosi_numKeys  (column)
 *
 *                  key = vosi_getKey  (column)			// Column keys
 *              key = vosi_keyByIndex  (column, index)
 *                 key = vosi_nextKey  (key)
 *          str = vosi_keyTargetTable  (key)
 *           str = vosi_keyFromColumn  (key)
 *         str = vosi_keyTargetColumn  (key)
 *          str = vosi_keyDescription  (key)
 *                str = vosi_keyUType  (key)
 *                int = vosi_keyIndex  (key)
 *
 *
 ** *************************************************************************/

char  xml_NameSpaceTagG[MAX_ATTR][SZ_ATTRNAME];
char  xml_NameSpaceG[MAX_ATTR][SZ_ATTRVAL];
int   xml_NameSpaceCountG = -1;


/*  Private procedures.
 */
static void  vosi_printk (char *format, char *string);
static char *vosi_getNameSpace (char *nstag);
static void  vosi_setNameSpace (handle_t top);
static void  vosi_nsSplit (char *token, char *ns, char *name);
static char *vosi_callocAC (void);






/** *************************************************************************
 **   CAPABILITIES SERVICE
 ** *************************************************************************/

static Capabilities capG;	/* use only one value - not thread safe!    */
static handle_t cap_hconG;	/* use only one value - not thread safe!
				 * handle context of capabilities
				 */

static char *vosi_standardID_LRN (char *standardID_value);
static char *vosi_standardID_IVORN (char *standardID_value);
static char *vosi_getAttribute_standardID (handle_t h);
static char *vosi_getAttribute_nstype (handle_t h, char *nstype, char *nstag);


/** 
 *  vosi_getAttribute_nstype -- Private function to process XML attribute ending with ":type"
 *
 *  @brief  Private function to process XML attribute ending with ":type"
 *  @fn     char *vosi_getAttribute_nstype (handle_t h, char *nstype, char *nstag)
 *
 *  @param  handle_t h     handle
 *  @param  char *nstype   XML attribute ending with ":type"
 *  @param  char *nstag    tag value of string before the colon
 *  @return char *   String with attribute value
 */
static char *
vosi_getAttribute_nstype (handle_t h, char *nstype, char *nstag) 
{
    char *new = NULL;
    char *work = NULL;
    char *attribute_name = NULL;
    char *attribute_value = NULL;
    char *ns = NULL;
    char *name = NULL;

    assert (NULL != nstype);
    assert (NULL != nstag);
    new = vosi_callocAC ();
    assert (NULL != new);
    ns = vosi_callocAC ();
    assert (NULL != ns);
    name = vosi_callocAC ();
    assert (NULL != name);
    attribute_name = nstype;
    work = strstr (attribute_name, ":type");
    if (NULL == work)		// attribute name did not contain ":type"
	return ("");
    attribute_value = xml_getAttr (h, attribute_name);
    if (NULL == attribute_value)
	return ("");
    if (attribute_value[0] == '\0')
	return ("");
    vosi_nsSplit (attribute_value, ns, name);
    assert (strlen (ns) < SZ_LINE);
    assert (strlen (name) < SZ_LINE);
    strncpy (nstag, ns, SZ_LINE);
    return (name);
}


/** 
 *  vosi_getAttribute_standardID -- Private function to process XML attribute standardID
 *
 *  @brief  Private function to process XML attribute standardID
 *  @fn     char *vosi_getAttribute_standardID (handle_t h)
 *
 *  @param  handle_t h   handle
 *  @return char *       String with attribute value
 */
static char *
vosi_getAttribute_standardID (handle_t h) 
{
    char *value;

    value = xml_getAttr (h, "standardID");
    if (NULL == value)
	return ("");
    return (value);
}


/** 
 *  vosi_standardID_IVORN -- Private function to extract the IVORN from the standardID
 *
 *  @brief  Private function to extract the IVORN from the standardID
 *  @fn     char *vosi_standardID_IVORN (char *standardID_value)
 *
 *  @param  char * standardID_value (String value of the standardID attribute)
 *  @return char * (String with the IVORN)
 */
static char *
vosi_standardID_IVORN (char *standardID_value) 
{
    char *new = NULL;
    char *spud = NULL;
    char *foo = NULL;
    int pos = -1;

    new = vosi_callocAC ();
    assert (NULL != new);
    spud = strstr (standardID_value, "ivo://");
    pos = spud - standardID_value;
    if (0 != pos)
	return (NULL);			// not a proper standardID
    foo = strchr (spud, '#');		// find the pound character
    if (NULL == foo)
	return (standardID_value); // no LRN present, so return standardID_value
    pos = foo - spud;
    assert (strlen (spud) < SZ_LINE);
    (void) strncpy (new, spud, SZ_LINE);
    new[pos] = '\0';
    return (new);
}


/** 
 *  vosi_standardID_LRN -- Private function to extract the LRN from the standardID
 *
 *  @brief  Private function to extract the LRN from the standardID
 *  @fn     char *vosi_standardID_LRN (char *standardID_value)
 *
 *  @param  char * standardID_value (String value of the standardID attribute)
 *  @return char * (String with the LRN)
 */
static char *
vosi_standardID_LRN (char *standardID_value) 
{
    char *new = NULL;
    char *spud = NULL;
    char *foo = NULL;
    int pos = -1;

    new = vosi_callocAC ();
    assert (NULL != new);
    spud = strstr (standardID_value, "ivo://");
    pos = spud - standardID_value;
    if (0 != pos)
	return (NULL);		// not a proper standardID
    foo = strchr (spud, '#');	// find the pound character
    if (NULL == foo)
	return ("");		// no LRN present, so return empty string
    foo++;			// advance beyond the pound character
    assert (strlen (foo) < SZ_LINE);
    (void) strncpy (new, foo, SZ_LINE);
    return (new);
}



/** 
 *  vosi_getCapabilities-- Access the VOSI 'capabilities' service 
 *
 *  @brief  Access the VOSI 'capabilities' service 
 *  @fn     handle_t vosi_getCapabilities (char *arg)
 *
 *  @param  arg 	The source of the XML document (fname|url|str)
 *  @return	 	Handle to the capabilites struct, -1 on error
 */
handle_t
vosi_getCapabilities (char *arg)
{
    handle_t xp, top, h, ret_handle = 0;
    handle_t h0, h1, h2;
    char *name = NULL;
    Capabilities *capGP = (Capabilities *) & capG;
    char *spud;
    char namesAC[SZ_XMLTAG];
    char spaceAC[2] = " ";
    char *element_name;
    char *element_value;
    char *attribute_name;
    char *attribute_value;
    char *nstag = NULL;
    int index = 0;
    char *h0_element_name;
    char *h0_element_value;
    char *h1_element_name;
    char *h1_element_value;

    // initialize character strings
    nstag = vosi_callocAC ();
    assert (NULL != nstag);
    h0_element_name = vosi_callocAC ();
    assert (NULL != h0_element_name);
    h0_element_value = vosi_callocAC ();
    assert (NULL != h0_element_value);
    h1_element_name = vosi_callocAC ();
    assert (NULL != h1_element_name);
    h1_element_value = vosi_callocAC ();
    assert (NULL != h1_element_value);

    /*  Open the argument, returning a handle to the parsed document.  The
     *  input arg may be a URL to the service, a filename containing the
     *  XML result, or the XML doc in a string retrieved from an earlier
     *  query.
     */

    if ((xp = xml_openXML (arg)) < 0)
	return (-1);

    top = xml_getToplevel (xp);	/*  get the toplevel handle     */

    /*  Error checking.
     */
    name = xml_getToplevelName (top);
    if (name == (char *) NULL || strcasecmp (name, "capabilities"))
	return (-1);

    // create namespace dictionary
    vosi_setNameSpace (top);

    // initialize handles system
    vosi_initHandles ();

    index = 0;
    // DECEND!
    for (h0 = xml_getChild (top); h0; h0 = xml_getSibling (h0)) {
	h = h0;
	element_name = xml_elemNameByHandle (h);
	element_value = xml_getValue (h);
	strncpy (h0_element_name, element_name, SZ_LINE);
	strncpy (h0_element_value, element_value, SZ_LINE);
	assert (0 == strcmp (h0_element_name, "capability"));
	{			// prepare the Capabilities structure
	    if (index == 0) {	// erase the existing Capabilities structure
		memset (&capG, 0, sizeof (Capabilities));
		capGP = &capG;
	    } else {		// allocate a new Capabilities structure
		spud = calloc ((size_t) 1, sizeof (Capabilities));
		assert (NULL != spud);
		capGP->next = spud;
		capGP = (Capabilities *) spud;
	    }
	    capGP->index = index;
	    index++;
	}
	strncpy (namesAC, xml_dumpNamesAttr (h), SZ_XMLTAG);
	attribute_name = strtok (namesAC, spaceAC);	// get first name
	while (attribute_name != NULL) {
	    attribute_value = xml_getAttr (h, attribute_name);
	    if (NULL != strstr (attribute_name, ":type")) {
		spud = vosi_getAttribute_nstype (h, attribute_name, nstag);
		assert (strlen (spud) < sizeof (capGP->e1a_nstype));
		(void) strncpy (capGP->e1a_nstype, spud,
				sizeof (capGP->e1a_nstype));
		spud = vosi_getNameSpace (nstag);
		assert (strlen (spud) <
			sizeof (capGP->e1a_nstype_namespace));
		(void) strncpy (capGP->e1a_nstype_namespace, spud,
				sizeof (capGP->e1a_nstype_namespace));
	    }
	    if (0 == strcmp (attribute_name, "standardID")) {
		spud = vosi_getAttribute_standardID (h);
		assert (strlen (spud) < sizeof (capGP->e1a_standardID));
		(void) strncpy (capGP->e1a_standardID, spud,
				sizeof (capGP->e1a_standardID));
		spud = vosi_standardID_IVORN (capGP->e1a_standardID);
		assert (strlen (spud) <
			sizeof (capGP->e1a_standardID_IVORN));
		(void) strncpy (capGP->e1a_standardID_IVORN, spud,
				sizeof (capGP->e1a_standardID_IVORN));
		spud = vosi_standardID_LRN (capGP->e1a_standardID);
		assert (strlen (spud) <
			sizeof (capGP->e1a_standardID_LRN));
		(void) strncpy (capGP->e1a_standardID_LRN, spud,
				sizeof (capGP->e1a_standardID_LRN));
	    }
	    attribute_name = strtok (NULL, spaceAC);
	}
	// DECEND!
	for (h1 = xml_getChild (h0); h1; h1 = xml_getSibling (h1)) {
	    h = h1;
	    element_name = xml_elemNameByHandle (h);
	    element_value = xml_getValue (h);
	    if (strcmp (element_name, "interface") == 0) {
		strncpy (namesAC, xml_dumpNamesAttr (h), SZ_XMLTAG);
		attribute_name = strtok (namesAC, spaceAC);	// get first name
		while (attribute_name != NULL) {
		    attribute_value = xml_getAttr (h, attribute_name);
		    if (NULL != strstr (attribute_name, ":type")) {
			spud =
			    vosi_getAttribute_nstype (h, attribute_name,
						      nstag);
			assert (strlen (spud) <
				sizeof (capGP->e2a_interface_nstype));
			(void) strncpy (capGP->e2a_interface_nstype, spud,
					sizeof (capGP->
						e2a_interface_nstype));
			spud = vosi_getNameSpace (nstag);
			assert (strlen (spud) <
				sizeof (capGP->
					e2a_interface_nstype_namespace));
			(void) strncpy (capGP->
					e2a_interface_nstype_namespace,
					spud,
					sizeof (capGP->
						e2a_interface_nstype_namespace));
		    }
		    if (0 == strcmp (attribute_name, "role")) {
			spud = attribute_value;
			assert (strlen (spud) <
				sizeof (capGP->e2a_interface_role));
			(void) strncpy (capGP->e2a_interface_role, spud,
					sizeof (capGP->
						e2a_interface_role));
		    }
		    attribute_name = strtok (NULL, spaceAC);
		}
		// DECEND!
		for (h2 = xml_getChild (h1); h2; h2 = xml_getSibling (h2)) {
		    h = h2;
		    element_name = xml_elemNameByHandle (h);
		    element_value = xml_getValue (h);
		    if (strcmp (element_name, "accessURL") == 0) {
			(void) strncpy (capGP->e3v_interface_accessURL,
					element_value,
					sizeof (capGP->
						e3v_interface_accessURL));
			strncpy (namesAC, xml_dumpNamesAttr (h),
				 SZ_XMLTAG);
			attribute_name = strtok (namesAC, spaceAC);	// get first name
			while (attribute_name != NULL) {
			    attribute_value =
				xml_getAttr (h, attribute_name);
			    if (0 == strcmp (attribute_name, "use")) {
				spud = attribute_value;
				assert (strlen (spud) <
					sizeof (capGP->
						e3a_interface_accessURL_use));
				(void) strncpy (capGP->
						e3a_interface_accessURL_use,
						spud,
						sizeof (capGP->
							e3a_interface_accessURL_use));
			    }
			    attribute_name = strtok (NULL, spaceAC);
			}
		    }
		}
		// ASCEND!
	    }
	}
	// ASCEND!
	{			// determine ServiceType
	    capGP->ServiceType = ST_UNKNOWN;	// inialize
	    if (NULL !=
		strcasestr (capGP->e1a_standardID, "VOSI#availability"))
		capGP->ServiceType = ST_VOSIAVAILABILITY;
	    if (NULL !=
		strcasestr (capGP->e1a_standardID, "VOSI#capabilities"))
		capGP->ServiceType = ST_VOSICAPABILITIES;
	    if (NULL != strcasestr (capGP->e1a_standardID, "VOSI#tables"))
		capGP->ServiceType = ST_VOSITABLES;
	    if (NULL != strcasestr (capGP->e1a_nstype, "ConeSearch"))
		capGP->ServiceType = ST_CONESEARCH;
	    if (NULL != strcasestr (capGP->e1a_standardID, "ConeSearch"))
		capGP->ServiceType = ST_CONESEARCH;
	    if (NULL !=
		strcasestr (capGP->e2a_interface_nstype, "WebBrowser"))
		capGP->ServiceType = ST_WEBBROWSER;
	    if (NULL !=
		strcasestr (capGP->e1a_nstype, "SimpleImageAccess"))
		capGP->ServiceType = ST_SIMPLEIMAGEACCESS;
	    if (NULL != strcasestr (capGP->e1a_standardID, "std/SIA"))
		capGP->ServiceType = ST_SIMPLEIMAGEACCESS;
	    if (NULL != strcasestr (capGP->e1a_nstype, "TableAccess"))
		capGP->ServiceType = ST_TABLEACCESS;
	    if (NULL != strcasestr (capGP->e1a_standardID, "std/TAP"))
		capGP->ServiceType = ST_TABLEACCESS;
	    spud = vosi_capServiceTypeNames (capGP->ServiceType);
	    strncpy (capGP->ServiceTypeName, spud,
		     sizeof (capGP->ServiceTypeName));
	}


	if (VOSI_DEBUG) {
	    printf ("\n");
	    printf ("                         index= %d\n", capGP->index);
	    vosi_printk ("                e1a_standardID= %s\n",
		     capGP->e1a_standardID);
	    vosi_printk ("          e1a_standardID_IVORN= %s\n",
		     capGP->e1a_standardID_IVORN);
	    vosi_printk ("            e1a_standardID_LRN= %s\n",
		     capGP->e1a_standardID_LRN);
	    vosi_printk ("                    e1a_nstype= %s\n",
		     capGP->e1a_nstype);
	    vosi_printk ("          e1a_nstype_namespace= %s\n",
		     capGP->e1a_nstype_namespace);
	    vosi_printk ("            e2a_interface_role= %s\n",
		     capGP->e2a_interface_role);
	    vosi_printk ("          e2a_interface_nstype= %s\n",
		     capGP->e2a_interface_nstype);
	    vosi_printk ("e2a_interface_nstype_namespace= %s\n",
		     capGP->e2a_interface_nstype_namespace);
	    vosi_printk ("       e3v_interface_accessURL= %s\n",
		     capGP->e3v_interface_accessURL);
	    vosi_printk ("   e3a_interface_accessURL_use= %s\n",
		     capGP->e3a_interface_accessURL_use);
	    printf ("                   ServiceType= %lu\n",
		(long) capGP->ServiceType);
	    vosi_printk ("               ServiceTypeName= %s\n",
		     capGP->ServiceTypeName);
	    printf ("\n");
	}

    }
    // ASCEND!

    xml_closeXML (xp);		/*  free the document resources */

    /*  Create a return handle to the Capabilities structure
     */
    cap_hconG = vosi_newHandleContext ("VOSI", 1025);
    ret_handle = vosi_newHandle (cap_hconG, &capG);	// KJM
    return (ret_handle);
}


/** 
 *  vosi_capServiceTypeNames -- Translates ServiceType bitmap values to string equivalents
 *
 *  @brief  Translates ServiceType bitmap values to string equivalents
 *  @fn     char *vosi_capServiceTypeNames (int ServiceType)
 *
 *  @param  int ServiceType     bitmap of capabilities service type
 *  @return String equivalent of the bit map value
 */
char *
vosi_capServiceTypeNames (int ServiceType) 
{
    char *spud = NULL;
    int len = 0;

    spud = vosi_callocAC ();
    assert (NULL != spud);
    if (ServiceType == ST_NONE)
	strcat (spud, "NONE ");
    if (ServiceType & ST_UNKNOWN)
	strcat (spud, "UNKNOWN ");
    if (ServiceType & ST_VOSIAVAILABILITY)
	strcat (spud, "VOSI#availability ");
    if (ServiceType & ST_VOSICAPABILITIES)
	strcat (spud, "VOSI#capabilities ");
    if (ServiceType & ST_VOSITABLES)
	strcat (spud, "VOSI#tables ");
    if (ServiceType & ST_CONESEARCH)
	strcat (spud, "ConeSearch ");
    if (ServiceType & ST_WEBBROWSER)
	strcat (spud, "WebBrowser ");
    if (ServiceType & ST_SIMPLEIMAGEACCESS)
	strcat (spud, "SimpleImageAccess ");
    if (ServiceType & ST_TABLEACCESS)
	strcat (spud, "TableAccessProtocol ");
    len = strlen (spud);
    if (len > 1)
	spud[len - 1] = '\0';
    return (spud);
}





/** *************************************************************************
 **   AVAILABILITY SERVICE
 ** *************************************************************************/

static Avail avail;		/* use only one value - not thread safe!    */
static int hcon;		/* use only one value - not thread safe!    
				 * handle context of avail
				 */

/** 
 *  vosi_getAvailability -- Access the VOSI 'availabilities' service 
 *
 *  @brief  Access the VOSI 'availabilities' service 
 *  @fn     handle_t vosi_getAvailability (char *arg)
 *
 *  @param  arg 	The source of the table (fname|url|str)
 *  @return	 	Handle to the availabilities struct, -1 on error
 */
handle_t
vosi_getAvailability (char *arg)
{
    handle_t xp, top, h, ret_handle = 0;
    char *name = NULL, *value = NULL;

    /*  Open the argument, returning a handle to the parsed document.  The
     *  input arg may be a URL to the service, a filename containing the
     *  XML result, or the XML doc in a string retrieved from an earlier
     *  query.
     */
    if ((xp = xml_openXML (arg)) < 0)
	return (-1);

    top = xml_getToplevel (xp);	/*  get the toplevel handle     */

    /*  Error checking.
     */
    name = xml_getToplevelName (top);
    if (name == (char *) NULL || strcasecmp (name, "availability"))
	return (-1);

    /*  Initialize.
     */
    memset (&avail, 0, sizeof (Avail));
    vosi_initHandles ();

    /*  The children are all in a sequence, so walk the tree and load 
     *  the structure.
     */
    for (h = xml_getChild (top); h; h = xml_getSibling (h)) {
	name = xml_elemNameByHandle (h);
	value = xml_getValue (h);

	if (strcasecmp (name, "available") == 0) {
	    avail.available = (strcasecmp ("true", value) == 0);
	} else if (strcasecmp (name, "upSince") == 0) {
	    strcpy (avail.upSince, value);
	} else if (strcasecmp (name, "downAt") == 0) {
	    strcpy (avail.downAt, value);
	} else if (strcasecmp (name, "backAt") == 0) {
	    strcpy (avail.backAt, value);
	} else if (strcasecmp (name, "note") == 0) {
	    strcpy (avail.note, value);
	} else {
	    fprintf (stderr, "Warning: unknown element '%s'\n", name);
	}
    }

    xml_closeXML (xp);		/*  free the document resources */

    /*  Create a return handle to the availability struct.
     */
    //ret_handle = vosi_newHandle (0, &avail); // KJM: fails because handle context has not been defined
    {				// replacement code
	hcon = vosi_newHandleContext ("VOSI", 1024);	// KJM
	ret_handle = vosi_newHandle (hcon, &avail);	// KJM
    }
    return (ret_handle);
}


/** 
 *  vosi_availAvailable -- Determine is the service is accepting requests.
 *
 *  @brief  Determine is the service is accepting requests.
 *  @fn     int vosi_availAvailable (handle_t avail)
 *
 *  @param  avail 	Handle to the parsed availabilities request.
 *  @return	 	0 is service is down, 1 if available, -1 on error
 */
int
vosi_availAvailable (handle_t avail)
{
    Avail *av = (Avail *) NULL;

    if (avail < 0)
	return (avail);

    av = (Avail *) vosi_H2P (avail);
    return (av->available);
}


/** 
 *  vosi_nextCapability -- get the next Capabilities handle
 *
 *  @brief   get the next Capabilities handle
 *  @fn      handle_t vosi_nextCapability (handle_t cap)
 *
 *  @param   cap handle to a Capabilities structure
 *  @return  handle to the next Capabilities structre (0 if it does not exist)
 */
handle_t
vosi_nextCapability (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return (0);		// handles must be positive
    capP = (Capabilities *) vosi_H2P (cap);
    if (NULL == capP->next)
	return (0);		// no more Capabilities structures
    else
	return (vosi_newHandle (cap_hconG, capP->next));
}


/** 
 *  vosi_numCapabilities -- get number of Capabilities
 *
 *  @brief   get the number of Capabilities
 *  @fn      int vosi_numCapabilities (void)
 *
 *  @param   void (no arguments)
 *  @return  number of Capabilities
 */
int vosi_numCapabilities (void)
{
    Capabilities *capP = (Capabilities *) & capG;
    int count = 1;

    while (NULL != capP->next) {
	count++;
	capP = capP->next;
    }
    return (count);
}


/** 
 *  vosi_capabilityByIndex -- get a capability with a given index
 *
 *  @brief   get a capability with a given index
 *  @fn      handel_t vosi_capabilityByIndex (index)
 *
 *  @param   index : index value of the capability
 *  @return  handle of the capability [zero (undefined) if (index<0) || (index>=vosi_numCapabilities()) ]
 */
handle_t
vosi_capabilityByIndex (int index)
{
    Capabilities *capP = (Capabilities *) & capG;

    if (index < 0)
	return (0);		// bad index value 
    assert (NULL != capP);
    if (index == 0)
	return (vosi_newHandle (cap_hconG, capP));
    while (NULL != capP->next) {
	capP = capP->next;
	if (capP->index == index)
	    return (vosi_newHandle (cap_hconG, capP));
    }
    return (0);			// no match
}


/** 
 *  vosi_capStandardID -- standardID value of the Capabilities structure
 *
 *  @brief  standardID value of the Capabilities structure
 *  @fn     char *vosi_capStandardID (handle_t cap)
 *
 *  @param  cap         handle to the Capabilities structure
 *  @return	 	string containing the standardID value (may be empty or NULL)
 */
char *
vosi_capStandardID (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return ((char *) NULL);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->e1a_standardID);
}


/** 
 *  vosi_capIVORN -- IVORN value of the Capaabilities structure
 *
 *  @brief  IVORN value of the Capabilities structure
 *  @fn     char *vosi_capIVORN (handle_t cap)
 *
 *  @param  cap         handle to the Capabilities structure
 *  @return	 	string containing the IVORN value (may be empty or NULL)
 */
char *
vosi_capIVORN (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return ((char *) NULL);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->e1a_standardID_IVORN);
}


/** 
 *  vosi_capLRN -- LRN value of the Capabilities structure
 *
 *  @brief  LRN value of the Capabilities structure
 *  @fn     char *vosi_capLRN (handle_t cap)
 *
 *  @param  cap         handle to the Capabilities structure
 *  @return	 	string containing the LRN value (may be empty or NULL)
 */
char *
vosi_capLRN (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return ((char *) NULL);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->e1a_standardID_LRN);
}


/** 
 *  vosi_capNStype -- interface nstype (name-space type) value of the Capabilities structure
 *
 *  @brief  interface nstype (name-space type) value of the Capabilities structure
 *  @fn     char *vosi_capNStype (handle_t cap)
 *
 *  @param  cap         handle to the Capabilities structure
 *  @return	 	string containing the nstype value (may be empty or NULL)
 */
char *
vosi_capNStype (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return ((char *) NULL);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->e2a_interface_nstype);
}


/** 
 *  vosi_capAccessURL -- interface access URL value of the Capabilities structure
 *
 *  @brief  interface access URL value of the Capabilities structure
 *  @fn     char *vosi_capAccessURL (handle_t cap)
 *
 *  @param  cap         handle to the Capabilities structure
 *  @return	 	string containing the access URL value (may be empty or NULL)
 */
char *
vosi_capAccessURL (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return ((char *) NULL);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->e3v_interface_accessURL);
}


/** 
 *  vosi_capIndex -- index of Capabilities structure
 *
 *  @brief  index of Capabilities structure
 *  @fn     int vosi_capIndex (handle_t cap)
 *
 *  @param  cap handle to the Capabilities structure
 *  @return int value of index of the Capabilities structure (-1 if error)
 */
int
vosi_capIndex (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    if (cap <= 0)
	return (-1);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->index);
}


/** 
 *  vosi_capServiceType -- ServiceType value of Capabilities structure
 *
 *  @brief  service type (int) of Capabilities structure
 *  @fn     int vosi_capServiceType (handle_t cap)
 *
 *  @param  cap handle to the Capabilities structure
 *  @return int value of the ServiceType of Capabilities structure 
 */
int
vosi_capServiceType (handle_t cap)
{
    Capabilities *capP = (Capabilities *) NULL;

    assert (cap > 0);

    capP = (Capabilities *) vosi_H2P (cap);
    return (capP->ServiceType);
}


/** 
 *  vosi_availUpSince -- Get duration of continuous uptime.
 *
 *  @brief  Get duration of continuous uptime.
 *  @fn     char *vosi_availUpSince (handle_t avail)
 *
 *  @param  avail 	Handle to the parsed availabilities request.
 *  @return	 	dateTime string containing uptime
 */
char *
vosi_availUpSince (handle_t avail)
{				// KJM
    Avail *av = (Avail *) NULL;

    if (avail < 0)
	return ((char *) NULL);

    av = (Avail *) vosi_H2P (avail);
    return (av->upSince);
}


/** 
 *  vosi_availDownAt -- Get time of next scheduled down time.
 *
 *  @brief  Get time of next scheduled down time.
 *  @fn     char *vosi_availDownAt (handle_t avail)
 *
 *  @param  avail 	Handle to the parsed availabilities request.
 *  @return	 	dateTime string of down time
 */
char *
vosi_availDownAt (handle_t avail)
{				// KJM
    Avail *av = (Avail *) NULL;

    if (avail < 0)
	return ((char *) NULL);

    av = (Avail *) vosi_H2P (avail);
    return (av->downAt);
}


/** 
 *  vosi_availBackAt -- Get time of return to service.
 *
 *  @brief  Get time of return to service.
 *  @fn     char *vosi_availBackAt (handle_t avail)
 *
 *  @param  avail 	Handle to the parsed availabilities request.
 *  @return	 	dateTime string of return time
 */
char *
vosi_availBackAt (handle_t avail)
{				// KJM
    Avail *av = (Avail *) NULL;

    if (avail < 0)
	return ((char *) NULL);

    av = (Avail *) vosi_H2P (avail);
    return (av->backAt);
}


/** 
 *  vosi_availNote -- Get availability note
 *
 *  @brief  Get availability note
 *  @fn     char *vosi_availNote (handle_t avail)
 *
 *  @param  avail 	Handle to the parsed availabilities request.
 *  @return	 	text string
 */
char *
vosi_availNote (handle_t avail)
{				// KJM
    Avail *av = (Avail *) NULL;

    if (avail < 0)
	return ((char *) NULL);

    av = (Avail *) vosi_H2P (avail);
    return (av->note);
}





/** *************************************************************************
 **   TABLES SERVICE 
 ** *************************************************************************/

static vosiTableSet tsetG;	/* use only one value - not thread safe! */
static handle_t tset_hconG;	/* use only one value - not thread safe! 
				 * handle context of tables
				 */

static int vosi_tableLevel1 (handle_t h0, vosiSchema ** schemaPP, 
	int *index_schemaP);
static int vosi_tableLevel2 (handle_t h1, vosiSchema * schemaP, 
	vosiTable ** tablePP, int *index_tableP);
static int vosi_tableLevel3 (handle_t h2, vosiTable * tableP, 
	vosiColumn ** columnPP, int *index_columnP);
static int vosi_tableLevel4 (handle_t h3, vosiColumn * columnP, 
	vosiForeignKey ** keyPP, int *index_keyP);
static int vosi_tableLevel5 (handle_t h4, vosiForeignKey * keyP);


static int 
vosi_tableLevel5 (handle_t h4, vosiForeignKey * keyP) 
{
    handle_t h5;
    handle_t h;
    char *element_name;
    char *element_value;

    assert (NULL != keyP);
    // assume no attributes
    //
    // get elements of the fkColumn element
    //
    // DECEND!
    for (h5 = xml_getChild (h4); h5; h5 = xml_getSibling (h5)) {
	h = h5;
	element_name = xml_elemNameByHandle (h);
	element_value = xml_getValue (h);
	if (strcasecmp (element_name, "fromColumn") == 0) {
	    keyP->fromColumn = vosi_callocAC ();
	    strncpy (keyP->fromColumn, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "targetColumn") == 0) {
	    keyP->targetColumn = vosi_callocAC ();
	    strncpy (keyP->targetColumn, element_value, SZ_LINE);
	    // assume no attributes
	}
	// assume no attributes
    }
    // ASCEND!
    return (EXIT_SUCCESS);
}


static int
vosi_tableLevel4 (handle_t h3, vosiColumn * columnP, vosiForeignKey ** keyPP, 
	int *index_keyP) 
{
    handle_t h4;
    handle_t h;
    char *element_name;
    char *element_value;
    int index_key = (*index_keyP);
    char *spud = NULL;
    vosiForeignKey *keyP = *keyPP;

    assert (NULL != columnP);
    index_key++;
    {				// create the vosiForeignKey structure
	spud = calloc ((size_t) 1, sizeof (vosiForeignKey));
	assert (NULL != spud);
	if (index_key == 0) {	// allocate first vosiForeignKey structure
	    columnP->keys = (vosiForeignKey *) spud;
	    keyP = (vosiForeignKey *) spud;	// alias
	    columnP->nkeys = 1;
	} else {		// allocate new vosiForeignKey structure
	    assert (NULL != keyP);
	    keyP->next = (vosiForeignKey *) spud;
	    keyP = (vosiForeignKey *) spud;	// reset tableP
	    columnP->nkeys++;
	}
	keyP->index = index_key;
	columnP->nkeys = index_key + 1;
    }
    // assume no attributes
    //
    // get elements of the foreignKey element
    //
    // DECEND!
    for (h4 = xml_getChild (h3); h4; h4 = xml_getSibling (h4)) {
	h = h4;
	element_name = xml_elemNameByHandle (h);
	element_value = xml_getValue (h);
	if (strcasecmp (element_name, "targetTable") == 0) {
	    keyP->targetTable = vosi_callocAC ();
	    strncpy (keyP->targetTable, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "description") == 0) {
	    keyP->description = vosi_callocAC ();
	    strncpy (keyP->description, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "utype") == 0) {
	    keyP->utype = vosi_callocAC ();
	    strncpy (keyP->utype, element_value, SZ_LINE);
	    // assume no attributes
	}
	// ***********************************
	// ***** FKCOLUMN ELEMENT: BEGIN *****
	// ***********************************
	if (strcasecmp (element_name, "fkColumn") == 0) {
	    vosi_tableLevel5 (h4, keyP);
	}
	// *********************************
	// ***** FKCOLUMN ELEMENT: END *****
	// *********************************
    }
    // ASCEND!
    *keyPP = keyP;
    *index_keyP = index_key;
    return (EXIT_SUCCESS);
}


static int
vosi_tableLevel3 (handle_t h2, vosiTable * tableP, vosiColumn ** columnPP, 
	int *index_columnP) 
{
    handle_t h3;
    vosiColumn *columnP = *columnPP;
    int index_column = (*index_columnP);
    handle_t h;
    char *element_name;
    char *element_value;
    int index_key = 0;
    char *spud = NULL;
    vosiForeignKey *keyP = NULL;

    assert (NULL != tableP);
    assert (NULL != tableP);
    index_key = -1;
    index_column++;
    {				// create the vosiColumn structure
	spud = calloc ((size_t) 1, sizeof (vosiColumn));
	assert (NULL != spud);
	if (index_column == 0) {	// allocate first vosiTable structure
	    tableP->cols = (vosiColumn *) spud;
	    columnP = (vosiColumn *) spud;	// alias
	} else {		// allocate new vosiTable structure
	    assert (NULL != columnP);
	    columnP->next = (vosiColumn *) spud;
	    columnP = (vosiColumn *) spud;
	}
	columnP->index = index_column;
	tableP->ncols = index_column + 1;
    }
    // assume no attributes
    //
    // get elements of the column element
    //
    // DECEND!
    for (h3 = xml_getChild (h2); h3; h3 = xml_getSibling (h3)) {
	h = h3;
	element_name = xml_elemNameByHandle (h);
	element_value = xml_getValue (h);
	if (strcasecmp (element_name, "name") == 0) {
	    columnP->name = vosi_callocAC ();
	    strncpy (columnP->name, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "description") == 0) {
	    columnP->description = vosi_callocAC ();
	    strncpy (columnP->description, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "unit") == 0) {
	    columnP->unit = vosi_callocAC ();
	    strncpy (columnP->unit, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "ucd") == 0) {
	    columnP->ucd = vosi_callocAC ();
	    strncpy (columnP->ucd, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "utype") == 0) {
	    columnP->utype = vosi_callocAC ();
	    strncpy (columnP->utype, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "datatype") == 0) {
	    columnP->datatype = vosi_callocAC ();
	    strncpy (columnP->datatype, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "flag") == 0) {
	    columnP->flag = 0;
	    if (NULL != strcasestr (element_value, "INDEXED"))
		columnP->flag |= COL_INDEXED;
	    if (NULL != strcasestr (element_value, "PRIMARY"))
		columnP->flag |= COL_PRIMARY;
	    if (NULL != strcasestr (element_value, "NULLABLE"))
		columnP->flag |= COL_NULLABLE;
	    // assume no attributes
	}
	if (strcasecmp (element_name, "std") == 0) {
	    columnP->std = -1;
	    if (NULL != strcasestr (element_value, "true"))
		columnP->std = TRUE;
	    if (NULL != strcasestr (element_value, "1"))
		columnP->std = TRUE;
	    if (NULL != strcasestr (element_value, "false"))
		columnP->std = FALSE;
	    if (NULL != strcasestr (element_value, "0"))
		columnP->std = FALSE;
	    assert ((columnP->std == TRUE) || (columnP->std == FALSE));
	    // assume no attributes
	}
	// *************************************
	// ***** FOREIGNKEY ELEMENT: BEGIN *****
	// *************************************
	if (strcasecmp (element_name, "foreignKey") == 0) {
	    vosi_tableLevel4 (h3, columnP, &keyP, &index_key);
	}
	// ***********************************
	// ***** FOREIGNKEY ELEMENT: END *****
	// ***********************************
    }
    // ASCEND!
    *columnPP = columnP;
    *index_columnP = index_column;
    return (EXIT_SUCCESS);
}


static int
vosi_tableLevel2 (handle_t h1, vosiSchema * schemaP, vosiTable ** tablePP, 
	int *index_tableP) 
{
    handle_t h2;
    vosiTable *tableP = *tablePP;
    int index_table = (*index_tableP);
    vosiColumn *columnP = NULL;
    handle_t h;
    int index_column = 0;
    char *spud = NULL;
    char namesAC[SZ_XMLTAG];
    char spaceAC[2] = " ";
    char *element_name;
    char *element_value;
    char *attribute_name;
    char *attribute_value;

    assert (NULL != schemaP);
    h = h1;
    index_column = -1;
    index_table++;
    {				// create the vosiTable structure
	spud = calloc ((size_t) 1, sizeof (vosiTable));
	assert (NULL != spud);
	if (index_table == 0) {	// allocate first vosiTable structure
	    schemaP->table = (vosiTable *) spud;
	    tableP = (vosiTable *) spud;
	} else {		// allocate new vosiTable structure
	    assert (NULL != tableP);
	    tableP->next = (vosiTable *) spud;
	    tableP = (vosiTable *) spud;	// reset tableP
	}
	tableP->index = index_table;
    }
    {				// get the type attribute (if it exists)
	strncpy (namesAC, xml_dumpNamesAttr (h), SZ_XMLTAG);
	attribute_name = strtok (namesAC, spaceAC);	// get first name
	while (attribute_name != NULL) {
	    attribute_value = xml_getAttr (h, attribute_name);
	    if (strcasecmp (attribute_name, "type") == 0) {
		tableP->type = vosi_callocAC ();
		strncpy (tableP->type, attribute_value, SZ_LINE);
	    }
	    attribute_name = strtok (NULL, spaceAC);
	}
    }
    //
    // get elements of the table element
    //
    // DECEND!
    for (h2 = xml_getChild (h1); h2; h2 = xml_getSibling (h2)) {
	h = h2;
	element_name = xml_elemNameByHandle (h);
	element_value = xml_getValue (h);
	if (strcasecmp (element_name, "name") == 0) {
	    tableP->name = vosi_callocAC ();
	    strncpy (tableP->name, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "title") == 0) {
	    tableP->title = vosi_callocAC ();
	    strncpy (tableP->title, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "description") == 0) {
	    tableP->description = vosi_callocAC ();
	    strncpy (tableP->description, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "utype") == 0) {
	    tableP->utype = vosi_callocAC ();
	    strncpy (tableP->utype, element_value, SZ_LINE);
	    // assume no attributes
	}
	// *********************************
	// ***** COLUMN ELEMENT: BEGIN *****
	// *********************************
	if (strcasecmp (element_name, "column") == 0) {
	    vosi_tableLevel3 (h2, tableP, &columnP, &index_column);
	}
	// *******************************
	// ***** COLUMN ELEMENT: END *****
	// *******************************
    }
    // ASCEND!
    *tablePP = tableP;
    *index_tableP = index_table;
    return (EXIT_SUCCESS);
}


static int
vosi_tableLevel1 (handle_t h0, vosiSchema ** schemaPP, int *index_schemaP) 
{
    handle_t h1;
    vosiSchema *schemaP = *schemaPP;
    int index_schema = (*index_schemaP);
    vosiTable *tableP = NULL;
    vosiTableSet *tsetGP = (vosiTableSet *) & tsetG;
    handle_t h;
    char *spud = NULL;
    char *element_name;
    char *element_value;
    int index_table;

    assert (NULL != tsetGP);
    h = h0;
    index_table = -1;
    index_schema++;
    {				// create the vosiSchema structure
	spud = calloc ((size_t) 1, sizeof (vosiSchema));
	assert (NULL != spud);
	if (index_schema == 0) {	// allocate first vosiSchema structure
	    tsetGP->schema = (vosiSchema *) spud;
	    schemaP = (vosiSchema *) spud;	// alias
	} else {		// allocate new vosiSchema structure
	    assert (NULL != schemaP);
	    schemaP->next = (vosiSchema *) spud;
	    schemaP = (vosiSchema *) spud;
	}
	schemaP->index = index_schema;
	tsetGP->nschema = index_schema + 1;
    }
    // assume no attributes
    //
    // get elements of the schema element
    //
    // DECEND!
    for (h1 = xml_getChild (h0); h1; h1 = xml_getSibling (h1)) {
	h = h1;
	element_name = xml_elemNameByHandle (h);
	element_value = xml_getValue (h);
	if (strcasecmp (element_name, "name") == 0) {
	    schemaP->name = vosi_callocAC ();
	    strncpy (schemaP->name, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "title") == 0) {
	    schemaP->title = vosi_callocAC ();
	    strncpy (schemaP->title, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "description") == 0) {
	    schemaP->description = vosi_callocAC ();
	    strncpy (schemaP->description, element_value, SZ_LINE);
	    // assume no attributes
	}
	if (strcasecmp (element_name, "utype") == 0) {
	    schemaP->utype = vosi_callocAC ();
	    strncpy (schemaP->utype, element_value, SZ_LINE);
	    // assume no attributes
	}
	// ********************************
	// ***** TABLE ELEMENT: BEGIN *****
	// ********************************
	if (strcasecmp (element_name, "table") == 0) {
	    vosi_tableLevel2 (h1, schemaP, &tableP, &index_table);
	}
	// ******************************
	// ***** TABLE ELEMENT: END *****
	// ******************************
    }
    // ASCEND!
    *schemaPP = schemaP;
    *index_schemaP = index_schema;
    return (EXIT_SUCCESS);
}


/** 
 *  vosi_getTables -- Access the VOSI 'tables' service 
 *
 *  @brief  Access the VOSI 'tables' service 
 *  @fn     handle_t vosi_getTables (char *arg)
 *
 *  @param  arg 	The source of the XML document (fname|url|str)
 *  @return	 	Handle to the vosiTableSet struct, -1 on error
 */
handle_t
vosi_getTables (char *arg)
{
    handle_t xp = 0;
    handle_t top = 0;
    handle_t h0 = 0;
    handle_t ret_handle = 0;
    vosiTableSet *tsetGP = (vosiTableSet *) & tsetG;
    vosiSchema *schemaP = NULL;
    char *element_name = NULL;
    char *element_value = NULL;
    char *name = NULL;
    int index_schema = 0;

    assert (NULL != tsetGP);	// the vosiTableSet structure must exist

    /* 
     * Open the argument, returning a handle to the parsed document.  
     *
     * The input arg may be a URL to the service, 
     * a filename containing the XML result, 
     * or the XML doc in a string retrieved from an earlier query.
     */
    if ((xp = xml_openXML (arg)) < 0)
	return (-1);

    // get the toplevel handle
    top = xml_getToplevel (xp);

    // Error checking
    name = xml_getToplevelName (top);
    if (name == (char *) NULL || strcasecmp (name, "tableset"))
	return (-1);

    // create namespace dictionary
    vosi_setNameSpace (top);

    // initialize handles system
    vosi_initHandles ();

    // erase the existing vosiTableSet structure
    memset (&tsetG, 0, sizeof (vosiTableSet));
    tsetGP = &tsetG;		// pointer to the structure

    index_schema = -1;		// initialize
    // DECEND!
    for (h0 = xml_getChild (top); h0; h0 = xml_getSibling (h0)) {
	element_name = xml_elemNameByHandle (h0);
	element_value = xml_getValue (h0);
	assert (0 == strcasecmp (element_name, "schema"));
	vosi_tableLevel1 (h0, &schemaP, &index_schema);
    }
    // ASCEND!

    // free the document resources 
    xml_closeXML (xp);

    // Create a return handle to the vosiTableSet structure
    tset_hconG = vosi_newHandleContext ("VOSI", 1026);	// handles need a "context"
    ret_handle = vosi_newHandle (tset_hconG, &tsetG);
    return (ret_handle);
}


/** 
 *  vosi_numSchema -- get number of vosiSchema structures 
 *
 *  @brief   get number of vosiSchema structures 
 *  @fn      int vosi_numSchema (handle_t tset)
 *
 *  @param   handle to the vosiTableSet structure
 *  @return  number of vosiSchema structures [-1 for bad handle]
 */
int vosi_numSchema (handle_t tableset)
{
    vosiTableSet *tP = NULL;
    vosiSchema *xP = NULL;
    int count = 0;

    if (tableset <= 0)
	return (-1);
    tP = (vosiTableSet *) vosi_H2P (tableset);
    assert (NULL != tP);
    xP = tP->schema;
    if (NULL == xP)
	return (count);
    count++;
    while (NULL != xP->next) {
	count++;
	xP = xP->next;
    }
    assert (count == tP->nschema);	// Num Me Vexo?
    return (count);
}


/** 
 *  vosi_schemaByIndex -- get a schema with a given index
 *
 *  @brief   get a schema with a given index
 *  @fn      handel_t vosi_schemaByIndex (handle_t tset, int index)
 *
 *  @param   handle to the vosiTableSet structure
 *  @param   index : index value of the schema
 *  @return  handle of the schema [-1 if bad handle][zero if (index<0) || (index>=vosi_numSchema (tset ) ) ]
 */
handle_t
vosi_schemaByIndex (handle_t tableset, int index)
{
    vosiTableSet *tP = NULL;
    vosiSchema *xP = NULL;

    if (tableset <= 0)
	return (-1);
    if (index < 0)
	return (0);		// bad index value 
    tP = (vosiTableSet *) vosi_H2P (tableset);
    assert (NULL != tP);
    xP = tP->schema;
    assert (NULL != xP);
    if (index == 0)
	return (vosi_newHandle (tset_hconG, xP));
    while (NULL != xP->next) {
	xP = xP->next;
	if (xP->index == index)
	    return (vosi_newHandle (tset_hconG, xP));
    }
    return (0);			// no match
}


/** 
 *  vosi_getSchema -- get the first schema
 *
 *  @brief   get the first schem
 *  @fn      handel_t vosi_getSchema (handle_t tset)
 *
 *  @param   handle to the vosiTableSet structure
 *  @return  handle of the first schema [-1 if bad handle]
 */
handle_t
vosi_getSchema (handle_t tableset)
{
    vosiTableSet *tP = NULL;
    vosiSchema *xP = NULL;

    if (tableset <= 0)
	return (-1);
    tP = (vosiTableSet *) vosi_H2P (tableset);
    assert (NULL != tP);
    xP = tP->schema;
    assert (NULL != xP);
    return (vosi_newHandle (tset_hconG, xP));
}


/** 
 *  vosi_nextSchema -- get the next schema 
 *
 *  @brief   get the next schema
 *  @fn      handel_t vosi_nextSchema (handle_t schema)
 *
 *  @param   handle to the vosiSchema structure
 *  @return  handle of the next schema (-1 if bad input handle) (0 if no next schema)
 */
handle_t
vosi_nextSchema (handle_t schema)
{
    vosiSchema *xP = NULL;

    if (schema <= 0)
	return (-1);		// bad handle
    xP = (vosiSchema *) vosi_H2P (schema);
    xP = xP->next;
    if (NULL == xP)
	return (0);
    return (vosi_newHandle (tset_hconG, xP));
}


/** 
 *  vosi_schemaIndex -- index of vosiSchema structure
 *
 *  @brief  index of vosiSchema structure
 *  @fn     int vosi_schemaIndex (handle_t schema)
 *
 *  @param  handle to the vosiSchema structure
 *  @return int value of index of the vosiSchema structure (-1 if error)
 */
int 
vosi_schemaIndex (handle_t schema)
{
    vosiSchema *xP = NULL;

    if (schema <= 0)
	return (-1);		// bad handle

    xP = (vosiSchema *) vosi_H2P (schema);
    return (xP->index);
}


/** 
 *  vosi_schemaName -- name of vosiSchema structure
 *
 *  @brief  name of vosiSchema structure
 *  @fn     char *vosi_schemaName (handle_t schema)
 *
 *  @param  handle to the vosiSchema structure
 *  @return string of name of the vosiSchema structure 
 */
char *
vosi_schemaName (handle_t schema)
{
    vosiSchema *xP = NULL;

    if (schema <= 0)
	return ((char *) NULL);	// bad handle

    xP = (vosiSchema *) vosi_H2P (schema);
    return (xP->name);
}


/** 
 *  vosi_schemaTitle -- title of vosiSchema structure
 *
 *  @brief  title of vosiSchema structure
 *  @fn     char *vosi_schemaTitle (handle_t schema)
 *
 *  @param  handle to the vosiSchema structure
 *  @return string of title of the vosiSchema structure 
 */
char *
vosi_schemaTitle (handle_t schema)
{
    vosiSchema *xP = NULL;

    if (schema <= 0)
	return ((char *) NULL);	// bad handle

    xP = (vosiSchema *) vosi_H2P (schema);
    return (xP->title);
}


/** 
 *  vosi_schemaDescription -- Description of vosiSchema structure
 *
 *  @brief  Description of vosiSchema structure
 *  @fn     char *vosi_schemaDescription (handle_t schema)
 *
 *  @param  handle to the vosiSchema structure
 *  @return string of Description of the vosiSchema structure 
 */
char *
vosi_schemaDescription (handle_t schema)
{
    vosiSchema *xP = NULL;

    if (schema <= 0)
	return ((char *) NULL);	// bad handle

    xP = (vosiSchema *) vosi_H2P (schema);
    return (xP->description);
}


/** 
 *  vosi_schemaUType -- UType of vosiSchema structure
 *
 *  @brief  UType of vosiSchema structure
 *  @fn     char *vosi_schemaUType (handle_t schema)
 *
 *  @param  handle to the vosiSchema structure
 *  @return string of UType of the vosiSchema structure (-1 if error)
 */
char *
vosi_schemaUType (handle_t schema)
{
    vosiSchema *xP = NULL;

    if (schema <= 0)
	return ((char *) NULL);	// bad handle

    xP = (vosiSchema *) vosi_H2P (schema);
    return (xP->utype);
}


/** 
 *  vosi_numTable -- get number of tables in a schema 
 *
 *  @brief   get number of tables in a schema
 *  @fn      int vosi_numTable (handle_t schema)
 *
 *  @param   handle to vosiSchema structure
 *  @return  int number of vosiTable structures in the schema
 */
int 
vosi_numTable (handle_t schema)
{
    vosiSchema *sP = NULL;
    vosiTable *tP = NULL;
    int count = 0;

    if (schema <= 0)
	return (-1);		// bad handle
    sP = (vosiSchema *) vosi_H2P (schema);
    tP = sP->table;
    if (NULL == tP)
	return (0);
    count++;
    while (NULL != tP->next) {
	count++;
	tP = tP->next;
    }
    return (count);
}


/** 
 *  vosi_getTable -- get the first table
 *
 *  @brief   get the first table
 *  @fn      handel_t vosi_getTable (handle_t schema)
 *
 *  @param   handle to vosiSchema structure
 *  @return  handle of the first table [0 if no tables exist][-1 if bad handle]
 */
handle_t
vosi_getTable (handle_t schema)
{
    vosiSchema *sP = NULL;
    vosiTable *tP = NULL;

    if (schema <= 0)
	return (-1);		// bad handle
    sP = (vosiSchema *) vosi_H2P (schema);
    tP = sP->table;
    if (NULL == tP)
	return (0);
    return (vosi_newHandle (tset_hconG, tP));
}


/** 
 *  vosi_tableName -- name of vosiTable structure
 *
 *  @brief  name of vosiTable structure
 *  @fn     char *vosi_tableName (handle_t table)
 *
 *  @param  handle to the vosiTable structure
 *  @return string of name of the vosiTable structure [NULL if bad handle] 
 */
char *vosi_tableName (handle_t table)
{
    vosiTable *xP = NULL;

    if (table <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiTable *) vosi_H2P (table);
    return (xP->name);
}

/** 
 *  vosi_tableTitle -- title of vosiTable structure
 *
 *  @brief  title of vosiTable structure
 *  @fn     char *vosi_tableTitle (handle_t table)
 *
 *  @param  handle to the vosiTable structure
 *  @return string of title of the vosiTable structure [NULL if bad handle] 
 */
char *vosi_tableTitle (handle_t table)
{
    vosiTable *xP = NULL;

    if (table <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiTable *) vosi_H2P (table);
    return (xP->title);
}


/** 
 *  vosi_tableDescription -- description of vosiTable structure
 *
 *  @brief  description of vosiTable structure
 *  @fn     char *vosi_tableDescription (handle_t table)
 *
 *  @param  handle to the vosiTable structure
 *  @return string of description of the vosiTable structure [NULL if bad handle] 
 */
char *
vosi_tableDescription (handle_t table)
{
    vosiTable *xP = NULL;

    if (table <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiTable *) vosi_H2P (table);
    return (xP->description);
}


/** 
 *  vosi_tableUType -- UType of vosiTable structure
 *
 *  @brief  UType of vosiTable structure
 *  @fn     char *vosi_tableUType (handle_t table)
 *
 *  @param  handle to the vosiTable structure
 *  @return string of description of the vosiTable structure [NULL if bad handle] 
 */
char *
vosi_tableUType (handle_t table)
{
    vosiTable *xP = NULL;

    if (table <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiTable *) vosi_H2P (table);
    return (xP->utype);
}


/** 
 *  vosi_tableIndex -- Index of vosiTable structure
 *
 *  @brief  Index of vosiTable structure
 *  @fn     int vosi_tableIndex (handle_t table)
 *
 *  @param  handle to the vosiTable structure
 *  @return string of description of the vosiTable structure [-1 if bad handle] 
 */
int 
vosi_tableIndex (handle_t table)
{
    vosiTable *xP = NULL;

    if (table <= 0)
	return (-1);		// bad handle
    xP = (vosiTable *) vosi_H2P (table);
    assert (NULL != xP);
    return (xP->index);
}


/** 
 *  vosi_nextTable -- get the next table 
 *
 *  @brief   get the next table
 *  @fn      handel_t vosi_nextTable (handle_t table)
 *
 *  @param   handle to the vosiTable structure
 *  @return  handle of the next table [-1 if bad input handle][0 if no next table]
 */
handle_t
vosi_nextTable (handle_t table)
{
    vosiTable *xP = NULL;

    if (table <= 0)
	return (-1);		// bad handle
    xP = (vosiTable *) vosi_H2P (table);
    xP = xP->next;
    if (NULL == xP)
	return (0);
    return (vosi_newHandle (tset_hconG, xP));
}


/** 
 *  vosi_tableByIndex -- get a table with a given index
 *
 *  @brief   get a table with a given index
 *  @fn      handel_t vosi_tableByIndex (handle_t schema, int index)
 *
 *  @param   handle to the vosiSchema structure
 *  @param   index : index value of the table
 *  @return  handle of the table [-1 if bad handle][zero if (index<0) || (index>=vosi_numTable (tset ) ) ]
 */
handle_t
vosi_tableByIndex (handle_t schema, int index)
{
    vosiSchema *sP = NULL;
    vosiTable *xP = NULL;

    if (schema <= 0)
	return (-1);
    if (index < 0)
	return (0);		// bad index value 
    sP = (vosiSchema *) vosi_H2P (schema);
    assert (NULL != sP);
    xP = sP->table;
    assert (NULL != xP);
    if (index == 0)
	return (vosi_newHandle (tset_hconG, xP));
    while (NULL != xP->next) {
	xP = xP->next;
	if (xP->index == index)
	    return (vosi_newHandle (tset_hconG, xP));
    }
    return (0);			// no match
}


/** 
 *  vosi_numCols -- get number of columns in a table
 *
 *  @brief   get number of columns in a table
 *  @fn      int vosi_numCols (handle_t table)
 *
 *  @param   handle to vosiTable structure
 *  @return  int number of vosiColumn structures in the table [-1 if bad handle]
 */
int vosi_numCols (handle_t table)
{
    vosiTable *tP = NULL;
    vosiColumn *xP = NULL;
    int count = 0;

    if (table <= 0)
	return (-1);		// bad handle
    tP = (vosiTable *) vosi_H2P (table);
    assert (NULL != tP);
    xP = tP->cols;
    if (NULL == xP)
	return (0);
    count++;
    while (NULL != xP->next) {
	count++;
	xP = xP->next;
    }
    assert (count == tP->ncols);	// Num Me Vexo?
    return (count);

}


/** 
 *  vosi_getColumn -- get the first column
 *
 *  @brief   get the first column
 *  @fn      handel_t vosi_getColumn (handle_t table)
 *
 *  @param   handle to vosiTable structure
 *  @return  handle of the first column [0 if no columns exist][-1 if bad handle]
 */
handle_t
vosi_getColumn (handle_t table)
{
    vosiTable *tP = NULL;
    vosiColumn *cP = NULL;

    if (table <= 0)
	return (-1);		// bad handle
    tP = (vosiTable *) vosi_H2P (table);
    cP = tP->cols;
    if (NULL == cP)
	return (0);
    return (vosi_newHandle (tset_hconG, cP));
}


/** 
 *  vosi_colIndex -- Index of vosiColumn structure
 *
 *  @brief  Index of vosiColumn structure
 *  @fn     int vosi_colIndex (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return int value of index of the vosiColumn structure [-1 if bad handle] 
 */
int 
vosi_colIndex (handle_t column)
{
    vosiColumn *xP = NULL;

    if (column <= 0)
	return (-1);		// bad handle
    xP = (vosiColumn *) vosi_H2P (column);
    assert (NULL != xP);
    return (xP->index);
}


/** 
 *  vosi_colName -- name of vosiColumn structure
 *
 *  @brief  name of vosiColumn structure
 *  @fn     char *vosi_colName (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return string of name of the vosiColumn structure [NULL if bad handle] 
 */
char *
vosi_colName (handle_t col)
{
    vosiColumn *xP = NULL;

    if (col <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiColumn *) vosi_H2P (col);
    return (xP->name);
}


/** 
 *  vosi_colDescription -- description of vosiColumn structure
 *
 *  @brief  description of vosiColumn structure
 *  @fn     char *vosi_colDescription (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return string of description of the vosiColumn structure [NULL if bad handle] 
 */
char *vosi_colDescription (handle_t col)
{
    vosiColumn *xP = NULL;

    if (col <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiColumn *) vosi_H2P (col);
    return (xP->description);
}


/** 
 *  vosi_colUnit -- unit of vosiColumn structure
 *
 *  @brief  unit of vosiColumn structure
 *  @fn     char *vosi_colUnit (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return string of unit of the vosiColumn structure [NULL if bad handle] 
 */
char *vosi_colUnit (handle_t col)
{
    vosiColumn *xP = NULL;

    if (col <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiColumn *) vosi_H2P (col);
    return (xP->unit);
}


/** 
 *  vosi_colUCD -- UCD of vosiColumn structure
 *
 *  @brief  UCD of vosiColumn structure
 *  @fn     char *vosi_colUCD (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return string of UCD of the vosiColumn structure [NULL if bad handle] 
 */
char *
vosi_colUCD (handle_t col)
{
    vosiColumn *xP = NULL;

    if (col <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiColumn *) vosi_H2P (col);
    return (xP->ucd);
}

/** 
 *  vosi_colUType -- UType of vosiColumn structure
 *
 *  @brief  UType of vosiColumn structure
 *  @fn     char *vosi_colUType (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return string of UType of the vosiColumn structure [NULL if bad handle] 
 */
char *
vosi_colUType (handle_t col)
{
    vosiColumn *xP = NULL;

    if (col <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiColumn *) vosi_H2P (col);
    return (xP->utype);
}


/** 
 *  vosi_colDatatype -- datatype of vosiColumn structure
 *
 *  @brief  datatype of vosiColumn structure
 *  @fn     char *vosi_colDatatype (handle_t column)
 *
 *  @param  handle to the vosiColumn structure
 *  @return string of datatype of the vosiColumn structure [NULL if bad handle] 
 */
char *
vosi_colDatatype (handle_t col)
{
    vosiColumn *xP = NULL;

    if (col <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiColumn *) vosi_H2P (col);
    return (xP->datatype);
}


/** 
 *  vosi_nextColumn -- get the next column 
 *
 *  @brief   get the next column
 *  @fn      handel_t vosi_nextColumn (handle_t column)
 *
 *  @param   handle to the vosiColumn structure
 *  @return  handle of the next column [-1 if bad input handle][0 if no next column]
 */
handle_t
vosi_nextColumn (handle_t column)
{
    vosiColumn *xP = NULL;

    if (column <= 0)
	return (-1);		// bad handle
    xP = (vosiColumn *) vosi_H2P (column);
    xP = xP->next;
    if (NULL == xP)
	return (0);
    return (vosi_newHandle (tset_hconG, xP));
}


/** 
 *  vosi_colByIndex -- get a column with a given index
 *
 *  @brief   get a column with a given index
 *  @fn      handel_t vosi_colByIndex (handle_t table, int index)
 *
 *  @param   handle to the vosiTable structure
 *  @param   index : index value of the column
 *  @return  handle of the column [-1 if bad handle][zero if (index<0) || (index>=vosi_numCols (table ) ) ]
 */
handle_t
vosi_colByIndex (handle_t table, int index)
{
    vosiTable *tP = NULL;
    vosiColumn *cP = NULL;

    if (table <= 0)
	return (-1);
    if (index < 0)
	return (0);		// bad index value 
    tP = (vosiTable *) vosi_H2P (table);
    assert (NULL != tP);
    cP = tP->cols;
    assert (NULL != cP);
    if (index == 0)
	return (vosi_newHandle (tset_hconG, cP));
    while (NULL != cP->next) {
	cP = cP->next;
	if (cP->index == index)
	    return (vosi_newHandle (tset_hconG, cP));
    }
    return (0);			// no match
}


/** 
 *  vosi_numKeys -- get number of keys in a column
 *
 *  @brief   get number of keys in a column
 *  @fn      int vosi_numKeys (handle_t column)
 *
 *  @param   handle to vosiColumn structure
 *  @return  int number of vosiForeignKey structures in a column [-1 if bad handle]
 */
int vosi_numKeys (handle_t column)
{
    vosiColumn *cP = NULL;
    vosiForeignKey *xP = NULL;
    int count = 0;

    if (column <= 0)
	return (-1);		// bad handle
    cP = (vosiColumn *) vosi_H2P (column);
    assert (NULL != cP);
    xP = cP->keys;
    if (NULL == xP)
	goto bye;
    count++;
    while (NULL != xP->next) {
	count++;
	xP = xP->next;
    }
  bye:
    assert (count == cP->nkeys);	// Num Me Vexo?
    return (count);
}


/** 
 *  vosi_getKey -- get the first ForeignKey in a column
 *
 *  @brief   get the first ForeignKey in a column
 *  @fn      handel_t vosi_getKey (handle_t column)
 *
 *  @param   handle to vosiColumn structure
 *  @return  handle of the first ForeignKey in a column [0 if no keys exist][-1 if bad handle]
 */
handle_t
vosi_getKey (handle_t column)
{
    vosiColumn *cP = NULL;
    vosiForeignKey *kP = NULL;

    if (column <= 0)
	return (-1);		// bad handle
    cP = (vosiColumn *) vosi_H2P (column);
    kP = cP->keys;
    if (NULL == kP)
	return (0);
    return (vosi_newHandle (tset_hconG, kP));
}


/** 
 *  vosi_nextKey -- get the next key in a column 
 *
 *  @brief   get the next key in a column
 *  @fn      handel_t vosi_nextKey (handle_t key)
 *
 *  @param   handle to the vosiKey structure
 *  @return  handle of the next column [-1 if bad input handle][0 if no next key]
 */
handle_t
vosi_nextKey (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return (-1);		// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    xP = xP->next;
    if (NULL == xP)
	return (0);
    return (vosi_newHandle (tset_hconG, xP));
}


/** 
 *  vosi_keyByIndex -- get a key with a given index
 *
 *  @brief   get a key with a given index
 *  @fn      handel_t vosi_keyByIndex (handle_t column, int index)
 *
 *  @param   handle to the vosiTable structure
 *  @param   index : index value of the key
 *  @return  handle of the key [-1 if bad handle][zero if bad index][-2 if no keys]
 */
handle_t 
vosi_keyByIndex (handle_t column, int index)
{
    vosiColumn *cP = NULL;
    vosiForeignKey *kP = NULL;

    if (column <= 0)
	return (-1);		// bad handle
    if (index < 0)
	return (0);		// bad index value 
    cP = (vosiColumn *) vosi_H2P (column);
    assert (NULL != cP);
    kP = cP->keys;
    if (NULL == kP)
	return (-2);
    if (index == 0)
	return (vosi_newHandle (tset_hconG, kP));
    while (NULL != kP->next) {
	kP = kP->next;
	if (kP->index == index)
	    return (vosi_newHandle (tset_hconG, kP));
    }
    return (0);			// no match (bad index value)
}


/** 
 *  vosi_keyTargetTable -- targetTable of vosiForeignKey structure
 *
 *  @brief  targetTable of vosiForeignKey structure
 *  @fn     char *vosi_keyTargetTable (handle_t key)
 *
 *  @param  handle to the vosiForeignKey structure
 *  @return string of targetTable of the vosiForeignKey structure [NULL if bad handle] 
 */
char *
vosi_keyTargetTable (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    return (xP->targetTable);
}


/** 
 *  vosi_keyFromColumn -- targetTable of vosiForeignKey structure
 *
 *  @brief  targetTable of vosiForeignKey structure
 *  @fn     char *vosi_keyFromColumn (handle_t key)
 *
 *  @param  handle to the vosiForeignKey structure
 *  @return string of targetTable of the vosiForeignKey structure [NULL if bad handle] 
 */
char *vosi_keyFromColumn (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    return (xP->fromColumn);
}


/** 
 *  vosi_keyTargetColumn -- targetTable of vosiForeignKey structure
 *
 *  @brief  targetTable of vosiForeignKey structure
 *  @fn     char *vosi_keyTargetColumn (handle_t key)
 *
 *  @param  handle to the vosiForeignKey structure
 *  @return string of targetTable of the vosiForeignKey structure [NULL if bad handle] 
 */
char *
vosi_keyTargetColumn (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    return (xP->targetColumn);
}


/** 
 *  vosi_keyDescription -- targetTable of vosiForeignKey structure
 *
 *  @brief  targetTable of vosiForeignKey structure
 *  @fn     char *vosi_keyDescription (handle_t key)
 *
 *  @param  handle to the vosiForeignKey structure
 *  @return string of targetTable of the vosiForeignKey structure [NULL if bad handle] 
 */
char *
vosi_keyDescription (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    return (xP->description);
}


/** 
 *  vosi_keyUType -- targetTable of vosiForeignKey structure
 *
 *  @brief  targetTable of vosiForeignKey structure
 *  @fn     char *vosi_keyUType (handle_t key)
 *
 *  @param  handle to the vosiForeignKey structure
 *  @return string of targetTable of the vosiForeignKey structure [NULL if bad handle] 
 */
char *
vosi_keyUType (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return ((char *) NULL);	// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    return (xP->utype);
}


/** 
 *  vosi_keyIndex -- Index of vosiForeignKey structure
 *
 *  @brief  Index of vosiForeignKey structure
 *  @fn     int vosi_keyIndex (handle_t key)
 *
 *  @param  handle to the vosiForeignKey structure
 *  @return int value of index of the vosiForeignKey structure [-1 if bad handle] 
 */
int
vosi_keyIndex (handle_t key)
{
    vosiForeignKey *xP = NULL;

    if (key <= 0)
	return (-1);		// bad handle
    xP = (vosiForeignKey *) vosi_H2P (key);
    assert (NULL != xP);
    return (xP->index);
}


/*****************************************************************************
 *  Private procedures.
 ****************************************************************************/

/** 
 *  vosi_printk -- Private printing function
 *
 *  @brief  Private printing function
 *  @fn     void vosi_printk (char *format, char *string)
 *
 *  @param  format      Format string with one %s 
 *  @param  avail 	String to be printed
 *  @return	 	void
 */
static void
vosi_printk (char *format, char *string) 
{
    if (NULL == format)
	return;
    if (NULL == string)
	return;
    if (string[0] != '\0')
	printf (format, string);	// only print non-empty strings
    return;
}


/** 
 *  vosi_getNameSpace -- Private function converts tag to name space
 *
 *  @brief  Private function converts tag to name space
 *  @fn     char *vosi_getNameSpace (char *nstag)
 *
 *  @param  nstag       String containing name space tag
 *  @return	 	String of name space
 */
static char *
vosi_getNameSpace (
    char *nstag			// be sure to *not* include the colon!
    ) 
{
    int j = -1;

    for (j = 0; j < MAX_ATTR; j++) {
	if (strcmp (xml_NameSpaceTagG[j], nstag) == 0)
	    return (xml_NameSpaceG[j]);
    }
    return (NULL);
}


/** 
 *  vosi_nsSplit -- Split a token into a namespace tag and a name
 *
 *  @brief  Split a token into a namespace tag and a name
 *  @fn     void vosi_nsSplit (char *token, char *ns, char *name)
 *
 *  @param  char *token     input token
 *  @param  char *ns        output namespace tag string
 *  @param  char *name      output name string
 *  @return void
 */
static void 
vosi_nsSplit (char *token, char *ns, char *name) 
{
    char *spud = NULL;
    int pos = -1;

    assert (NULL != token);
    assert (NULL != ns);
    assert (NULL != name);
    assert (strlen (token) < SZ_LINE);
    // initialize
    ns[0] = '\0';
    name[0] = '\0';
    // make sure token has a colon character
    spud = strchr (token, ':');
    if (NULL == spud)
	return;			// nope!
    pos = spud - token;		// position of colon character
    (void) strncpy (ns, token, pos + 1);
    ns[pos] = '\0';
    spud++;			// advance one char beyond the colon character
    (void) strncpy (name, spud, strlen (spud) + 1);
    return;
}


/** 
 *  vosi_callocAC -- Private function allocate a string of sized SZ_LINE
 *
 *  @brief  Private function allocate a string of sized SZ_LINE
 *  @fn     char *vosi_callocAC()
 *
 *  @return   a new array of char of size SZ_LINE
 */
static char *
vosi_callocAC (void) 
{
    char *new = NULL;

    new = (char *) calloc (SZ_LINE, sizeof (char));
    assert (NULL != new);
    return (new);
}


/** 
 *  vosi_setNameSpace -- Private function creates NameSpace dictionary
 *
 *  @brief  Private function creates NameSpace dictionary
 *  @fn     void vosi_setNameSpace (handle_t top)
 *
 *  @param  top         Handle of top level
 *  @return	 	void
 */
static void 
vosi_setNameSpace (handle_t top) 
{
    handle_t h;
    char space[2] = " ";
    char names[SZ_XMLTAG];
    char *token = NULL;
    char *value = NULL;
    char *ns = NULL;
    char *name = NULL;
    int j = -1;
    int count = 0;

    ns = vosi_callocAC ();
    assert (NULL != ns);
    name = vosi_callocAC ();
    assert (NULL != name);
    // initialize
    for (j = 0; j < MAX_ATTR; j++) {
	xml_NameSpaceTagG[j][0] = '\0';
	xml_NameSpaceG[j][0] = '\0';
    }
    count = 0;
    xml_NameSpaceCountG = count;
    h = top;
    strncpy (names, xml_dumpNamesAttr (h), SZ_XMLTAG);
    token = strtok (names, space);	// get first name
    while (token != NULL) {
	vosi_nsSplit (token, ns, name);
	if ((ns[0] != '\0')
	    && (name[0] != '\0')
	    && ((strstr (ns, "xmlns") - ns) == 0)) {
	    assert (strlen (name) < SZ_ATTRNAME);
	    strncpy (xml_NameSpaceTagG[count], name, SZ_ATTRNAME);
	    value = xml_getAttr (h, token);
	    assert (strlen (value) < SZ_ATTRVAL);
	    strncpy (xml_NameSpaceG[count], value, SZ_ATTRVAL);
	    count++;
	    xml_NameSpaceCountG = count;
	}
	token = strtok (NULL, space);
    }
    return;
}


