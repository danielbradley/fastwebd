#ifndef LIBHTTPSERVER_H
#define LIBHTTPSERVER_H

#include "libbase.h"

typedef struct _HTTPServer  HTTPServer;

HTTPServer* HTTPServer_new_port         ( short        port );
HTTPServer* HTTPServer_free             ( HTTPServer** self );
bool        HTTPServer_listen           ( HTTPServer*  self );
void        HTTPServer_acceptConnections( HTTPServer*  self );

void        HTTPServer_Panic();
void        HTTPServer_InvalidPort();

#endif