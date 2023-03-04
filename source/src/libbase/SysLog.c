#include "libbase.h"

#include <syslog.h>

//
//  https://stackoverflow.com/questions/380172/reading-syslog-output-on-a-mac
//
//  On MacOS you can use the following to see log entries:
//
//  log show --predicate 'process == "fastwebd"' --last 2d
//


void
SysLog_Start()
{
    openlog( "fastwebd", LOG_PID, LOG_USER );

    setlogmask(LOG_UPTO(LOG_INFO));
}

void
SysLog_Stop()
{
    closelog();
}

void
SysLog_Log_string( const String* string )
{
    SysLog_Log_chars( String_getChars( string ) );
}

void
SysLog_Log_chars ( const char* chars )
{
    syslog( LOG_ALERT, "%s", chars );
}
