// 2014AUG19 KJM 
// 2014AUG18 KJM z3test (based on zztest)
// 2014AUG15 KJM
// 2014AUG14 MF

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "xmlParse.h"
#include "xmlParseP.h"
#include "vosi.h"

void
xml_dumpAllAttributesNameValueFromHandle
(
 handle_t h,
 int verbose,
 int level
 )
{ // KJM
  char namesAC[SZ_XMLTAG];
  char *name;
  char *value;
  char spaceAC[2] = " ";
  { // show all attribute names and values (in reverse order)
    strncpy( namesAC, xml_dumpNamesAttr( h ), SZ_XMLTAG );
    name = strtok( namesAC, spaceAC ); // get first name
    while ( name != NULL )
      {
	value = xml_getAttr( h, name );
	if (verbose)
	  printf ("<%d>  attribute: %s=\"%s\"\n", level, name, value );
	name = strtok(NULL, spaceAC);
      }
  }
  return;
}

void
xml_dumpElementNameValueFromHandle
(
 handle_t h,
 char **namePAC,
 char **valuePAC,
 int verbose,
 int level
 )
{ // KJM
  assert( NULL != namePAC );
  assert( NULL != valuePAC );
  *namePAC = xml_elemNameByHandle(h);
  *valuePAC = xml_getValue(h);
  if (verbose)
    {
      if (strlen(*valuePAC) == 0)
	printf ("<%d>  element: %s\n", level, *namePAC );
      else
	printf ("<%d>  element: %s = %s\n", level, *namePAC, *valuePAC );
    }
  return;
}

