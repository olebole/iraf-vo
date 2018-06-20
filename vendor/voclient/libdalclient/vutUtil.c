/**
 *  VUTUTIL.C -- DALClient utility classes.
 *
 * These routines are mostly internal support code specific to DALClient.
 * Some have adapted from VOClient with minor changes, in order for
 * DALClient to be buildable stand-alone as well as a component of
 * VOClient.
 *
 * @file       vutUtil.c
 * @author     Doug Tody, Mike Fitzpatrick
 * @version    January 2014
 *
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <curl/curl.h>
#ifdef OLD_CURL
#include <curl/types.h>
#endif
#include <curl/easy.h>

#define _DALCLIENT_LIB_
#include "dalclient.h"
#include "votParse.h"

/* Private functions. */
static char vut_toHex (char code);
static size_t vut_memoryCallback (void *ptr,
    size_t size, size_t nmemb, void *data);


/*
 * URL Builder (generic code)
 * -------------------------------
 */

typedef struct vutUrl {
    char	*urlbuf;
    int		len;
} vutUrl_t;


/**
 * vut_urlOpen -- Create a URL composer instance.
 *
 * @brief   Create a URL composer instance.
 * @fn	    urlBuilder = vut_urlOpen (baseURL)
 *
 * @param   baseURL		Initial static component of URL
 * @param   resource		Additional resource to be added to baseURL
 * @return			Opaque handle for URL builder
 *
 * The URL builder is called by the service-specific getQueryURL methods.
 *
 * The baseURL is the fixed URL used to access the service.  An additional
 * term may optionally be added to this to reference a specific resource,
 * e.g., "sync" or "async", or some resource pathname.  Keyword=value
 * parameters may then be added.
 */
void *
vut_urlOpen (char *baseURL, char *resource)
{
    int   questSeen = 0;
    char *ip, *op;

    /* Get descriptor. */
    vutUrl_t *vutUrl = (vutUrl_t *) calloc (1, sizeof(vutUrl));
    vutUrl->urlbuf = (char *) calloc (1, SZ_URL);

    /* Copy the baseURL. */
    for (ip=baseURL, op=vutUrl->urlbuf;  (*op = *ip++) != '\0';  op++) {
        if (*op == '?') {
            questSeen++;
        } else if (*op == '&') {
            if (strncmp (ip-1, "&amp;", 5) == 0)
                ip += 4;
        }
    }

    /* Append the given resource, if any. */
    if (resource) {
	if (*(op-1) != '/') {
	    *op++ = '/';
	    *op = '\0';
	}

	for (ip=resource;  (*op = *ip++) != '\0';  op++)
	    ;
    }

    /* Make sure the URL is ready for parameters. */
    if (*(op-1) != '?' && *(op-1) != '&') {
        *op++ = questSeen ? '&' : '?';
	*op = '\0';
    }
    vutUrl->len = op - vutUrl->urlbuf;

    return ((void *) vutUrl);
}


/**
 * vut_urlAddParam -- Add a parameter sequence to the URL.
 *
 * @brief   Add a parameter sequence to the URL.
 * @fn	    void vut_urlAddParam (urlBuilder, param, valstr)
 *
 * @param   urlBuilder		Opaque handle for URL builder
 * @param   param		Parameter name
 * @param   valstr		Value string
 * @return			Nothing
 */
void
vut_urlAddParam (void *urlBuilder, char *param, char *valstr)
{
    vutUrl_t *vutUrl = (vutUrl_t *)urlBuilder;
    char ch, *ip, *op = vutUrl->urlbuf + vutUrl->len;

    /* Crude check to avoid overflow. */
    if ((vutUrl->len + strlen(param) + strlen(valstr) + 64) > SZ_URL)
	return;

    /* Append the "&PARAM=" */
    if (op && *(op-1) != '?' && *(op-1) != '&')
	*op++ = '&';
    for (ip=param;  (*op = *ip++) != '\0';  op++)
	;
    *op++ = '=';

    /* Append the value string, URL-encoding in the process (may be null).
     * We do not need to URL-encode characters that are legal in the param
     * (rather than path) part of the URL.
     */
    for (ip=valstr;  (*op = ch = *ip++) != '\0';  op++) {
	if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~'
	    || ch == ',' || ch == ':' || ch == ';' || ch == '/') {
	    ;
	} else if (ch == ' ') {
	    *op = '+';
	} else {
	    *op = '%';
	    *++op = vut_toHex (ch >> 4);
	    *++op = vut_toHex (ch & 15);
	}
    }

    vutUrl->len = op - vutUrl->urlbuf;
}


/**
 * vut_urlGetString -- Get the composed URL string.
 *
 * @brief   Get the composed URL string
 * @fn	    str = vut_urlGetString (urlBuilder)
 *
 * @param   urlBuilder		Opaque handle for URL builder
 * @return			The composed URL as a string
 */
