// 2014SEP09 KJM

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

#include "xmlParse.h"
#include "xmlParseP.h"
#include "vosi.h"

#define SILLYSTRING // changes appearance of parser dumps
#undef SILLYSTRING

void
xml_dumpAllAttributesNameValueFromHandle
(
 handle_t h,
 int verbose,
 int level
 )
{ 
  char namesAC[SZ_XMLTAG];
  char *name;
  char *value;
  char spaceAC[2] = " ";
#ifdef SILLYSTRING
  char enderC = ' ';
#else
  char enderC = '\n';
#endif
  { // show all attribute names and values (unfortunately IN REVERSE ORDER)
#ifdef SINGLE_LINE_VERSION
    { // single-line alternative
      strncpy( namesAC, xml_dumpAllAttr( h ), SZ_XMLTAG );
      printf( "{%d}  attribute(s): %s%c", level, namesAC, enderC );
    }
#endif
    strncpy( namesAC, xml_dumpNamesAttr( h ), SZ_XMLTAG );
    name = strtok( namesAC, spaceAC ); // get first name
    while ( name != NULL )
      {
	value = xml_getAttr( h, name );
	if (verbose)
	  printf ("{%d}  attribute: %s=\"%s\"%c", level, name, value, enderC );
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
#ifdef SILLYSTRING
  char enderC = ' ';
#else
  char enderC = '\n';
#endif
  assert( NULL != namePAC );
  assert( NULL != valuePAC );
  *namePAC = xml_elemNameByHandle(h);
  *valuePAC = xml_getValue(h);
  if (verbose)
    {
      if (strlen(*valuePAC) == 0)
	printf ("{%d}  element: %s%c", level, *namePAC, enderC );
      else
	printf ("{%d}  element: %s = %s%c", level, *namePAC, *valuePAC, enderC );
    }
  return;
}

void
xml_dumper
(
 handle_t h,
 int verbose,
 int levelmax,
 int level
 )
{ 
  handle_t hh;
  char *element_name;
  char *element_value;
  //int level = levelp;
  assert( h > 0); // defined handles are positive
  assert( level >= 0 );
  if (xml_getChild( h ) <= 0)
    return;  // abort with no more data
  level++; // descend
  if (level > levelmax)
    return; // abort when level exceeds maximum
  //printf( "xml_dumper: %d =level  %d =levelmax\n", level, levelmax );
  for (  hh = xml_getChild( h ); hh; hh = xml_getSibling( hh ) ) 
    {
      xml_dumpElementNameValueFromHandle( hh, &element_name, &element_value, verbose, level );
      xml_dumpAllAttributesNameValueFromHandle( hh, verbose, level );
      xml_dumper( hh, verbose, levelmax, level );
    }
  return;
}

int
main (int argc, char **argv)
{
  int  xp = 0, top;
  handle_t uws;
  char *name = NULL, *ns = NULL;

  if (argc < 2) {
    fprintf (stderr, "Usage:  z4test <arg>\n");
    exit (1);
  }

  printf ("z4test \"%s\"\n", argv[1]);
  printf ("%s =argv[0]\n", argv[0]);
  printf ("%s =argv[1]\n", argv[1]);
  printf ("\n");

  printf ("Hello world,  arg = '%s'\n", argv[1]);


  xp = xml_openXML (argv[1]);
  printf( "z4test: %d =xp (root node handle of the XML doc)\n", xp );
  assert( xp > 0 ); // handles must be postivie
  printf( "z4test: xp appears to be OK\n" );

  /*  Get the toplevel element name and print some information based on
   *  the XML tree directly.
   */
  printf( "z4test: [0]\n" );
  top = xml_getToplevel (xp);
  printf( "z4test: [1]\n" );
  name = xml_getToplevelName (xp);
  printf( "z4test: [2] %s=name\n", name );
  ns = xml_getToplevelNSpace (xp);
  printf( "z4test: [3]\n" );

  printf ("top:  h='%s'  func='%s'  ns='%s'\n", 
	  xml_elemNameByHandle(top), name, ns);

  printf( "z4test: [4]\n" );

#define EXPERIMENTAL
#ifdef EXPERIMENTAL
  { int h, hh;
    int level = 0;
    int indent = 4;

    xml_dumpXMLpublic( top, level, indent, stdout ); ///// EXPERIMENTAL
    printf( "\n\n" );
    hh = xml_getChild( top );
    xml_dumpXMLpublic( hh, level, indent, stdout ); ///// EXPERIMENTAL

    {
      int result;
      char *filename = NULL;
      FILE *spud = NULL;
      int new_file_mode;
      int j;
      char buffer[100] = "";
      int c;
      int ch;
      int jmax;
      filename = tempnam( "/tmp", "spud" );
      printf( "%s =filename\n", filename );
      spud = fopen( filename, "wx" );
      assert( NULL != spud );
      fprintf( spud, "hi\nthere\nbig boy!\n" );
      fclose( spud );
      printf( "%s closed\n" );
      spud = NULL;
      spud = fopen( filename, "r" );
      assert( NULL != spud );
      printf( "ok!\n" );
      j = 0;
      jmax = sizeof(buffer) - 1;
      printf( "%d =jmax  %d\n", jmax, (int)sizeof(buffer) );
      while ( ((ch = fgetc(spud)) != EOF) && (j < jmax) )
	{
	  printf( "[%d]%c<=%d\n", j, ch, ch );
	  buffer[j] = ch;
	  j++;
	}
      printf( "%d =j  %d =jmax\n", j, jmax );
      assert( j <= jmax );
      buffer[j] = '\0';
      printf( "so far so good...\n" );
      fclose( spud );
      remove( filename );
      printf( "so far so good......\n" );
      printf( "%s =buffer filled by fgetc result\n", buffer );
    }

  }
#endif

  { 
    int h;
    int level = 0;
    int verbose = 1;

    printf( "***** DUMP ELEMENTS *****: BEGIN\n" );

    if (verbose)
      {
	if (strlen(ns) != 0)
	  printf ("{%d}  element: %s:%s\n", level, ns, name );
	else
	  printf ("{%d}  element: %s\n", level, name );
      }

    h = top;
    xml_dumpAllAttributesNameValueFromHandle( h, verbose, level );
    xml_dumper( h, verbose, 10, level );

    printf( "***** DUMP ELEMENTS *****: END\n" );
 }




  xml_closeXML (xp);

  //**************************************************************************
  //***** UNIVERSAL WORKER SERVICES ******************************************
  //**************************************************************************

  printf( "\n\n***** UNIVERSAL WORKER SERVICES *****\n\n" );


  uws = vosi_getUniversalWorkerServices( argv[1] ); 
  assert( uws > 0 ); // defined handles are positive

  printf( "\n\n" );
  printf( "              jobID= %s\n", vosi_uws_jobId( uws ) );
  printf( "            ownerId= %s\n", vosi_uws_ownerId( uws ) );
  printf( "              phase= %s\n", vosi_uws_phase( uws ) );
  printf( "          startTime= %s\n", vosi_uws_startTime( uws ) );
  printf( "            endTime= %s\n", vosi_uws_endTime( uws ) );
  printf( "  executionDuration= %s\n", vosi_uws_executionDuration( uws ) );
  printf( "        destruction= %s\n", vosi_uws_destruction( uws ) );
  printf( "              quote= %s\n", vosi_uws_quote( uws ) );
  printf( "              error= %s\n", vosi_uws_error( uws ) );
  printf( "      hasParameters= %s\n", (vosi_uws_hasParameters( uws )?"true":"false" ));
  printf( "         hasResults= %s\n", (vosi_uws_hasResults( uws )?"true":"false" ));
  printf( "    hasErrorSummary= %s\n", (vosi_uws_hasErrorSummary( uws )?"true":"false" ));
  printf( "         hasJobInfo= %s\n", (vosi_uws_hasJobInfo( uws )?"true":"false" ));

#ifdef NOTYET
  printf( "            ownerID= %s\n", uwsGP->ownerId );
  printf( "              phase= %s\n", uwsGP->phase );
  printf( "              quote= %s\n", uwsGP->quote );
  printf( "          startTime= %s\n", uwsGP->startTime );
  printf( "            endTime= %s\n", uwsGP->endTime );
  printf( "  executionDuration= %s\n", uwsGP->executionDuration );
  printf( "        destruction= %s\n", uwsGP->destruction );
  printf( "      hasParameters= %s\n", ((uwsGP->hasParameters)?"true":"false") );
  printf( "         hasResults= %s\n", ((uwsGP->hasResults)?"true":"false") );
  printf( "    hasErrorSummary= %s\n", ((uwsGP->hasErrorSummary)?"true":"false") );
  printf( "         hasJobInfo= %s\n", ((uwsGP->hasJobInfo)?"true":"false") );
#endif

  //**************************************************************************
  //***** SHUTDOWN ***********************************************************
  //**************************************************************************

  printf( "\n\nThat's all folks!\n" );
  
  return (0);
}

