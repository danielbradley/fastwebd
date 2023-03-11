#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>
#include <sys/wait.h>

#include "libbase.h"
#include "libhttp.h"
#include "libhttpserver.h"

static bool Debug = false;

static void         HTTPServer_Process_srvDir_peer_connection_defaultDomain( const Path*  srvDir,  const Address*     peer,              IO*   connection, const String* defaultDomain );
static ArrayOfFile* HTTPServer_DetermineFiles__srvDir_request              ( const Path*  srvDir,  const HTTPRequest* request                                                          );
static void         HTTPServer_DiscoverFilesFrom_siteDir_resources_files   ( const Path*  siteDir, const String*      resource,                            ArrayOfFile* const files    );
File*               JuxtaPage_FindFile_siteDir_jxPathParts_filename        ( const Path* siteDir,  const Array*       jxPathParts, const char* filename                                );
void                JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files  ( const Path* siteDir,  const Array*       jxPathParts, const char* pattern,    ArrayOfFile* const files    );

static IO*     ServerSocket;
static bool    KeepAlive = true;

typedef void (*sig_t) (int);

static sig_t signal1  = null;
static sig_t signal2  = null;
static sig_t signal13 = null;

void signalHandler( int signal )
{
    KeepAlive = false;

    IO_close( ServerSocket );
}

void ignoreSigpipe( int signal )
{
    if ( Debug ) printf( "Signal: %i - ignoring SIGPIPE\n", signal );
}

struct _HTTPServer
{
    short    port;
    Address* loopback;
    IO*      socket;
    String*  defaultDomain;
};

HTTPServer*
HTTPServer_new_port( short port )
{
    HTTPServer* self = New( sizeof( HTTPServer ) );
    {
        self->port          = port;
        self->loopback      = Address_new_port( port );
        self->socket        = IO_Socket();
        self->defaultDomain = null;
    }

    signal1  = signal(  1, signalHandler );
    signal2  = signal(  2, signalHandler );
    signal13 = signal( 13, ignoreSigpipe );

    ServerSocket = self->socket;

    return self;
}

HTTPServer*
HTTPServer_free( HTTPServer** self )
{
    if ( *self )
    {
        (*self)->port = 0;
        Delete      ( &(*self)->loopback      );
        IO_free     ( &(*self)->socket        );
        String_free ( &(*self)->defaultDomain );
    }
    return Delete( self );
}

bool
HTTPServer_listen_wait( HTTPServer* self, int wait )
{
    bool status = false;

    if ( IO_bind_address_wait( self->socket, self->loopback, wait ) )
    {
        printf( "Bound socket to any\n" );

        Security_DropPrivilegesOrAbort();

        if ( IO_listen( self->socket ) )
        {
            printf( "Listening on port: %i\n", self->port );
            status = true;
        }
    }
    return status;
}

void
HTTPServer_acceptConnections( HTTPServer* self )
{
    bool     use_fork   = true;
    Address* peer       = Address_new_port( 0 );
    IO*      connection = NULL;
    Path*    srvDir     = Path_CurrentDirectory();

    while ( KeepAlive )
    {
        if ( IO_accept( self->socket, peer, &connection ) )
        {
            if ( use_fork )
            {
                if ( Platform_Fork() )
                {
                    Platform_Wait();
                }
                else
                {
                    HTTPServer_Process_srvDir_peer_connection_defaultDomain( srvDir, peer, connection, self->defaultDomain );
                    KeepAlive = false;
                }
            }
            else
            {
                HTTPServer_Process_srvDir_peer_connection_defaultDomain( srvDir, peer, connection, self->defaultDomain );
            }
            IO_free( &connection );
        }
    }

    Path_free( &srvDir );
    Delete( &peer );
}

bool
HTTPServer_hasLocalDomain( HTTPServer* self )
{
    return (null != self->defaultDomain);
}

