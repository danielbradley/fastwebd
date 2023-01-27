#ifndef LIBBASE_CHARSTRING_H
#define LIBBASE_CHARSTRING_H

char*     CharString_new  ( const char*  ch   );
char*    CharString_free  (       char** self );
int      CharString_length( const char*  self );

#endif