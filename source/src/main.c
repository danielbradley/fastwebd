#include <stdlib.h>
#include "libhttpserver.h"

int main( int argc, char** argv )
{
    SysLog_Start();

    SysLog_Log_chars( "Started " );
    {
        String* default_domain = null;
        int     port           = 8080;

        for ( int i=1; i < argc; i++ )
        {
            String* arg = String_new( argv[i] );
            if ( String_contentEquals( arg, "--default-domain" ) )
            {
                if ( (i+1) < argc )
                {
                    default_domain = String_new( argv[i+1] );
                    i++;
                }
            }
            else
            if ( String_contentEquals( arg, "--port" ) )
            {
                if ( (i+1) < argc )
                {
                    port = atoi( argv[i+1] );
                    i++;
                }
            }
            String_free( &arg );
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
            HTTPServer_setDefaultDomain ( server, default_domain );
            HTTPServer_acceptConnections( server               );
        }

        HTTPServer_free( &server );
        String_free( &default_domain );
    }

    SysLog_Stop();

    Exit( 0 );
}
