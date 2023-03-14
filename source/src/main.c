#include "libhttpserver.h"

int main( int argc, char** argv )
{
    SysLog_Start();

    Arguments* arguments   = Arguments_new_count_arguments( argc, argv );
    String*    default_srv = String_new( "." );

    SysLog_Log_chars( "Started " );
    {
        const String* serve_dir      = Arguments_getStringFor_flag_default( arguments, "--serve-dir",      default_srv );
        const String* default_domain = Arguments_getStringFor_flag_default( arguments, "--default-domain", null        );
        int           port           = Arguments_getIntFor_flag_default   ( arguments, "--port",           8080        );
        int           wait           = Arguments_getIntFor_flag_default   ( arguments, "--wait",           10          );
        bool          single         = Arguments_has_flag                 ( arguments, "--single"                      );

        if ( !Arguments_has_flag( arguments, "--wait" ) )
        {
            wait = 1; // One attempt.
        }

        HTTPServer* server   = HTTPServer_new_port( port );
        Path*       srv_path = Path_new( String_getChars( serve_dir ) );
        {
            if ( !Platform_ChangeDir( srv_path ) )
            {
                IO_PrintError( stderr );
            }
            else
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
                bool fork_process = !single;

                HTTPServer_setDefaultDomain      ( server, default_domain );
                HTTPServer_acceptConnections_fork( server, fork_process   );
            }
        }
        Delete( &server   );
        Delete( &srv_path );
    }

    SysLog_Stop();

    Delete( &arguments   );
    Delete( &default_srv );

    Exit( 0 );
}
