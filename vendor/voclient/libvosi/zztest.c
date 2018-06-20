// 2014SEP11 KJM
// 2014AUG15 KJM
// 2014AUG14 MF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "xmlParse.h"
#include "xmlParseP.h"
#include "vosi.h"

#define SILLYSTRING		// changes appearance of parser dumps
#undef SILLYSTRING

void
 xml_dumpAllAttributesNameValueFromHandle
    (handle_t h, int verbose, int level) {
    char namesAC[SZ_XMLTAG];
    char *name;
    char *value;
    char spaceAC[2] = " ";

#ifdef SILLYSTRING
    char enderC = ' ';
#else
    char enderC = '\n';
#endif
    {				// show all attribute names and values (unfortunately IN REVERSE ORDER)
#ifdef SINGLE_LINE_VERSION
	{			// single-line alternative
	    strncpy (namesAC, xml_dumpAllAttr (h), SZ_XMLTAG);
	    printf ("{%d}  attribute(s): %s%c", level, namesAC, enderC);
	}
#endif
	strncpy (namesAC, xml_dumpNamesAttr (h), SZ_XMLTAG);
	name = strtok (namesAC, spaceAC);	// get first name
	while (name != NULL) {
	    value = xml_getAttr (h, name);
	    if (verbose)
		printf ("{%d}  attribute: %s=\"%s\"%c", level, name, value,
			enderC);
	    name = strtok (NULL, spaceAC);
	}
    }
    return;
}

void
 xml_dumpElementNameValueFromHandle (handle_t h, char **namePAC, char **valuePAC, int verbose, int level) {	// KJM
#ifdef SILLYSTRING
    char enderC = ' ';
#else
    char enderC = '\n';
#endif
    assert (NULL != namePAC);
    assert (NULL != valuePAC);
    *namePAC = xml_elemNameByHandle (h);
    *valuePAC = xml_getValue (h);
    if (verbose) {
	if (strlen (*valuePAC) == 0)
	    printf ("{%d}  element: %s%c", level, *namePAC, enderC);
	else
	    printf ("{%d}  element: %s = %s%c", level, *namePAC, *valuePAC,
		    enderC);
    }
    return;
}

void
 xml_dumper (handle_t h, int verbose, int levelmax, int level) {
    handle_t hh;
    char *element_name;
    char *element_value;

    //int level = levelp;
    assert (h > 0);		// defined handles are positive
    assert (level >= 0);
    if (xml_getChild (h) <= 0)
	return;			// abort with no more data
    level++;			// descend
    if (level > levelmax)
	return;			// abort when level exceeds maximum
    //printf( "xml_dumper: %d =level  %d =levelmax\n", level, levelmax );
    for (hh = xml_getChild (h); hh; hh = xml_getSibling (hh)) {
	xml_dumpElementNameValueFromHandle (hh, &element_name,
					    &element_value, verbose,
					    level);
	xml_dumpAllAttributesNameValueFromHandle (hh, verbose, level);
	xml_dumper (hh, verbose, levelmax, level);
    }
    return;
}

int main (int argc, char **argv)
{
    int xp = 0, h, top, avail;
    char *name = NULL, *ns = NULL;


    if (argc < 2) {
	fprintf (stderr, "Usage:  zztest <arg>\n");
	exit (1);
    }

    {				// KJM
	printf ("zztest \"%s\"\n", argv[1]);
	printf ("%s =argv[0]\n", argv[0]);
	printf ("%s =argv[1]\n", argv[1]);
	printf ("\n");
    }

    printf ("Hello world,  arg = '%s'\n", argv[1]);

    xp = xml_openXML (argv[1]);

    /*  Get the toplevel element name and print some information based on
     *  the XML tree directly.
     */
    top = xml_getToplevel (xp);
    name = xml_getToplevelName (xp);
    ns = xml_getToplevelNSpace (xp);

    printf ("top:  h='%s'  func='%s'  ns='%s'\n\n",
	    xml_elemNameByHandle (top), name, ns);

    for (h = xml_getChild (top); h; h = xml_getSibling (h)) {
	//printf ("%s = %s\n", xml_elemNameByHandle(h), xml_getValue(h));
    }

    {
	int level = 0;
	int indent = 4;

	printf ("\n***** dumpXMLpublic *****: BEGIN\n");
	xml_dumpXMLpublic (top, level, indent, stdout);	///// EXPERIMENTAL
	printf ("***** dumpXMLpublic *****: END\n\n");
    }



    {
	int h;
	int level = 0;
	int verbose = 1;

	printf ("***** DUMP ELEMENTS *****: BEGIN\n");

	if (verbose) {
	    if (strlen (ns) != 0)
		printf ("{%d}  element: %s:%s\n", level, ns, name);
	    else
		printf ("{%d}  element: %s\n", level, name);
	}

	h = top;
	xml_dumpAllAttributesNameValueFromHandle (h, verbose, level);
	xml_dumper (h, verbose, 10, level);

	printf ("***** DUMP ELEMENTS *****: END\n");
    }

    xml_closeXML (xp);

    //**************************************************************************
    //***** AVAILABILITIES *******************************************************
    //**************************************************************************

    printf ("\n\n***** AVAILABILITIES *****\n\n");

    /* Now process the same argument using the VOSI interface.
     */
    avail = vosi_getAvailability (argv[1]);
    {				//KJM
	printf ("\nservice available:  %s\n",
		(vosi_availAvailable (avail) == 1) ? "True" : "False");
	printf ("upSince:  %s\n", vosi_availUpSince (avail));
	printf ("downAt:  %s\n", vosi_availDownAt (avail));
	printf ("backAt:  %s\n", vosi_availBackAt (avail));
	printf ("note:  %s\n", vosi_availNote (avail));
    }

    return (0);
}