void
HTTPServer_setDefaultDomain( HTTPServer*  self, const String* defaultDomain )
{
    if ( defaultDomain )
    {
        String_free( &self->defaultDomain );
        self->defaultDomain = String_substring_index( defaultDomain, 0 );
    }
}

void
HTTPServer_Panic()
{
    fprintf( stderr, "Panic! Could not allocate memory for HTTP server.\n" );
}

void
HTTPServer_InvalidPort()
{
    fprintf( stderr, "Error! Could not bind to privileged or in use port.\n" );
}

void
HTTPServer_Process_srvDir_peer_connection_defaultDomain( const Path* srvDir, const Address* peer, IO* connection, const String* defaultDomain )
{
    if ( Debug ) fprintf( stdout, "START\n" );

    /*
     *  Kludge to stop Safari from crashing.
     */
    //Platform_MilliSleep( 10 );

    HTTPRequest* request = HTTPRequest_Parse( peer, connection, defaultDomain );

    if ( 1 )
    {
        const String* start_line = HTTPRequest_getStartLine( request );
        const String* host       = HTTPRequest_getHost     ( request );
        const String* port       = HTTPRequest_getPort     ( request );
        const String* method     = HTTPRequest_getMethod   ( request );
        const String* origin     = HTTPRequest_getOrigin   ( request );

        if ( Debug )
        {
            fprintf( stdout, "Request: %s\n", String_getChars( start_line ) );
            fprintf( stdout, "Method:  %s\n", String_getChars( method     ) );
            fprintf( stdout, "Host:    %s\n", String_getChars( host       ) );
            fprintf( stdout, "Port:    %s\n", String_getChars( port       ) );
            fprintf( stdout, "Origin:  %s\n", String_getChars( origin     ) );
        }

        if ( 0 )
        {}
        else
        if ( !HTTPRequest_isValid( request ) )
        {
            const char* status = "HTTP/1.0 400 BAD_REQUEST \r\n";
            const char* end    = "\r\n";

            IO_write( connection, status );
            IO_write( connection, end    );

            if ( Debug ) fprintf( stdout, "Response: %s\n", status );

            HTTPRequest_log_status( request, status, 400 );
        }
        else
        if ( HTTPRequest_isIPTarget( request ) )
        {
            const char* status = "HTTP/1.0 202 Accepted \r\n";
            const char* end    = "\r\n";

            IO_write( connection, status );
            IO_write( connection, end    );

            if ( Debug ) fprintf( stdout, "Response: %s\n", status );

            HTTPRequest_log_status( request, status, 202 );
        }
        else
        if ( 1 )
        {
            ArrayOfFile* files = HTTPServer_DetermineFiles__srvDir_request( srvDir, request );

            if ( 0 )
            {}
            else
            if ( !files || !ArrayOfFile_count( files ) )
            {
                const char* status = "HTTP/1.0 404 ERROR \r\n";
                const char* end    = "\r\n";

                //fprintf( stdout, "Response: %s (%s)", status, String_getChars( File_getFilePath( file ) ) );
                IO_write( connection, status );
                IO_write( connection, end    );

                if ( Debug ) fprintf( stdout, "Response: %s\n", status );

                HTTPRequest_log_status( request, status, 404 );
            }
            else
            if ( String_contentEquals( method, "OPTIONS" ) )
            {
                StringBuffer* headers = StringBuffer_new();
                {
                    const char* status = "HTTP/1.1 204 No Content \r\n";
                    const char* end    = "\r\n";

                    StringBuffer_append_chars( headers, "Connection: close\r\n"                                              );
                    StringBuffer_append_chars( headers, "Access-Control-Allow-Origin: "                                      );
                    StringBuffer_append_chars( headers, String_getChars( origin )                                            );
                    StringBuffer_append_chars( headers, end                                                                  );
                    StringBuffer_append_chars( headers, "Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE, HEAD\r\n" );
                    StringBuffer_append_chars( headers, "Access-Control-Allow-Private-Network: true\r\n"                     );
                    StringBuffer_append_chars( headers, "Access-Control-Max-Age: 86400\r\n"                                  );

                    IO_write( connection, status                           );
                    IO_write( connection, StringBuffer_getChars( headers ) );
                    IO_write( connection, end                              );

                    if ( Debug ) fprintf( stdout, "Options Response: %s", status                           );
                    if ( Debug ) fprintf( stdout, "Options Response: %s", StringBuffer_getChars( headers ) );

                    HTTPRequest_log_status( request, status, 204 );
                }
                StringBuffer_free( &headers );
            }
            else
            if ( 1 )
            {
                StringBuffer* headers = StringBuffer_new();
                {
                    const char* status = "HTTP/1.0 200 OK \r\n";
                    const char* end    = "\r\n";

                    const File* file           = ArrayOfFile_get_index( files, 0 );
                    const char* mime_type      = File_getMimeType( file );
                    int         content_length = ArrayOfFile_sizeOfFiles( files );

                    StringBuffer_append_chars ( headers, "Content-Type: "                   );
                    StringBuffer_append_chars ( headers, mime_type                          );
                    StringBuffer_append_chars ( headers, end                                );
                    StringBuffer_append_chars ( headers, "Content-Length: "                 );
                    StringBuffer_append_number( headers, content_length                     );
                    StringBuffer_append_chars ( headers, end                                );

                    if ( String_getLength( origin ) )
                    {
                        StringBuffer_append_chars( headers, "Access-Control-Allow-Origin: " );
                        StringBuffer_append_chars( headers, String_getChars( origin )       );
                        StringBuffer_append_chars( headers, end                             );
                    }

                    StringBuffer_append_chars( headers, "Access-Control-Allow-Methods: POST, GET, HEAD, PUT, OPTIONS, PATCH, DELETE\r\n" );
                    StringBuffer_append_chars( headers, "Access-Control-Allow-Credentials: false\r\n" );
                    StringBuffer_append_chars( headers, "Connection: Close\r\n" );
                    StringBuffer_append_chars ( headers, end );

                    const char* _headers = StringBuffer_getChars( headers );

                    IO_write( connection,  status  );
                    IO_write( connection, _headers );

                    if ( !String_contentEquals( method, "HEAD" ) )
                    {
                        File_open( (File*) file );
                        IO_sendFile( connection, File_getIO( file ) );
                        File_close( (File*) file );

                        int n = ArrayOfFile_count( files );

                        for ( int i=1; i < n; i++ )
                        {
                            const File* file = ArrayOfFile_get_index( files, i );
                            File_open( (File*) file );
                            IO_sendFile( connection, File_getIO( file ) );
                            File_close( (File*) file );

                            if ( Debug )
                            {
                                const char* _filepath = String_getChars( File_getFilePath( file ) );
                                int         _bytesize = File_getByteSize( file );

                                fprintf( stdout, "Returning file length: %i, name: %s\n", _bytesize, _filepath );
                            }
                        }
                    }

                    if ( Debug ) fprintf( stdout, "Response: %s", status                           );
                    if ( Debug ) fprintf( stdout, "Response: %s", StringBuffer_getChars( headers ) );

                    HTTPRequest_log_status( request, status, 200 );
                }
                StringBuffer_free( &headers );
            }
            ArrayOfFile_free( &files );
        }
        if ( Debug ) fprintf( stdout, "END\n" );
        fflush( stdout );
    }
    HTTPRequest_free( &request );

    if ( Debug ) MemInfo();
}

