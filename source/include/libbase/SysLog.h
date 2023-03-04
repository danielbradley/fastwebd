#ifndef LIBBASE_SYSLOG_H
#define LIBBASE_SYSLOG_H

void SysLog_Start();
void SysLog_Stop();

void SysLog_Log_string( const String* string );
void SysLog_Log_chars ( const char*   string );

#endif
