#ifndef LIBHTTP_HTTPREQUEST_H
#define LIBHTTP_HTTPREQUEST_H

HTTPRequest*  HTTPRequest_new();
//HTTPRequest*  HTTPRequest_free           (       HTTPRequest** self );

      bool    HTTPRequest_isValid        ( const HTTPRequest*  self );
      bool    HTTPRequest_isIPTarget     ( const HTTPRequest*  self );
const String* HTTPRequest_getStartLine   ( const HTTPRequest*  self );
const String* HTTPRequest_getMethod      ( const HTTPRequest*  self );
const String* HTTPRequest_getResource    ( const HTTPRequest*  self );
const String* HTTPRequest_getVersion     ( const HTTPRequest*  self );
const String* HTTPRequest_getHost        ( const HTTPRequest*  self );
const String* HTTPRequest_getPort        ( const HTTPRequest*  self );
const String* HTTPRequest_getOrigin      ( const HTTPRequest*  self );
const String* HTTPRequest_getForwardedFor( const HTTPRequest*  self );
void          HTTPRequest_log_status     ( const HTTPRequest*  self, const char* status, int statusNum );

void          HTTPRequest_setStartLine   (       HTTPRequest*  self,       const char*   method      );
void          HTTPRequest_setMethod      (       HTTPRequest*  self,       const char*   method      );
void          HTTPRequest_setResource    (       HTTPRequest*  self,       const char*   resource    );
void          HTTPRequest_setVersion     (       HTTPRequest*  self,       const char*   version     );
void          HTTPRequest_setHost        (       HTTPRequest*  self,       const char*   host        );
void          HTTPRequest_setOrigin      (       HTTPRequest*  self,       const char*   origin      );
void          HTTPRequest_setForwardedFor(       HTTPRequest*  self,       const char*   origin      );


HTTPRequest*  HTTPRequest_Parse          ( const Address*      peer, IO*  connection, const String* localDomain );

#endif
