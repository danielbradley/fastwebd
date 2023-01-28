#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>

#include "libbase.h"
#include "libhttp.h"
#include "libhttpserver.h"

static void    HTTPServer_Process_srvDir_connection    ( const Path* srvDir,       IO*          connection );
static Path*   HTTPServer_DetermineFile__srvDir_request( const Path* srvDir, const HTTPRequest* request    );

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

	printf( "Signal: %i\n", signal );
	//fprintf( stdout, " \b\b\b" );
}

void ignoreSigpipe( int signal )
{
	printf( "Signal: %i - ignoring SIGPIPE\n", signal );
}

struct _HTTPServer
{
	short    port;
	Address* loopback;
	IO*      socket;
};

HTTPServer*
HTTPServer_new_port( short port )
{
	HTTPServer* self = New( sizeof( HTTPServer ) );
	{
		self->port     = port;
		self->loopback = Address_new_port( port );
		self->socket   = IO_Socket();
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
		Address_free( &(*self)->loopback );
		IO_free     ( &(*self)->socket   );
	}
	return Delete( self );
}

bool
HTTPServer_listen( HTTPServer* self )
{
	bool status = false;

	if ( IO_bind( self->socket, self->loopback ) )
	{
		printf( "Bound socket to loopback\n" );

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
	Address* peer       = Address_new_port( 0 );
	IO*      connection = NULL;
	Path*    srvDir     = Path_CurrentDirectory();

	while ( KeepAlive && IO_accept( self->socket, peer, &connection ) )
	{
		int factor = 1000;

		usleep( 1 * factor );

		HTTPServer_Process_srvDir_connection( srvDir, connection );
		IO_free( &connection );

	}

	Path_free( &srvDir );
	Address_free( &peer );
}

void
HTTPServer_Panic()
{
	fprintf( stdout, "Panic! Could not allocate memory for HTTP server.\n" );
}

void
HTTPServer_InvalidPort()
{
	fprintf( stdout, "Error! Could not bind to privileged or in use port.\n" );
}

void
HTTPServer_Process_srvDir_connection( const Path* srvDir, IO* connection )
{
	HTTPRequest* request = HTTPRequest_Parse( connection );
	{
		const String* start_line = HTTPRequest_getStartLine( request );
		const String* host       = HTTPRequest_getHost     ( request );
		const String* port       = HTTPRequest_getPort     ( request );
		const String* method     = HTTPRequest_getMethod   ( request );
		const String* origin     = HTTPRequest_getOrigin   ( request );

		fprintf( stdout, "Request: %s\n", String_getChars( start_line ) );
		fprintf( stdout, "Method:  %s\n", String_getChars( method     ) );
		fprintf( stdout, "Host:    %s\n", String_getChars( host       ) );
		fprintf( stdout, "Port:    %s\n", String_getChars( port       ) );
		fprintf( stdout, "Origin:  %s\n", String_getChars( origin     ) );

		if ( !HTTPRequest_isValid( request ) )
		{
			const char* status = "HTTP/1.0 400 BAD_REQUEST \r\n";

			IO_write( connection, status );
		}
		else
		if ( 1 )
		{
			Path* resource = HTTPServer_DetermineFile__srvDir_request( srvDir, request );

			if ( 1 )
			{
				const char* _path = Path_getAbsolute( resource );
				File*        file = File_new        ( _path    );

				if ( !File_exists( file ) )
				{
					const char* status = "HTTP/1.0 404 ERROR \r\n";
					const char* end    = "\r\n";

					fprintf( stdout, "Response: %s (%s)", status, _path  );
					IO_write( connection, status );
					IO_write( connection, end    );
				}
				else
				if ( String_contentEquals( method, "OPTIONS" ) )
				{
					StringBuffer* headers = StringBuffer_new();
					{
						const char* status = "HTTP/1.1 204 No Content \r\n";
						const char* end    = "\r\n";

						StringBuffer_append_chars( headers, "Connection: close\r\n" );
						StringBuffer_append_chars( headers, "Access-Control-Allow-Origin: " );
						StringBuffer_append_chars( headers, String_getChars( origin ) );
						StringBuffer_append_chars( headers, end );
						StringBuffer_append_chars( headers, "Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE, HEAD\r\n" );
						StringBuffer_append_chars( headers, "Access-Control-Allow-Private-Network: true\r\n" );
						StringBuffer_append_chars( headers, "Access-Control-Max-Age: 86400\r\n" );
			
						IO_write( connection, status                           );
						IO_write( connection, StringBuffer_getChars( headers ) );
						IO_write( connection, end                              );

						fprintf( stdout, "Options Response: %s", status                           );
						fprintf( stdout, "Options Response: %s", StringBuffer_getChars( headers ) );
					}
					StringBuffer_free( &headers );
				}
				else
				if ( 1 )
				{
					StringBuffer* headers = StringBuffer_new();
					{
						const char* status    = "HTTP/1.0 200 OK \r\n";
						const char* end       = "\r\n";

						File_open( file );

						StringBuffer_append_chars ( headers, "Content-Type: " );
						StringBuffer_append_chars ( headers, File_getMimeType( file ) );
						StringBuffer_append_chars ( headers, end );

						StringBuffer_append_chars ( headers, "Content-Length: " );
						StringBuffer_append_number( headers, File_getByteSize( file ) + 2 );
						StringBuffer_append_chars ( headers, end );

						if ( String_getLength( origin ) )
						{
							StringBuffer_append_chars( headers, "Access-Control-Allow-Origin: " );
							StringBuffer_append_chars( headers, String_getChars( origin ) );
							StringBuffer_append_chars ( headers, end );
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
							IO_sendFile( connection, File_getIO( file ) );
						}
						IO_write   ( connection, end );

						fprintf( stdout, "Response: %s", status                           );
						fprintf( stdout, "Response: %s", StringBuffer_getChars( headers ) );
					}
					StringBuffer_free( &headers );
				}

				File_free( &file );
			}
			Path_free( &resource );
		}
		fprintf( stdout, "END\n" );
	}
	HTTPRequest_free( &request );
}

Path*
HTTPServer_DetermineFile__srvDir_request( const Path* srvDir, const HTTPRequest* request )
{
	Path* path = null;

	const String* host     = HTTPRequest_getHost    ( request  );
	const String* resource = HTTPRequest_getResource( request  );
	const char*  _host     = String_getChars        ( host     );
	const char*  _resource = String_getChars        ( resource );
	int           len      = String_getLength       ( resource ); 

	fprintf( stdout, "srv:      %s\n", Path_getAbsolute( srvDir ) );
	fprintf( stdout, "host:     %s\n", _host     );
	fprintf( stdout, "resource: %s\n", _resource );

	Path* site = Path_child( srvDir, _host );
	{
		path = Path_child( site,   _resource );

		if ( '/' == _resource[len - 1] )
		{
			Path* tmp = Path_child( path, "index.html" );

			Path_free( &path );

			path = tmp;
		}
	}
	Path_free( &site );

	return path;
}
