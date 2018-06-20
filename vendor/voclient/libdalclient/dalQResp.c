/**
 * DALQRESPONSE -- Query Response Object for a DAL (or similar) query.
 *
 * The QueryResponse class is used to walk through or randomly access the
 * processed response from a DAL (or similar VOTable-based) query.
 *
 * ---- Query Execution ---------
 *
 *        qr = dal_executeQuery (query)
 *   qr = dal_initQueryResponse (dal, votable, resource, table)
 *      	 (etc.)
 *
 * ---- Query Response ---------
 *
 *     int = dal_getRecordCount (qr)
 *     int = dal_getColumnCount (qr)
 *       int = dal_getInfoCount (qr)
 *      int = dal_getFieldCount (qr)
 *       int = dal_getPropCount (qr)
 *        str = dal_getPropName (qr, index)
 *        str = dal_getInfoAttr (qr, index, attr_code)
 *       str = dal_getFieldAttr (qr, index, attr_code)
 *	int = dal_getFieldIndex (qr, key, attr_code)
 *
 *          rec = dal_getRecord (qr, recnum)
 *            dal_releaseRecord (rec)
 *     int = dal_getIntProperty (rec, property)
 *  dval = dal_getFloatProperty (rec, property)
 *  str = dal_getStringProperty (rec, property)
 *
 *	  int = dal_getIntField (rec, index)
 *     dval = dal_getFloatField (rec, index)
 *     str = dal_getStringField (rec, index)
 *
 *	stat = dal_getIntColumn (qr, index, iptr)
 *    stat = dal_getFloatColumn (qr, index, dptr)
 *   stat = dal_getStringColumn (qr, index, sptr)
 *
 *        stat = dal_getDataset (rec, acref, fname) 
 *
 * The tables to be processed are often modest sized, but can potentially
 * be very large, with tens of thousands of rows and hundreds of columns.
 * All processing of the input VOTable is sequential, with one pass to parse
 * the XML and a second to process the parsed table into a QueryResponse
 * object.  The QueryResponse object is stored in memory and is randomly
 * accessible, fully indexed and hashed.
 *
 * While the intended use of this class is mainly to process the VOTable
 * results from VO DAL or Registry queries, it may be used to walk through
 * any VOTable, accessing table fields by index or by tag (ID, NAME, UTYPE,
 * UCD, etc.).  For example, this could be used to access data VOTables
 * such as from a TAP or Cone Search query, or in some cases, VOTable-encoded
 * data objects such as a Spectrum or ImageDM Data-element descriptor.
 *
 * This interface mostly treats VOTable FIELDs and PARAMs as the same type of
 * object, although the specific object type can be queried at a lower level.
 * For applications such as querying the attributes of a dataset described in
 * a table row they are equivalent.  Higher level object properties are also
 * supported; these are abstract properties of the data model for the object
 * stored in the table, and have the important characteristic of being
 * represented consistently regardless of the representation (in terms of ID,
 * NAME, UTYPE, UCD, etc.) in the lower level VOTable and service version.
 *
 * There is no explicit support here for VOTable mechanisms such as GROUP,
 * hence for example UTYPES are assumed to be unique in a table (this is the
 * case with current VO interfaces).
 *
 * @file	dalQResp.c
 * @author	Doug Tody
 * @version	January 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <float.h>
#include <sys/file.h>
#include <curl/curl.h>

#define _DALCLIENT_LIB_
#include "dalclient.h"


/*
 * Query Response Interaction
 * -------------------------------
 */


/**
 * dal_getRecordCount -- Get the number of records (rows) in a QueryResponse.
 *
 * @brief   Get the number of records (rows) in a QueryResponse.
 * @fn	    int = dal_getRecordCount (qr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @returns			Number of records in QueryResponse
 */
int
dal_getRecordCount (QResponse qr_h)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    return (qr->nrows);
}


