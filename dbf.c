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
// http://www.clicketyclick.dk/databases/xbase/format/dbf.html#DBF_NOTE_15_SOURCE


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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef unsigned char byte;
typedef void * HANDLE;

/*
// structs
typedef struct dbfIIheader		// supports dbase II
{
	byte[1] version[1];
	byte[2] numberofrecords;
	byte[3] lastupdate;
	byte[2] recordlength;
	// array of field descriptors
	// single byte terminator
	// array of records
} dbfheader;

typedef struct dbfIIfield
{
	byte[10] fieldname;
	byte     fieldtype;
	byte     fieldlength;
	byte[2]  fieldaddress;
	byte     fieldcount;
}
*/

typedef struct dbfIIIheader 	// supports dbase III - V
{
	byte version[1];
	byte lastupdate[3];
	byte numberofrecords[4];
	byte headerlength[2];		// record data starts after this
	byte recordlength[2];		// record length
	byte reserved1[2];
	byte incompletetransaction[1];
	byte encrypted[1];
	byte lanfreerecord[4];
	byte reserved2[8];
	byte mdx[1];
	byte language[1];
	byte reserved3[2];
	// array of field descriptors
	// single byte terminator
	// array of records
} dbfIIIheader;

typedef struct dbfIIIfield
{
	byte fieldname[11];
	byte fieldtype;
	byte fieldaddress[4];		// do not use
	byte fieldlength;
	byte fieldecimalcount;
	byte reserved1[2];
	byte workareaid[1];
	byte reserved2[2];
	byte setfields;
	byte reserved3[7];
	byte indexflag;
} dbfIIIfield;

// HANDLE dbfinit( byte * mem )
// map file into memory and call this to initialize and read
// field definitions etc.
HANDLE dbfinit(byte * mem)
{
	// just poiint the handle at memory
	HANDLE handle = 0;
	switch( *(byte *)mem )
	{
		case 3:
		case 4:
		case 5:
			handle = mem;
			break;
		default:
			printf("Unsupported DBF version: %0x\n",*mem);
			handle = 0;
			exit(1);
	}
	return handle;
}

// dbfIIIfield ** getcolumns( HANDLE )
// get all the fields, NULL terminated array
// can use to populate list of columns and types
dbfIIIfield ** dbfgetcolumns( HANDLE handle )
{
	// check handle is known type
	byte * mem = (byte *)handle;
	if( *mem<3 || *mem>5 ) {
		// error, unsupported
		printf("Unknown DBF version: %0x\n",*mem);
		exit(1);
	}
	// count how many pointers will we need
	int columns = 0;
	byte * pos = mem+sizeof(dbfIIIheader);
	while( *pos!=0 && *pos!=0xD )
		columns++, pos+=sizeof(dbfIIIfield);
	// grab some memory to return pointers, include guard null
	dbfIIIfield ** array = (dbfIIIfield**)malloc((columns+1)*sizeof(dbfIIIfield*));
	// copy pointers
	pos = mem+sizeof(dbfIIIheader);
	for(int i=0;i<columns;++i) {
		array[i] = (dbfIIIfield*)(pos+i*sizeof(dbfIIIfield));
	}
	array[columns] = (dbfIIIfield*)0;
	return array;
}

// byte * getrawcolumn( HANDLE handle )
// returns a pointer to an array of requested column
// assuming just a few columns are needed, very fast just to blit the data.
byte * dbfgetrawcolumn( HANDLE handle,int col, int *collen, int *count )
{
	// check handle is known type
	byte * mem = (byte *)handle;
	if( *mem<3 || *mem>5 ) {
		// error, unsupported
		printf("Unknown DBF version: %0x\n",*mem);
		exit(1);
	}
	// get record count
	dbfIIIheader * header = (dbfIIIheader *)mem;
	*count = *(int*)&header->numberofrecords;
	// get header length
	int headerlength = *(unsigned short*)&header->headerlength;
	// get record length
	int recordlength = *(unsigned short*)&header->recordlength;
	// find where in the record we need to copy from
	int colnum = 0;
	int recordoff = 1;
	byte * pos = mem+sizeof(dbfIIIheader);
	dbfIIIfield * field = (dbfIIIfield*)pos;
	while(*pos!=0 && *pos!=0xD && colnum<col) {
		printf("col: %d ",colnum);
		colnum++;
		field = (dbfIIIfield*)pos;
		recordoff += field->fieldlength;
		printf("width: %d cum: %d\n",field->fieldlength,recordoff);
		pos+=sizeof(dbfIIIfield);
	}
	// create data
	field = (dbfIIIfield*)pos;
	*collen = field->fieldlength;
	byte * data = (byte *)malloc(*count**collen);
	byte * datapos = data;
	// step through and blit data
	pos = mem+headerlength;
	for(int i=0;i<*count;++i) {
		memcpy(datapos,pos+recordoff,*collen);
		pos += recordlength;
		datapos += *collen;
	}
	return data;
}

// test the api
int main( int argc, char * argv[] )
{
	// debugging
	printf("sizeof(dbfIIIheader): %ld\n",sizeof(dbfIIIheader));
	printf("sizeof(dbfIIIfield): %ld\n",sizeof(dbfIIIfield));
	
	// usage
	if( argc<2 ) {
		printf("usage: %s [-c] [-r name] [-v] file\n  -c lists the column names\n  -r lists all rows for named column\n  -v dump as csv.\n",argv[0]);
		exit(1);
	}
	
	// parse command line args
	
	// get filename
	char * filename = "/Users/jonathan/Development/ShapeFiles/TM_WORLD_BORDERS-0.3.dbf";
	
	// memory map filename
	int fd = open(filename,O_RDONLY);
	byte * mem = mmap(0,64*1024,PROT_READ,MAP_SHARED,fd,0);
	
	// initialize
	HANDLE handle = dbfinit(mem);
	{
		// list columns
		dbfIIIfield ** columns = dbfgetcolumns(handle);
		for(int i=0;columns[i]!=0;++i) {
			dbfIIIfield * field = columns[i];
			printf("%s\n",field->fieldname);
		}
		
	}
	
	{
		// list named column
		int colnum = atoi(argv[argc-1]);
		int collen = 0;
		int count  = 0;
		byte * data = dbfgetrawcolumn(handle,colnum,&collen,&count);
		for(int i=0;i<count;++i) {
			printf("row: %d col: %.*s\n",i,collen,data+i*collen);
		}
	}
	
	{
		// dump as csv file
	}
}






























