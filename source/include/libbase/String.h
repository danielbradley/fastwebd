#ifndef FASTWEBD_STRING_H
#define FASTWEBD_STRING_H

      String* String_new                   ( const char*    ch   );
//    String* String_free                  (       String** self );
const char*   String_getChars              ( const String*  self );
      int     String_getLength             ( const String*  self );

      bool    String_contains              ( const String*  self, const char* substring );
      bool    String_contentEquals         ( const String*  self, const char* string );
      bool    String_endsWith              ( const String*  self, const char* suffix );

      String* String_extension             ( const String*, const char separator );
      bool    String_isNumeric             ( const String*  self );
      bool    String_startsWith            ( const String*  self, const char* prefix );
      String* String_substring_index       ( const String*  self, int index );
      String* String_substring_index_length( const String*  self, int index, int len );
      int     String_toNumber_default      ( const String*  self, int _default       );
      int     String_indexOf_ch_skip       ( const String*  self, char ch, int skip  );

      String* String_deroot                (       String*  self );
      String* String_trimEnd               (       String*  self );
      Array*  String_toArray_separator     ( const String*  self, char separator );
      String* String_reverseParts_separator( const String*  self, char separator );

#endif