/**
 * dal_getColumnCount -- Get the number of table columns in a QueryResponse.
 *
 * @brief   Get the number of table columns in a QueryResponse.
 * @fn	    int = dal_getColumnCount (qr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @returns			Number of table columns in QueryResponse
 *
 * In the case of a VOTable, the number of table columns is the number of
 * FIELD elements (physical table columns in the table; PARAMs are not
 * counted.  This is not the same as the "Field" object in the QueryResponse,
 * which treats the VOTable PARAM and FIELD equally.
 */
int
dal_getColumnCount (QResponse qr_h)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    return (qr->ncols);
}


/**
 * dal_getInfoCount -- Get the number of INFO elements in a QueryResponse.
 *
 * @brief   Get the number of INFO elements in a QueryResponse.
 * @fn	    int = dal_getInfoCount (qr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @returns			Number of INFO elements in the QueryResponse
 */
int
dal_getInfoCount (QResponse qr_h)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    return (vll_count (qr->infos));
}


/**
 * dal_getFieldCount -- Get number of FIELD/PARAM elements in a QueryResponse.
 *
 * @brief   Get the number of FIELD/PARAM elements in a QueryResponse
 * @fn	    int = dal_getFieldCount (qr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @returns			Num. of FIELD/PARAM elements in QueryResponse
 */
int
dal_getFieldCount (QResponse qr_h)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    return (vll_count (qr->fields));
}


/**
 * dal_getPropCount -- Get the number of properties in a QueryResponse.
 *
 * @brief   Get the number of properties n a QueryResponse
 * @fn	    int = dal_getPropCount (qr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @returns			Number of properties in the QueryResponse
 */
int
dal_getPropCount (QResponse qr_h)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    return (vll_count (qr->properties));
}


/**
 * dal_getPropName -- Get the name of a property in a QueryResponse.
 *
 * @brief   Get the name of a property n a QueryResponse
 * @fn	    str = dal_getPropName (qr, index)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @param   index		Index of property (0-indexed)
 * @returns			The property name
 */
char *
dal_getPropName (QResponse qr_h, int index)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (NULL);

    dalProp_t *prop = (dalProp_t *) vll_seek (qr->properties, index, SEEK_SET);
    if (!prop)
	return (dal_nError (qr->query->dal, DAL_PROPNOTFOUND));

    return (prop->name);
}


/**
 * dal_getInfoAttr -- Get the specified attribute of an INFO instance.
 *
 * @brief   Get the specified attribute of an INFO instance.
 * @fn	    str = dal_getInfoAttribute (qr, index, attr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @param   index		Index of INFO element
 * @param   attr		Code for INFO attribute to be returned
 * @returns			The value of the given attribute as a string
 */
char *
dal_getInfoAttr (QResponse qr_h, int index, int attr)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (NULL);

    /* Recover DAL connection context. */
    dalConn_t *dal = qr->query->dal;

    /* Get the INFO (stored in a Field struct). */
    dalField_t *elem = (dalField_t *) vll_seek (qr->infos, index, SEEK_SET);
    if (elem == NULL)
	return (dal_nError (dal, DAL_QRINFOERR));

    /* Return the requested attribute. */
    switch (attr) {
    case DAL_ID:
	return (elem->id);
	break;
    case DAL_NAME:
	return (elem->name);
	break;
    case DAL_UTYPE:
	return (elem->utype);
	break;
    case DAL_UCD:
	return (elem->ucd);
	break;
    case DAL_UNIT:
	return (elem->unit);
	break;
    case DAL_XTYPE:
	return (elem->xtype);
	break;
    case DAL_DATATYPE:
	return (elem->datatype);
	break;
    case DAL_ARRAYSIZE:
	return (elem->arraysize);
	break;
    case DAL_WIDTH:
	return (elem->width);
	break;
    case DAL_PRECISION:
	return (elem->precision);
	break;
    case DAL_VALUE:
	return (elem->value);
	break;
    default:
	return (dal_nError (dal, DAL_ATTRNOTFOUND));
	break;
    }

    return (NULL);
}


