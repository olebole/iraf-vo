/**
 * DALQEXEC -- Execution of VO Data Access Layer (DAL) Queries.
 *
 * These routines take a DAL service Connection object and corresponding
 * DAL Query object containing the parameters defining a query, execute the
 * query, retrieve the query response VOTable, and parse the VOTable,
 * building up a Query Response object.
 *
 * ---- Query Execution ---------
 *
 *         qr = dal_executeQuery (query)
 *      str = dal_executeVOTable (query)
 *         stat = dal_accessData (query, fname)
 *
 *     qr = dal_getQueryResponse (query)
 *    qr = dal_initQueryResponse (dal, votable, resource, table)
 *	  dal_closeQueryResponse (qr)		# free resources
 *
 *      csv_tab = dal_executeCSV (query)
 *      tsv_tab = dal_executeTSV (query)
 *       ascii = dal_executeText (query)
 *
 * The result of these routines is normally a QueryResponse object, but can
 * also be the raw VOTable or processed serializations of the QueryResponse,
 * e.g., text such as CSV, TSV or a formatted text table. In the case of
 * AccessData an Image subset is returned, produced by directly accessing
 * the remote Image dataset.
 *
 * dal_executeQuery provides a QueryResponse constructor that executes a
 * predefined Query and processes the response to produce a QueryResponse
 * object for the query response VOTAble.  The lower level initQueryResponse
 * provides a QueryResponse constructor for any generic VOTable.
 *
 * @file	dalQExec.c
 * @author	Doug Tody, Mike Fitzpatrick
 * @version	January 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <float.h>

#define _DALCLIENT_LIB_
#include "dalclient.h"
#include "votParse.h"


/*
 * Query Execution.
 * -------------------------------
 */


/**
 * dal_executeQuery -- Execute a query and build a QueryResponse object.
 *
 * @brief   Execute a query and build a QueryResponse object.
 * @fn	    qr = dal_executeQuery (query)
 *
 * @param   query		Query object handle
 * @returns			Runtime QueryResponse object
 *
 * The query must have been prepared as a Query object before the query
 * can be executed and processed into a QueryResponse object.
 */
QResponse
dal_executeQuery (Query query_h)
{
    /* Recover the Query object from its Handle. */
    dalQuery_t *query = svr_H2P (query_h);
    dalConn_t *dal = query->dal;

    /* Get the Query URL. */
    char *queryURL = dal_getQueryURL (query_h);
    if (queryURL == NULL)
	return (dal_qError (query, DAL_FORMQUERYURL));

    /* Execute the query and retrieve the query response VOTable. */
    dal_clearError (dal->dal_h);
    char *votable = vut_getURL (dal, queryURL);

    if (votable == NULL) {
	/* Pass back any lower-level error. */
	int errcode = dal_getError (dal->dal_h);
	return (dal_qError (query,
	    (errcode > 0) ? errcode : DAL_EXECUTEQUERY));
    }

    /* Init the QueryResponse descriptor from the votable. */
    query->qr_h = dal_initQueryResponse (dal->dal_h, votable, 0, 0);
    if (query->qr_h == DAL_ERROR)
	return (DAL_ERROR);

    dalQR_t *qr = svr_H2P (query->qr_h);
    qr->query = query;

    /* Process the QueryResponse and compute object properties.  */
    switch (dal->type) {
    case CONE_CONN:
	scsp_initProperties (qr);
	break;
    case SIAP_CONN:
	siap_initProperties (qr);
	break;
    case SSAP_CONN:
	ssap_initProperties (qr);
	break;
    default:
	;
    }
    
    /* Return an opaque handle for the QR. */
    return (query->qr_h);
}


/**
 * dal_accessData -- Execute an AccessData request as specified by the
 * given Query object.
 *
 * @brief   Execute an AccessData request.
 * @fn      stat = dal_accessData (query, fname)
 *
 * @param   query               Query object handle
 * @param   fname               Output file to be written
 * @returns                     Integer status value
 *
 * The AccessData request must have been prepared as a Query object before
 * the request can be processed.  The remote (Image) dataset is directly
 * accessed, computing the specified subset and returning this as a new Image
 * in the specified file.  The input Query object may be reused, possibly after
 * edits, for subsequent requests.
 */
