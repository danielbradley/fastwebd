#ifndef FASTWEBD_STRINGBUFFER_H
#define FASTWEBD_STRINGBUFFER_H

      StringBuffer* StringBuffer_new          ();
      StringBuffer* StringBuffer_free         (       StringBuffer** self );

      void          StringBuffer_append_chars (       StringBuffer*  self, const char*   chars  ); 
      void          StringBuffer_append_string(       StringBuffer*  self, const String* string );
      void          StringBuffer_append_number(       StringBuffer*  self, int           number );

      const char*   StringBuffer_getChars     ( const StringBuffer*  self );

#endif