/**
 * DALQUERY -- Compose a query to a VO Data Access Layer (DAL) Service.
 *
 * These routines are used to create a DALClient context to be used to query
 * a DAL service, and then compose the query.
 *  
 * ---- Service Connection ---------
 *
 *       dal = dal_openConnection (baseurl, protocol, version)
 *   dal = dal_openConeConnection (baseurl, version)
 *   dal = dal_openSiapConnection (baseurl, version)
 *   dal = dal_openSsapConnection (baseurl, version)
 *            dal_closeConnection (dal)
 *
 *                 dal_setBaseUrl (dal, url)		# setters/getters
 *           url = dal_getBaseUrl (dal)
 *
 *         errcode = dal_getError (dal)
 *                 dal_clearError (dal)
 *
 * ---- Query Parameters ---------
 *
 *           query = dal_getQuery (dal)
 *		   dal_closeQuery (query)		# free resources
 *
 *       query = dal_getConeQuery (dal, ra, dec, sr)
 *       query = dal_getSiapQuery (dal, ra, dec, ra_size, dec_size, format)
 *       query = dal_getSsapQuery (dal, ra, dec, size, band, time, format)
 *
 *       query = dal_genConeQuery (dal, pos)
 *       query = dal_genSiapQuery (dal, pos, band, time, pol, format, mode)
 *       query = dal_genSsapQuery (dal, pos, band, time, pol, format, mode)
 *
 *	      pos = dal_getObjPos (objname|NULL)
 *	         pos = dal_setPos (c1, c2, size, frame|NULL)
 *	        pos = dal_setPos2 (c1, c2, size1, size2, frame|NULL)
 *	             <additional-constructors>
 *	             <additional-setters>
 *	         dal_posGetCoords (pos, c1, c2, &frame)
 *	  	     dal_closePos (pos)
 *
 *	  int = dal_getParamCount (query)
 *         str = dal_getParamName (query, index)
 *         stat = dal_addIntParam (query, pname, ival)
 *       stat = dal_addFloatParam (query, pname, dval)
 *      stat = dal_addStringParam (query, pname, str)
 *		     dal_setParam (query, pname, str)
 *	       str = dal_getParam (query, pname, index)
 *	  	     dal_delParam (query, pname)
 *
 *	    str = dal_getQueryURL (query)
 *
 * A separate connection is required for each service to be queried.  Simple
 * queries may be composed directly via a service class-specific constructor,
 * or queries may be built up with parameters, or both.
 *
 * All public routines in this interface that return an object handle, e.g.,
 * for a Dal or Query object, return an opaque object handle (not a normal C
 * object pointer).
 *
 *
 * @file	dalQuery.c
 * @author	Doug Tody
 * @version	January 2014
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <float.h>

#define _DALCLIENT_LIB_
#include "dalclient.h"


/*
 * Service Connection Management.
 * -------------------------------
 */


/**
 * dal_openConnection -- Create a new connection context for a DAL service.
 *
 * @brief   Create a new connection context for a DAL service.
 * @fn	    dal = dal_openConnection (svc_url, protocol, version)
 *
 * @param   baseurl		Base URL for the service
 * @param   protocol		Service protocol
 * @param   version		Service version
 * @return			Connection handle or error
 *
 * A connection context is required to access a service.  Each service has
 * its own connection context.
 *
 * The currently recognized service types or protocols are "scs", "sia", and
 * "ssa" (case not significant; may also be specified as "scsp", "siap", etc.).
 * Custom protocol names may be specified, but only generic processing will be
 * performed if the protocol is not recognized.  Generic processing includes
 * parameter marshalling, queryURL formatting, query execution, and processing
 * of the query response VOTable, but does not include service-specific query
 * formulation, or generation of abstract properties in the query response.
 *
 * DALClient public object handles are opaque integer values that are
 * converted internal to the interface to and from object pointers (this
 * provides a valid object handle in a distributed multiprocess scenario).
 */
