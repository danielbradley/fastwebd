#ifndef LIBBASE_KEYVALUE_H
#define LIBBASE_KEYVALUE_H

KeyValue*     KeyValue_new            ( const char*      key, const char* value  );
//KeyValue*     KeyValue_destruct       (       KeyValue*  self                    );
//KeyValue*     KeyValue_free           (       KeyValue** self                    );
const String* KeyValue_getKey         ( const KeyValue*  self                    );
const String* KeyValue_getValue       ( const KeyValue*  self                    );
bool          KeyValue_keyEquals_chars( const KeyValue*  self, const char* chars );

#endif