char *
vut_urlGetString (void *urlBuilder)
{
    vutUrl_t *vutUrl = (vutUrl_t *)urlBuilder;
    return (vutUrl->urlbuf);
}


/**
 * vut_urlClose -- Close the URL builder and free resources.
 *
 * @brief   Close the URL builder and free resources.
 * @fn	    void vut_urlClose (urlBuilder)
 *
 * @param   urlBuilder		Opaque handle for URL builder
 * @return			Nothing
 *
 * The URL builder object is freed.  The buffer for the composed URL string
 * is not freed as this should have been handed off to the caller.
 */
void
vut_urlClose (void *urlBuilder)
{
    vutUrl_t *vutUrl = (vutUrl_t *)urlBuilder;
    free ((void *)vutUrl->urlbuf);
    free (urlBuilder);
}


/**
 *  vut_toHex -- Converts an integer value to its hex character.
 *  (cribbed from elsewhere in voclient along with URL encoder)
 */
static char
vut_toHex (char code)
{
    static char hex[] = "0123456789abcdef";
    return (hex[code & 15]);
}

/*
 * URL Handling.
 * -----------------
 */

/* CURL memory struct.  */
struct MemoryStruct {
  char   *memory;
  size_t  size;
};


/* vut_getURLtoFile -- Copy the contents of a URL to a local file.
 */
int
vut_getURLtoFile (dalConn_t *dal, char *url, char *fname)
{
    CURL *curl;
    CURLcode stat;
    int errcode;

    if (url == NULL)
	return (dal_error (dal, DAL_INVALIDURL));
    if (fname == NULL)
	return (dal_error (dal, DAL_INVALIDFNAME));

    /* Prepare to write the output file. */
    FILE *fp = fopen (fname, "w");
    if (fp == NULL)
	return (dal_error (dal, DAL_CANNOTOPENFILE));

    /* Init the Curl session. */
    curl_global_init (CURL_GLOBAL_ALL);
    if ((curl = curl_easy_init()) == NULL) {
	fclose (fp);
	return (dal_error (dal, DAL_CURLINITERR));
    }

    /* Set Curl options. */
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *)fp);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, "dalclient/1.0");
    curl_easy_setopt (curl, CURLOPT_FAILONERROR, 1L);

    /* Perform the request. */
    stat = curl_easy_perform (curl);
    curl_easy_cleanup (curl);

    /* Clean up if operation fails. */
    if (stat != CURLE_OK) {
	fclose (fp);
	unlink (fname);

	/* Process some common error codes. */
	switch (stat) {
	case CURLE_URL_MALFORMAT:
	    errcode = DAL_INVALIDURL;
	    break;
	case CURLE_COULDNT_RESOLVE_HOST:
	case CURLE_COULDNT_CONNECT:
	case CURLE_RECV_ERROR:
	    errcode = DAL_HOSTNOCONNECT;
	    break;
	case CURLE_HTTP_RETURNED_ERROR:
	    errcode = DAL_HTTPERROR;
	    break;
	case CURLE_OPERATION_TIMEDOUT:
	    errcode = DAL_HTTPTIMEOUT;
	    break;
	case CURLE_TOO_MANY_REDIRECTS:
	    errcode = DAL_HTTPREDIRLOOP;
	    break;
	case CURLE_FILESIZE_EXCEEDED:
	    errcode = DAL_FILETOOLARGE;
	    break;
	default:
	    errcode = DAL_CURLERRBASE + stat;
	    break;
	}

	return (dal_error (dal, errcode));
    }

    return (DAL_OK);
}


/* vut_getURL -- Return the contents of a URL as a string.
 * (adapted from the VOClient svr_getURL to be more self-contained)
 */