DAL
dal_openConnection (char *baseurl, char *protocol, char *version)
{
    /* Allocate the connection descriptor. */
    dalConn_t *dal = calloc ((size_t)1, sizeof(dalConn_t));
    if (dal == NULL)
	return (DAL_ERROR);

    if (protocol == NULL)
        return (dal_error (dal, DAL_NOPROTOCOL));
    if (version == NULL)
        return (dal_error (dal, DAL_NOVERSION));
    if (baseurl == NULL)
        return (dal_error (dal, DAL_NOBASEURL));

    /* Initialize descriptor. */
    strncpy (dal->protocol, protocol, SZ_NAME-1);
    strncpy (dal->version, version, SZ_NAME-1);
    strncpy (dal->baseurl, baseurl, SZ_URL-1);

    /* Check for a standard service type. */
    if (strncasecmp (protocol, "scs", 3) == 0)
	dal->type = CONE_CONN;
    else if (strncasecmp (protocol, "sia", 3) == 0)
	dal->type = SIAP_CONN;
    else if (strncasecmp (protocol, "ssa", 3) == 0)
	dal->type = SSAP_CONN;
    else
	dal->type = DAL_CONN;

    /* Get handle context, used to convert ptrs to handles. */
    dal->context = svr_newHandleContext ("dal", 0);
    if (dal->context < 0) {
	free ((void *)dal);
	return (dal_error (dal, DAL_BADHANDLE));
    }

    /* Return an opaque handle for the new service connection. */
    if ((dal->dal_h = svr_newHandle (dal->context, dal)) == 0)
	return (dal_error (dal, DAL_BADHANDLE));
    else
	return (dal->dal_h);
}


/**
 * dal_openConeConnection -- Create a new context for a Cone service.
 *
 * @brief   Create a new context for a Cone service.
 * @fn	    dal = dal_openConeConnection (svc_url, version)
 *
 * @param   baseurl		Base URL for the service
 * @param   version		Service version
 */
DAL
dal_openConeConnection (char *baseurl, char *version)
{
    return (dal_openConnection (baseurl, "scs", version));
}


/**
 * dal_openSiapConnection -- Create a new context for a Siap service.
 *
 * @brief   Create a new context for a Siap service.
 * @fn	    dal = dal_openSiapConnection (svc_url, version)
 *
 * @param   baseurl		Base URL for the service
 * @param   version		Service version
 */
DAL
dal_openSiapConnection (char *baseurl, char *version)
{
    return (dal_openConnection (baseurl, "sia", version));
}


/**
 * dal_openSsapConnection -- Create a new context for a Ssap service.
 *
 * @brief   Create a new context for a Ssap service.
 * @fn	    dal = dal_openSsapConnection (svc_url, version)
 *
 * @param   baseurl		Base URL for the service
 * @param   version		Service version
 */
DAL
dal_openSsapConnection (char *baseurl, char *version)
{
    return (dal_openConnection (baseurl, "ssa", version));
}


/**
 * dal_closeConnection -- Close a service connection.
 *
 * @brief   Close a service connection
 * @fn	    void dal_closeConnection (dal)
 *
 * @param   dal			Service handle
 *
 * Closing a connection does not automatically free any associated Query
 * or QueryResponse objects.
 */
void
dal_closeConnection (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    /* Free the Handle context. */
    svr_closeHandleContext (dal->context);

    /* Free the service descriptor. */
    free ((void *) dal);
}



/*
 * Utility setter/getter functions.
 * --------------------------------
 */

void  dal_setBaseUrl (DAL dal_h, char *base_url);
char *dal_getBaseUrl (DAL dal_h);

/**
 * dal_setBaseUrl -- Set the base service URL for the connection.
 *
 * @brief   Set the base service URL for the connection.
 * @fn	    dal_setBaseUrl (dal, base_url)
 *
 * @param   dal			DAL service connection
 * @param   base_url		Base service URL
 * @return			Nothing
 */
void
dal_setBaseUrl (DAL dal_h, char *base_url)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    if (dal && base_url)
        strncpy (dal->baseurl, base_url, SZ_URL-1);
}


/**
 * dal_getBaseUrl -- Get the base service URL for the connection.
 *
 * @brief   Get the base service URL for the connection.
 * @fn	    base_url = dal_getBaseUrl (dal)
 *
 * @param   dal			DAL service connection
 * @return			Base service URL
 */
char *
dal_getBaseUrl (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    return ( (dal ? dal->baseurl : NULL) );
}


/*
 * High Level Query Constructors
 * -------------------------------
 */


/**
 * dal_getQuery -- Get a Query object for a service.
 *
 * @brief   Get a query object for a service
 * @fn	    query = dal_getQuery (dal)
 *
 * @param   dal			DAL service connection
 * @return			Query object handle
 *
 * A query object for the service is created and returned, ready to add
 * query parameters.
 */