int
dal_accessData (Query query_h, char *fname)
{
    /* Recover the Query object from its Handle. */
    dalQuery_t *query = svr_H2P (query_h);
    dalConn_t *dal = query->dal;

    /* Get the Query URL. */
    char *queryURL = dal_getQueryURL (query_h);
    if (queryURL == NULL)
        return (dal_qError (query, DAL_FORMQUERYURL));

    /* Execute the request and retrieve the data subset. */
    return (vut_getURLtoFile (dal, queryURL, fname));
}


/**
 *  dal_getQueryResponse -- Get QueryResponse object for an already exec;d query
 *
 * @brief   Get the QueryResponse object for an already executed query.
 * @fn	    qr = dal_getQueryResponse (query)
 *
 * @param   query		Query object handle
 * @returns			Runtime QueryResponse object
 *
 * Returns the handle for an already prepared QueryReponse object, or error if
 * the query has not yet be executed.
 */
QResponse
dal_getQueryResponse (Query query_h)
{
    dalQuery_t *query = svr_H2P (query_h);
    if (query->qr_h == 0)
	return (dal_qError (query, DAL_QUERYNOTEXEC));
    else
	return (query->qr_h);
}


/**
 * dal_initQueryResponse -- Init a QueryResponse object from a VOTable.
 *
 * @brief   Init a QueryResponse object from a VOTable.
 * @fn	    qr = dal_initQueryResponse (dal, votable, resource, table)
 *
 * @param   dal			DAL service context
 * @param   votable		VOTable text
 * @param   resource		RESOURCE element to be used (0-indexed)
 * @param   table		TABLE element to be used (0-indexed)
 * @returns			Runtime QueryResponse object
 *
 * Constructor to create a QueryResponse object from a VOTable, input as
 * a block of text.  The client should not free the votable text buffer
 * as management of this buffer is taken over by the QueryResponse object,
 * and it will be freed when the QueryResponse is closed.
 */
