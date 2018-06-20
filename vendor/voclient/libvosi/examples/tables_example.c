// 2014SEP18 KJM

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "vosi.h"

int main( int argc, char **argv)
{
  handle_t h_schema = 0;
  handle_t h_table = 0;
  handle_t h_tableset = 0;
  handle_t h_column = 0;
  handle_t h_key = 0;

  if (argc < 2) {
    fprintf (stderr, "\nUsage:  tables_example <arg>\n");
    fprintf (stderr, "\nExample: tables_example data/tables.xml\n\n");
    exit (1);
  }

  printf ("# %s \"%s\"\n", argv[0], argv[1]); // show the command

  // open the XML document and return base handle 
  h_tableset = vosi_getTables( argv[1] ); 
  // defined handles are positive
  assert( h_tableset > 0 ); 

  printf( "\n%d =numSchema\n", vosi_numSchema( h_tableset ) );

  for ( h_schema = vosi_getSchema( h_tableset );
	h_schema;
	h_schema = vosi_nextSchema( h_schema ) )
  {
    printf( "\nschema ##################################################\n" );
    printf( "%d =schemaIndex\n", vosi_schemaIndex( h_schema ) );
    printf( "%s =schemaName\n", vosi_schemaName( h_schema ) );
    vosi_printk( "%s =schemaTitle\n", vosi_schemaTitle( h_schema ) );
    vosi_printk( "%s =schemaDescription\n", vosi_schemaDescription( h_schema ) );
    vosi_printk( "%s =schemaUType\n", vosi_schemaUType( h_schema ) );
    printf( "%d =numTable\n", vosi_numTable( h_schema ) );
    //
    for ( h_table = vosi_getTable( h_schema );
	  h_table;
	  h_table = vosi_nextTable( h_table ) )
      {
	printf( "\n***** table *************************************\n" );
	printf( "%d =schemaIndex\n", vosi_schemaIndex( h_schema ) );
	printf( "%d =tableIndex\n", vosi_tableIndex( h_table ) );
	printf( "%s =tableName\n", vosi_tableName( h_table ) );
	vosi_printk( "%s =tableTitle\n", vosi_tableTitle( h_table ) );
	vosi_printk( "%s =tableDescription\n", vosi_tableDescription( h_table ) );
	vosi_printk( "%s =tableUType\n", vosi_tableUType( h_table ) );
	printf( "%d =numCols\n", vosi_numCols( h_table ) );
	//
	for ( h_column = vosi_getColumn( h_table );
	      h_column;
	      h_column = vosi_nextColumn( h_column ) )
	  {
	    printf( "========== column ====================\n" );
	    printf( "%d =colIndex\n", vosi_colIndex( h_column ) );
	    printf( "%s =colName\n", vosi_colName( h_column ) );
	    vosi_printk( "%s =colDescription\n", vosi_colDescription( h_column ) );
	    vosi_printk( "%s =colUnit\n", vosi_colUnit( h_column ) );
	    vosi_printk( "%s =colUCD\n", vosi_colUCD( h_column ) );
	    vosi_printk( "%s =colUType\n", vosi_colUType( h_column ) );
	    vosi_printk( "%s =colDatatype\n", vosi_colDatatype( h_column ) );
	    printf( "%d =numKeys\n", vosi_numKeys( h_column ) );
	    //
	    for ( h_key = vosi_getKey( h_column );
		  h_key;
		  h_key = vosi_nextKey( h_key ) )
	      {
		printf( "--------------- key ----------\n" );
		printf( "%d =keyIndex\n", vosi_keyIndex( h_key ) );
		vosi_printk( "%s =keyTargetTable\n", vosi_keyTargetTable( h_key ) );
		vosi_printk( "%s =keyFromColumn\n", vosi_keyFromColumn( h_key ) );
		vosi_printk( "%s =keyTargetColumn\n", vosi_keyTargetColumn( h_key ) );
		vosi_printk( "%s =keyDescription\n", vosi_keyDescription( h_key ) );
		vosi_printk( "%s =keyUType\n", vosi_keyUType( h_key ) );
	      } // h_key
	  } // h_column
      } // h_table
  } // h_schema
  printf( "\n\nThat's all folks!\n\n" );
  return (0);
}