Query
dal_getQuery (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    /* Allocate the Query descriptor. */
    dalQuery_t *query = calloc ((size_t)1, sizeof(dalQuery_t));
    if (query == NULL)
	return (dal_error (dal, DAL_MEMALLOCFAIL));

    /* Initialize descriptor. */
    query->dal = dal;
    query->params = vll_init (0);
    if (query->params == NULL) {
	free ((void *)query);
	return (dal_error (dal, DAL_VLLINIT));
    }
    query->pnames = vht_init (SZ_PNAMEHASH, NULL, "nocase");
    if (query->pnames == NULL) {
	vll_destroy (query->params, NULL);
	free ((void *)query);
	return (dal_error (dal, DAL_VHTINIT));
    }

    /* Return an opaque handle for the new service connection. */
    return (query->query_h = svr_newHandle (dal->context, query));
}


/**
 * dal_closeQuery -- Close a Query object and free resources.
 *
 * @brief   Close a Query object and free resources
 * @fn	    void dal_closeQuery (query)
 *
 * @param   query		Query object handle
 * @return			Nothing
 *
 * The query object is closed, freeing all resources.
 * Note that any derived objects such as a QueryResponse, are not
 * automatically freed.
 */
void
dal_closeQuery (Query query_h)
{
    /* Recover the Query object from its Handle. */
    dalQuery_t *query = svr_H2P (query_h);

    vht_destroy (query->pnames);
    vll_destroy (query->params, free);

    svr_freeHandle (query->query_h);
    free ((void *)query);
}


/**
 * dal_getConeQuery -- Get a Query object for a Cone (catalog) service.
 *
 * @brief   Get a Query object for a Cone (catalog) service.
 * @fn	    query = dal_getConeQuery (dal, ra, dec, sr)
 *
 * @param   dal			DAL service connection
 * @param   ra			Right ascension, ICRS
 * @param   dec			Declination, ICRS
 * @param   sr			Search radius, degrees
 * @return			Query object handle
 *
 * A Query object for the service is created, initialized with the given
 * parameters.  Additional parameters may be added later individually if
 * desired.
 */
Query
dal_getConeQuery (DAL dal_h, double ra, double dec, double sr)
{
    /* Get an empty Query object. */
    Query query_h = dal_getQuery (dal_h);
    if (!query_h || query_h == DAL_ERROR)
	return (DAL_ERROR);

    dalQuery_t *query = svr_H2P (query_h);
    query->request = DAL_QueryData;

    /* Initialize the parameters. */
    dal_clearError (dal_h);
    dal_addFloatParam (query_h, "RA", ra);
    dal_addFloatParam (query_h, "DEC", dec);
    dal_addFloatParam (query_h, "SR", sr);
    if (dal_getError (dal_h) != 0) {
	dal_closeQuery (query_h);
	return (DAL_ERROR);
    }

    return (query_h);
}


/**
 * dal_getSiapQuery a Siap (image) service.
 *
 * @brief   Get a Query object for a Siap (image) service.
 * @fn	    query = dal_getSiapQuery (dal, ra, dec, ra_size, dec_size, format)
 *
 * @param   dal			DAL service connection
 * @param   ra			Right ascension, ICRS
 * @param   dec			Declination, ICRS
 * @param   ra_size		Width of region of interest (degrees)
 * @param   dec_size		Height of region of interest (degrees)
 * @param   format		Desired image format or formats (optional)
 * @return			Query object
 *
 * A Query object for the service is created, initialized with the given
 * parameters.  Additional parameters may be added later individually if
 * desired.
 */
Query
dal_getSiapQuery (DAL dal_h,
    double ra, double dec, double ra_size, double dec_size, char *format)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    /* Get an empty Query object. */
    Query query_h = dal_getQuery (dal_h);
    if (!query_h || query_h == DAL_ERROR)
        return (DAL_ERROR);

    /* Specify the service REQUEST to be executed (omit for SIAV1). */
    dalQuery_t *query = svr_H2P (query_h);
    query->request = DAL_QueryData;
    if (strncmp (dal->version, "2", 1) == 0)
        dal_addStringParam (query_h, "REQUEST", "queryData");

    /* Format the POS and SIZE parameters. */
    char pos[SZ_NAME], size[SZ_NAME];
    sprintf (pos, "%0.3lf,%0.3lf", ra, dec);

    double d = ra_size - dec_size;
    if (d < 0) d = -d;

    if (d <= FLT_EPSILON)
        sprintf (size, "%0.3lf", ra_size);
    else
        sprintf (size, "%0.3lf,%0.3lf", ra_size, dec_size);

    /* Initialize the parameters. */
    dal_clearError (dal_h);
    dal_addStringParam (query_h, "POS", pos);
    dal_addStringParam (query_h, "SIZE", size);
    if (format != NULL)
        dal_addStringParam (query_h, "FORMAT", format);
    if (dal_getError (dal_h) != 0) {
        dal_closeQuery (query_h);
        return (DAL_ERROR);
    }

    return (query_h);
}


