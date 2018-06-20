/**
 * DALSCSP -- Simple Cone Search protocol-specific code.
 *
 * Most of the DALClient code is generic and common to all service types.
 * The files dalSCSP (cone simple search), dalSIAP (simple image access),
 * and so forth contain the protocol-specific code for each class of
 * service.  In particular this is where we deal with multiple versions
 * of the VO prtotocols, and shielding the client from the details of
 * the VO protocols as they evolve.
 *
 * The Simple Cone Search protocol (SCSP) is used to query arbitrary
 * astronomical catalogs for all records for which the position described
 * by the object in a record (table row) falls within the circular
 * region centered at RA,DEC with radius SR.
 *
 * ------ Query Parameters ---------
 *
 *  The following standard query parameters are recognized by the SCSP driver
 *  and may be used for custom protocol-specific translation.  All other
 *  query parameters are merely passed through to the remote service.
 *
 *  	RA		Right ascension (ICRS, decimal degrees)
 *  	DEC		Declination (ICRS, decimal degrees)
 *  	SR		Search radius (decimal degrees)
 *
 *  	POS-object	Not yet implemented.  This will be a generic spatial
 *  			region object that gets translated into whatever the
 *  			specific protocol requires, in this case ra,dec,sr.
 *
 * ------ Object Properties ---------
 *
 *  Cone search is used for catalog queries hence there is no real object
 *  data model for which to define properties, however the following
 *  properties are defined to describe table rows, and the protocol itself:
 *
 * 	protocol	Protocol implemented ("SCS")
 * 	version		Protocol version ("1.0")
 *
 *  	id		Unique key for table row / object (ID_MAIN)
 *  	ra		Right ascension
 *  	dec		Declination
 *
 * ------ SCSP routines (internal) ---------
 *
 *       str = scsp_getQueryURL (query)
 *     void scsp_initProperties (qr)
 *
 * The routines in this file are internal to the DALClient code and are not
 * intended to be called directly by applications.
 *
 * @file	dalSCSP.c
 * @author	Doug Tody
 * @version	January 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <float.h>

#define _DALCLIENT_LIB_
#include "dalclient.h"


/*
 * Internal routines
 * -------------------------------
 */


/**
 * scsp_getQueryURL -- Compose the query URL from the input parameter set.
 *
 * @brief   Compose the query URL from the input parameter set.
 * @fn	    str = scsp_getQueryURL (query)
 *
 * @param   query		Query object
 * @returns			The query URL as a string
 *
 * The input parameter set must be composed before building the query URL.
 * The baseURL for the service, service version, etc. was set when the
 * service connection context was defined.
 */
char *
scsp_getQueryURL (dalQuery_t *query)
{
    dalConn_t *dal = query->dal;
    vocList_t *params = query->params;
    dalParam_t *param;

    /* Compose the query URL */
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
 * scsp_initProperties -- Compute the SCSP properties for a QueryResponse.
 *
 * @brief   Compute the SCSP properties for a QueryResponse.
 * @fn	    void scsp_initProperties (qr)
 *
 * @param   qr			QueryResponse object
 * @returns			Nothing, other than updates to the QR
 *
 * The contents of the query response object are used to compute the object
 * properties, which are then added to the query response object.
 */
void
scsp_initProperties (dalQR_t *qr)
{
    dalQuery_t *query = qr->query;
    int i;

    /* Add the SCSP properties. */
    vut_addProperty (qr, "protocol", "scs", DAL_PARAM);
    vut_addProperty (qr, "version", "1.0", DAL_PARAM);

    vut_addProperty (qr, "id", NULL, DAL_FIELD);
    vut_addProperty (qr, "ra", NULL, DAL_FIELD);
    vut_addProperty (qr, "dec", NULL, DAL_FIELD);

    /* Get the required column numbers for the current data table. */
    int id_column = dal_getFieldIndex (query->qr_h, "ID_MAIN", DAL_UCD);
    int ra_column = dal_getFieldIndex (query->qr_h, "POS_EQ_RA_MAIN", DAL_UCD);
    int dec_column = dal_getFieldIndex (query->qr_h, "POS_EQ_DEC_MAIN", DAL_UCD);

    /* Set the values of the record properties.  If any of the required
     * Fields are not found, this sets the associated property values to NULL.
     */
    for (i=0;  i < qr->nrows;  i++) {
	QRecord rec_h = dal_getRecord (query->qr_h, i);

	vut_setProperty (rec_h, "id", dal_getStringField(rec_h,id_column));
	vut_setProperty (rec_h, "ra", dal_getStringField(rec_h,ra_column));
	vut_setProperty (rec_h, "dec", dal_getStringField(rec_h,dec_column));

	dal_releaseRecord (rec_h);
    }
}
