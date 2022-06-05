#ifndef LIBBASE_IO_H
#define LIBBASE_IO_H

IO*     IO_new     ( FD* descriptor );
IO*     IO_free    ( IO** self );
bool    IO_bind    ( IO* self, Address* toAddress );
bool    IO_listen  ( IO* self );
bool    IO_accept  ( IO* self, Address* peer, IO** connection );
int     IO_sendFile( IO* self, IO* file ); 
int     IO_write   ( IO* self, const char* ch );
String* IO_readline( IO* self );
void    IO_close   ( IO* self );

IO*     IO_Socket  ();

#endif