/**
 * dal_getFieldAttr -- Get the specified attribute of FIELD/PARAM instance.
 *
 * @brief   Get the specified attribute of an FIELD/PARAM instance.
 * @fn	    str = dal_getFieldAttribute (qr, index, attr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @param   index		Index of Field element
 * @param   attr		Code for Field attribute to be returned
 * @returns			The value of the given attribute as a string
 *
 * Both PARAM and FIELD instances are stored in the same list structure,
 * with PARAM instances first (low index values), followed by FIELD instances.
 *
 * PARAMs have values that can be returned as attributes, but FIELDs do not.
 * To treat PARAM and FIELD equivalently it is best to get the values of both
 * using the get*Field methods of the Record object.
 */
char *
dal_getFieldAttr (QResponse qr_h, int index, int attr)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (NULL);

    /* Recover DAL connection context. */
    dalConn_t *dal = qr->query->dal;

    /* Get the Field instance. */
    dalField_t *elem = (dalField_t *) vll_seek (qr->fields, index, SEEK_SET);
    if (elem == NULL)
	return (dal_nError (dal, DAL_QRFIELDERR));

    /* Return the requested attribute. */
    switch (attr) {
    case DAL_ID:
	return (elem->id);
	break;
    case DAL_NAME:
	return (elem->name);
	break;
    case DAL_UTYPE:
	return (elem->utype);
	break;
    case DAL_UCD:
	return (elem->ucd);
	break;
    case DAL_UNIT:
	return (elem->unit);
	break;
    case DAL_XTYPE:
	return (elem->xtype);
	break;
    case DAL_DATATYPE:
	return (elem->datatype);
	break;
    case DAL_ARRAYSIZE:
	return (elem->arraysize);
	break;
    case DAL_WIDTH:
	return (elem->width);
	break;
    case DAL_PRECISION:
	return (elem->precision);
	break;
    case DAL_VALUE:
	return (elem->value);
	break;
    default:
	return (dal_nError (dal, DAL_ATTRNOTFOUND));
	break;
    }

    return (NULL);
}


/**
 * dal_getFieldIndex -- Search for a Field by the given attribute.
 *
 * @brief   Search for a Field by the given attribute.
 * @fn	    index = dal_getFieldIndex (qr, key, attr)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @param   key			Attribute value to search for
 * @param   attr		Code for Field attribute for search
 * @returns			The index of the Field, or DAL_ERROR
 *
 * Search for a Field (votable FIELD or PARAM) given the value of the
 * indicated attribute, i.e., ID, NAME, UTYPE, UCD, etc.  For example,
 * one might search for a Field with UTYPE attribute having the key value
 * "Curation.PublisherDID".  Searches are case-insensitive.
 */
int
dal_getFieldIndex (QResponse qr_h, char *key, int attr)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    /* Search for the Field by the given attribute. */
    dalField_t *field = NULL;

    switch (attr) {
    case DAL_ID:
	field = (dalField_t *) vht_findKey (qr->hash_id, key);
	break;
    case DAL_NAME:
	field = (dalField_t *) vht_findKey (qr->hash_name, key);
	break;
    case DAL_UTYPE:
	field = (dalField_t *) vht_findKey (qr->hash_utype, key);
	break;
    case DAL_UCD:
	field = (dalField_t *) vht_findKey (qr->hash_ucd, key);
	break;
    default:
	return (dal_qrError (qr, DAL_ATTRNOTFOUND));
    }

    if (field == NULL)
	return (dal_qrError (qr, DAL_FIELDNOTFOUND));

    /* Return the Field index. */
    return (field->index);
}


/**
 * dal_getRecord -- Get a Record object (table row).
 *
 * @brief   Get a Record object (table row).
 * @fn	    record = dal_getRecord (qr, recnum)
 *
 * @param   qr			Opaque handle for QueryResponse object
 * @param   recnum		Record number (0-indexed)
 * @returns			An Opaque handle for the Record object
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.
 */
QRecord
dal_getRecord (QResponse qr_h, int recnum)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    if (recnum < 0 ||recnum >= qr->nrows)
	return (dal_qrError (qr, DAL_RECORDNOTFOUND));

    dalConn_t *dal = qr->query->dal;
    dalRecord_t *rec = &qr->rows[recnum];

    return (svr_newHandle (dal->context, rec));
}