/**
 * dal_getSsapQuery a Ssap (spectrum) service.
 *
 * @brief   Get a Query object for a Ssap (spectrum) service.
 * @fn	    query = dal_getSsapQuery (dal, ra, dec, size, band, time, format)
 *
 * @param   dal			DAL service connection
 * @param   ra			Right ascension, ICRS
 * @param   dec			Declination, ICRS
 * @param   size		Diameter of search region (degrees)
 * @param   band		Spectral band or region (meters)
 * @param   time		Time or time-range of observation (ISO 8601)
 * @param   format		Desired image format or formats (optional)
 * @return			Query object handle
 *
 * A Query object for the service is created, initialized with the given
 * parameters.  Additional parameters may be added later individually if
 * desired.
 */
Query
dal_getSsapQuery (DAL dal_h,
    double ra, double dec, double size, char *band, char *time, char *format)
{
    /* Get an empty Query object. */
    Query query_h = dal_getQuery (dal_h);
    if (!query_h || query_h == DAL_ERROR)
        return (DAL_ERROR);

    /* Specify the service REQUEST to be executed. */
    dalQuery_t *query = svr_H2P (query_h);
    query->request = DAL_QueryData;
    dal_addStringParam (query_h, "REQUEST", "queryData");

    /* Format the POS and SIZE parameters. */
    char posval[SZ_NAME], sizeval[SZ_NAME];
    sprintf (posval, "%0.3lf,%0.3lf", ra, dec);
    sprintf (sizeval, "%0.3lf", size);

    /* Initialize the parameters. */
    dal_clearError (dal_h);
    dal_addStringParam (query_h, "POS", posval);
    dal_addStringParam (query_h, "SIZE", sizeval);
    if (band != NULL)
        dal_addStringParam (query_h, "BAND", band);
    if (time != NULL)
        dal_addStringParam (query_h, "TIME", time);
    if (format != NULL)
        dal_addStringParam (query_h, "FORMAT", format);
    if (dal_getError (dal_h) != 0) {
        dal_closeQuery (query_h);
        return (DAL_ERROR);
    }

    return (query_h);
}


/*
 * dal_getAccessDataQuery -- Get a Query object for an AccessData request.
 *
 * @brief   Get a Query object for an AccessData request.
 * @fn      query = dal_getAccessDataQuery (dal)
 *
 * @param   dal                 DAL service connection
 * @return                      Query object
 *
 * A Query object for an AccessData request is created for the service type
 * associated with the given DAL connection.  At the present time, accessData
 * requests are only supported for SIAP (image) services.
 *
 * Since there is no fixed pattern for AccessData requests, no parameters
 * are set when the Query object is created.  AddParam calls must be made
 * following creation to set the query parameters required for the type of
 * access desired by the client.
 */
Query
dal_getAccessDataQuery (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    /* Get an empty Query object. */
    Query query_h = dal_getQuery (dal_h);
    if (!query_h || query_h == DAL_ERROR)
        return (DAL_ERROR);

    /* Specify the service REQUEST to be executed (omit for SIAV1). */
    dalQuery_t *query = svr_H2P (query_h);
    query->request = DAL_AccessData;
    if (strncmp (dal->version, "2", 1) == 0)
        dal_addStringParam (query_h, "REQUEST", "accessData");

    dal_clearError (dal_h);

    return (query_h);
}



/*
 * Query Parameter Management.
 * -------------------------------
 */


/**
 * dal_getParamCount -- Get the current number of parameters.
 *
 * @brief   Get the current number of parameters
 * @fn	    int = dal_getParamCount (query)
 *
 * @param   query		Query object handle
 * @returns			The current number of parameters
 *
 * Returns the number of parameters in the current query parameter set.
 * The parameters are stored in a List object (query.params).
 */
