
#include "libhttpserver.h"

int main()
{
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