
#include <stdlib.h>
#include "libhttpserver.h"

int main( int argc, char** argv )
{
    int port = 8080;
    
    if ( argc > 1 )
    {
        port = atoi( argv[1] );
        
        if ( !port ) port = 8080;
    }

	HTTPServer* server = HTTPServer_new_port( port );

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

	Exit( 0 );
}
