#ifndef LIBBASE_ARGUMENTS_H
#define LIBBASE_ARGUMENTS_H

Arguments* Arguments_new_count_arguments(              int  count, char** arguments );
Arguments* Arguments_destruct           (       Arguments*  self                    );
Arguments* Arguments_free               (       Arguments** self                    );
String*    Arguments_getIntFor_flag     ( const Arguments*  self, const char* flag  );
String*    Arguments_getStringFor_flag  ( const Arguments*  self, const char* flag  );

#endif
