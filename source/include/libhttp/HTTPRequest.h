#ifndef LIBHTTP_HTTPREQUEST_H
#define LIBHTTP_HTTPREQUEST_H

HTTPRequest*  HTTPRequest_new();
HTTPRequest*  HTTPRequest_free        (       HTTPRequest** self );

      bool    HTTPRequest_isValid     ( const HTTPRequest*  self );
const String* HTTPRequest_getStartLine( const HTTPRequest*  self );
const String* HTTPRequest_getMethod   ( const HTTPRequest*  self );
const String* HTTPRequest_getResource ( const HTTPRequest*  self );
const String* HTTPRequest_getVersion  ( const HTTPRequest*  self );
const String* HTTPRequest_getHost     ( const HTTPRequest*  self );

void          HTTPRequest_setStartLine(       HTTPRequest*  self, const char* method   );
void          HTTPRequest_setMethod   (       HTTPRequest*  self, const char* method   );
void          HTTPRequest_setResource (       HTTPRequest*  self, const char* resource );
void          HTTPRequest_setVersion  (       HTTPRequest*  self, const char* version  );

HTTPRequest*  HTTPRequest_Parse       ( IO* connection );

#endif