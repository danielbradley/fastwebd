#include <stdio.h>

#include "libbase.h"
#include "libadt.h"
#include "libhttp.h"

static void    HTTP_validate( HTTPRequest* self );
static String* HTTPRequest_ReadLine( IO* connection    );

struct _HTTPHeader
{
	String* line;
	String* name;
	String* value;
};

struct _HTTPRequest
{
	bool    valid;
	String* startLine;
	String* method;
	String* resource;
	String* version;
	String* host;
	Array*  headers;
};

HTTPRequest*
HTTPRequest_new()
{
	HTTPRequest* self = New( sizeof( HTTPRequest ) );
	if ( self )
	{
		self->valid     = false;
		self->startLine = String_new( "" );
		self->method    = String_new( "" );
		self->resource  = String_new( "" );
		self->version   = String_new( "" );
		self->host      = String_new( "" );
		self->headers   = Array_new();
	}
	return self;
}

HTTPRequest*
HTTPRequest_free( HTTPRequest** self )
{
	if ( *self )
	{
		String_free( &(*self)->startLine );
		String_free( &(*self)->method    );
		String_free( &(*self)->resource  );
		String_free( &(*self)->version   );
		String_free( &(*self)->host      );
		Array_free_destructor( &(*self)->headers, (void* (*)( void** )) HTTPHeader_free );
	}

	return Delete( self );
}

bool
HTTPRequest_isValid( const HTTPRequest* self )
{
	return self->valid;
}

void
HTTPRequest_setStartLine( HTTPRequest* self, const char* startLine )
{
	String_free( &self->startLine );

	self->startLine = String_new( startLine );
}

void
HTTPRequest_setMethod( HTTPRequest* self, const char* method )
{
	String_free( &self->method );

	self->method = String_new( method );
}

void
HTTPRequest_setResource( HTTPRequest* self, const char* resource )
{
	String_free( &self->resource );

	self->resource = String_new( resource );
}

void
HTTPRequest_setVersion ( HTTPRequest* self, const char* version  )
{
	String_free( &self->version );

	self->version = String_new( version );
}

const String*
HTTPRequest_getStartLine( const HTTPRequest* self )
{
	return self->startLine;
}

const String*
HTTPRequest_getMethod( const HTTPRequest* self )
{
	return self->method;
}

const String*
HTTPRequest_getResource( const HTTPRequest* self )
{
	return self->resource;
}

const String*
HTTPRequest_getVersion ( const HTTPRequest* self )
{
	return self->version;
}

const String*
HTTPRequest_getHost    ( const HTTPRequest*  self )
{
	return self->host;
}

void
HTTPRequest_validate( HTTPRequest* self )
{
	self->valid =   String_contentEquals( self->method,   "GET"      )
	            &&  String_contentEquals( self->version,  "HTTP/1.1" )
	            && !String_contentEquals( self->host,     ""         )
	            && !String_contains     ( self->resource, ".."       );
}

HTTPRequest*
HTTPRequest_Parse( IO* connection )
{
	HTTPRequest* request = HTTPRequest_new();
	if ( 1 )
	{

		String* line = HTTPRequest_ReadLine( connection );

		if ( line )
		{
			int end_method   = String_indexOf_ch_skip( line, ' ', 0 );
			int end_resource = String_indexOf_ch_skip( line, ' ', 1 );
			int len          = end_resource - (end_method + 1);

			String* method   = String_substring_index_length( line,                0, end_method );
			String* resource = String_substring_index_length( line, end_method   + 1, len        );
			String* version  = String_substring_index       ( line, end_resource + 1             );
			{
				HTTPRequest_setStartLine( request, String_getChars( line     ) );
				HTTPRequest_setMethod   ( request, String_getChars( method   ) );
				HTTPRequest_setResource ( request, String_getChars( resource ) );
				HTTPRequest_setVersion  ( request, String_getChars( version  ) );
			}
			String_free( &method   );
			String_free( &resource );
			String_free( &version  );
		}
		String_free( &line );

		if ( 1 )
		{
			bool loop = true;
			do
			{
				if ( (line = HTTPRequest_ReadLine( connection )) )
				{
					int len = String_getLength( line );
					{
						HTTPHeader* header = HTTPHeader_new( &line );

						if ( String_contentEquals( header->name, "Host" ) )
						{
							int index;
							String_free( &request->host );

							if ( -1 == (index = String_indexOf_ch_skip( header->value, ':', 0 )) )
							{
								request->host = String_substring_index( header->value, 0 );
							}
							else
							{
								request->host = String_substring_index_length( header->value, 0, index );
							}
						}
						fprintf( stdout, "[%s][%s]\n", String_getChars( header->name ), String_getChars( header->value ) );

						Array_append_element( request->headers, (void**) &header );
					}
					if ( 0 == len ) loop = false;
				}
				else
				{
					loop = false;
				}
			} while ( loop );
		}

		HTTPRequest_validate( request );
	}
	return request;
}

String*
HTTPRequest_ReadLine( IO* connection )
{
	return IO_readline( connection );
}

static String* HTTPHeader_ExtractHeaderName ( const String* line );
static String* HTTPHeader_ExtractHeaderValue( const String* line );

HTTPHeader*
HTTPHeader_new( String** line )
{
	HTTPHeader* self = New( sizeof( HTTPHeader ) );
	if ( self )
	{
		self->line  = *line; *line = null;
		self->name  = HTTPHeader_ExtractHeaderName ( self->line );
		self->value = HTTPHeader_ExtractHeaderValue( self->line );
	}
	return self;
}

HTTPHeader*
HTTPHeader_free( HTTPHeader** self )
{
	if ( *self )
	{
		String_free( &(*self)->line  );
		String_free( &(*self)->name  );
		String_free( &(*self)->value );
	}
	return Delete( self );
}

String*
HTTPHeader_ExtractHeaderName( const String* line )
{
	//	012345678901234567890
	//	Host: 127.0.0.1:8080

	int index = String_indexOf_ch_skip( line, ':', 0 );

	return String_substring_index_length( line, 0, index );
}

String*
HTTPHeader_ExtractHeaderValue( const String* line )
{
	//	012345678901234567890
	//	Host: 127.0.0.1:8080

	const char* chx   = String_getChars       ( line         );
	      int   len   = String_getLength      ( line         );
	      int   index = String_indexOf_ch_skip( line, ':', 0 ) + 1;

	while ( (index < len) && (' ' == chx[index]) ) index++;

	return (index < len) ? String_substring_index( line, index ) : String_new( "" );
}

