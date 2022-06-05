#ifndef FASTWEBD_STRING_H
#define FASTWEBD_STRING_H

      String* String_new                   ( const char*    ch   );
      String* String_free                  (       String** self );
const char*   String_getChars              ( const String*  self );
      int     String_getLength             ( const String*  self );

      bool    String_contains              ( const String*  self, const char* substring );
      bool    String_contentEquals         ( const String*  self, const char* string );
      String* String_extension             ( const String*, const char separator );
      bool    String_startsWith            ( const String*  self, const char* prefix );
      String* String_substring_index       ( const String*  self, int index );
      String* String_substring_index_length( const String*  self, int index, int len );
      int     String_indexOf_ch_skip       ( const String*  self, char ch, int skip );

      String* String_deroot                (       String*  self );
      String* String_trimEnd               (       String*  self );


      String* String_cat                   ( const String*  self, char optional, const String* other );

#endif