#ifndef LIBBASE_ARRAY_OF_FILE_H
#define LIBBASE_ARRAY_OF_FILE_H

ArrayOfFile* ArrayOfFile_new();
//ArrayOfFile* ArrayOfFile_free       (       ArrayOfFile** self              );
void         ArrayOfFile_append_file(       ArrayOfFile*  self, File** file );
int          ArrayOfFile_count      ( const ArrayOfFile*  self              );
const File*  ArrayOfFile_get_index  ( const ArrayOfFile*  self, int index   );
int	         ArrayOfFile_sizeOfFiles( const ArrayOfFile*  self              );

#endif
