/**
 * DALPOS -- Spatial region object
 *
 * This class provides a general mechanism for defining a spatial region
 * or list of regions.  The various constructors and editors are used to
 * create or edit the POS object, which is then input to the service-specific
 * query code to define the spatial region for the query.  The service 
 * code then uses the POS object to construct whatever parameter input is
 * required for a given service class.
 *
 *   ---- Query Parameters ---------
 *
 *         query = dal_getQuery (dal)
 *		 dal_closeQuery (query)		# free resources
 *
 *     query = dal_getConeQuery (dal, ra, dec, sr)
 *     query = dal_getSiapQuery (dal, ra, dec, ra_size, dec_size, format)
 *     query = dal_getSsapQuery (dal, ra, dec, size, band, time, format)
 *
 *     query = dal_genConeQuery (dal, pos)
 *     query = dal_genSiapQuery (dal, pos, band, time, pol, format, mode)
 *     query = dal_genSsapQuery (dal, pos, band, time, pol, format, mode)
 *
 *	    pos = dal_getObjPos (objname|NULL)
 *	       pos = dal_setPos (c1, c2, size, frame|NULL)
 *	      pos = dal_setPos2 (c1, c2, size1, size2, frame|NULL)
 *	           <additional-constructors>
 *	           <additional-setters>
 *	       dal_posGetCoords (pos, c1, c2, &frame)
 *		   dal_closePos (pos)
 *
 *
 * Status: Not yet implemented (except for stubbing out a simple constructor).
 * We are using the lower level query constructors for the moment.
 *
 * @file	dalPos.c
 * @author	Doug Tody
 * @version	January 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <float.h>

#define _DALCLIENT_LIB_
#include "dalclient.h"


/**
 * dal_setPos -- Simple POS object constructor.
 *
 * @brief   Simple POS object constructor.
 * @fn      pos = dal_setPos (dal_h, c1, c2, size, frame)
 *
 * @param   dal_h               DAL object handle
 * @param   c1                  First coordinate value
 * @param   c2                  Second coordinate value
 * @param   size                Size of POS object
 * @param   frame               Coordinate frame (nut currently used)
 * @returns                     Handle to POS object
 *
 */
POS
dal_setPos (DAL dal_h, double c1, double c2, double size, char *frame)
{
    /* Recover the DAL object from its Handle. */
    dalConn_t *dal = svr_H2P (dal_h);

    /* Allocate the POS instance. */
    dalPos_t *pos = (dalPos_t *) malloc (sizeof (dalPos_t));

    /* Set POS data. */
    pos->c1 = c1;
    pos->c2 = c2;
    pos->size1 = pos->size2 = size;
    pos->objname[0] = '\0';
    if (frame)
	strncpy (pos->frame, frame, SZ_NAME-1);
    else
	pos->frame[0] = '\0';

    /* Return an opaque handle for the POS object. */
    POS pos_h;
    if ((pos_h = svr_newHandle (dal->context, pos)) == 0)
    	return (dal_error (dal, DAL_BADHANDLE));
    else
	return (pos_h);
}