/**
 * dal_releaseRecord -- Release a Record object (table row).
 *
 * @brief   Release a Record object (table row).
 * @fn	    void dal_releaseRecord (rec)
 *
 * @param   rec			Opaque handle for Record object
 * @returns			Nothing
 *
 * Release any resources associated with the Record object.
 */
void
dal_releaseRecord (QRecord rec_h)
{
    svr_freeHandle (rec_h);
}


/**
 * dal_getIntProperty -- Get the value of a Property as an integer.
 *
 * @brief   Get the value of a Property as an integer.
 * @fn	    int = dal_getIntProperty (rec, property)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   property		Property name
 * @returns			Integer value of the property
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.  Properties may be constant for the entire QR (for
 * example the data model name or version), or may vary for each table row.
 */
int
dal_getIntProperty (QRecord rec_h, char *property)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (DAL_ERROR);

    /* Lookup the named property. */
    dalProp_t *prop =
	(dalProp_t *) vht_findKey (rec->qr->hash_prop, property);
    if (prop == NULL)
	return (dal_qrError (rec->qr, DAL_PROPNOTFOUND));

    /* Get the property value as a string. */
    char *valstr, *op;
    if (prop->type == DAL_PARAM)
	valstr = prop->value;
    else
	valstr = rec->pdata[prop->colnum];

    /* Convert to an integer (long) value. */
    if (!valstr)
	return (dal_qrError (rec->qr, DAL_INVALIDINTPROP));

    long lval = strtol (valstr, &op, 10);
    if (*op != '\0' && *op != ' ')
        return (dal_qrError (rec->qr, DAL_INVALIDINTPROP));

    return ((int) lval);
}


/**
 * dal_getFloatProperty -- Get the value of a Property as a floating value.
 *
 * @brief   Get the value of a Property as a floating value.
 * @fn	    double = dal_getFloatProperty (rec, property)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   property		Property name
 * @returns			Floating value of the property
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.  Properties may be constant for the entire QR (for
 * example the data model name or version), or may vary for each table row.
 */
double
dal_getFloatProperty (QRecord rec_h, char *property)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (DAL_ERROR);

    /* Lookup the named property. */
    dalProp_t *prop = vht_findKey (rec->qr->hash_prop, property);
    if (prop == NULL)
	return (dal_qrError (rec->qr, DAL_PROPNOTFOUND));

    /* Get the property value as a string. */
    char *valstr, *op;
    if (prop->type == DAL_PARAM)
	valstr = prop->value;
    else
	valstr = rec->pdata[prop->colnum];

    if (!valstr)
	return (dal_qrError (rec->qr, DAL_INVALIDFLOATPROP));

    double dval = strtod (valstr, &op);
    if (*op != '\0' && *op != ' ')
	return (dal_qrError (rec->qr, DAL_INVALIDFLOATPROP));

    return (dval);
}


/**
 * dal_getStringProperty -- Get the value of a Property as a string.
 *
 * @brief   Get the value of a Property as a string
 * @fn	    string = dal_getStringProperty (rec, property)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   property		Property name
 * @returns			String value of the property
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.  Properties may be constant for the entire QR (for
 * example the data model name or version), or may vary for each table row.
 */
char *
dal_getStringProperty (QRecord rec_h, char *property)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (NULL);

    /* Recover Query and DAL connection context. */
    dalQuery_t *query = rec->qr->query;
    dalConn_t *dal = query->dal;

    /* Lookup the named property. */
    dalProp_t *prop = vht_findKey (rec->qr->hash_prop, property);
    if (prop == NULL)
	return (dal_nError (dal, DAL_PROPNOTFOUND));

    /* Get the property value as a string. */
    char *valstr;
    if (prop->type == DAL_PARAM)
	valstr = prop->value;
    else
	valstr = rec->pdata[prop->colnum];

    return (valstr);
}


