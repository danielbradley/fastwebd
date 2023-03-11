#include <stdlib.h>
#include "libhttpserver.h"

int main( int argc, char** argv )
{
    SysLog_Start();

    Arguments* arguments = Arguments_new_count_arguments( argc, argv );

    SysLog_Log_chars( "Started " );
    {
        const String* default_domain = Arguments_getStringFor_flag_default( arguments, "--default-domain", null );
        int           port           = Arguments_getIntFor_flag_default   ( arguments, "--port",           8080 );
        int           wait           = Arguments_getIntFor_flag_default   ( arguments, "--wait",           10   );

        if ( !Arguments_has_flag( arguments, "--wait" ) )
        {
            wait = 1; // One attempt.
        }

        HTTPServer* server = HTTPServer_new_port( port );

        if ( !server )
        {
            HTTPServer_Panic();
        }
        else
        if ( !HTTPServer_listen_wait( server, wait ) )
        {
            HTTPServer_InvalidPort();
        }
        else
        {
            HTTPServer_setDefaultDomain ( server, default_domain );
            HTTPServer_acceptConnections( server                 );
        }

        Delete( &server );
    }

    SysLog_Stop();

    Delete( &arguments );

    Exit( 0 );
}