static
Path*
GenerateAltPath( const Path* site_dir, const String* resource )
{
    Path* alt_path = null;

    if ( String_startsWith( resource, "/resources/" ) )
    {
        //   01234567890
        //  "/resources/"
        //
        String* real_resource = String_substring_index( resource, 10 );
        StringBuffer* buffer  = StringBuffer_new();
        {
            StringBuffer_append_chars ( buffer, "/_resources/" );
            StringBuffer_append_string( buffer, real_resource  );

            alt_path = Path_child( site_dir, StringBuffer_getChars( buffer ) );
        }
        String_free      ( &real_resource );
        StringBuffer_free( &buffer        );
    }
    return alt_path;
}

ArrayOfFile*
HTTPServer_DetermineFiles__srvDir_request( const Path* srvDir, const HTTPRequest* request )
{
    ArrayOfFile* files = ArrayOfFile_new();

    const char*  _srv          = Path_getAbsolute             ( srvDir        );
    const String* host         = HTTPRequest_getHost          ( request       );
    const char*  _host         = String_getChars              ( host          );
    const String* resource     = HTTPRequest_getResource      ( request       );
    const char*  _resource     = String_getChars              ( resource      );
    int           len          = String_getLength             ( resource      );
    String*       reverse_host = String_reverseParts_separator( host, '.'     );
    const char*  _reverse_host = String_getChars              ( reverse_host  );

    if ( Debug )
    {
        fprintf( stdout, "host:     %s --> %s\n", _host, _reverse_host            );
        fprintf( stdout, "srv:      %s\n",        _srv                            );
        fprintf( stdout, "resource: %s\n",        _resource                       );
        fprintf( stdout, "target:   %s/%s%s\n",   _srv, _reverse_host, _resource );
    }

    if ( 1 )
    {
        Path* site_dir = Path_child( srvDir, _reverse_host );

        if ( 1 )
        {
            Path* alt  = GenerateAltPath( site_dir,  resource );
            Path* path = Path_child     ( site_dir, _resource );

            if ( 1 )
            {
                if ( '/' == _resource[len - 1] )
                {
                    HTTPServer_DiscoverFilesFrom_siteDir_resources_files( site_dir, resource, files );

                    if ( 0 == ArrayOfFile_count( files ) )
                    {
                        ArrayOfFile_free( &files );
                    }
                }
                else
                if ( 1 )
                {
                    File* file = File_new( Path_getAbsolute( path ) );
                    if ( File_exists( file ) )
                    {
                        ArrayOfFile_append_file( files, &file );
                    }
                    else
                    {
                        File* alt_file = alt ? File_new( Path_getAbsolute( alt ) ) : null;
                        if ( alt_file && File_exists( alt_file ) )
                        {
                            ArrayOfFile_append_file( files, &alt_file );
                        }
                        else
                        {
                            File_free( &alt_file );
                            ArrayOfFile_free( &files );
                        }
                        File_free( &file );
                    }
                }
            }
            Path_free  ( &alt  );
            Path_free  ( &path );
        }
        Path_free( &site_dir );
    }
    String_free( &reverse_host );

    return files;
}