int
dal_getParamCount (Query query_h)
{
    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (DAL_ERROR);

    return (vll_count (query->params));
}


/**
 * dal_getParamName -- Get the name of the indicated parameter.
 *
 * @brief   Get the name of the indicated parameter.
 * @fn	    str = dal_getParamName (query, index)
 *
 * @param   query		Query object handle
 * @param   index		Index of the desired parameter
 * @returns			The current number of parameters
 *
 * Returns the name of a parameter given its index in the parameter set
 * (zero-indexed).  NULL is returned at the end of the list, or for any
 * invalid index.
 */
char *
dal_getParamName (Query query_h, int index)
{
    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (NULL);

    /* Get the Param object at the given index. */
    dalParam_t *param =
	(dalParam_t *) vll_seek (query->params, index, SEEK_SET);

    if (!param)
	return (dal_nError (query->dal, DAL_PARAMNOTFOUND));
    else
	return (param->name);
}


/**
 * dal_addIntParam -- Add an integer parameter to the parameter set.
 *
 * @brief   Add an integer parameter to the parameter set.
 * @fn	    status = dal_addIntParam (query, pname, value)
 *
 * @param   query		Query object handle
 * @param   pname		Parameter name
 * @param   value		Parameter value
 * @returns			Status (DAL_OK or DAL_ERROR)
 *
 * A integer parameter instance is appended to the parameter set.  If the
 * parameter already exists, another instance is added.
 */
int
dal_addIntParam (Query query_h, char *pname, long value)
{
    /* Format the parameter value. */
    char valstr[SZ_VALSTR];
    sprintf (valstr, "%ld", value);

    /* Add the parameter to the pset. */
    return (dal_addStringParam (query_h, pname, valstr));
}


/**
 * dal_addFloatParam -- Add a float parameter to the parameter set.
 *
 * @brief   Add a float parameter to the parameter set.
 * @fn	    status = dal_addFloatParam (query, pname, value)
 *
 * @param   query		Query object handle
 * @param   pname		Parameter name
 * @param   value		Parameter value
 * @returns			Status value (DAL_OK or DAL_ERROR)
 *
 * A floating parameter instance is appended to the parameter set.  If the
 * parameter already exists, another instance is added.
 */
int
dal_addFloatParam (Query query_h, char *pname, double value)
{
    /* Format the parameter value. */
    char valstr[SZ_VALSTR];
    sprintf (valstr, "%.*g", DBL_DIG, value);

    /* Add the parameter to the pset. */
    return (dal_addStringParam (query_h, pname, valstr));
}


/**
 * dal_addStringParam -- Add a string parameter to the parameter set.
 *
 * @brief   Add a string parameter to the parameter set.
 * @fn	    status = dal_addStringParam (query, pname, value)
 *
 * @param   query		Query object handle
 * @param   pname		Parameter name
 * @param   value		Parameter value
 * @returns			Status value (DAL_OK or DAL_ERROR)
 *
 !* A string parameter instance is appended to the parameter set.  If the
 * parameter already exists, another instance is added.  [An alternative
 * here would be to have a single instance of each Param, permitting a
 * list of values, using the List object.]
 */
int
dal_addStringParam (Query query_h, char *pname, char *value)
{
    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (DAL_ERROR);

    /* Create the Param instance. */
    dalParam_t *obj;
    dalParam_t *param = (dalParam_t *) calloc ((size_t)1, sizeof(dalParam_t));
    if (param == NULL)
	return (dal_qError (query, DAL_MEMALLOCFAIL));

    strncpy (param->name, pname, SZ_NAME-1);
    strncpy (param->value, value, SZ_VALSTR-1);

    /* Append it to the parameter list. */
    obj = (dalParam_t *) vll_append (query->params,
	(void *)param, &param->index);
    if (obj == NULL) {
	free ((void *)param);
	return (dal_qError (query, DAL_PARAMAPPEND));
    }

    /* Hash the parameter name if not already present, otherwise
     * add the new instance to the list of instances.
     */
    obj = (dalParam_t *) vht_findKey (query->pnames, pname);
    if (obj) {
	/* Append the instance to the instance list for the param. */
	dalParam_t *last;
	for (last=obj;  (obj=obj->next) != NULL;  last=obj)
	    ;
	last->next = param;
    } else {
	/* Hash the new parameter name. */
	obj = (dalParam_t *)
	    vht_insertKey (query->pnames, pname, (void *)param);
	if (obj == NULL) {
	    free ((void *)param);
	    return (dal_qError (query, DAL_PARAMHASH));
	}
    }

    return (DAL_OK);
}