/**
 * dal_getIntField -- Get the value of a Field as an integer.
 *
 * @brief   Get the value of a Field as an integer.
 * @fn	    int = dal_getIntField (rec, index)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   index		Index of Field
 * @returns			Integer value of the Field
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.  Field objects may be constant for the entire QR
 * (VOTable PARAMs) or may vary for each table row (VOTable FIELDs).
 * Since Fields are named by multiple attributes, the Field index is used
 * here to identify the Field.
 */
int
dal_getIntField (QRecord rec_h, int index)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (DAL_ERROR);

    /* Check for an out of bounds index. */
    if (index < 0 || index >= rec->qr->ncols)
	return (dal_qrError (rec->qr, DAL_INDEXOUTOFBOUNDS));

    /* Lookup the named Field. */
    dalField_t *field =
	(dalField_t *) vll_seek (rec->qr->fields, index, SEEK_SET);
    if (field == NULL)
	return (dal_qrError (rec->qr, DAL_FIELDNOTFOUND));

    /* Get the Field value as a string. */
    char *valstr, *op;
    if (field->type == DAL_PARAM)
	valstr = field->value;
    else
	valstr = rec->cdata[field->colnum];

    if (!valstr)
        return (dal_qrError (rec->qr, DAL_INVALIDINTFIELD));

    long lval = strtol (valstr, &op, 10);
    if (*op != '\0' && *op != ' ')
        return (dal_qrError (rec->qr, DAL_INVALIDINTFIELD));

    return ((int) lval);
}


/**
 * dal_getFloatField -- Get the value of a Field as a floating value.
 *
 * @brief   Get the value of a Field as a floating value.
 * @fn	    double = dal_getFloatField (rec, index)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   index		Index of Field
 * @returns			Floating value of the Field
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.  Field objects may be constant for the entire QR
 * (VOTable PARAMs) or may vary for each table row (VOTable FIELDs).
 * Since Fields are named by multiple attributes, the Field index is used
 * here to identify the Field.
 */
double
dal_getFloatField (QRecord rec_h, int index)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (DAL_ERROR);

    /* Check for an out of bounds index. */
    if (index < 0 || index >= rec->qr->ncols)
	return (dal_qrError (rec->qr, DAL_INDEXOUTOFBOUNDS));

    /* Lookup the named Field. */
    dalField_t *field =
	(dalField_t *) vll_seek (rec->qr->fields, index, SEEK_SET);
    if (field == NULL)
	return (dal_qrError (rec->qr, DAL_FIELDNOTFOUND));

    /* Get the Field value as a string. */
    char *valstr, *op;
    if (field->type == DAL_PARAM)
	valstr = field->value;
    else
	valstr = rec->cdata[field->colnum];

    if (!valstr)
        return (dal_qrError (rec->qr, DAL_INVALIDFLOATFIELD));

    double dval = strtod (valstr, &op);
    if (*op != '\0' && *op != ' ')
        return (dal_qrError (rec->qr, DAL_INVALIDFLOATFIELD));

    return (dval);
}


/**
 * dal_getStringField -- Get the value of a Field as a string.
 *
 * @brief   Get the value of a Field as a string.
 * @fn	    string = dal_getStringField (rec, index)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   index		Index of Field
 * @returns			String value of the Field
 *
 * The QueryResponse object contains data for a number of Records, one per
 * table row.  Records have both abstract Properties and data Fields, with
 * getters for each.  Field objects may be constant for the entire QR
 * (VOTable PARAMs) or may vary for each table row (VOTable FIELDs).
 * Since Fields are named by multiple attributes, the Field index is used
 * here to identify the Field.
 */
char *
dal_getStringField (QRecord rec_h, int index)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (NULL);

    /* Recover Query and DAL connection context. */
    dalQuery_t *query = rec->qr->query;
    dalConn_t *dal = query->dal;

    /* Check for an out of bounds index. */
    if (index < 0 || index >= rec->qr->ncols)
	return (dal_nError (dal, DAL_INDEXOUTOFBOUNDS));

    /* Lookup the named Field. */
    dalField_t *field =
	(dalField_t *) vll_seek (rec->qr->fields, index, SEEK_SET);
    if (field == NULL)
	return (dal_nError (dal, DAL_FIELDNOTFOUND));

    /* Get the Field value as a string. */
    char *valstr;
    if (field->type == DAL_PARAM)
	valstr = field->value;
    else
	valstr = rec->cdata[field->colnum];

    return (valstr);
}


