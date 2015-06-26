////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// References:
//
// DBF Specification and considerations
// http://www.digitalpreservation.gov/formats/fdd/fdd000325.shtml
// http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf (see page 25)
// http://webhelp.esri.com/arcgisdesktop/9.3/index.cfm?TopicName=Geoprocessing_considerations_for_shapefile_output
// http://www.clicketyclick.dk/databases/xbase/format/db2_dbf.html#DBII_DBF_STRUCT


////////////////////////////////////////////////////////////////////////////////
// overall design
// this is the barest of bare-bones apis
// fields are all fixed length, and we know how many rows we have.
// copy field data verbatim
//
// Reading Interface
//------------------
//
// HANDLE initdbf( byte * )
// map file into memory and call this to initialize and read
// field definitions etc.
//
// dbfield ** getcolumns( HANDLE )
// get all the fields, NULL terminated array
// can use to populate list of columns and types
//
// void * getrawcolumn( byte column )
// returns a pointer to an array of requested column
// assuming just a few columns are needed, very fast just to blit the data.
//
// char   *getcharactercolumn( byte column )
// double *getnumericcolumn( byte column )
// char   *getdatecolumn( byte column )
// char   *gettimecolumn( byte column )
// simple type wrappers around void *getrawcolumn()
//
// Writing Interface
//------------------
// write version
// write number of records
// write last update
// write record length
// write each field specifier
// write terminator
// write records


typedef unsigned char byte;

// structs
typedef struct dbfheader 
{
	byte[1] version;
	byte[2] numberofrecords;
	byte[3] lastupdate;
	byte[2] recordlength;
	// array of field descriptors
	// single byte terminator
	// array of records
} dbfheader;

typedef struct dbffield
{
	byte[10] fieldname;
	byte     fieldtype;
	byte     fieldlength;
	byte[2]  fieldaddress;
	byte     fieldcount;
}

