/**
 * dal_setParam -- Set the value of a parameter, adding it if necessary.
 *
 * @brief   Set the value of a parameter, adding it if necessary.
 * @fn	    status = dal_setParam (query, pname, value)
 *
 * @param   query		Query object handle
 * @param   pname		Parameter name
 * @param   value		Parameter value
 * @returns			Status value (OK or ERR)
 *
 * The value of the named parameter is set if the parameter already exists,
 * otherwise the parameter is added to the pset as a new parameter.  If an
 * existing parameter has multiple instances, all will be deleted except
 * the first.
 */
int
dal_setParam (Query query_h, char *pname, char *value)
{
    dalParam_t *node, *obj;

    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (DAL_ERROR);

    /* Find the first param instance via the hash table.  Add the parameter if
     * it does not already exist.
     */
    dalParam_t *param = (dalParam_t *) vht_findKey (query->pnames, pname);
    if (param == NULL)
	return (dal_addStringParam (query_h, pname, value));

    /* Replace the value of an existing parameter.  Start by deleting all
     * instances but the first.
     */
    vll_seek (query->params, -1, SEEK_SET);  /* rewind list */

    for (obj=param->next;  obj != NULL;  obj=node) {
	obj = vll_find (query->params, obj, &obj->index);
	if (obj == NULL)
	    break;
	node = obj->next;
	vll_remove (query->params, -1);
	free ((void *)obj);
    }

    /* Set the new value of the first instance. */
    strncpy (param->value, value, SZ_VALSTR-1);
    param->next = NULL;

    return (DAL_OK);
}


/**
 * dal_getParam -- Get the value of parameter or parameter instance.
 *
 * @brief   Get the value of parameter or parameter instance.
 * @fn	    str = dal_getParam (query, pname, index)
 *
 * @param   query		Query object handle
 * @param   pname		Parameter name
 * @param   index		Parameter instance
 * @returns			String value of parameter
 *
 * The value of the named parameter is returned.  If an index value greater
 * than zero is input, and the parameter has multiple instance values, the
 * value of the given instance is returned, otherwise NULL.
 * the first.
 */
char *
dal_getParam (Query query_h, char *pname, int index)
{
    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (NULL);

    /* Find the parameter. */
    dalParam_t *param = (dalParam_t *) vht_findKey (query->pnames, pname);
    if (param == NULL)
	return (dal_nError (query->dal, DAL_PARAMNOTFOUND));

    /* Return the first value if that is all that was requested. */
    if (index == 0)
	return (param->value);

    /* Return the requested instance value. */
    dalParam_t *obj;  int i;
    for (obj=param, i=1;  (obj = obj->next) != NULL;  i++) {
	if (i == index)
	    return (obj->value);
    }

    return (NULL);
}


/**
 * dal_delParam -- Delete the named parameter.
 *
 * @brief   Delete the named parameter
 * @fn	    status = dal_delParam (query, pname)
 *
 * @param   query		Query object handle
 * @param   pname		Parameter name
 * @returns			Status value (OK or ERR)
 *
 * All instances of the named parameter are deleted.  It is not an error if
 * the parameter does not exist.
 */
int
dal_delParam (Query query_h, char *pname)
{
    dalParam_t *node, *obj;

    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (DAL_ERROR);

    /* Find the initial param instance via the hash table. */
    node = (dalParam_t *) vht_findKey (query->pnames, pname);

    /* Delete all instances of the parameter.  */
    vll_seek (query->params, -1, SEEK_SET);   /* rewind list */

    for (obj=node;  obj != NULL;  obj=node) {
	obj = vll_find (query->params, obj, &obj->index);
	if (obj == NULL)
	    break;
	node = obj->next;
	vll_remove (query->params, -1);
	free ((void *)obj);
    }

    /* Delete the hash entry for the param. */
    vht_removeKey (query->pnames, pname);

    return (DAL_OK);
}


/**
 * dal_getQueryURL -- Return the query URL for the current Query param set.
 *
 * @brief   Return the query URL for the current Query param set
 * @fn	    url = getQueryURL (query)
 *
 * @param   query		Query object handle
 * @returns			The query URL as a string
 *
 * Construction of the query URL from a Query parameter set may in general
 * be service-specific requiring custom treatment of the Query parameters
 * depending upon the service type and version.
 */
