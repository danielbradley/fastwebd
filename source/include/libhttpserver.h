#ifndef LIBHTTPSERVER_H
#define LIBHTTPSERVER_H

#include "libbase.h"

typedef struct _HTTPServer  HTTPServer;

HTTPServer* HTTPServer_new_port              ( short        port                              );
bool        HTTPServer_listen_wait           ( HTTPServer*  self, int wait                    );
void        HTTPServer_acceptConnections_fork( HTTPServer*  self, bool fork                   );
bool        HTTPServer_hasDefaultDomain      ( HTTPServer*  self                              );
void        HTTPServer_setDefaultDomain      ( HTTPServer*  self, const String* defaultDomain );

void        HTTPServer_Panic      ();
void        HTTPServer_InvalidPort();

#endif
