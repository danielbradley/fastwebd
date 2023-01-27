#ifndef LIBHTTP_HTTPHEADER_H
#define LIBHTTP_HTTPHEADER_H

HTTPHeader*  HTTPHeader_new ( String**     line );
HTTPHeader*  HTTPHeader_free( HTTPHeader** self );

const String* HTTPHeader_getName ( const HTTPHeader* self );
const String* HTTPHeader_getValue( const HTTPHeader* self );

#endif