char *
vut_getURL (dalConn_t *dal, char *url)
{
    struct MemoryStruct chunk;
    CURL  *curl;
    char  *data;
    int    stat;

    chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0;    /* no data at this point */


    /*  Init the Curl session.  */
    curl_global_init (CURL_GLOBAL_ALL);
    curl = curl_easy_init ();

    /*  Specify the Curl options.  */
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, vut_memoryCallback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, "dalclient/1.0");
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt (curl, CURLOPT_FAILONERROR, 1);

    /*  Do the download.  */
    if ((stat = curl_easy_perform (curl)) != CURLE_OK) {
        data = NULL; 			/* error in download.  	*/
    } else 
	data = (char *) calloc (1, chunk.size + 2);
    
    curl_easy_cleanup (curl); 	/* cleanup curl stuff 	*/
    curl_global_cleanup();		

    if (chunk.memory) {
	memcpy (data, chunk.memory, chunk.size);
	free ((void *) chunk.memory);
    }

    /* Set error code if an error occurred. */
    if (stat != CURLE_OK) {
	int errcode = DAL_CURLEXECERR;

	/* Process some common CURL error codes. */
	switch (stat) {
	case CURLE_URL_MALFORMAT:
	    errcode = DAL_INVALIDURL;
	    break;
	case CURLE_COULDNT_RESOLVE_HOST:
	case CURLE_COULDNT_CONNECT:
	case CURLE_RECV_ERROR:
	    errcode = DAL_HOSTNOCONNECT;
	    break;
	case CURLE_HTTP_RETURNED_ERROR:
	    errcode = DAL_HTTPERROR;
	    break;
	case CURLE_OPERATION_TIMEDOUT:
	    errcode = DAL_HTTPTIMEOUT;
	    break;
	case CURLE_TOO_MANY_REDIRECTS:
	    errcode = DAL_HTTPREDIRLOOP;
	    break;
	case CURLE_FILESIZE_EXCEEDED:
	    errcode = DAL_FILETOOLARGE;
	    break;
	default:
	    errcode = DAL_CURLERRBASE + stat;
	    break;
	}

	return (dal_nError (dal, errcode));
    }

    return (data);
}


/* vut_memoryCallback -- Callback function for Curl downloader.
 * (adapted from the VOClient svr_getURL to be more self-contained)
 */
static size_t
vut_memoryCallback (void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) data;

    if (ptr)
        mem->memory = realloc (mem->memory, (mem->size + realsize + 2));
    else
        mem->memory = calloc (1, mem->size + realsize + 2);

    if (mem->memory) {
        memcpy (&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = '\0';
    }

    return (realsize);
}


/* vut_setString -- Set the value of a string.
 *
 * This routine is equivalent to strncpy except that the input string may
 * be NULL.  The given input string is copied to the output string, while
 * allowing for a NULL value of the input string.  The resultant output
 * string is returned as the function value.
 */
char *
vut_setString (char *out, char *in, int size)
{
    if (in)
	return (strncpy (out, in, size-1));

    memset ((void *)out, '\0', size);
    return (out);
}


/*
 * CMEM -- Simple block-memory allocator for string data.
 *
 * This is used to provide temporary storage for many small strings
 * (potentially many thousands) in a few blocks of memory, without overly
 * taxing the main system memory allocator.  The whole lot can be freed
 * all at once when no longer needed, without having to worry about
 * memory leaks.
 */

#define	MB_SIZE		8192		/* Memory block size */
#define MB_LARGESTR	1024		/* Put in its own buffer */

typedef struct vutCblock_t {
    int		size;
    char	*buf;
    char	*op;			/* set to NULL when full */
    struct	vutCblock_t *next;
} vutCblock_t;

typedef struct vutCmem_t {
    int		nbufs;			/* statistics */
    int		nstrs;			/* statistics */
    vutCblock_t	*first;
    vutCblock_t	*last;
    vutCblock_t	*active;		/* currently receiving data */
} vutCmem_t;


/* vut_cmemInit -- Initialize simple string buffer allocator
 *
 * This simple memory allocator allocates blocks of memory from which buffers
 * are sequentially allocated for static storage of (usually many small)
 * delimited strings.  Eventually the entire thing is freed, by merely
 * freeing a few large buffers.
 */
void *
vut_cmemInit ()
{
    /* Allocate main descriptor. */
    vutCmem_t *md = (vutCmem_t *) calloc (1, sizeof (vutCmem_t));
    if (md == NULL)
	return (NULL);

    /* Allocate first buffer. */
    vutCblock_t *mb = (vutCblock_t *) calloc (1, sizeof (vutCblock_t));
    if (mb == NULL) {
	free ((void *)md);
	return (NULL);
    }

    /* Init descriptor. */
    md->nbufs = 1;
    md->nstrs = 0;
    md->first = mb;
    md->last = mb;
    md->active = mb;

    /* Init buffer storage. */
    if ((mb->buf = (char *) calloc (1, MB_SIZE)) == NULL) {
	free ((void *)md);
	free ((void *)mb);
	return (NULL);
    }

    mb->size = MB_SIZE;
    mb->op = mb->buf;
    mb->next = NULL;

    return ((void *)md);
}


/* vut_cmemDestroy -- Free all memory.
 */
void
vut_cmemDestroy (void *cmem)
{
    vutCmem_t *md = (vutCmem_t *) cmem;
    vutCblock_t *mb, *next;

    for (mb = md->first;  mb;  mb = next) {
	next = mb->next;
	free ((void *)mb->buf);
	free ((void *)mb);
    }

    free ((void *)md);
}


/* vut_cmemCopy -- Copy a string to Cmem storage.
 */
