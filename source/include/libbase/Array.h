#ifndef LIBBASE_ARRAY_H
#define LIBBASE_ARRAY_H

     Array* Array_new                         (                                                  );
//   Array* Array_new_free                    (        void*(*free)(void**)                      );
      void  Array_setFree                     (       Array* self, void* (*free)( void** )       );
      int   Array_count                       ( const Array*  self                               );
      void  Array_empty                       (       Array*  self                               );
      int   Array_getFirstIndex               ( const Array*  self                               );
      int   Array_getLastIndex                ( const Array*  self                               );
const void* Array_getFirst                    ( const Array*  self                               );
const void* Array_get_index                   ( const Array*  self, int    index                 );
const void* Array_getLast                     ( const Array*  self                               );
      void  Array_append_element              (       Array*  self, void** element               );
      void* Array_replace_element_index       (       Array*  self, int    index, void** element );
      void* Array_remove_index                (       Array*  self, int    index                 );
    String* Array_joinStrings_separator_number( const Array*  self, char   separator, int number );

#endif
