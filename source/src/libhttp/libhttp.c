#include <stdio.h>

#include "libbase.h"
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
    bool    ip_target;
    String* startLine;
    String* method;
    String* resource;
    String* version;
    String* host;
    String* port;
    String* origin;
    String* forwardedFor;
    Array*  headers;
};

HTTPRequest*
HTTPRequest_new()
{
    HTTPRequest* self = New( sizeof( HTTPRequest ) );
    if ( self )
    {
        self->valid        = false;
        self->ip_target    = false;
        self->startLine    = String_new( "" );
        self->method       = String_new( "" );
        self->resource     = String_new( "" );
        self->version      = String_new( "" );
        self->host         = String_new( "" );
        self->port         = String_new( "" );
        self->origin       = String_new( "" );
        self->forwardedFor = String_new( "" );
        self->headers      = Array_new();
    }
    return self;
}

HTTPRequest*
HTTPRequest_free( HTTPRequest** self )
{
    if ( *self )
    {
        String_free( &(*self)->startLine    );
        String_free( &(*self)->method       );
        String_free( &(*self)->resource     );
        String_free( &(*self)->version      );
        String_free( &(*self)->host         );
        String_free( &(*self)->port         );
        String_free( &(*self)->origin       );
        String_free( &(*self)->forwardedFor );
        Array_free_destructor( &(*self)->headers, (void* (*)( void** )) HTTPHeader_free );
    }

    return Delete( self );
}

bool
HTTPRequest_isValid( const HTTPRequest* self )
{
    return self->valid;
}

bool
HTTPRequest_isIPTarget( const HTTPRequest*  self )
{
    return self->ip_target;
}

void
HTTPRequest_setStartLine( HTTPRequest* self, const char* startLine )
{
    String_free( &self->startLine );

    self->startLine = String_new( startLine );

    String_trimEnd( self->startLine );
}

void
HTTPRequest_setMethod( HTTPRequest* self, const char* method )
{
    String_free( &self->method );

    self->method = String_new( method );

    String_trimEnd( self->method );
}

void
HTTPRequest_setResource( HTTPRequest* self, const char* resource )
{
    String_free( &self->resource );

    self->resource = String_new( resource );

    String_trimEnd( self->resource );
}

void
HTTPRequest_setVersion ( HTTPRequest* self, const char* version  )
{
    String_free( &self->version );

    self->version = String_new( version );

    String_trimEnd( self->version );
}

void
HTTPRequest_setHost    ( HTTPRequest* self, const char* host )
{
    String_free( &self->host );

    self->host = String_new( host );

    String_trimEnd( self->host );
}

void
HTTPRequest_setPort    ( HTTPRequest* self, const char* port )
{
    String_free( &self->port );

    self->port = String_new( port );

    String_trimEnd( self->port );
}

void
HTTPRequest_setOrigin  ( HTTPRequest* self, const char* origin )
{
    String_free( &self->origin );

    self->origin = String_new( origin );

    String_trimEnd( self->origin );
}

void
HTTPRequest_setForwardedFor( HTTPRequest* self, const char* forwardedFor )
{
    String_free( &self->forwardedFor );

    self->forwardedFor = String_new( forwardedFor );

    String_trimEnd( self->forwardedFor );
}

const String*
HTTPRequest_getStartLine( const HTTPRequest* self )
{
    return self->startLine;
}

const String*
HTTPRequest_getMethod( const HTTPRequest*    self )
{
    return self->method;
}

const String*
HTTPRequest_getResource( const HTTPRequest*  self )
{
    return self->resource;
}

const String*
HTTPRequest_getVersion ( const HTTPRequest*  self )
{
    return self->version;
}

const String*
HTTPRequest_getHost    ( const HTTPRequest*  self )
{
    return self->host;
}

const String*
HTTPRequest_getPort    ( const HTTPRequest*  self )
{
    return self->port;
}

const String*
HTTPRequest_getOrigin  ( const HTTPRequest*  self )
{
    return self->origin;
}

const String*
HTTPRequest_getForwardedFor( const HTTPRequest*  self )
{
    return self->forwardedFor;
}

void
HTTPRequest_log_status( const HTTPRequest* self, const char* status, int statusNum )
{
    //
    //  [xff 192.168.1.1:12345] [client 192.168.1.162:53758] Duration: ms. GET host:/url/ status
    //61

    char   origin[1024];
    char      xff[1024];
    char   method[1024];
    char     host[1024];
    char resource[1024];
    char  logline[6144];

    snprintf(   origin, 1023, "[origin %s]", String_getChars( HTTPRequest_getOrigin      ( self ) ) );
    snprintf(      xff, 1023, "[xff %s]",    String_getChars( HTTPRequest_getForwardedFor( self ) ) );
    snprintf(   method, 1023, "%5s",         String_getChars( HTTPRequest_getMethod      ( self ) ) );
    snprintf(     host, 1023, "%s",          String_getChars( HTTPRequest_getHost        ( self ) ) );
    snprintf( resource, 1023, "%s",          String_getChars( HTTPRequest_getResource    ( self ) ) );

    if ( 0 < String_getLength( HTTPRequest_getForwardedFor( self ) ) )
    {
        snprintf( logline, 6143, "[%3i] %s   %s %s %s%s", statusNum, origin, xff, method, host, resource );
    }
    else
    {
        snprintf( logline, 6143, "[%3i] %-24s %s %s%s", statusNum, origin, method, host, resource );
    }

    SysLog_Log_chars( logline );
}