char *
vut_cmemCopy (void *cmem, char *str)
{
    if (str == NULL)
	return (NULL);

    vutCmem_t *md = (vutCmem_t *) cmem;
    vutCblock_t *mb, *active;
    int nleft, len = strlen (str);

    /* Give large strings their own dedicated buffer. */
    if ((len+1) >= MB_LARGESTR) {
	mb = (vutCblock_t *) calloc (1, sizeof (vutCblock_t));
	if (mb == NULL)
	    return (NULL);
	mb->buf = (char *) calloc (1, len+1);
	if (mb->buf == NULL)
	    return (NULL);

	/* Append to global buffer list. */
	md->nbufs++;
	md->last->next = mb;
	md->last = mb;

	/* Init buffer. */
	mb->next = NULL;
	mb->size = len + 1;
	mb->op = NULL;   /* mark buffer full */

	return (strcpy (mb->buf, str));
    }

    /* See if we have space in the current active buffer. */
    active = md->active;
    if (active->op == NULL)
	nleft = 0;
    else
	nleft = active->size - (active->op - active->buf);

    /* Add a new active buffer. */
    if (nleft < (len+1)) {
	mb = (vutCblock_t *) calloc (1, sizeof (vutCblock_t));
	if (mb == NULL)
	    return (NULL);
	if ((mb->buf = (char *) calloc (1, MB_SIZE)) == NULL) {
	    free ((void *)mb);
	    return (NULL);
	}
	mb->size = MB_SIZE;
	mb->op = mb->buf;
	mb->next = NULL;

	md->nbufs++;
	md->active = active = mb;
	md->last->next = mb;
	md->last = mb;
    }

    /* Store the string in the current active buffer. */
    char *cp = strcpy (active->op, str);
    active->op += (len + 1);
    md->nstrs++;

    return (cp);
}


/*  VOTable Utilities
 */

/**
 * vut_votableToDelimited -- Convert a VOTable to delimited text.
 *
 * @brief   Convert a VOTable to delimited text.
 * @fn	    delimited = vut_votableToDelimited (votable, delim)
 *
 * @param   votable		VOTable text
 * @param   delim		Table delimiter
 * @return			Allocated pointer to delimited table
 *
 * The input VOTable is converted to a string of text delimited by the
 * specified character.  Note that in this version we assume only one
 * table <RESOURCE> is present.  The caller is expected to free the
 * returned pointer.
 */
char *
vut_votableToDelimited (char *votable, char delim)
{
    handle_t vot, res, tab, field, data, tdata, tr, td;
    char  *delimited = NULL, *name = NULL, *id = NULL, *ucd = NULL, *s;
    char   buf[SZ_VALSTR], dbuf[2];
    int    i, ncols = 0;


    if ( !votable || (vot = vot_openVOTABLE (votable)) < 0 )
       return (NULL);
    else {
	/*  We know the delimited table will be no larger than the original
	 *  XML, so waste a little space rather than write a temporary file
	 *  and read back the exact size.
         */
	delimited = calloc (1, strlen (votable));
	memset (dbuf, 0, 2);
	sprintf (dbuf, "%c", delim);
    }

    /*  Get the table and data elements.
     */
    if ( ! (res = vot_getRESOURCE (vot)) )
	return (NULL);
    if ( ! (tab = vot_getTABLE (res)) )
	return (NULL);
    if ( ! (data = vot_getDATA (tab)) )
	return (NULL);
    if ( ! (tdata = vot_getTABLEDATA (data)) )
	return (NULL);
    ncols = vot_getNCols (tdata);


    /* Build the header line.
     */
    strcpy (delimited, "# ");
    i = 0;
    for (field = vot_getFIELD (tab); field; field = vot_getNext (field)) {
        name = vot_getAttr (field, "name");     /* find a reasonable value */
        id   = vot_getAttr (field, "id");
        ucd  = vot_getAttr (field, "ucd");
        if (name || id || ucd) {
	    strcat (delimited, (name ? name : (id ? id : ucd)) );
        } else {
	    memset (buf, 0, SZ_VALSTR);
            sprintf (buf, "col%d", i);
	    strcat (delimited, buf);
	}
        if (i < (ncols-1))
	    strcat (delimited, dbuf);
        i++;
    }
    strcat (delimited, "\n");

    /* Now dump the data.
    */
    for (tr=vot_getTR (tdata); tr; tr=vot_getNext(tr)) {
        for (td=vot_getTD(tr),i=0; td; td=vot_getNext(td),i++) {
            s = ((s =vot_getValue (td)) ? s : "");
            if (strchr (s, (int) delim) || strchr (s, (int) ' ')) {
		memset (buf, 0, SZ_VALSTR);
                sprintf (buf, "\"%s\"", s);
                strcat (delimited, buf);
            } else
                strcat (delimited, s);

            if (i < (ncols-1))
                strcat (delimited, dbuf);
        }
        strcat (delimited, "\n");
    }

    /* Clean up. */
    vot_closeVOTABLE (vot);

    return (delimited);
}
