#ifndef LIBADT_ARRAY_H
#define LIBADT_ARRAY_H

     Array* Array_new                  (                                               );
     Array* Array_free                 (       Array** self                            );
      int   Array_count                ( const Array*  self                            );
      int   Array_getFirstIndex        ( const Array*  self                            );
	  int   Array_getLastIndex         ( const Array*  self                            );
const void* Array_getFirst             ( const Array*  self                            );
const void* Array_get_index            ( const Array*  self, int index                 );
const void* Array_getLast              ( const Array*  self                            );
      void  Array_append_element       (       Array*  self, void** element            );
      void* Array_replace_element_index(       Array*  self, int index, void** element );
      void* Array_remove_index         (       Array*  self, int index                 );

#endif