QResponse
dal_initQueryResponse (DAL dal_h, char *votable, int resource, int table)
{
    int i, res, info, param, tab, field;
    int data, tdata, tr, td, nrows, ncols;

     /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    /* Allocate the QueryResponse descriptor. */
    dalQR_t *qr = calloc ((size_t)1, sizeof(dalQR_t));
    if (qr == NULL)
	return (dal_error (dal, DAL_MEMALLOCFAIL));

    /* Parse the VOTable into a VOTable object. */
    qr->votable = votable;
    vot_setWarnings (0);	/* disable warning messages */
    handle_t vot = vot_openVOTABLE (qr->votable);
    if (vot <= 0) {
	free ((void *)qr);
	return (dal_error (dal, DAL_VOTABLEPARSE));
    }

    /* Get the RESOURCE element to be used. */
    for (res=vot_getRESOURCE(vot), i=0;  res;  res = vot_getNext(res), i++)
	if (i == resource)
	    break;
    if (!res)
	return (dal_error (dal, DAL_VOTABLENORESOURCE));

    /* Check for QUERY_STATUS=ERROR. */
    for (info=vot_getINFO(res);  info;  info = vot_getNext(info)) {
        if (strcmp (vot_getAttr(info, "name"), "QUERY_STATUS") == 0) {
            if (strcmp (vot_getAttr(info, "value"), "ERROR") == 0) {
                vot_closeVOTABLE (vot);
                free ((void *)qr);
                return (dal_error (dal, DAL_VOQUERYERROR));
            }
        }
    }

    /* Get the TABLE element to be used. */
    for (tab=vot_getTABLE(res), i=0;  tab;  tab = vot_getNext(tab), i++)
	if (i == table)
	    break;
    if (!tab)
	return (dal_error (dal, DAL_VOTABLENOTABLE));

    /* Table data elements. */
    data = vot_getDATA (tab);
    if (!data)
	return (dal_error (dal, DAL_VOTABLENODATA));

    tdata = vot_getTABLEDATA (data);
    if (!tdata)
	return (dal_error (dal, DAL_VOTABLENOTDATA));

    nrows = vot_getNRows (tdata);
    ncols = vot_getNCols (tdata);

    /* Initialize an empty Properties list. */
    if ((qr->properties = vll_init(0)) == NULL)
	return (dal_error (dal, DAL_VLLINIT));
    qr->hash_prop = vht_init (SZ_FNAMEHASH, NULL, "nocase");

    /* Process the VOTable object into a QueryResponse object. */
    qr->nrows = nrows;
    qr->ncols = ncols;

    /* Get INFOs. */
    if ((qr->infos = vll_init(0)) == NULL)
	return (dal_error (dal, DAL_VLLINIT));

    for (info=vot_getINFO(res);  info;  info = vot_getNext(info)) {
	dalField_t *elem = (dalField_t *) calloc ((size_t)1, sizeof(dalField_t));

	elem->type  = DAL_INFO;
        vut_setString (elem->id, vot_getAttr(info, "id"), SZ_NAME-1);
        vut_setString (elem->name, vot_getAttr(info, "name"), SZ_NAME-1);
        vut_setString (elem->utype, vot_getAttr(info, "utype"), SZ_KEY-1);
        vut_setString (elem->ucd, vot_getAttr(info, "ucd"), SZ_NAME-1);
        vut_setString (elem->value, vot_getAttr(info, "value"), SZ_VALSTR-1);

	vll_append (qr->infos, (void *)elem, NULL);
    }

    /* Get PARAMs (stored with FIELDs, and hashed). */
    if ((qr->fields = vll_init(0))== NULL)
	return (dal_error (dal, DAL_VLLINIT));

    qr->hash_id = vht_init (SZ_FNAMEHASH, NULL, "nocase");
    qr->hash_name = vht_init (SZ_FNAMEHASH, NULL, "nocase");
    qr->hash_utype = vht_init (SZ_FNAMEHASH, NULL, "nocase");
    qr->hash_ucd = vht_init (SZ_FNAMEHASH, NULL, "nocase");
    if (!qr->hash_id || !qr->hash_name || !qr->hash_utype || !qr->hash_ucd)
	return (dal_error (dal, DAL_VHTINIT));

    for (param=vot_getPARAM(tab);  param;  param = vot_getNext(param)) {
	dalField_t *elem = (dalField_t *) calloc ((size_t)1, sizeof(dalField_t));

	/* Get the PARAM/FIELD metadata. */
	elem->type  = DAL_PARAM;
        vut_setString (elem->id, vot_getAttr(param, "id"), SZ_NAME-1);
        vut_setString (elem->name, vot_getAttr(param, "name"), SZ_NAME-1);
        vut_setString (elem->utype, vot_getAttr(param, "utype"), SZ_KEY-1);
        vut_setString (elem->ucd, vot_getAttr(param, "ucd"), SZ_NAME-1);
        vut_setString (elem->unit, vot_getAttr(param, "unit"), SZ_NAME-1);
        vut_setString (elem->xtype, vot_getAttr(param, "xtype"), SZ_NAME-1);
        vut_setString (elem->datatype, vot_getAttr(param, "datatype"), SZ_NAME-1);
        vut_setString (elem->arraysize, vot_getAttr(param, "arraysize"), SZ_NAME-1);
        vut_setString (elem->width, vot_getAttr(param, "width"), SZ_NAME-1);
        vut_setString (elem->precision, vot_getAttr(param, "precision"), SZ_NAME-1);
        vut_setString (elem->value, vot_getAttr(param, "value"), SZ_VALSTR-1);

	/* Insert into list and hash keys. */
	vll_append (qr->fields, (void *)elem, &elem->index);
	vht_insertKey (qr->hash_id, elem->id, (void *)elem);
	vht_insertKey (qr->hash_name, elem->name, (void *)elem);
	vht_insertKey (qr->hash_utype, elem->utype, (void *)elem);
	vht_insertKey (qr->hash_ucd, elem->ucd, (void *)elem);
    }

    /* Add FIELDs */
    int colnum = 0;
    for (field=vot_getFIELD(tab);  field;  field = vot_getNext(field)) {

        dalField_t *elem = (dalField_t *) calloc ((size_t)1, sizeof(dalField_t));
        elem->type  = DAL_FIELD;
        elem->colnum = colnum++;

        /* Get the PARAM/FIELD metadata. */
        vut_setString (elem->id, vot_getAttr(field, "id"), SZ_NAME-1);
        vut_setString (elem->name, vot_getAttr(field, "name"), SZ_NAME-1);
        vut_setString (elem->utype, vot_getAttr(field, "utype"), SZ_KEY-1);
        vut_setString (elem->ucd, vot_getAttr(field, "ucd"), SZ_NAME-1);
        vut_setString (elem->unit, vot_getAttr(field, "unit"), SZ_NAME-1);
        vut_setString (elem->xtype, vot_getAttr(field, "xtype"), SZ_NAME-1);
        vut_setString (elem->datatype, vot_getAttr(field, "datatype"), SZ_NAME-1);
        vut_setString (elem->arraysize, vot_getAttr(field, "arraysize"), SZ_NAME-1);
        vut_setString (elem->width, vot_getAttr(field, "width"), SZ_NAME-1);
        vut_setString (elem->precision, vot_getAttr(field, "precision"), SZ_NAME-1);

        /* Insert into list and hash keys. */
        vll_append (qr->fields, (void *)elem, &elem->index);
        vht_insertKey (qr->hash_id, elem->id, (void *)elem);
        vht_insertKey (qr->hash_name, elem->name, (void *)elem);
        vht_insertKey (qr->hash_utype, elem->utype, (void *)elem);
        vht_insertKey (qr->hash_ucd, elem->ucd, (void *)elem);
    }

    /* Allocate character data storage. */
    if ((qr->cmem = vut_cmemInit()) == NULL)
	return (dal_error (dal, DAL_CMEMINIT));

    /* Allocate row storage.  To keep things simple we have a max number of
     * properties or columns per row, but storage is only allocated for the
     * actual number of rows, and storage in dalRecored only stores pointers
     * to the values, encoded as strings.  Bulk character data is stored
     * in CMEM.
     */
    qr->rows = (dalRecord_t *) calloc (nrows, sizeof (dalRecord_t));

    /* Get table data. */
    int recnum = 0;
    for (tr=vot_getTR(tdata);  tr;  tr=vot_getNext(tr), recnum++) {
	dalRecord_t *rec = &qr->rows[recnum];
	rec->qr = qr;
	rec->recnum = recnum;
        for (td=vot_getTD(tr), colnum=0;  td;  td=vot_getNext(td))
	    rec->cdata[colnum++] = vut_cmemCopy (qr->cmem, vot_getValue(td));
    }

    /* Close the VOTable object; it is no longer needed. */
    vot_closeVOTABLE (vot);

    /* Return an opaque handle for the QR. */
    return (qr->qr_h = svr_newHandle (dal->context, qr));
}


