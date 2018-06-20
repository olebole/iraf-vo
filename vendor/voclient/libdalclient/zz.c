/**
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include "dalclient.h"

#define LOC_SIA		"http://skyview.gsfc.nasa.gov/cgi-bin/vo/sia.pl?"
#define VAO_SIA		"http://skyview.gsfc.nasa.gov/cgi-bin/vo/sia.pl?"


#define	ROWSTART	1
#define	ROWEND		2
#define DELPARAM	3
#define UPDPARAM	4

#define	RA		5
#define	DEC		6
#define	SIZE		7
#define	BAND		8
#define	TIME		9
#define	POL		10
#define	IMTYPE		11
#define	MAXREC		12
#define	COLLECTION	13

void psetDump (FILE *out, Query query);
void qrDump (FILE *out,
    QResponse qr, int dumpTable, int row1, int row2, int attr);
int saveToFile (char *votable, char *text);


/*
 * dalSIAP_test -- Main program.
 */
int
main (int argc, char *argv[])
{
    int dumpTable=1, delParam=1, updParam=1;
    int row1=0, row2=9, attr=DAL_NAME, image=0, ch;
    char *last, *outfile = "dataset.fits";
    char *service = VAO_SIA;
    char *version = "1.0";
    char *votable = "queryResponse.xml";
    int timeSet = 0;

    /* Default query params. */
    double ra=180.0, dec=0.0, size=0.1;
    char *band = "1.0E-8/5";			/* meters in vacumn */
    char *time = "1995/1997";			/* ISO 8601 range */
    char *pol = NULL;				/* stokes,i,q,u,v, "any", etc. */
    char *mode = "archival";			/* and/or cutout,match */
    char *collection = "alma,jvla,vla";		/* data collection(s) */
    char *imtype = NULL;			/* nN image type */
    char *maxrec = NULL;			/* max records in QR */

    if (!updParam)
	time = "1990-07-04/2014";

    /* Command line arguments. */
    static char keyopts[] = "a:di:f:s:v:x:";
    static struct option longopts[] = {
	{ "dump",	no_argument,		NULL,	'd' },
	{ "attr",	required_argument,	NULL,	'a' },
	{ "image",	required_argument,	NULL,	'i' },
	{ "outfile",	required_argument,	NULL,	'f' },
	{ "votable",	required_argument,	NULL,	'x' },
	{ "service",	required_argument,	NULL,	's' },
	{ "version",	required_argument,	NULL,	'v' },
	{ "rs",		required_argument,	NULL,	ROWSTART },
	{ "re",		required_argument,	NULL,	ROWEND },
	{ "dp",		no_argument,		NULL,	DELPARAM },
	{ "up",		no_argument,		NULL,	UPDPARAM },
	{ "ra",		required_argument,	NULL,	RA },
	{ "dec",	required_argument,	NULL,	DEC },
	{ "size",	required_argument,	NULL,	SIZE },
	{ "band",	required_argument,	NULL,	BAND },
	{ "time",	required_argument,	NULL,	TIME },
	{ "pol",	required_argument,	NULL,	POL },
	{ "imtype",	required_argument,	NULL,	IMTYPE },
	{ "maxrec",	required_argument,	NULL,	MAXREC },
	{ "col",	required_argument,	NULL,	COLLECTION },
	{ NULL,		0,			NULL,	0 },
    };

    /* Process command line options. */
    while ((ch = getopt_long(argc, argv, keyopts, longopts, NULL)) != -1) {
	switch (ch) {
	case 'a':
	    /* Table Field attribute to be used to tags fields when dumping
	     * table rows.  The default is to use the Field NAME attribute.
	     * For example use "-a utype" to print Field UTYPEs as the key
	     * when dumping table rows.
	     */
	    if (strcmp (optarg, "id") == 0)
		attr = DAL_ID;
	    else if (strcmp (optarg, "name") == 0)
		attr = DAL_NAME;
	    else if (strcmp (optarg, "utype") == 0)
		attr = DAL_UTYPE;
	    else if (strcmp (optarg, "ucd") == 0)
		attr = DAL_UCD;
	    else {
		fprintf (stderr, "Unknown table Field attribute (%s)\n", optarg);
		exit (1);
	    }
	    break;

	case 'd':
	    /* Toggle the dump table rows flag. */
	    dumpTable = (dumpTable ? 0 : 1);
	    break;

	case ROWSTART:
	    /* The range of table rows to be dumped (0-indexed). */
	    row1 = strtol (optarg, &last, 10);
	    if (*last != '\0')
		row1 = 0;
	    else
		row1 = (row1 < 0) ? 0 : row1;
	    if (image == 0)
		image = row1;
	    break;

	case ROWEND:
	    row2 = strtol (optarg, &last, 10);
	    if (*last != '\0')
		row2 = 9999;
	    break;

	case DELPARAM:
	    delParam++;
	    break;
	case UPDPARAM:
	    updParam++;
	    break;

	case 'i':
	    /* Retrieve the indicated dataset (defaults to rowstart). */
	    image = strtol (optarg, NULL, 10);
	    if (image < 0)
		image = 0;
	    break;

	case 'f':
	    /* Dataset output filename (default dataset.fits) */
	    outfile = optarg;
	    break;

	case 'x':
	    /* Save query response VOTable to a file. */
	    if (strcmp (optarg, "null") == 0)
		votable = NULL;
	    else
		votable = optarg;
	    break;

	case 's':
	    /* The baseURL of the service to be queried.
	     * LOC_SIAV2 is the reference service running on localhost
	     * VAO_SIAV2 is the reference service running on vaosa-vm1
	     * Otherwise the baseURL input is used.
	     */
	    if (strncmp (optarg, "local", 1) == 0)
		service = LOC_SIA;
	    else if (strncmp (optarg, "vao", 1) == 0)
		service = VAO_SIA;
	    else
		service = optarg;
	    break;
	case 'v':
	    /* Service version. */
	    version = optarg;
	    break;

	case RA:
	    ra = strtod (optarg, NULL);
	    break;
	case DEC:
	    dec = strtod (optarg, NULL);
	    break;
	case SIZE:
	    size = strtod (optarg, NULL);
	    break;

	case BAND:
	    if (strcmp (optarg, "null") == 0)
		band = NULL;
	    else
		band = optarg;
	    break;
	case TIME:
	    if (strcmp (optarg, "null") == 0)
		time = NULL;
	    else
		time = optarg;
	    timeSet = 1;
	    break;
	case POL:
	    if (strcmp (optarg, "null") == 0)
		pol = NULL;
	    else
		pol = optarg;
	    break;

	case IMTYPE:
	    if (strcmp (optarg, "null") == 0)
		imtype = NULL;
	    else
		imtype = optarg;
	    break;
	case MAXREC:
	    if (strcmp (optarg, "null") == 0)
		maxrec = NULL;
	    else
		maxrec = optarg;
	    break;
	case COLLECTION:
	    if (strcmp (optarg, "null") == 0)
		collection = NULL;
	    else
		collection = optarg;
	    break;

	default:
	    fprintf (stderr, "unknown option (%s)\n", optarg);
	    exit (2);
	    break;
	}
    }

    argc -= optind;
    argv += optind;

    printf ("SIAP Test Query\n");
    printf ("==================================\n\n");

    /* Initialize the service connection.  This doesn't actually contact
     * the remote service, it just creates a connection context.
     */
    DAL dal = dal_openSiapConnection (service, version);
    if (dal == DAL_ERROR) {
	fprintf (stderr, "service connection failed (%d)\n", dal_getError(dal));
	exit (dal_getError (dal));
    }

    /* Create a Query object to query the remote service.  This constructor
     * sets the SIAP POS and SIZE parameters.  The default search region for
     * the reference service specified here is the whole sky, as the test
     * archive only contains a few hundred images.  For a normal archive
     * one would use a smaller region to avoid QR table overflow.  Once
     * the cutout MODE is fully implemented, the search region may be
     * narrowed to specify a subset of parameter space, and mode=cutout
     * would cause the service to automatically generate the description
     * of one or more virtual images optimized to cover the cutout region
     * The accessData method to come later will allow the client to specify
     * the subset region explicitly for a single image.
     */
    Query query = dal_getSiapQuery (dal, ra, dec, size, size, NULL);
    if (query == DAL_ERROR) {
	fprintf (stderr, "query constructor failed (%d)\n", dal_getError(dal));
	exit (dal_getError (dal));
    }

    /* Dump the current parameter set. */
    printf ("------ Query Parameters ------\n");
    psetDump (stdout, query);
    printf ("\n");


    /*
     * Execute the query!
     * ---------------------------------
     * The now composed query is executed by the service, the query
     * response votable is read, and processed to generate the query response
     * object, the handle for which is returned as the function value.
     */
    printf ("------ Query remote service and process QR: ------\n");

    /* Let's see what we get for a queryURL for the above. */
    printf ("queryURL: %s\n", dal_getQueryURL(query));
    fflush (stdout);

    /* Execute query and see how long it takes. */
    struct timeval t1;  gettimeofday (&t1, NULL);
    QResponse qr = dal_executeQuery (query);
fprintf (stderr, "dal_executeQuery = %d\n", qr);
    struct timeval t2;  gettimeofday (&t2, NULL);

    if (qr == DAL_ERROR) {
	fprintf (stderr, "query execution failed (%d)\n", dal_getError(dal));
	exit (dal_getError (dal));
    } else {
	printf ("    status = ok  (%ld msec)\n\n",
	    (t2.tv_sec * 1000 + t2.tv_usec / 1000) -
	    (t1.tv_sec * 1000 + t1.tv_usec / 1000));
	fflush (stdout);
    }

    /* Dump the query response. */
//    printf ("------ Dump query response ------\n");
//    qrDump (stdout, qr, dumpTable, row1, row2, attr);
//    fflush (stdout);

    /* Save the QueryResponse VOTAble to a file. */
    if (votable) {
	printf ("------ Save QueryResponse VOTable ------\n");
	char *text = dal_executeVOTable (query);
	saveToFile (votable, text);
	printf ("Query response votable saved to '%s'\n\n", votable);
	fflush (stdout);
    }

    /*
     * Retrieve a sample dataset.
     * ---------------------------------
     * The current implementation will return only uncompressed datasets.
     * Datasets can be stored in the remote archive in compressed form
     * (only GZIP currently) and will be uncompressed on the fly during
     * the download.
     */

    /* Get the table record (one per available image). */
    QRecord rec = dal_getRecord (qr, image);
fprintf (stderr, "dal_getRecord = %d\n", rec);
    if (rec == DAL_ERROR) {
	fprintf (stderr, "cannot access requested record (%d)\n",
	    dal_getError(dal));
	exit (dal_getError (dal));
    }

    /* Make sure the record has an acref URL.  It is possible for a record
     * to describe an image that is not available for download, e.g., an
     * image for which the proprietary period is not yet up.
     */
    char *acref = dal_getStringProperty (rec, "acref");
    if (acref == NULL || strlen(acref) == 0) {
	fprintf (stderr, "no access reference (%d)\n", dal_getError(dal));
	exit (dal_getError (dal));

    } else {
	/* Initiate the download. */
	printf ("-------- Downloading dataset %d: ---------\n", image);
	fflush (stdout);

	struct timeval t1;  gettimeofday (&t1, NULL);
	int stat = dal_getDataset (rec, acref, outfile);
	struct timeval t2;  gettimeofday (&t2, NULL);

	if (stat == DAL_ERROR)
	    fprintf (stderr, "dataset retrieval failed (%d)\n", dal_getError(dal));
	else {
	    fprintf (stdout, "Image successfully downloaded as '%s' (%ld msec)\n", outfile,
		(t2.tv_sec * 1000 + t2.tv_usec / 1000) -
		(t1.tv_sec * 1000 + t1.tv_usec / 1000));
	}
    }

    /* Free the record and any associated resources. */
    dal_releaseRecord (rec);
    fflush (stdout);

    /* Clean up.  Since a connection context may support multiple queries
     * none of this is automatic; one must close each object explicitly to
     * free resources.
     */
    dal_closeQueryResponse (qr);
    dal_closeQuery (query);
    dal_closeConnection (dal);

    exit (0);
}