/**
 * dal_getIntColumn -- Get a table column as an array of integers.
 *
 * @brief   Get a table column as an array of integers
 * @fn	    status = dal_getIntColumn (qr, index, iptr)
 *
 * @param   rec			Opaque handle for QueryResponse object
 * @param   index		Index of Field
 * @param   iptr		Output array of integers (type long)
 * @returns			Length of output array or DAL_ERROR
 *
 * The column at the given index is extracted and returned as an array
 * of long integer values.  The caller must supply the array IPTR which must
 * be at least NROWS in length.  The length of the extracted array, or 0, is
 * returned as the function value.
 */
int
dal_getIntColumn (QResponse qr_h, int index, long *iptr)
{
    /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    /* Check for an out of bounds index. */
    if (index < 0 || index >= qr->ncols)
	return (dal_qrError (qr, DAL_INDEXOUTOFBOUNDS));

    /* Lookup the named Field. */
    dalField_t *field =
	(dalField_t *) vll_seek (qr->fields, index, SEEK_SET);
    if (field == NULL)
	return (dal_qrError (qr, DAL_FIELDNOTFOUND));

    /* Extract the column as an array of strings. */
    int rownum = 0;
    while (rownum < qr->nrows) {
	char *op;
	if (field->type == DAL_PARAM) {
	    iptr[rownum++] = strtol (field->value, &op, 10);
	    break;
	} else
	    iptr[rownum++] = strtol (qr->rows->cdata[field->colnum], &op, 10);

	if (*op != '\0' && *op != ' ')
	    return (dal_qrError (qr, DAL_INVALIDINTFIELD));
    }

    return (rownum);
}


/**
 * dal_getFloatColumn -- Get a table column as an array of doubles.
 *
 * @brief   Get a table column as an array of doubles
 * @fn	    status = dal_getFloatColumn (qr, index, dptr)
 *
 * @param   rec			Opaque handle for QueryResponse object
 * @param   index		Index of Field
 * @param   dptr		Output array of doubles
 * @returns			Length of output array or DAL_ERROR
 *
 * The column at the given index is extracted and returned as an array
 * of integer values.  The caller must supply the array IPTR which must be
 * at least NROWS in length.  The length of the extracted array, or 0, is
 * returned as the function value.
 */
int
dal_getFloatColumn (QResponse qr_h, int index, double *dptr)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    /* Check for an out of bounds index. */
    if (index < 0 || index >= qr->ncols)
	return (dal_qrError (qr, DAL_INDEXOUTOFBOUNDS));

    /* Lookup the named Field. */
    dalField_t *field =
	(dalField_t *) vll_seek (qr->fields, index, SEEK_SET);
    if (field == NULL)
	return (dal_qrError (qr, DAL_FIELDNOTFOUND));

    /* Extract the column as an array of strings. */
    int rownum = 0;
    while (rownum < qr->nrows) {
	char *op;
	if (field->type == DAL_PARAM) {
	    dptr[rownum++] = strtod (field->value, &op);
	    break;
	} else
	    dptr[rownum++] = strtod (qr->rows->cdata[field->colnum], &op);

	if (*op != '\0' && *op != ' ')
	    return (dal_qrError (qr, DAL_INVALIDFLOATFIELD));
    }

    return (rownum);
}


/**
 * dal_getStringColumn -- Get a table column as an array of strings.
 *
 * @brief   Get a table column as an array of strings
 * @fn	    status = dal_getStringColumn (qr, index, sptr)
 *
 * @param   rec			Opaque handle for QueryResponse object
 * @param   index		Index of Field
 * @param   sptr		Output array of string pointers
 * @returns			Length of output array or DAL_ERROR
 *
 * The column at the given index is extracted and returned as an array
 * of string pointers.  The caller must supply the array SPTR which must be
 * at least NROWS in length.  The length of the extracted array, or 0, is
 * returned as the function value.
 */
