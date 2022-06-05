#include <stdio.h>
#include <signal.h>

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
}

void ignoreSigpipe( int signal )
{
	printf( "Signal: %i - ignoring SIGPIE\n", signal );
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
		if ( !HTTPRequest_isValid( request ) )
		{
			const char* status = "HTTP/1.1 400 BAD_REQUEST \r\n";

			IO_write( connection, status );
		}
		else
		if ( 1 )
		{
			fprintf( stdout, "Request: %s\n", String_getChars( HTTPRequest_getStartLine( request ) ) );
			fprintf( stdout, "Request: %s\n", String_getChars( HTTPRequest_getHost     ( request ) ) );

			Path* resource = HTTPServer_DetermineFile__srvDir_request( srvDir, request );
			if ( 1 )
			{
				char  headers[1024];

				File* file = File_new( Path_getAbsolute( resource ) );

				if ( !File_exists( file ) )
				{
					const char* status = "HTTP/1.1 404 ERROR \r\n";

					fprintf( stderr, "Response: %s", status  );
					fprintf( stderr, "Response: %s\n", Path_getAbsolute( resource ) );

					IO_write( connection, status );
				}
				else
				if ( 1 )
				{
					const char* status    = "HTTP/1.1 200 OK \r\n";
					const char* end       = "\r\n";
					const char* mime_type = File_getMimeType( file );

					File_open( file );

					sprintf( headers, "Content-Type: %s\r\nContent-Length: %lli\r\n\r\n", mime_type, File_getByteSize( file ) + 2 );

					fprintf( stdout, "Response: %s", status  );
					fprintf( stdout, "Response: %s", headers );

					IO_write   ( connection, status             );
					IO_write   ( connection, headers            );
					IO_sendFile( connection, File_getIO( file ) );
					IO_write   ( connection, end                );
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

	printf( "srv:      %s\n", Path_getAbsolute( srvDir ) );
	printf( "host:     %s\n", _host     );
	printf( "resource: %s\n", _resource );

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
