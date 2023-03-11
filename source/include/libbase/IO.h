#ifndef LIBBASE_IO_H
#define LIBBASE_IO_H

#include <stdio.h>

IO*     IO_new              ( FD*   descriptor );
IO*     IO_free             ( IO**  self );
bool    IO_bind_address_wait( IO*   self, Address* address, bool wait );
bool    IO_listen           ( IO*   self );
bool    IO_accept           ( IO*   self, Address* peer, IO** connection );
void    IO_flushAll         ();
int     IO_sendFile         ( IO*   self, IO* file );
IO*     IO_open_mode        ( IO*   self, const char* mode );
int     IO_write            ( IO*   self, const char* ch );
String* IO_readline         ( IO*   self );
void    IO_close            ( IO*   self );
void    IO_PrintError       ( FILE* out  );
IO*     IO_Socket           ();

#endif