int
dal_getStringColumn (QResponse qr_h, int index, char **sptr)
{
   /* Recover the QueryResponse object from its Handle. */
    dalQR_t *qr = svr_H2P (qr_h);
    if (qr == NULL)
	return (DAL_ERROR);

    /* Check for an out of bounds index. */
    if (index < 0 || index >= qr->ncols)
	return (dal_qrError (qr, DAL_INDEXOUTOFBOUNDS));

    /* Lookup the named Field. */
    dalField_t *field =
	(dalField_t *) vll_seek (qr->fields, index, SEEK_SET);
    if (field == NULL)
	return (dal_qrError (qr, DAL_FIELDNOTFOUND));

    /* Extract the column as an array of strings. */
    int rownum = 0;
    while (rownum < qr->nrows) {
	if (field->type == DAL_PARAM) {
	    sptr[rownum++] = field->value;
	    break;
	} else
	    sptr[rownum++] = qr->rows->cdata[field->colnum];
    }

    return (rownum);
}


/**
 * dal_getDataset -- Copy a remote dataset to a local file.
 *
 * @brief   Copy remote dataset to a local file
 * @fn	    stat = dal_getDataset (rec, acref, fname)
 *
 * @param   rec			Opaque handle for Record (table row)
 * @param   acref		Access reference URL
 * @param   fname		Output filename
 *
 * The dataset referenced by the given access reference is retrieved and
 * copied to the named local file.  If the output file already exists
 * it is overwritten.
 *
 * The idea here is that the Record describes a dataset that is
 * pointed to by an access reference (acref), the acref being a
 * property of the Record.  While the Record may provide additional
 * information about the dataset, all we really need to retrieve
 * the dataset is the acref URL, so in practice we ignore the rest
 * of the Record here.
 */
int
dal_getDataset (QRecord rec_h, char *acref, char *fname)
{
    /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (DAL_ERROR);

    /* Recover Query and DAL connection context. */
    dalQuery_t *query = rec->qr->query;
    dalConn_t *dal = query->dal;

    return (vut_getURLtoFile (dal, acref, fname));
}


/*
 * Private Functions and Data
 * -------------------------------
 */


/*
 * Property Management (internal)
 */


/* vut_addProperty -- Add a new property to the list of properties.
 */
dalProp_t *
vut_addProperty (dalQR_t *qr, char *propname, char *value, int type)
{
    /* Allocate a new Property object. */
    dalProp_t *prop = (dalProp_t *) calloc (1, sizeof(dalProp_t));
    if (prop == NULL)
	return (NULL);

    /* Initialize the Property */
    prop->type = type;
    strncpy (prop->name, propname, SZ_NAME-1);

    /* Set the value if a PARAM, otherwise assign the column number. */
    if (type == DAL_PARAM) {
	if (value)
	    strncpy (prop->value, value, SZ_VALSTR-1);
    } else
	prop->colnum = vll_count (qr->properties);

    /* Add to the Properties list and hash table. */
    vll_append (qr->properties, (void *)prop, &prop->index);
    vht_insertKey (qr->hash_prop, propname, (void *)prop);

    return (prop);
}

/* vut_setProperty -- Set the value of the named property.
 */
dalProp_t *
vut_setProperty (QRecord rec_h, char *propname, char *value)
{
   /* Recover the Record object from its Handle. */
    dalRecord_t *rec = svr_H2P (rec_h);
    if (rec == NULL)
	return (NULL);

    /* Get the query response object. */
    dalQR_t *qr = rec->qr;
	
    /* Lookup the property. */
    dalProp_t *prop = (dalProp_t *) vht_findKey (qr->hash_prop, propname);
    if (prop == NULL)
	return (NULL);

    /* If it is a global property just reset the value and we are done. */
    if (prop->type == DAL_PARAM) {
	strncpy (prop->value, value, SZ_VALSTR-1);
	return (prop);
    }

    /* Set the value of a per-record property. */
    rec->pdata[prop->colnum] = vut_cmemCopy (qr->cmem, value);

    return (prop);
}

