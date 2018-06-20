// 2014SEP12 KJM 
// 2014SEP11 KJM 
// 2014AUG19 KJM 
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
 xml_dumpAllAttributesNameValueFromHandle (handle_t h, int verbose, int level) {	// KJM
    char namesAC[SZ_XMLTAG];
    char *name;
    char *value;
    char spaceAC[2] = " ";

    {				// show all attribute names and values (in reverse order)
	strncpy (namesAC, xml_dumpNamesAttr (h), SZ_XMLTAG);
	name = strtok (namesAC, spaceAC);	// get first name
	while (name != NULL) {
	    value = xml_getAttr (h, name);
	    if (verbose)
		printf ("<%d>  attribute: %s=\"%s\"\n", level, name,
			value);
	    name = strtok (NULL, spaceAC);
	}
    }
    return;
}

void
 xml_dumpElementNameValueFromHandle (handle_t h, char **namePAC, char **valuePAC, int verbose, int level) {	// KJM
    assert (NULL != namePAC);
    assert (NULL != valuePAC);
    *namePAC = xml_elemNameByHandle (h);
    *valuePAC = xml_getValue (h);
    if (verbose) {
	if (strlen (*valuePAC) == 0)
	    printf ("<%d>  element: %s\n", level, *namePAC);
	else
	    printf ("<%d>  element: %s = %s\n", level, *namePAC,
		    *valuePAC);
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
    int xp = 0, top;
    char *name = NULL, *ns = NULL;
    handle_t h;
    handle_t h_schema = 0;
    handle_t h_table = 0;
    handle_t h_tableset = 0;
    handle_t h_column = 0;
    handle_t h_key = 0;


    if (argc < 2) {
	fprintf (stderr, "Usage:  z7test <arg>\n");
	fprintf (stderr, "\nHint: ./z7test data/tables.xml\n");
	exit (1);
    }

    printf ("z7test \"%s\"\n", argv[1]);
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
	    xml_elemNameByHandle (top), name, ns);

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

    //**************************************************************************
    //***** #tables *******************************************************
    //**************************************************************************

    printf ("\n\n***** #tables *****\n\n");

    h_tableset = vosi_getTables (argv[1]);	// base handle (index = 0)
    assert (h_tableset > 0);	// defined handles are positive
    h = h_tableset;

    printf ("\n%d =vosi_numSchema( h )\n", vosi_numSchema (h));

    h_schema = vosi_getSchema (h);
    h = h_schema;
    assert (h > 0);
    printf ("\n%d =schemaP->index (should be 0)\n", vosi_schemaIndex (h));
    printf ("%s =schemaP->name\n", vosi_schemaName (h));
    printf ("%s =schemaP->title\n", vosi_schemaTitle (h));
    printf ("%s =schemaP->description\n", vosi_schemaDescription (h));
    printf ("%s =schemaP->utype\n", vosi_schemaUType (h));
    printf ("%d =vosi_numTable( h )\n", vosi_numTable (h));

    h = vosi_nextSchema (h);
    assert (h > 0);
    printf ("\n%d =schemaP->index (should be 1)\n", vosi_schemaIndex (h));
    printf ("%s =schemaP->name\n", vosi_schemaName (h));
    printf ("%s =schemaP->title\n", vosi_schemaTitle (h));
    printf ("%s =schemaP->description\n", vosi_schemaDescription (h));
    printf ("%s =schemaP->utype\n", vosi_schemaUType (h));
    printf ("%d =vosi_numTable( h )\n", vosi_numTable (h));

    h_schema = vosi_schemaByIndex (h_tableset, 2);
    h = h_schema;
    assert (h > 0);
    printf ("\n%d =schemaP->index (should be 2)\n", vosi_schemaIndex (h));
    printf ("%s =schemaP->name\n", vosi_schemaName (h));
    printf ("%s =schemaP->title\n", vosi_schemaTitle (h));
    printf ("%s =schemaP->description\n", vosi_schemaDescription (h));
    printf ("%s =schemaP->utype\n", vosi_schemaUType (h));
    printf ("%d =vosi_numTable( h )\n", vosi_numTable (h));

    h_table = vosi_getTable (h_schema);
    h = h_table;
    printf ("\n%s =tableP->name\n", vosi_tableName (h));
    printf ("%s =tableP->title\n", vosi_tableTitle (h));
    printf ("%s =tableP->description\n", vosi_tableDescription (h));
    printf ("%s =tableP->utype\n", vosi_tableUType (h));
    printf ("%d =tableP->index\n", vosi_tableIndex (h));

    h_table = vosi_nextTable (h);
    h = h_table;
    printf ("\n%s =tableP->name\n", vosi_tableName (h));
    printf ("%s =tableP->title\n", vosi_tableTitle (h));
    printf ("%s =tableP->description\n", vosi_tableDescription (h));
    printf ("%s =tableP->utype\n", vosi_tableUType (h));
    printf ("%d =tableP->index\n", vosi_tableIndex (h));

    h_table = vosi_tableByIndex (h_schema, 0);
    h = h_table;
    printf ("\n%s =tableP->name\n", vosi_tableName (h));
    printf ("%s =tableP->title\n", vosi_tableTitle (h));
    printf ("%s =tableP->description\n", vosi_tableDescription (h));
    printf ("%s =tableP->utype\n", vosi_tableUType (h));
    printf ("%d =tableP->index (should be 0)\n", vosi_tableIndex (h));

    printf ("\n%d =tableP->ncols\n", vosi_numCols (h));

    h_column = vosi_getColumn (h_table);
    h = h_column;

    printf ("\n%s =columnP->name\n", vosi_colName (h));
    printf ("%s =columnP->description\n", vosi_colDescription (h));
    printf ("%s =columnP->unit\n", vosi_colUnit (h));
    printf ("%s =columnP->ucd\n", vosi_colUCD (h));
    printf ("%s =columnP->utype\n", vosi_colUType (h));
    printf ("%s =columnP->datatype\n", vosi_colDatatype (h));
    printf ("%d =columnP->index (should be 0)\n", vosi_colIndex (h));

    h_column = vosi_nextColumn (h_column);
    h = h_column;

    printf ("\n%s =columnP->name\n", vosi_colName (h));
    printf ("%s =columnP->description\n", vosi_colDescription (h));
    printf ("%s =columnP->unit\n", vosi_colUnit (h));
    printf ("%s =columnP->ucd\n", vosi_colUCD (h));
    printf ("%s =columnP->utype\n", vosi_colUType (h));
    printf ("%s =columnP->datatype\n", vosi_colDatatype (h));
    printf ("%d =columnP->index (should be 1)\n", vosi_colIndex (h));

    h_column = vosi_colByIndex (h_table, 2);
    h = h_column;

    printf ("\n%s =columnP->name\n", vosi_colName (h));
    printf ("%s =columnP->description\n", vosi_colDescription (h));
    printf ("%s =columnP->unit\n", vosi_colUnit (h));
    printf ("%s =columnP->ucd\n", vosi_colUCD (h));
    printf ("%s =columnP->utype\n", vosi_colUType (h));
    printf ("%s =columnP->datatype\n", vosi_colDatatype (h));
    printf ("%d =columnP->index (should be 2)\n", vosi_colIndex (h));

    h_column = vosi_nextColumn (h_column);
    h = h_column;

    printf ("\n%s =columnP->name\n", vosi_colName (h));
    printf ("%s =columnP->description\n", vosi_colDescription (h));
    printf ("%s =columnP->unit\n", vosi_colUnit (h));
    printf ("%s =columnP->ucd\n", vosi_colUCD (h));
    printf ("%s =columnP->utype\n", vosi_colUType (h));
    printf ("%s =columnP->datatype\n", vosi_colDatatype (h));
    printf ("%d =columnP->index (should be 3)\n", vosi_colIndex (h));

    h_column = vosi_nextColumn (h_column);
    h = h_column;

    printf ("\n%s =columnP->name\n", vosi_colName (h));
    printf ("%s =columnP->description\n", vosi_colDescription (h));
    printf ("%s =columnP->unit\n", vosi_colUnit (h));
    printf ("%s =columnP->ucd\n", vosi_colUCD (h));
    printf ("%s =columnP->utype\n", vosi_colUType (h));
    printf ("%s =columnP->datatype\n", vosi_colDatatype (h));
    printf ("%d =columnP->index (should be 4)\n", vosi_colIndex (h));

    h_column = vosi_getColumn (h_table);
    h = h_column;

    printf ("\n%s =columnP->name\n", vosi_colName (h));
    printf ("%s =columnP->description\n", vosi_colDescription (h));
    printf ("%s =columnP->unit\n", vosi_colUnit (h));
    printf ("%s =columnP->ucd\n", vosi_colUCD (h));
    printf ("%s =columnP->utype\n", vosi_colUType (h));
    printf ("%s =columnP->datatype\n", vosi_colDatatype (h));
    printf ("%d =columnP->index (should be 0)\n", vosi_colIndex (h));

    //bingo
    printf ("\n%d =vosi_numSchema( h_tableset )\n",
	    vosi_numSchema (h_tableset));
    h_schema = vosi_getSchema (h_tableset);
    printf ("%d =vosi_numTable( h_schema )\n", vosi_numTable (h_schema));
    h_table = vosi_getTable (h_schema);
    printf ("%d =vosi_numCols( h_table )\n", vosi_numCols (h_table));
    h_column = vosi_getColumn (h_table);
    printf ("%d =vosi_numKeys( h_column )\n", vosi_numKeys (h_column));
    h_key = vosi_getKey (h_column);
    printf ("\n%s =keyP->targetTable\n", vosi_keyTargetTable (h_key));
    printf ("%s =keyP->fromColumn\n", vosi_keyFromColumn (h_key));
    printf ("%s =keyP->targetColumn\n", vosi_keyTargetColumn (h_key));
    printf ("%s =keyP->description\n", vosi_keyDescription (h_key));
    printf ("%s =keyP->utype\n", vosi_keyUType (h_key));
    h_key = vosi_nextKey (h_key);
    printf ("\n%s =keyP->targetTable\n", vosi_keyTargetTable (h_key));
    printf ("%s =keyP->fromColumn\n", vosi_keyFromColumn (h_key));
    printf ("%s =keyP->targetColumn\n", vosi_keyTargetColumn (h_key));
    printf ("%s =keyP->description\n", vosi_keyDescription (h_key));
    printf ("%s =keyP->utype\n", vosi_keyUType (h_key));

    //**************************************************************************
    //***** SHUTDOWN ***********************************************************
    //**************************************************************************

    printf ("\n\nThat's all folks!\n");

    return (0);
}