/**
 * dal_closeQueryResponse -- Close a QueryResponse object and free resources.
 *
 * @brief   Close a QueryResponse object and free all resources.
 * @fn	    void dal_closeQueryResponse (qr)
 *
 * @param   qr			QueryResponse object handle
 * @returns			Nothing
 *
 * Returns the handle for an already prepared QueryReponse object, or zero if
 * the query has not yet be executed.
 */
void
dal_closeQueryResponse (QResponse qr_h)
{
    /* Recover the QueryResponse object. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return;

    /* Free character data storage. */
    vut_cmemDestroy (qr->cmem);

    /* Free row storage. */
    free ((void *)qr->rows);

    /* Free all hash tables. */
    vht_destroy (qr->hash_ucd);
    vht_destroy (qr->hash_utype);
    vht_destroy (qr->hash_name);
    vht_destroy (qr->hash_id);
    vht_destroy (qr->hash_prop);

    /* Free the list of properties and associated nodes. */
    vll_destroy (qr->properties, free);

    /* Free the FIELD and INFO lists and associated nodes. */
    vll_destroy (qr->fields, free);
    vll_destroy (qr->infos, free);

    /* Free the votable text. */
    if (qr->votable)
	free ((void *)qr->votable);
    if (qr->delimited)
	free ((void *)qr->delimited);

    /* Free the QueryResponse handle and object. */
    svr_freeHandle (qr->qr_h);
    free ((void *)qr);
}


/**
 * dal_executeVOTable -- Execute a query and return the VOTable text.
 *
 * @brief   Execute a query and return the VOTable text
 * @fn	    votable = dal_executeVOTable (query)
 *
 * @param   query		Query object handle
 * @returns			A pointer to the cached VOTable text
 *
 * If the query has already executed a pointer to the cached votable text is
 * merely returned, otherwise the query is executed and a pointer to the cached
 * VOTable text is returned.  The referenced text buffer is managed by the
 * Query object hence should not be freed by the client.
 */
