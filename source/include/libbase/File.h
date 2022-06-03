#ifndef LIBBASE_FILE_H
#define LIBBASE_FILE_H

File*         File_new          ( const char*   filepath );
File*         File_free         (       File**  self     );
IO*           File_getIO        ( const File*   self     );
long long     File_getByteSize  ( const File*   self     );
const String* File_getExtension ( const File*   self     );
const char*   File_getMimeType  ( const File*   self     );

bool          File_exists       ( const File*   self     );
String*       File_readLine     ( const File*   self     );
File*         File_open         (       File*   self     );

#endif