#ifndef LIBBASE_ARGUMENTS_H
#define LIBBASE_ARGUMENTS_H

Arguments*    Arguments_new_count_arguments      (              int  count, char** arguments                        );
int           Arguments_getIntFor_flag_default   ( const Arguments*  self, const char* flag, int           _default );
const String* Arguments_getStringFor_flag_default( const Arguments*  self, const char* flag, const String* _default );
bool          Arguments_has_flag                 ( const Arguments*  self, const char* flag                         );

#endif