void
HTTPRequest_validate( HTTPRequest* self )
{
    self->valid =   
    (
        String_contentEquals( self->method,   "POST"     )
        ||
        String_contentEquals( self->method,   "GET"      )
        ||
        String_contentEquals( self->method,   "HEAD"     )
        ||
        String_contentEquals( self->method,   "OPTIONS"  )
    )
    &&
    (
        String_contentEquals( self->version,  "HTTP/1.0" )
        ||
        String_contentEquals( self->version,  "HTTP/1.1" )
    )
    && !String_contentEquals( self->host,     ""         )
    && !String_contains     ( self->resource, ".."       );
}

HTTPRequest*
HTTPRequest_Parse( const Address* peer, IO* connection, const String* localDomain )
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

            String* origin   = Address_origin               ( peer                               );
            String* method   = String_substring_index_length( line,                0, end_method );
            String* resource = String_substring_index_length( line, end_method   + 1, len        );
            String* version  = String_substring_index       ( line, end_resource + 1             );
            {
                HTTPRequest_setStartLine( request, String_getChars( line     ) );
                HTTPRequest_setMethod   ( request, String_getChars( method   ) );
                HTTPRequest_setResource ( request, String_getChars( resource ) );
                HTTPRequest_setVersion  ( request, String_getChars( version  ) );
                HTTPRequest_setOrigin   ( request, String_getChars( origin   ) );
            }
            String_free( &origin   );
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
                    String_trimEnd( line );
                    {
                        int len = String_getLength( line );
                        {
                            HTTPHeader* header = HTTPHeader_new( &line );

                            if ( String_contentEquals( header->name, "Host" ) )
                            {
                                const String* value = HTTPHeader_getValue( header );

                                if ( String_contains( value, ":" ) )
                                {
                                    int index = String_indexOf_ch_skip( value, ':', 0 );
                                    String* host = String_substring_index_length( value,     0, index );
                                    String* port = String_substring_index       ( value, index        );
                                    {
                                        HTTPRequest_setHost( request, String_getChars( host ) );
                                        HTTPRequest_setPort( request, String_getChars( port ) );
                                        {
                                            int     dot_index  = String_indexOf_ch_skip( host, '.', 0 );
                                            String* first_quad = String_substring_index_length( host, 0, dot_index );

                                            if ( String_isNumeric( first_quad ) )
                                            {
                                                int number = String_toNumber( first_quad );

                                                if ( number < 255 ) request->ip_target = true;

                                                if ( localDomain )
                                                {
                                                    HTTPRequest_setHost( request, String_getChars( localDomain ) );
                                                    request->ip_target = false;
                                                }
                                            }
                                            String_free( &first_quad );
                                        }
                                    }
                                    String_free( &host );
                                    String_free( &port );
                                }
                                else
                                {
                                    HTTPRequest_setHost( request, String_getChars( value ) );
                                }
                            }
                            else
                            if ( String_contentEquals( header->name, "Origin" ) )
                            {
                                const String* value = HTTPHeader_getValue( header );

                                HTTPRequest_setOrigin( request, String_getChars( value ) );
                            }
                            else
                            if ( String_contentEquals( header->name, "X-Forwarded-For" ) )
                            {
                                const String* value = HTTPHeader_getValue( header );

                                HTTPRequest_setForwardedFor( request, String_getChars( value ) );
                            }

                            //printf( stdout, "[%s][%s]\n", String_getChars( header->name ), String_getChars( header->value ) );

                            Array_append_element( request->headers, (void**) &header );
                        }
                        if ( 0 == len ) loop = false;
                    }
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

const String*
HTTPHeader_getName( const HTTPHeader* self )
{
    return self->name;
}

const String*
HTTPHeader_getValue( const HTTPHeader* self )
{
    return self->value;
}

String*
HTTPHeader_ExtractHeaderName( const String* line )
{
    //  012345678901234567890
    //  Host: 127.0.0.1:8080

    if ( String_getLength( line ) )
    {
        int index = String_indexOf_ch_skip( line, ':', 0 );

        return String_substring_index_length( line, 0, index );
    }
    else
    {
        return String_new( "" );
    }
}

String*
HTTPHeader_ExtractHeaderValue( const String* line )
{
    //  012345678901234567890
    //  Host: 127.0.0.1:8080

    if ( String_getLength( line ) )
    {
        const char* chx   = String_getChars       ( line         );
              int   len   = String_getLength      ( line         );
              int   index = String_indexOf_ch_skip( line, ':', 0 ) + 1;

        while ( (index < len) && (' ' == chx[index]) ) index++;

        return (index < len) ? String_substring_index( line, index ) : String_new( "" );
    }
    else
    {
        return String_new( "" );
    }
}