char *
dal_executeVOTable (Query query_h)
{
    /* Recover the Query object from its Handle. */
    dalQuery_t *query = svr_H2P (query_h);
    dalConn_t *dal = query->dal;
    dalQR_t *qr = svr_H2P (query->qr_h);
    int      len;


    /* If the query has already executed merely return the votable text */
    if (qr != NULL && qr->votable != NULL)
	return (qr->votable);

    /* Allocate the QueryResponse descriptor. */
    if ((qr = (dalQR_t *) calloc ((size_t)1, sizeof(dalQR_t))) == NULL)
	return (dal_nError (dal, DAL_MEMALLOCFAIL));
    else
	query->qr_h = svr_newHandle (dal->context, dal);

    /* Get the Query URL. */
    char *queryURL = dal_getQueryURL (query_h);
    if (queryURL == NULL) {
	free ((void *)qr);
	return (dal_nError (dal, DAL_FORMQUERYURL));
    }

    /* Execute the query and retrieve the query response VOTable. */
    dal_clearError (dal->dal_h);
    qr->votable = vut_getURL (dal, queryURL);

    if (qr->votable == NULL) {
	free ((void *)qr);

	/* Pass back any lower-level error. */
	int errcode = dal_getError (dal->dal_h);
	return (dal_nError (dal,
	    (errcode > 0) ? errcode : DAL_EXECUTEQUERY));
    } else {
        len = strlen (qr->votable);
	if (qr->votable[len-1] != '\n')		/* ensure a newline	*/
	    strcat (qr->votable, "\n");
    }

    return (qr->votable);
}


/**
 * dal_executeCSV -- Execute a query and return the result as CSV text.
 *
 * @brief   Execute a query and return the result as CSV text
 * @fn	    csv = dal_executeCSV (query)
 *
 * @param   query		Query object handle
 * @returns			A pointer to the cached VOTable text
 *
 * If the query has already executed a pointer to the cached votable text is
 * merely returned, otherwise the query is executed and a pointer to the cached
 * VOTable text is returned.  The referenced text buffer is managed by the
 * Query object hence should not be freed by the client.
 */
char *
dal_executeCSV (Query query_h)
{
    return ( dal_executeDelimited (query_h, ',') );
}


/**
 * dal_executeTSV -- Execute a query and return the result as TSV text.
 *
 * @brief   Execute a query and return the result as TSV text
 * @fn	    csv = dal_executeTSV (query)
 *
 * @param   query		Query object handle
 * @returns			A pointer to the cached VOTable text
 *
 * If the query has already executed a pointer to the cached votable text is
 * merely returned, otherwise the query is executed and a pointer to the cached
 * VOTable text is returned.  The referenced text buffer is managed by the
 * Query object hence should not be freed by the client.
 */
char *
dal_executeTSV (Query query_h)
{
    return ( dal_executeDelimited (query_h, '\t') );
}


/**
 * dal_executeASCII -- Execute a query and return the result as ASCII text.
 *
 * @brief   Execute a query and return the result as ASCII text
 * @fn	    csv = dal_executeASCII (query)
 *
 * @param   query		Query object handle
 * @returns			A pointer to the cached VOTable text
 *
 * If the query has already executed a pointer to the cached votable text is
 * merely returned, otherwise the query is executed and a pointer to the cached
 * VOTable text is returned.  The referenced text buffer is managed by the
 * Query object hence should not be freed by the client.
 */
char *
dal_executeASCII (Query query_h)
{
    return ( dal_executeDelimited (query_h, ' ') );
}


/**
 * dal_executeDelimited -- Execute query and return result as delimited text.
 *
 * @brief   Execute a query and return the result as delimited text
 * @fn	    csv = dal_executeDelimited (query, delim)
 *
 * @param   query		Query object handle
 * @param   delim		Character delimiter
 * @returns			A pointer to the cached delimited text
 *
 * If the query has already executed a pointer to the cached votable text is
 * merely returned, otherwise the query is executed and a pointer to the cached
 * VOTable text is returned.  The referenced text buffer is managed by the
 * Query object hence should not be freed by the client.
 */
char *
dal_executeDelimited (Query query_h, char delim)
{
    /* Execute the query to generate the VOTable response. */
    char *votable = dal_executeVOTable (query_h);

    /* Recover the Query and Response objects from their handles. */
    dalQuery_t *query = svr_H2P (query_h);
    dalQR_t *qr = svr_H2P (query->qr_h);

    /* If the query has already executed merely return the votable text */
    qr->votable = votable;
    if (qr != NULL && qr->votable != NULL) {

        /* Convert the VOTable to the delimited result table requested. We
	 * also free up the VOTable text at this point.
         */
        qr->delimited = vut_votableToDelimited (votable, delim);
	if (qr->delimited) {
	    if (qr->votable)
		free ( (char *) qr->votable);
	    qr->votable = NULL;
	    return (qr->delimited);
	}
    }

    return (NULL);
}
