
#include "libhttpserver.h"
#include <stdio.h>
char* Join( const char* dirname, const char* basename );

int main()
{
	printf( "%s\n", Join( "www", "index.html" ) );

	HTTPServer* server = HTTPServer_new_port( 8080 );

	if ( !server )
	{
		HTTPServer_Panic();
	}
	else
	if ( !HTTPServer_listen( server ) )
	{
		HTTPServer_InvalidPort();
	}
	else
	{
		HTTPServer_acceptConnections( server );
	}

	HTTPServer_free( &server );
}