void
HTTPServer_DiscoverFilesFrom_siteDir_resources_files( const Path* siteDir, const String* resource, ArrayOfFile* const files )
{
    File* f;

    if ( (f = File_CreateIfExists_path( (Path**) Give( Path_child( siteDir, "index.html" ) ) )) )
    {
        ArrayOfFile_append_file( files, &f );
    }
    else
    if ( 1 )
    {
        //  resource = "/"                  --> "/_content/_index/
        //           = "/somepage/"         --> "/_content/somepage/"
        //           = "/somepage/subpage/" --> "/_content/somepage-subpage/"
        //
        //
        //

        Array* jx_path_parts = String_toArray_separator( resource, '/' );
        if ( 1 )
        {
            if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "document.htm" )) )
            {
                ArrayOfFile_append_file( files, &f );
            }
            else
            {
                if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "html_start.htm" )) )
                {
                    ArrayOfFile_append_file( files, &f );
                }

                if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "head.htm" )) )
                {
                    ArrayOfFile_append_file( files, &f );
                }
                else
                {
                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "head_start.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "title.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "meta.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "csp.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "link.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "link?.htm", files );

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "styles.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "styles?.htm", files );

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "script.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "script?.htm", files );

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "javascript.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "javascript?.htm", files );

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "head_end.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                }

                if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "body.htm" )) )
                {
                    ArrayOfFile_append_file( files, &f );
                }
                else
                {
                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "body_start.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "main.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    else
                    {
                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "main_start.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }

                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "aside.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }

                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "article.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }

                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "main_end.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "footer.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "nav.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    else
                    {
                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "nav_start.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }

                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "breadcrumbs.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }

                        JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "nav?.htm", files );

                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "nav_end.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "header.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "menu.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    else
                    {
                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "menu_start.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }

                        JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "menu?.htm", files );

                        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "menu_end.htm" )) )
                        {
                            ArrayOfFile_append_file( files, &f );
                        }
                    }

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "dialogs.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                    JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( siteDir, jx_path_parts, "dialogs?.htm", files );

                    if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "body_end.htm" )) )
                    {
                        ArrayOfFile_append_file( files, &f );
                    }
                }

                if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jx_path_parts, "html_end.htm" )) )
                {
                    ArrayOfFile_append_file( files, &f );
                }
            }
        }
        Array_setFree( jx_path_parts, (void *(*)(void **)) String_free );
        Delete( &jx_path_parts );
    }
}

