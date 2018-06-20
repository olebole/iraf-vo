/**
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>



/** 
 *  VOS_GETURL -- Utility routine to do a simple URL download to the file.
 */
int 
vos_getURL (char *url, char *fname)
{
    int  stat = 0;
    char errBuf[SZ_LINE];
    FILE *fd;
    CURL *curl_handle;


    if (access (fname, F_OK) == 0)	/* see if file already exists	*/
	unlink (fname);


    /*  For the CURL operation to download the file.
     */
    curl_global_init (CURL_GLOBAL_ALL);     	/* init curl session	*/
    curl_handle = curl_easy_init ();

    /*  Open the output file.
     */
    if ((fd = fopen (fname, "wb")) == NULL) { 	
	if (verbose)
	    fprintf (stderr, "Error: cannot open output file '%s'\n", fname);
        curl_easy_cleanup (curl_handle);
        return 0;
    }

    /*  Set cURL options
     */
    curl_easy_setopt (curl_handle, CURLOPT_URL, url);
    curl_easy_setopt (curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, fd);
    curl_easy_setopt (curl_handle, CURLOPT_ERRORBUFFER, errBuf);
    curl_easy_setopt (curl_handle, CURLOPT_FOLLOWLOCATION, 1);

    /*  Do the download.
     */
    if ((stat = curl_easy_perform (curl_handle)) != 0) {
	/*  Error in download, clean up.
	 */
	fprintf (stderr, "Error: can't download '%s' : %s\n", url, errBuf);
	unlink (fname);
        fclose (fd); 			    	/* close the file 	*/
        curl_easy_cleanup (curl_handle);    	/* cleanup curl stuff 	*/
	return (0);
    }

    fflush (fd);
    fclose (fd); 			    	/* close the file 	*/
    curl_easy_cleanup (curl_handle); 	    	/* cleanup curl stuff 	*/

    return (1);
}