char *
dal_getQueryURL (Query query_h)
{
    /* Get the Query object descriptor. */
    dalQuery_t *query = svr_H2P (query_h);
    if (query == NULL)
	return (NULL);

    dalConn_t *dal = query->dal;
    switch (dal->type) {
    case CONE_CONN:
	return (scsp_getQueryURL (query));
	break;
    case SIAP_CONN:
	return (siap_getQueryURL (query));
	break;
    case SSAP_CONN:
	return (ssap_getQueryURL (query));
	break;
    }

    /* Compose a generic query URL. */
    vocList_t *params = query->params;
    dalParam_t *param;

    /* Compose the URL */
    void *urlBuilder = vut_urlOpen (dal->baseurl, NULL);

    param = (dalParam_t *) vll_seek (params, 0, SEEK_SET);
    while (param != NULL) {
	vut_urlAddParam (urlBuilder, param->name, param->value);
	param = (dalParam_t *) vll_next (params);
    }

    char *url = strdup (vut_urlGetString (urlBuilder));
    vut_urlClose (urlBuilder);

    return (url);
}


/**
 * dal_getError -- Get the error status.
 *
 * @brief   Get the error status
 * @fn	    status = dal_getError (dal)
 *
 * @param   dal			Service handle
 * @return			Error code or zero
 *
 * The current error status is returned, or zero if no error is currently
 * posted.  A non-zero error code is set whenever an error occurs.  The
 * error status remains in effect until explicitly cleared, or until it
 * is overwritten by a subsequent error.  The DALClient error codes are
 * defined in dalclient.h.
 */
int
dal_getError (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    if (dal)
	return (dal->errcode);
    else
	return (DAL_BADHANDLE);
}


/*
 * dal_getErrorMsg -- Get the error message, if any.
 *
 * @brief   Get the error message string
 * @fn      ptr = dal_getErrorMsg (dal)
 *
 * @param   dal                 Service handle
 * @return                      Pointer to error message or NULL
 *
 * Not all errors set the error message string, in which case the
 * saved value may correspond to an older error condition.  Clearing
 * the error status will also clear the save error message if any.
 */
char *
dal_getErrorMsg (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    if (dal && dal->errmsg[0] != '\0')
        return (dal->errmsg);
    else
        return (NULL);
}


/**
 * dal_clearError -- Clear the error status for the connection.
 *
 * @brief   Clear the error status for the connection
 * @fn	    void dal_clearError (dal)
 *
 * @param   dal			Service handle
 * @return			Nothing
 */
void
dal_clearError (DAL dal_h)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);
    
    if (dal) {
	dal->status = DAL_OK;
	dal->errcode = 0;
	dal->errmsg[0] = '\0';
    }
}


/*
 * Internal routines.
 * --------------------
 */


/**
 * dal_error -- Post an error for the connection.
 *
 * The given error code is stored in the DAL connection context for later
 * retrieval.  The error status is saved until explicitly cleared.
 * DAL_ERROR is returned as the status value, allowing the error status 
 * to be posted within a return statement.
 */
int
dal_error (dalConn_t *dal, int errcode)
{
    dal->errcode = errcode;
    return (dal->status = DAL_ERROR);
}


/**
 * dal_setErrorMsg -- Set the error message string.
 */
void
dal_setErrorMsg (dalConn_t *dal, char *errmsg)
{
    strncpy (dal->errmsg, errmsg, SZ_ERRMSG-1);
}


/**
 * dal_nError -- Post an error for the connection, returning NULL.
 * NULL is returned as the status value, allowing the error status 
 * to be posted within a return statement.
 */
void *
dal_nError (dalConn_t *dal, int errcode)
{
    dal->errcode = errcode;
    dal->status = DAL_ERROR;
    return (NULL);
}


/**
 * dal_qError -- Convenience routine for posting an error within the
 * query construction code.
 */
int
dal_qError (dalQuery_t *query, int errcode)
{
    dalConn_t *dal = query->dal;
    return (dal_error (dal, errcode));
}


/**
 * dal_qrError -- Convenience routine for posting an error within the
 * query response code.
 */
int
dal_qrError (dalQR_t *qr, int errcode)
{
    dalConn_t *dal = qr->query->dal;
    return (dal_error (dal, errcode));
}
