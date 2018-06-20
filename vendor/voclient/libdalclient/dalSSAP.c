/**
 * DALSSAP -- Simple Spectral Access Protocol-specific code.
 *
 * Most of the DALClient code is generic and common to all service types.
 * The files dalSCSP (cone simple search), dalSIAP (simple image access),
 * and so forth contain the protocol-specific code for each class of
 * service.  In particular this is where we deal with multiple versions
 * of the VO prtotocols, and shielding the client from the details of
 * the VO protocols as they evolve.
 *
 * The Simple Spectral Access Protocol (SSAP) is used to discover, download,
 * and access 1-dimensional spectra.
 *
 * ------ Query Parameters ---------
 *
 *  The following standard query parameters are recognized by the SSAP driver
 *  and may be used for custom protocol-specific translation.  All other
 *  query parameters are merely passed through to the remote service.
 *
 *  	POS		Encoded RA,DEC position encoded as a list
 *  	SIZE		Region size (one or two values).
 *  	BAND		Spectral band
 *  	TIME		Time band
 *  	POL		Polarisation states
 *  	FORMAT		Desired image formats
 *
 *  	POS-object	Not yet implemented.  This will be a generic spatial
 *  			region object that gets translated into whatever the
 *  			specific protocol requires, in this case ra,dec,sr.
 *
 * Not all versions of the protocol implement all these parameters.
 * Parameters not implemented by the target service version are ignored.
 *
 * ------ Object Properties ---------
 *
 * Each row of the SSAP query response describes a single available image.
 * The following Image dataset properties describe each dataset.
 *
 * 	protocol	Protocol implemented ("SSA")
 * 	version		Protocol version (e.g., "1.0" or "2.0")
 *
 *      title		Image title
 *      ra		Right ascension of image center (ICRS, decimal degrees)
 *      dec		Declination of image center (ICRS, decimal degrees)
 *      format		Image format (MIME type)
 *      acref		Access reference (URL)
 *	estsize		Estimated dataset size (kB)
 *
 * Other properties may be added but this is a good start...
 *
 * ------ SSAP routines (internal) ---------
 *
 *       str = ssap_getQueryURL (query)
 *     void ssap_initProperties (qr)
 *
 * The routines in this file are internal to the DALClient code and are not
 * intended to be called directly by applications.
 *
 * @file	dalSSAP.c
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
 * ssap_getQueryURL -- Compose the query URL from the input parameter set.
 *
 * @brief   Compose the query URL from the input parameter set.
 * @fn	    str = ssap_getQueryURL (query)
 *
 * @param   query		Query object
 * @returns			The query URL as a string
 *
 * The input parameter set must be composed before building the query URL.
 * The baseURL for the service, service version, etc. was set when the
 * service connection context was defined.
 */
char *
ssap_getQueryURL (dalQuery_t *query)
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
 * ssap_initProperties -- Compute the SSAP properties for a QueryResponse.
 *
 * @brief   Compute the SSAP properties for a QueryResponse.
 * @fn	    void ssap_initProperties (qr)
 *
 * @param   qr			QueryResponse object
 * @returns			Nothing, other than updates to the QR
 *
 * The contents of the query response object are used to compute the object
 * properties, which are then added to the query response object.
 */
void
ssap_initProperties (dalQR_t *qr)
{
    dalQuery_t *query = qr->query;
    dalConn_t *dal = query->dal;

    /* Get the SSAP version.
     * SSA has versions 1.0 and 1.1; ignore for now.
    int version = (dal->version[0] == '1') ? 1 : 2;
     */

    /* Add the SSAP properties.   We can add more properties here, but
     * this is enough to get started.
     */
    vut_addProperty (qr, "protocol", "ssa", DAL_PARAM);
    vut_addProperty (qr, "version", dal->version, DAL_PARAM);

    vut_addProperty (qr, "title", NULL, DAL_FIELD);
    vut_addProperty (qr, "ra", NULL, DAL_FIELD);
    vut_addProperty (qr, "dec", NULL, DAL_FIELD);
    vut_addProperty (qr, "format", NULL, DAL_FIELD);
    vut_addProperty (qr, "acref", NULL, DAL_FIELD);
    vut_addProperty (qr, "estsize", NULL, DAL_FIELD);

    /* Get the required column numbers for the current data table.
     */
    int title_column, ra_column, dec_column;
    int format_column, acref_column, estsize_column;
    int i;

    title_column = dal_getFieldIndex (query->qr_h,
	"DataID.Title", DAL_UTYPE);
    ra_column = dal_getFieldIndex (query->qr_h,
	"Char.SpatialAxis.Coverage.Location.Coord.Position2D.Value2.C1",
	DAL_UTYPE);
    dec_column = dal_getFieldIndex (query->qr_h,
	"Char.SpatialAxis.Coverage.Location.Coord.Position2D.Value2.C2",
	DAL_UTYPE);
    format_column = dal_getFieldIndex (query->qr_h,
	"Access.Format", DAL_UTYPE);
    acref_column = dal_getFieldIndex (query->qr_h,
	"Access.Reference", DAL_UTYPE);
    estsize_column = dal_getFieldIndex (query->qr_h, "Access.Size", DAL_UTYPE);

    /* Set the values of the record properties.  If any of the required
     * Fields are not found, this sets the associated property values to NULL.
     */
    for (i=0;  i < qr->nrows;  i++) {
	QRecord rec_h = dal_getRecord (query->qr_h, i);

	vut_setProperty (rec_h,
	    "title", dal_getStringField(rec_h,title_column));
	vut_setProperty (rec_h,
	    "ra", dal_getStringField(rec_h,ra_column));
	vut_setProperty (rec_h,
	    "dec", dal_getStringField(rec_h,dec_column));
	vut_setProperty (rec_h,
	    "format", dal_getStringField(rec_h,format_column));
	vut_setProperty (rec_h,
	    "acref", dal_getStringField(rec_h,acref_column));
	vut_setProperty (rec_h,
	    "estsize", dal_getStringField(rec_h,estsize_column));

	dal_releaseRecord (rec_h);
    }
}

