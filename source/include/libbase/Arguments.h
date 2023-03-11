#ifndef LIBBASE_ARGUMENTS_H
#define LIBBASE_ARGUMENTS_H

Arguments*    Arguments_new_count_arguments      (              int  count, char** arguments                        );
Arguments*    Arguments_destruct                 (       Arguments*  self                                           );
Arguments*    Arguments_free                     (       Arguments** self                                           );
int           Arguments_getIntFor_flag_default   ( const Arguments*  self, const char* flag, int           _default );
const String* Arguments_getStringFor_flag_default( const Arguments*  self, const char* flag, const String* _default );

#endif
