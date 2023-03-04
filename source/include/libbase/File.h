#ifndef LIBBASE_FILE_H
#define LIBBASE_FILE_H

File*         File_new             ( const char*   filepath );
File*         File_free            (       File**  self     );
IO*           File_getIO           ( const File*   self     );

long long     File_getByteSize     ( const File*   self     );
const String* File_getExtension    ( const File*   self     );
const String* File_getFilePath     ( const File*   self     );
const char*   File_getMimeType     ( const File*   self     );

bool          File_exists          ( const File*   self     );
String*       File_readLine        ( const File*   self     );
File*         File_open            (       File*   self     );
File*         File_close           (       File*   self     );

bool          File_isMimeType      ( const File* self, const char* mimeType );

File*         File_CreateIfExists_path( Path** path );
const char*   File_DetermineMimeType( const String* extension );

#endif