/*
 * psetDump -- Dump a parameter set (Query) object to the output stream.
 */
void
psetDump (FILE *out, Query query)
{
    int i, j, nparams = dal_getParamCount(query);
    fprintf (out, "Number of params = %d:\n", nparams);

    for (i=0;  i < nparams;  i++) {
	char *pname = dal_getParamName (query, i);
	char *pval;
	fprintf (out, "    %3d %s = ", i, pname);
	for (j=0;  (pval = dal_getParam(query,pname,j)) != NULL;  j++)
	    fprintf (out, "%s", pval);
	fprintf (out, "\n");
    }
}


/*
 * qrDump -- Dump a QueryResponse object to the output file.
 *
 * Normal use in an application is to access properties by name or fields by
 * the value of an attribute such as field Name, Utype, or UCD (resolving
 * said key to the field index).  Below however we don't know anything about
 * what is in the table, so we just dump the contents by iterating through
 * rows and columns.
 */
void
qrDump (FILE *out, QResponse qr, int dumpTable, int row1, int row2, int attr)
{
    /* Get QR sizes. */
    int nrows = dal_getRecordCount(qr);
    int ncols = dal_getColumnCount(qr);
    int nInfo = dal_getInfoCount(qr);
    int nProp = dal_getPropCount(qr);
    int nField = dal_getFieldCount(qr);

    /* Print summary of contents. */
    fprintf (out,
	"QueryResponse nrows=%d ncols=%d ninfo=%d nprop=%d nfields=%d\n\n",
	nrows, ncols, nInfo, nProp, nField);

    /* List the INFO elements for the current RESOURCE. */
    fprintf (out, "Infos:\n");
    int i, j, lastRow;

    for (i=0;  i < nInfo;  i++) {
	fprintf (out, "    %s = %s\n",
	    dal_getInfoAttr (qr, i, DAL_NAME),
	    dal_getInfoAttr (qr, i, DAL_VALUE));
    }
    fprintf (out, "\n");

    if (!dumpTable)
	return;

    lastRow = (row2 > (nrows-1)) ? nrows - 1 : row2;
    for (i=row1;  i <= lastRow;  i++) {
	QRecord rec = dal_getRecord (qr, i);
	fprintf (out, "================== Row %d: =================\n\n", i);

	/* List the Properties for the given table row. */
	fprintf (out, "------ Properties -------\n");
	for (j=0;  j < nProp;  j++) {
	    char *propName = dal_getPropName (qr, j);
	    fprintf (out, "    %s = %s\n", propName,
		dal_getStringProperty (rec, propName));
	}
	fprintf (out, "\n");

	/* List the Fields for the given table row. */
	fprintf (out, "------ Fields -------\n");
	for (j=0;  j < nField;  j++) {
	    char *fieldAttr = dal_getFieldAttr (qr, j, attr);
	    fprintf (out, "    %s = %s\n", fieldAttr,
		dal_getStringField (rec, j));
	}

	fprintf (out, "\n");
	dal_releaseRecord (rec);
    }
}


/*
 * saveToFile -- Save a block of text to the named file.
 * Any existing file is overwritten (so be careful).
 */
int
saveToFile (char *votable, char *text)
{
    FILE *out = fopen (votable, "w");
    if (!out)
	return (ERR);

    fputs (text, out);
    fclose (out);
    return (OK);
}

