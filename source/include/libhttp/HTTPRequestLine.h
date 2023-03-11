#ifndef LIBHTTP_HTTPREQUESTLINE_H
#define LIBHTTP_HTTPREQUESTLINE_H

HTTPRequestLine*  HTTPRequestLine_new ( String* line );

const String* HTTPRequestLine_getMethod  ( const HTTPRequestLine* self );
const String* HTTPRequestLine_getResource( const HTTPRequestLine* self );
const String* HTTPRequestLine_getVersion ( const HTTPRequestLine* self );
      bool    HTTPRequestLine_isValid    ( const HTTPRequestLine* self );


#endif

