/**
 * DALSIAP -- Simple Image Access Protocol-specific code.
 *
 * Most of the DALClient code is generic and common to all service types.
 * The files dalSCSP (cone simple search), dalSIAP (simple image access),
 * and so forth contain the protocol-specific code for each class of
 * service.  In particular this is where we deal with multiple versions
 * of the VO prtotocols, and shielding the client from the details of
 * the VO protocols as they evolve.
 *
 * The Simple Image Access Protocol (SIAP) is used to discover, download,
 * and access multi-dimensional astronomical images.
 *
 * ------ Query Parameters ---------
 *
 *  The following standard query parameters are recognized by the SIAP driver
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
 *  			specific protocol requires, in this case pos,size.
 *
 * Not all versions of the protocol implement all these parameters.
 * Parameters not implemented by the target service version are ignored.
 *
 * ------ Object Properties ---------
 *
 * Each row of the SIAP query response describes a single available image.
 * The following Image dataset properties describe each dataset.
 *
 * 	protocol	Protocol implemented ("SIA")
 * 	version		Protocol version (e.g., "1.0" or "2.0")
 *
 *      title		Image title
 *      ra		Right ascension of image center (ICRS, decimal degrees)
 *      dec		Declination of image center (ICRS, decimal degrees)
 *      naxes		Number of image axes (dimensionality)
 *      naxis		Length of each axis (blank-delimited array)
 *      wcsaxes		WCS axes
 *      scale		Image scale (degrees per pixel)
 *      bandname        Bandpass or filter name
 *      bandlo          Numerical bandpass lower limit (meters)
 *      bandhi          Numerical bandpass upper limit (meters)
 *      format		Image format (MIME type)
 *      acref		Access reference (URL)
 *      estsize		Estimated dataset size (kB)
 *
 * Other properties may be added but this is a good start...
 *
 * At the lower level, SIAPV1 uses UCDs to tag object attributes, whereas
 * SIAPV2 usesa UTYPEs.  Refer to the IVOA specifications for details.
 *
 * ------ SIAP routines (internal) ---------
 *
 *       str = siap_getQueryURL (query)
 *     void siap_initProperties (qr)
 *
 * The routines in this file are internal to the DALClient code and are not
 * intended to be called directly by applications.
 *
 * @file	dalSIAP.c
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
 * siap_getQueryURL -- Compose the query URL from the input parameter set.
 *
 * @brief   Compose the query URL from the input parameter set.
 * @fn	    str = siap_getQueryURL (query)
 *
 * @param   query		Query object
 * @returns			The query URL as a string
 *
 * The input parameter set must be composed before building the query URL.
 * The baseURL for the service, service version, etc. was set when the
 * service connection context was defined.
 *
 * At this point this code works for both the QueryData and AccessData
 * requests without change (REQUEST is set upstream when the Query object is
 * created).  AccessData uses most of the same parameters as QueryData, and
 * any AccessData-specific parameters can be treated as extension params.
 */