File* JuxtaPage_FindFile_siteDir_jxPathParts_filename( const Path* siteDir, const Array* jxPathParts, const char* filename )
{
    File* f = null;

    Path*   content_dir   = Path_child( siteDir, "_content" );
    {
        if ( 0 == Array_count( jxPathParts ) )
        {
            //                     |
            //  www/test.juxtapage/_content/_index

            Path* index_dir = Path_child( siteDir, "_content/_index" );
            {
                Path* trial_path = Path_child( index_dir, filename );

                f = File_CreateIfExists_path( &trial_path );

                if ( f )
                {
                    Path* trial_path = Path_child( index_dir, filename );
                    if ( Debug ) fprintf( stderr, "Found: %20s --> %s\n", filename, Path_getAbsolute( trial_path ) );
                    Path_free( &trial_path );
                }
            }
            Path_free( &index_dir );
        }

        int number = Array_count( jxPathParts );

        while ( (0 < number) && !f )
        {
            String* target_name = Array_joinStrings_separator_number( jxPathParts, '-', number-- );
            {
                Path* target_path = Path_child( content_dir, String_getChars( target_name ) );
                {
                    Path* trial_path = Path_child( target_path, filename );

                    f = File_CreateIfExists_path( &trial_path );

                    if ( f )
                    {
                        Path* trial_path = Path_child( target_path, filename );
                        if ( Debug ) fprintf( stderr, "Found: %20s --> %s\n", filename, Path_getAbsolute( trial_path ) );
                        Path_free( &trial_path );
                    }
                }
                Path_free( &target_path );
            }
            String_free( &target_name );
        }

        if ( !f )
        {
            //                     |
            //  www/test.juxtapage/_content/_site

            Path* site_dir = Path_child( siteDir, "_content/_site" );
            {
                Path* trial_path = Path_child( site_dir, filename );

                f = File_CreateIfExists_path( &trial_path );

                if ( f )
                {
                    Path* trial_path = Path_child( site_dir, filename );
                    if ( Debug ) fprintf( stderr, "Found: %20s --> %s\n", filename, Path_getAbsolute( trial_path ) );
                    Path_free( &trial_path );
                }
            }
            Path_free( &site_dir );
        }
    }
    Path_free( &content_dir );

    return f;
}

void
JuxtaPage_FindFiles_siteDir_jxPathParts_pattern_files( const Path* siteDir,  const Array* jxPathParts, const char* pattern, ArrayOfFile* const files )
{
    File*   f        = null;
    String* filename = String_new( pattern );
    int     index    = String_indexOf_ch_skip( filename, '?', 0 );
    char*  _filename = (char*) String_getChars( filename );

    for ( int i=0; i < 9; i++ )
    {
        _filename[index] = (char) '0' + i;

        if ( (f = JuxtaPage_FindFile_siteDir_jxPathParts_filename( siteDir, jxPathParts, _filename )) )
        {
            ArrayOfFile_append_file( files, &f );
        }
    }
    String_free( &filename );
}
