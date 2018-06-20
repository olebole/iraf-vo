// 2014AUG15 KJM
// 2014AUG14 MF

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xmlParse.h"
#include "vosi.h"

int
main (int argc, char **argv)
{
    int  xp = 0, h, top, avail;
    char *name = NULL, *ns = NULL;


    if (argc < 2) {
	fprintf (stderr, "Usage:  zztest <arg>\n");
	exit (1);
    }

    { // KJM
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

    printf ("top:  h='%s'  func='%s'  ns='%s'\n", 
	xml_elemNameByHandle(top), name, ns);

    for (h=xml_getChild (top); h; h = xml_getSibling (h)) {
	printf ("%s = %s\n", xml_elemNameByHandle(h), xml_getValue(h));
    }

    xml_closeXML (xp);

    /* Now process the same argument using the VOSI interface.
     */
    avail = vosi_getAvailability (argv[1]);
    { //KJM
      printf ("\nservice available:  %s\n",
	      (vosi_Available( avail ) == 1) ? "True" : "False"); 
      printf ("upSince:  %s\n", vosi_upSince( avail ) ); 
      printf ("downAt:  %s\n", vosi_downAt( avail ) ); 
      printf ("backAt:  %s\n", vosi_backAt( avail ) ); 
      printf ("note:  %s\n", vosi_Note( avail ) ); 
    }

    return (0);
}