char *
siap_getQueryURL (dalQuery_t *query)
{
    dalConn_t *dal = query->dal;
    vocList_t *params = query->params;
    dalParam_t *param;

    /* Get the SIAP version. */
    int version = (dal->version[0] == '1') ? 1 : 2;

    /* SIAPV2 requires that the sync or async resource be specified. */
    char *resource = (version >= 2) ? "sync" : NULL;

    /* Compose the query URL */
    void *urlBuilder = vut_urlOpen (dal->baseurl, resource);

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
 * siap_initProperties -- Compute the SIAP properties for a QueryResponse.
 *
 * @brief   Compute the SIAP properties for a QueryResponse.
 * @fn	    void siap_initProperties (qr)
 *
 * @param   qr			QueryResponse object
 * @returns			Nothing, other than updates to the QR
 *
 * The contents of the query response object are used to compute the object
 * properties, which are then added to the query response object.
 */
void
siap_initProperties (dalQR_t *qr)
{
    dalQuery_t *query = qr->query;
    dalConn_t *dal = query->dal;
    int i;

    /* Get the SIAP version. */
    int version = (dal->version[0] == '1') ? 1 : 2;

    /* Add the SIAP properties.   We can add more properties here, but
     * this is enough to get started.
     */
    vut_addProperty (qr, "protocol", "sia", DAL_PARAM);
    vut_addProperty (qr, "version", dal->version, DAL_PARAM);

    vut_addProperty (qr, "title", NULL, DAL_FIELD);
    vut_addProperty (qr, "ra", NULL, DAL_FIELD);
    vut_addProperty (qr, "dec", NULL, DAL_FIELD);
    vut_addProperty (qr, "naxes", NULL, DAL_FIELD);
    vut_addProperty (qr, "naxis", NULL, DAL_FIELD);
    vut_addProperty (qr, "wcsaxes", NULL, DAL_FIELD);
    vut_addProperty (qr, "scale", NULL, DAL_FIELD);
    vut_addProperty (qr, "bandname", NULL, DAL_FIELD);
    vut_addProperty (qr, "bandlo", NULL, DAL_FIELD);
    vut_addProperty (qr, "bandhi", NULL, DAL_FIELD);
    vut_addProperty (qr, "format", NULL, DAL_FIELD);
    vut_addProperty (qr, "acref", NULL, DAL_FIELD);
    vut_addProperty (qr, "estsize", NULL, DAL_FIELD);

    /* Get the required column numbers for the current data table.
     * This is completely different for SIA V1 and V2 due to the introduction
     * of UTYPEs following SIAV1 (motivated by the VOX: UCDs!).
     */
    int title_column, ra_column, dec_column;
    int naxes_column, naxis_column, wcsaxes_column, scale_column;
    int format_column, acref_column, estsize_column;
    int bandname_column, bandlo_column, bandhi_column;

    if (version == 1) {
	title_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_Title", DAL_UCD);
	ra_column = dal_getFieldIndex (query->qr_h,
	    "POS_EQ_RA_MAIN", DAL_UCD);
	dec_column = dal_getFieldIndex (query->qr_h,
	    "POS_EQ_DEC_MAIN", DAL_UCD);
	naxes_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_Naxes", DAL_UCD);
	naxis_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_Naxis", DAL_UCD);
	wcsaxes_column = dal_getFieldIndex (query->qr_h,
	    "Image.WCSAxes", DAL_UTYPE);    /* will be missing in V1 */
	scale_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_Scale", DAL_UCD);
        bandname_column = dal_getFieldIndex (query->qr_h,
            "VOX:BandPass_ID", DAL_UCD);
        bandlo_column = dal_getFieldIndex (query->qr_h,
            "VOX:BandPass_LoLImit", DAL_UCD);
        bandhi_column = dal_getFieldIndex (query->qr_h,
            "VOX:BandPass_HiLImit", DAL_UCD);
	format_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_Format", DAL_UCD);
	acref_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_AccessReference", DAL_UCD);
	estsize_column = dal_getFieldIndex (query->qr_h,
	    "VOX:Image_FileSize", DAL_UCD);
    } else {
	title_column = dal_getFieldIndex (query->qr_h,
	    "im:DataID.Title", DAL_UTYPE);
	ra_column = dal_getFieldIndex (query->qr_h,
	    "im:Char.SpatialAxis.Coverage.Location.Coord.Position2D.Value2.C1",
	    DAL_UTYPE);
	dec_column = dal_getFieldIndex (query->qr_h,
	    "im:Char.SpatialAxis.Coverage.Location.Coord.Position2D.Value2.C2",
	    DAL_UTYPE);
	naxes_column = dal_getFieldIndex (query->qr_h,
	    "im:Image.Naxes", DAL_UTYPE);
	naxis_column = dal_getFieldIndex (query->qr_h,
	    "im:Image.Naxis", DAL_UTYPE);
	wcsaxes_column = dal_getFieldIndex (query->qr_h,
	    "im:Image.WCSAxes", DAL_UTYPE);
	scale_column = dal_getFieldIndex (query->qr_h,
	    "im:Char.SpatialAxis.Sampling.SampleExtent", DAL_UTYPE);
        bandname_column = dal_getFieldIndex (query->qr_h,
            "im:Provenance.ObsConfig.Bandpass", DAL_UTYPE);
        bandlo_column = dal_getFieldIndex (query->qr_h,
            "im:Char.SpectralAxis.Coverage.Bounds.Limits.LoLimit", DAL_UTYPE);
        bandhi_column = dal_getFieldIndex (query->qr_h,
            "im:Char.SpectralAxis.Coverage.Bounds.Limits.HiLimit", DAL_UTYPE);
	format_column = dal_getFieldIndex (query->qr_h,
	    "im:Access.Format", DAL_UTYPE);
	acref_column = dal_getFieldIndex (query->qr_h,
	    "im:Access.Reference", DAL_UTYPE);
	estsize_column = dal_getFieldIndex (query->qr_h,
	    "im:Access.Size", DAL_UTYPE);
    }

    /* Set the values of the per-record properties.  If any of the required
     * Fields are not found, this sets the associated property values to NULL.
     */
    for (i=0;  i < qr->nrows;  i++) {
	QRecord rec_h = dal_getRecord (query->qr_h, i);
        char bandName[SZ_NAME];

	vut_setProperty (rec_h,
	    "title", dal_getStringField(rec_h,title_column));
	vut_setProperty (rec_h,
	    "ra", dal_getStringField(rec_h,ra_column));
	vut_setProperty (rec_h,
	    "dec", dal_getStringField(rec_h,dec_column));
	vut_setProperty (rec_h,
	    "naxes", dal_getStringField(rec_h,naxes_column));
	vut_setProperty (rec_h,
	    "naxis", dal_getStringField(rec_h,naxis_column));
	vut_setProperty (rec_h,
	    "wcsaxes", dal_getStringField(rec_h,wcsaxes_column));
	vut_setProperty (rec_h,
	    "scale", dal_getStringField(rec_h,scale_column));
	vut_setProperty (rec_h,
	    "format", dal_getStringField(rec_h,format_column));
	vut_setProperty (rec_h,
	    "acref", dal_getStringField(rec_h,acref_column));
	vut_setProperty (rec_h,
	    "estsize", dal_getStringField(rec_h,estsize_column));

        /* Try to compute a numerical bandname value if no band name given.  */
        char *bandname = dal_getStringField (rec_h, bandname_column);
        if (!bandname) {
            dal_clearError (dal->dal_h);
            double lo = dal_getFloatField(rec_h, bandlo_column);
            double hi = dal_getFloatField(rec_h, bandhi_column);
            if (dal_getError(dal->dal_h) != DAL_ERROR) {
                sprintf (bandName, "%10.3g", (hi - lo) / 2.0 + lo);
                bandname = bandName;
            }
        }

        /* Set the bandpass properties. */
        vut_setProperty (rec_h, "bandname", bandname);
        vut_setProperty (rec_h,
            "bandlo", dal_getStringField(rec_h,bandlo_column));
        vut_setProperty (rec_h,
            "bandhi", dal_getStringField(rec_h,bandhi_column));

	dal_releaseRecord (rec_h);
    }
}