int
main (int argc, char **argv)
{
  int  xp = 0, top, cap;
  char *name = NULL, *ns = NULL;

  if (argc < 2) {
    fprintf (stderr, "Usage:  z3test <arg>\n");
    exit (1);
  }

  printf ("z3test \"%s\"\n", argv[1]);
  printf ("%s =argv[0]\n", argv[0]);
  printf ("%s =argv[1]\n", argv[1]);
  printf ("\n");

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

#define DUMPXML
  //#undef DUMPXML
#ifdef DUMPXML
{   
  int  h, hh, hhh, hhhh, hhhhh, hhhhhh, hhhhhhh, hhhhhhhh, hhhhhhhhh;
  int level = 0;
  char *element_name;
  char *element_value;
  int verbose = 1;
  printf( "***** DUMP ELEMENTS *****: BEGIN\n" );
  verbose = 1;
  level = 0; 
  if (verbose)
    {
      if (strlen(ns) != 0)
	printf ("<%d>  element: %s:%s\n", level, ns, name );
      else
	printf ("<%d>  element: %s\n", level, name );
    }
  h = top;
  xml_dumpAllAttributesNameValueFromHandle( h, verbose, level );
  level++;
  for ( h = xml_getChild( top ); h; h = xml_getSibling( h ) ) 
    {
      xml_dumpElementNameValueFromHandle( h, &element_name, &element_value, verbose, level );
      xml_dumpAllAttributesNameValueFromHandle( h, verbose, level );
      level++;
      for ( hh = xml_getChild( h ); hh; hh = xml_getSibling( hh ) ) 
	{
	  xml_dumpElementNameValueFromHandle( hh, &element_name, &element_value, verbose, level );
	  xml_dumpAllAttributesNameValueFromHandle( hh, verbose, level );
	  level++;
	  for ( hhh = xml_getChild( hh ); hhh; hhh = xml_getSibling( hhh ) ) 
	    {
	      xml_dumpElementNameValueFromHandle( hhh, &element_name, &element_value, verbose, level );
	      xml_dumpAllAttributesNameValueFromHandle( hhh, verbose, level );
	      level++;
	      for ( hhhh = xml_getChild( hhh ); hhhh; hhhh = xml_getSibling( hhhh ) ) 
		{
		  xml_dumpElementNameValueFromHandle( hhhh, &element_name, &element_value, verbose, level );
		  xml_dumpAllAttributesNameValueFromHandle( hhhh, verbose, level );
		  level++;
		  for ( hhhhh = xml_getChild( hhhh ); hhhhh; hhhhh = xml_getSibling( hhhhh ) ) 
		    {
		      xml_dumpElementNameValueFromHandle( hhhhh, &element_name, &element_value, verbose, level );
		      xml_dumpAllAttributesNameValueFromHandle( hhhhh, verbose, level );
		      level++;
		      for ( hhhhhh = xml_getChild( hhhhh ); hhhhhh; hhhhhh = xml_getSibling( hhhhhh ) ) 
			{
			  xml_dumpElementNameValueFromHandle( hhhhhh, &element_name, &element_value, verbose, level );
			  xml_dumpAllAttributesNameValueFromHandle( hhhhhh, verbose, level );
			  level++;
			  for ( hhhhhhh = xml_getChild( hhhhhh ); hhhhhhh; hhhhhhh = xml_getSibling( hhhhhhh ) ) 
			    {
			      xml_dumpElementNameValueFromHandle( hhhhhhh, &element_name, &element_value, verbose, level );
			      xml_dumpAllAttributesNameValueFromHandle( hhhhhhh, verbose, level );
			      level++;
			      for ( hhhhhhhh = xml_getChild( hhhhhhh ); hhhhhhhh; hhhhhhhh = xml_getSibling( hhhhhhhh ) ) 
				{
				  xml_dumpElementNameValueFromHandle( hhhhhhhh, &element_name, &element_value, verbose, level );
				  xml_dumpAllAttributesNameValueFromHandle( hhhhhhhh, verbose, level );
				  level++;
				  for ( hhhhhhhhh = xml_getChild( hhhhhhhh ); hhhhhhhhh; hhhhhhhhh = xml_getSibling( hhhhhhhhh ) ) 
				    {
				      xml_dumpElementNameValueFromHandle( hhhhhhhhh, &element_name, &element_value, verbose, level );
				      xml_dumpAllAttributesNameValueFromHandle( hhhhhhhhh, verbose, level );
				      /* CAVEAT EMPTOR: assume that the XML document does not go deeper than 9 levels
				       */
				      assert( xml_getChild( hhhhhhhhh ) == 0 );
				    }
				  level--;
				}
			      level--;
			    }
			  level--;
			}
		      level--;
		    }
		  level--;
		}
	      level--;
	    }
	  level--;
	}
      level--;
    }
  level--;
  xml_closeXML (xp);
  printf( "***** DUMP ELEMENTS *****: END\n" );
}
#endif

  //**************************************************************************
  //***** CAPABILITIES *******************************************************
  //**************************************************************************

  printf( "\n\n***** CAPABILITIES *****\n\n" );

  cap = vosi_getCapabilities( argv[1] ); // base handle (index = 0)
  assert( cap != 0 ); // zero-valued handles are not defined

  { // loop over all Capabilities
    handle_t h;
    char * spud;
    int index;
    unsigned long ServiceType = 00;
    unsigned long services = 00;
    for ( h = cap; h; h = vosi_cap_nextHandle( h ) ) 
      {
	printf( "\n" );
	assert( h > 0 ); // defined handles are postive
	index = vosi_cap_index( h );
	if ( index >= 0 )
	  printf( "%d =index\n", index );
	//spud = vosi_cap_standardID( h );
	//if ( spud[0] != '\0' )
	//  printf( "%s =standardID\n", spud );
	spud = vosi_cap_IVORN( h );
	if ( spud[0] != '\0' )
	  printf( "%s =IVORN\n", spud );
	//spud = vosi_cap_LRN( h );
	//if ( spud[0] != '\0' )
	//  printf( "%s =LRN\n", spud );
	//spud = vosi_cap_nstype( h );
	//if ( spud[0] != '\0' )
	//  printf( "%s =nstype\n", spud );
	spud = vosi_cap_accessURL( h );
	if ( spud[0] != '\0' )
	  printf( "%s =accessURL\n", spud );
	ServiceType = vosi_cap_ServiceType( h );
	//printf( "%lu =ServiceType\n", ServiceType );
#ifdef BUG
	services += ServiceType;
#endif
	services |= ServiceType;
	spud = vosi_cap_ServiceTypeNames( ServiceType );
	if ( spud[0] != '\0' )
	  printf( "%s =ServiceTypeName\n", spud );
      }
    printf( "\nAll services provided: %s\n", vosi_cap_ServiceTypeNames( services ) );
  }

  //**************************************************************************
  //***** SHUTDOWN ***********************************************************
  //**************************************************************************

  printf( "\n\nThat's all folks!\n" );
  
  return (0);
}

