#ifndef LIBBASE_PATH_H
#define LIBBASE_PATH_H

      Path*   Path_new        ( const char* absolute );
      Path*   Path_free       (       Path** self );
const char*   Path_getAbsolute( const Path*  self );
      Path*   Path_parent     ( const Path*  self );
      Path*   Path_child      ( const Path*  self, const char* child );
      Path*   Path_CurrentDirectory();

#endif
