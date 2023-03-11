#include "libbase.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/ip.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <grp.h>
#include <signal.h>

#define NOT_FOUND -1

#define DEFAULT_CAPACITY      255
#define NOT_FOUND            -1

typedef enum _ObjectType
{
    ARRAY  = 0xCAFEBABE,
    OBJECT = 0xBABECAFE

} ObjectType;

static       String* FGetLine( FILE* stream, size_t* len );

//static
       char* Join( const char* dirname, const char* basename );

static int Objects = 0;
static int Arrays  = 0;

struct _Object
{
    ObjectType type;
    void*(*destruct)(void*);
};

struct _Arguments
{
    Object super;
    Array* keyValues;
};

struct _Array
{
    void*(*free)(void**);
    int    count;
    int    capacity;
    void** elements;
};

struct _ArrayOfFile
{
    Array* files;
};

static void Array_resize( Array* self, int index );

struct _Address
{
    struct sockaddr inner;
};

struct _File
{
    String*     filepath;
    String*     extension;
    const char* mimeType;
    IO*         io;
    long long   byteSize;
};

struct _IO
{
    FD    descriptor;
    FILE* stream;
};

struct _KeyValue
{
    Object  super;
    String* key;
    String* value;
};

struct _Path
{
    String* absolute;
};

struct _String
{
    int   length;
    char* data;
};

struct _StringBuffer
{
    int   capacity;
    int   length;
    char* data;
};

/*
void*
New( int size ) // Same as Platform_Alloc
{
    return NewArray( size, 0 );
}

void*
NewArray( int size, int count ) // Same as Platform_Alloc
{
    if ( 0 == count )
    {
        Objects++;
        return calloc( 1, size );
    }
    else
    {
        Arrays++;
        return calloc( count, size );
    }
}
*/

/*
void*
Del( void** element )
{
    if ( element && *element )
    {
        Object* obj = *element;

        if ( OBJECT == obj->type )
        {
            obj->destruct( *element );
        }
        free( *element ); *element = 0;
        Objects--;
    }
    return null;
}

void*
DelArray( void** array )
{
    if ( array && *array )
    {
        free( *array ); *array = 0;
        Arrays--;
    }
    return *array;
}
*/

void* Platform_New( int size, int count )
{
    if ( 0 == count )
    {
        Objects++;
        return calloc( 1, size );
    }
    else
    {
        Arrays++;
        return calloc( count, size );
    }
}

void*
Platform_Delete( void** element )
{
    if ( element && *element )
    {
        Object* obj = *element;

        switch ( obj->type )
        {
        case ARRAY:
        case OBJECT:
            obj->destruct( *element );
            break;
        }
        free( *element ); *element = 0;
        Objects--;
    }
    return null;
}

void*
Platform_DeleteArray( void** array )
{
    if ( array && *array )
    {
        free( *array ); *array = 0;
        Arrays--;
    }
    return *array;
}

void MemInfo()
{
    printf( "Objects: %i, Arrays: %i\n", Objects, Arrays );
}

int Exit( int exit )
{
    if ( Objects + Arrays ) MemInfo();

    return exit;
}

Arguments*
Arguments_new_count_arguments( int count, char** arguments )
{
    Arguments* self = New( sizeof( Arguments ) );
    if ( self )
    {
        Object_init( &self->super, (Destructor) Arguments_destruct );
        self->keyValues = Array_new_free( (Free) KeyValue_free );

        for ( int i=1; i < count; i++ )
        {
            KeyValue* key_value = KeyValue_new( arguments[i-1], arguments[i] );

            Array_append_element( self->keyValues, (void**) &key_value );
        }
    }
    return self;
}

Arguments*
Arguments_destruct( Arguments* self )
{
    if ( self )
    {
        Array_free     ( &self->keyValues );
        Object_destruct( &self->super     );
    }
    return self;
}

Arguments*
Arguments_free( Arguments** self )
{
    if ( self && *self )
    {
        Arguments_destruct( *self );
        Delete( self );
    }
    return null;
}

int
Arguments_getIntFor_flag_default( const Arguments* self, const char* flag, int _default )
{
    int ret = _default;
    int n   = Array_count( self->keyValues );

    for ( int i=0; i < n; i++ )
    {
        const KeyValue* keyval = Array_get_index( self->keyValues, i );
        if ( KeyValue_keyEquals_chars( keyval, flag ) )
        {
            const String* value = KeyValue_getValue( keyval );
            ret                 = String_toNumber( value );
            break;
        }
    }
    return ret;
}

const String*
Arguments_getStringFor_flag_default( const Arguments* self, const char* flag, const String* _default )
{
    const String* ret = _default;
    int           n   = Array_count( self->keyValues );

    for ( int i=0; i < n; i++ )
    {
        const KeyValue* keyval = Array_get_index( self->keyValues, i );
        if ( KeyValue_keyEquals_chars( keyval, flag ) )
        {
            ret = KeyValue_getValue( keyval );
            break;
        }
    }
    return ret;
}

Array*
Array_new()
{
    Array* self = New( sizeof( Array ) );
    if ( self )
    {
        self->free     = null;
        self->count    = 0;
        self->capacity = DEFAULT_CAPACITY;
        self->elements = NewArray( sizeof( void* ), self->capacity );
    }
    return self;
}

Array*
Array_new_free( void*(*free)(void**) )
{
    Array* self = New( sizeof( Array ) );
    if ( self )
    {
        self->free     = free;
        self->count    = 0;
        self->capacity = DEFAULT_CAPACITY;
        self->elements = NewArray( sizeof( void* ), self->capacity );
    }
    return self;
}

Array*
Array_destruct( Array* self )
{
    Array_empty( self );

    self->count    = 0;
    self->capacity = 0;

    DeleteArray( &self->elements );

    return self;
}

Array*
Array_free( Array** self )
{
    if ( self && *self )
    {
        Array_destruct( *self );

        Delete( self );
    }
    return null;
}

Array*
Array_free_destructor( Array** self, void* (*free)( void** ) )
{
    if ( *self )
    {
        (*self)->free = free;

        Array_free( self );
    }
    return 0;
}

int
Array_count( const Array* self )
{
    return self->count;
}

void
Array_empty( Array* self )
{
    int n = self->capacity;

    for ( int i=0; i < n; i++ )
    {
        if ( self->elements[i] )
        {
            if ( self->free )
            {
                self->free( &(self->elements[i]) );
            }
            else
            {
                self->elements[i] = 0;
            }
            self->count--;
        }
    }
}

int
Array_getFirstIndex( const Array* self )
{
    int index = 0;

    while ( (index < self->capacity) && !self->elements[index] ) index++;

    if ( index == self->capacity )
    {
        return NOT_FOUND;
    }
    else
    {
        return index;
    }
}

int
Array_getLastIndex( const Array* self )
{
    int index = self->capacity - 1;

    while ( (-1 < index) && !self->elements[index] ) index--;

    if ( -1 == index )
    {
        return NOT_FOUND;
    }
    else
    {
        return index;
    }
}

const void*
Array_getFirst( const Array* self )
{
    int index = Array_getFirstIndex( self );

    return (NOT_FOUND == index) ? null : Array_get_index( self, index );
}

const void* Array_get_index( const Array* self, int index )
{
    return (0 <= index) && (index < self->capacity) ? self->elements[index] : null;
}

const void*
Array_getLast( const Array* self )
{
    int index = Array_getLastIndex( self );

    return (NOT_FOUND == index) ? null : Array_get_index( self, index );
}

void
Array_append_element( Array* self, void** element )
{
    int index = Array_getLastIndex( self );

    index = (NOT_FOUND == index) ? 0 : index + 1;

    Array_replace_element_index( self, index, element );

    self->count++;
}

void*
Array_replace_element_index( Array* self, int index, void** element )
{
    Array_resize( self, index );

    void* ret = self->elements[index];

    if ( element && *element )
    {
        self->elements[index] = *element; *element = null;
    }

    return ret;
}

void*
Array_remove_index( Array* self, int index )
{
    void* ret = null;

    if ( (0 <= index) && (index < self->capacity) )
    {
        ret = self->elements[index]; self->elements[index] = null;
    }
    if ( ret )
    {
        self->count--;
    }
    return ret;
}

static void Array_resize( Array* self, int index )
{
    int new_size = self->capacity;

    while ( new_size < (index + 1) )
    {
        new_size = new_size + new_size;
    }

    if ( self->capacity != new_size )
    {
        void** elements = NewArray( sizeof(void*), new_size );

        for ( int i=0; i < self->capacity; i++ )
        {
            elements[i] = self->elements[i]; self->elements[i] = 0;
        }

        DeleteArray( &self->elements );

        self->elements = elements;
        self->capacity = new_size;
    }
}

String*
Array_joinStrings_separator_number( const Array* self, char separator, int number )
{
    String*       joined = null;
    StringBuffer* buffer = StringBuffer_new();
    {
        char sep[2] = { separator, '\0' };
        int count = Array_count( self );

        if ( count )
        {
            for ( int i=0; i < count; i++ )
            {
                if ( number-- > 0 )
                {
                    StringBuffer_append_string( buffer, (String*) Array_get_index( self, i ) );

                    if ( number )
                    {
                        StringBuffer_append_chars( buffer, sep );
                    }
                }
            }
        }
    }
    joined = String_new( StringBuffer_getChars( buffer ) );

    StringBuffer_free( &buffer );

    return joined;
}

ArrayOfFile* ArrayOfFile_new()
{
    ArrayOfFile* self = New( sizeof(ArrayOfFile) );
    if ( self )
    {
        self->files = Array_new();
    }
    return self;
}

ArrayOfFile* ArrayOfFile_free( ArrayOfFile** self )
{
    if ( *self )
    {
        Array_free_destructor( &(*self)->files, (void *(*)(void **)) File_free );
        Array_free( &(*self)->files );
    }
    return Delete( self );
}

void ArrayOfFile_append_file( ArrayOfFile* self, File** file )
{
    void* f = Take( (void**) file );

    Array_append_element( self->files, &f );
}

int ArrayOfFile_count( const ArrayOfFile* self )
{
    return Array_count( self->files );
}

const File* ArrayOfFile_get_index( const ArrayOfFile* self, int index )
{
    return (const File*) Array_get_index( self->files, index );
}

int
ArrayOfFile_sizeOfFiles( const ArrayOfFile* self )
{
    int size = 0;
    int n    = ArrayOfFile_count( self );

    for ( int i=0; i < n; i++ )
    {
        const File* file = ArrayOfFile_get_index( self, i );
        size += File_getByteSize( file );
    }

    return size;
}

Address*
Address_new_port( short port )
{
    Address* address = New( sizeof( Address ) );
    if ( address )
    {
        struct sockaddr_in* inner = (struct sockaddr_in*) &address->inner;

        inner->sin_family      = AF_INET;
        inner->sin_port        = htons( port );
        inner->sin_addr.s_addr = htonl( INADDR_ANY );
    }
    return address;
}

Address*
Address_free( Address** self )
{
    return Delete( self );
}

String*
Address_origin( const Address* self )
{
    struct sockaddr_in* inner = (struct sockaddr_in*) &self->inner;

    char str[INET_ADDRSTRLEN];

    // now get it back and print it
    inet_ntop(AF_INET, &(inner->sin_addr), str, INET_ADDRSTRLEN);

    return String_new( str );
}

char*
CharString_new( const char* src )
{
    char* dst = NewArray( sizeof(char), strlen( src ) + 1 );

    strcpy( dst, src );

    return dst;
}

char*
CharString_free( char** self )
{
    return DeleteArray( self );
}

int
CharString_length( const char* self )
{
    return strlen( self );
}

File* File_new ( const char* filepath )
{
    struct stat buf;

    File* self = New( sizeof( File ) );
    if ( self )
    {
        self->filepath  = String_new( filepath );
        self->extension = String_extension( self->filepath, '.' );
        self->mimeType  = File_DetermineMimeType( self->extension );

        if ( -1 != lstat( filepath, &buf ) )
        {
            self->byteSize = buf.st_size;
        }
    }
    return self;
}

File* File_free( File** self )
{
    if ( *self )
    {
        String_free( &(*self)->filepath  );
        String_free( &(*self)->extension );

        (*self)->byteSize = 0;

        if ( (*self)->io )
        {
            IO_free( &(*self)->io );
        }
    }
    return Delete( self );
}

IO* File_getIO( const File* self )
{
    return self->io;
}

long long File_getByteSize( const File* self )
{
    return self->byteSize;
}

const String* File_getExtension( const File* self )
{
    return self->extension;
}

const String* File_getFilePath( const File* self )
{
    return self->filepath;
}

const char* File_getMimeType( const File* self )
{
    return self->mimeType;
}

bool
File_exists( const File* self )
{
    struct stat buf;

    return (0 == lstat( String_getChars( self->filepath ), &buf ));
}

/*
String*
File_readLine( const File* self )
{
    String* ret = NULL;

    //      start = "GET /index.html 1.1\r\n"  len = 21
    //      copy  = "                       0" len = 22

    size_t len;

    char* start = fgetln( self->stream, &len );
    char* copy  = calloc( len + 1, sizeof( char ) );

    strncpy( copy, start, len );

    ret = String_new( copy );
    
    free( copy );

    return ret;
}
*/

File* File_open( File* self )
{
    struct stat buf;
    const char* fpath = String_getChars( self->filepath );

    if ( -1 == lstat( fpath, &buf ) )
    {
        fprintf( stdout, "Aborting, file not found: %s\n", fpath );
        abort();
    }
    else
    {
        int fd         = open( fpath, O_RDONLY );
        self->io       = IO_open_mode( IO_new( &fd ), "rb" );
        self->byteSize = buf.st_size;
    }

    return self;
}

File* File_close( File* self )
{
    IO_free( &self->io );

    return self;
}

bool
File_isMimeType( const File* self, const char* mimeType )
{
    bool is_mime_type = false;

    String* mime = String_new( File_getMimeType( self ) );

    if ( String_startsWith( mime, mimeType ) )
    {
        is_mime_type = true;
    }
    String_free( &mime );

    return is_mime_type;
}

File* File_CreateIfExists_path( Path** path )
{
    Path* _path = Take( (void**) path );
    File*  file = File_new( Path_getAbsolute( _path ) );
    if ( !File_exists( file ) )
    {
        File_free( &file );
    }
    Path_free( &_path );

    return file;
}

const char* File_DetermineMimeType( const String* extension )
{
    const char* _extension = String_getChars( extension );

    //
    //   From:
    //   https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types
    //

    if ( '.' == _extension[0] )
    {
        switch( _extension[1] )
        {
        case 'a':
            if ( String_endsWith( extension, ".aac"  ) ) return       "audio/aac";
            if ( String_endsWith( extension, ".abw"  ) ) return "application/x-abiword";
            if ( String_endsWith( extension, ".arc"  ) ) return "application/x-freearc";
            if ( String_endsWith( extension, ".avif" ) ) return       "image/avif";
            if ( String_endsWith( extension, ".avi"  ) ) return       "video/x-msvideo";
            if ( String_endsWith( extension, ".azw"  ) ) return "application/vnd.amazon.ebook";
            break;

        case 'b':
            if ( String_endsWith( extension, ".bin"  ) ) return "application/octet-stream";
            if ( String_endsWith( extension, ".bmp"  ) ) return       "image/bmp";
            if ( String_endsWith( extension, ".bz"   ) ) return "application/x-bzip";
            if ( String_endsWith( extension, ".bz2"  ) ) return "application/x-bzip2";
            break;

        case 'c':
            if ( String_endsWith( extension, ".cda"  ) ) return "application/x-cdf";
            if ( String_endsWith( extension, ".csh"  ) ) return "application/x-csh";
            if ( String_endsWith( extension, ".css"  ) ) return        "text/css";
            if ( String_endsWith( extension, ".csv"  ) ) return        "text/csv";
            break;

        case 'd':
            if ( String_endsWith( extension, ".doc"  ) ) return "application/msword";
            if ( String_endsWith( extension, ".docx" ) ) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
            break;

        case 'e':
            if ( String_endsWith( extension, ".eot"  ) ) return "application/vnd.ms-fontobject";
            if ( String_endsWith( extension, ".epub" ) ) return "application/epub+zip";
            break;

        case 'g':
            if ( String_endsWith( extension, ".gz"   ) ) return "application/gzip";
            if ( String_endsWith( extension, ".gif"  ) ) return "image/gif";
            break;

        case 'h':
            if ( String_endsWith( extension, ".htm"  ) ) return "text/html";
            if ( String_endsWith( extension, ".html" ) ) return "text/html";
            break;

        case 'i':
            if ( String_endsWith( extension, ".ico"  ) ) return "image/vnd.microsoft.icon";
            if ( String_endsWith( extension, ".ics"  ) ) return "text/calendar";
            break;

        case 'j':
            if ( String_endsWith( extension, ".jar"    ) ) return "application/java-archive";
            if ( String_endsWith( extension, ".jpeg"   ) ) return "image/jpeg";
            if ( String_endsWith( extension, ".jpg"    ) ) return "image/jpeg";
            if ( String_endsWith( extension, ".js"     ) ) return "text/javascript";
            if ( String_endsWith( extension, ".json"   ) ) return "application/json";
            if ( String_endsWith( extension, ".jsonld" ) ) return "application/ld+json";
            break;

        case 'm':
            if ( String_endsWith( extension, ".mid"    ) ) return "audo/x-midi";
            if ( String_endsWith( extension, ".midi"   ) ) return "audo/x-midi";
            if ( String_endsWith( extension, ".mjs"    ) ) return "text/javascript";
            if ( String_endsWith( extension, ".mp3"    ) ) return "audio/mpeg";
            if ( String_endsWith( extension, ".mp4"    ) ) return "video/mp4";
            if ( String_endsWith( extension, ".mpeg"   ) ) return "video/mpeg";
            if ( String_endsWith( extension, ".mpkg"   ) ) return "application/vnd.apple.installer+xml";
            break;

        case 'o':
            if ( String_endsWith( extension, ".odp"    ) ) return "application/vnd.oasis.opendocument.presentation";
            if ( String_endsWith( extension, ".ods"    ) ) return "application/vnd.oasis.opendocument.spreadsheet";
            if ( String_endsWith( extension, ".odt"    ) ) return "application/vnd.oasis.opendocument.text";
            if ( String_endsWith( extension, ".oga"    ) ) return "audio/ogg";
            if ( String_endsWith( extension, ".ogv"    ) ) return "video/ogg";
            if ( String_endsWith( extension, ".ogx"    ) ) return "application/ogg";
            if ( String_endsWith( extension, ".opus"   ) ) return "audio/opus";
            if ( String_endsWith( extension, ".otf"    ) ) return "font/otf";
            break;

        case 'p':
            if ( String_endsWith( extension, ".png"    ) ) return "image/png";
            if ( String_endsWith( extension, ".pdf"    ) ) return "application/pdf";
            if ( String_endsWith( extension, ".php"    ) ) return "application/x-httpd-php";
            if ( String_endsWith( extension, ".ppt"    ) ) return "application/vnd.ms-powerpoint";
            if ( String_endsWith( extension, ".pptx"   ) ) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
            break;

        case 'r':
            if ( String_endsWith( extension, ".rar"    ) ) return "application/vnd.rar";
            if ( String_endsWith( extension, ".rft"    ) ) return "application/rtf";
            break;

        case 's':
            if ( String_endsWith( extension, ".sh"     ) ) return "application/x-sh";
            if ( String_endsWith( extension, ".svg"    ) ) return "image/svg+xml";
            break;

        case 't':
            if ( String_endsWith( extension, ".tar"    ) ) return "application/x-tar";
            if ( String_endsWith( extension, ".tif"    ) ) return "image/tiff";
            if ( String_endsWith( extension, ".tiff"   ) ) return "image/tiff";
            if ( String_endsWith( extension, ".ts"     ) ) return "video/mp2t";
            if ( String_endsWith( extension, ".ttf"    ) ) return "font/ttf";
            if ( String_endsWith( extension, ".txt"    ) ) return "text/plain";
            break;

        case 'v':
            if ( String_endsWith( extension, ".vsd"    ) ) return "application/vnd.visio";
            break;

        case 'w':
            if ( String_endsWith( extension, ".wav"    ) ) return "audio/wav";
            if ( String_endsWith( extension, ".weba"   ) ) return "audio/webm";
            if ( String_endsWith( extension, ".webm"   ) ) return "audio/webm";
            if ( String_endsWith( extension, ".webp"   ) ) return "image/webp";
            if ( String_endsWith( extension, ".woff"   ) ) return "font/woff";
            if ( String_endsWith( extension, ".woff2"  ) ) return "font/woff2";
            break;

        case 'x':
            if ( String_endsWith( extension, ".xhtml"  ) ) return "application/xhtml+xml";
            if ( String_endsWith( extension, ".xls"    ) ) return "application/vnd.ms-excel";
            if ( String_endsWith( extension, ".xlsx"   ) ) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
            if ( String_endsWith( extension, ".xml"    ) ) return "application/xml";
            if ( String_endsWith( extension, ".xul"    ) ) return "application/vnd.mozilla.xul+xml";
            break;

        case 'z':
            if ( String_endsWith( extension, ".zip"    ) ) return "application/zip";
            break;

        case '3':
            if ( String_endsWith( extension, ".3gp"    ) ) return "video/3gpp";
            if ( String_endsWith( extension, ".3g2"    ) ) return "video/3gpp2";
            break;

        case '7':
            if ( String_endsWith( extension, ".7z"     ) ) return "application/x-7z-compressed";
            break;

        default:
            break;
        }
    }
    return "plain/text";
}

static void* stash[100];

void** Give( void* pointer )
{
    void** tmp = stash;

    while ( *tmp ) tmp++;

    *tmp = pointer;

    return tmp;
}

IO* IO_new( FD* descriptor )
{
    IO* self = New( sizeof( IO ) );
    if ( self )
    {
        self->descriptor  = *descriptor; *descriptor = 0;
        self->stream      = null;
    }
    return self;
}

IO* IO_free( IO** self )
{
    if ( *self )
    {
        IO_close( *self );
    }
    return Delete( self );
}

bool IO_bind( IO* self, Address* toAddress )
{
    int result = bind( self->descriptor, &toAddress->inner, sizeof( Address ) );

    if ( 0 != result )
    {
        //PrintError( result );
    }
    return (0 == result);
}

bool IO_listen( IO* self )
{
    int result = listen( self->descriptor, 255 );

    if ( 0 != result )
    {
        //PrintError( result );
    }
    return (0 == result);
}

bool
IO_accept( IO* self, Address* peer, IO** connection )
{
    socklen_t addrlen = sizeof( Address );

    int fd = accept( self->descriptor, &peer->inner, &addrlen );

    if ( -1 == fd )
    {
        switch ( errno )
        {
        case EBADF:
            //  Ignore as FD is invalidated when CTRL + C is pressed.
            break;

        default:
            fprintf( stdout, "IO_accept: error: %i - %s\n", errno, strerror( errno ) );
        }
        return false;
    }
    else
    {
        *connection = IO_open_mode( IO_new( &fd ), "r+" );

        return true;
    }
}

void
IO_flushAll()
{
    fflush( stdout );
    fflush( stderr );
}

IO* IO_open_mode( IO* self, const char* mode )
{
    if ( self->descriptor )
    {
        self->stream = fdopen( self->descriptor, mode );
    }
    return self;
}

int
IO_write( IO* self, const char* ch )
{
    int result = write( self->descriptor, ch, strlen( ch ) );

    if ( -1 == result ) IO_PrintError( stdout );

    return result;
}

void
IO_close( IO* self )
{
    if ( self->descriptor )
    {
        close ( self->descriptor ); self->descriptor = 0;
    }

    if ( self->stream )
    {
        fclose( self->stream  ); self->stream = NULL;
    }
}

IO*
IO_Socket()
{
    int domain   = AF_INET;
    int type     = SOCK_STREAM;
    int protocol = 0;
    int fd       = socket( domain, type, protocol );

    return IO_new( &fd );
}

KeyValue*
KeyValue_new( const char* key, const char* value )
{
    KeyValue* self = New( sizeof( KeyValue ) );
    if ( self )
    {
        Object_init( &self->super, (Destructor) KeyValue_destruct );

        self->key   = String_new( key );
        self->value = String_new( value );
    }
    return self;
}

KeyValue*
KeyValue_destruct( KeyValue* self )
{
    if ( self )
    {
        String_free( &self->key   );
        String_free( &self->value );
    }
    return self;
}

KeyValue*
KeyValue_free( KeyValue** self )
{
    if ( self && *self )
    {
        KeyValue_destruct( *self );

        Delete( self );
    }
    return null;
}

const String*
KeyValue_getKey( const KeyValue* self )
{
    return self->key;
}

const String*
KeyValue_getValue( const KeyValue*  self )
{
    return self->value;
}

bool
KeyValue_keyEquals_chars( const KeyValue*  self, const char* chars )
{
    return String_contentEquals( self->key, chars );
}

String*
IO_readline( IO* self )
{
    return FGetLine( self->stream, NULL );
}

void
IO_PrintError( FILE* out )
{
    fprintf( out, "Error: (%i) %s\n", errno, strerror( errno ) );
}

static bool IsWhitespace( char ch )
{
    return (ch <= 32) || (127 <= ch);
}

void
Object_init( Object* self, Destructor destruct )
{
    self->type     = OBJECT;
    self->destruct = destruct;
}

void
Object_destruct( Object* self )
{
    self->type     = null;
    self->destruct = null;
}

Path* Path_new( const char* absolute )
{
    Path* self = New( sizeof( Path ) );
    if ( self )
    {
        self->absolute = String_new( absolute );
    }
    return self;
}

Path* Path_free( Path** self )
{
    if ( *self )
    {
        String_free( &(*self)->absolute );
    }
    return Delete( self );
}

const char* Path_getAbsolute( const Path* self )
{
    return String_getChars( self->absolute );
}

Path* Path_parent( const Path* self )
{
    char* copy = CharString_new( String_getChars( self->absolute ) );

    Path* path = Path_new( dirname( copy ) );

    CharString_free( &copy );

    return path;
}

Path* Path_child( const Path* self, const char* child )
{
    char* joined = Join( Path_getAbsolute( self ), child );
    Path* path   = Path_new( joined );

    CharString_free( &joined );

    return path;
}

Path* Path_CurrentDirectory()
{
    char* cwd = getcwd( NULL, 0 );

    Path* path = Path_new( cwd );

    free( cwd );

    return path;
}

int
Platform_Fork()
{
    IO_flushAll();

    return fork();
}

void
Platform_MicroSleep( int microseconds )
{
    usleep( microseconds );
}

void
Platform_MilliSleep( int milliseconds )
{
    int microseconds = milliseconds * 1000;

    Platform_MicroSleep( microseconds );
}

void
Platform_SecondSleep( int seconds )
{
    int milliseconds = seconds * 1000;

    Platform_MilliSleep( milliseconds );
}

void
Platform_Wait()
{
    pid_t any_children = -1;                    // Wait for any child process.
    int*  stat_loc     = NULL;                  // Don't care about result.
    int   dont_block   = WNOHANG | WUNTRACED;   // Do not block if no processes.

    while ( 0 < waitpid( any_children, stat_loc, dont_block ) );
}

void
Security_DropPrivilegesOrAbort()
{
    /* Pretty sure that this code assumes something is being run using sudo
     * and so has a getguid value that is not 0 (root).
     */

    uid_t  uid = getuid();
    uid_t euid = geteuid();
    gid_t  gid = getgid();
    gid_t egid = getegid();

    struct passwd* nobody = getpwnam( "nobody" );

    fprintf( stdout, "Existing privileges, uid: %i, euid: %i, gid: %i egid: %i\n", uid, euid, gid, egid );

    if ( 0 != uid )
    {
        fprintf( stdout, "Keeping existing privileges\n" );
    }
    else
    if ( !nobody )
    {
        fprintf( stdout, "Aborting: could not retrieve details of unprivileged 'nobody' account.\n" );
        abort();
    }
    else
    {
        gid_t groups[2];

        if ( 0 != setgid( nobody->pw_gid ) )
        {
            fprintf( stdout, "Aborting: error calling setgid to drop root group privileges.\n" );
            abort();
        }
        else
        if ( nobody->pw_gid != getgid() )
        {
            fprintf( stdout, "Aborting: group not changed to expected value, expected: %i, got: %i.\n", nobody->pw_gid, getgid() );
            abort();
        }
        else
        {
            fprintf( stdout, "Dropped root group privileges\n" );
        }

        groups[0] = nobody->pw_gid;

        if ( 0 != setgroups( 1, groups ) )
        {
            fprintf( stdout, "Aborting: error calling setgroups to drop root groups list.\n" );
            abort();
        }
        else
        if ( (1 != getgroups( 2, groups )) && (nobody->pw_gid == groups[0]) )
        {
            fprintf( stdout, "Aborting: group list contains more goups than the one expected, found %i groups.\n", getgroups( 0, groups ) );
            abort();
        }
        else
        {
            fprintf( stdout, "Dropped root supplementary groups privileges\n" );
        }

        if ( 0 != setuid( nobody->pw_uid ) )
        {
            fprintf( stdout, "Aborting: error calling setuid to drop root userprivileges.\n" );
            abort();
        }
        else
        if ( nobody->pw_uid != getgid() )
        {
            fprintf( stdout, "Aborting: user not changed to expected value, expected: %i, got: %i.\n", nobody->pw_uid, getuid() );
            abort();
        }
        else
        {
            fprintf( stdout, "Dropped root user privileges\n" );
        }
    }
}

char* Join( const char* dirname, const char* basename )
{
    int  len_dirname  = strlen( dirname  );
    int  len_basename = strlen( basename );
    bool insert_slash = false;

    if
    (
        ('/' != dirname[len_dirname - 1])
        &&
        ('/' != basename[0] )
    )
    {
        len_dirname++;
        insert_slash = true;
    }

    int len = len_dirname + len_basename + 1;

    char* dst = NewArray( sizeof(char), len );

    strncpy( dst, dirname, len_dirname );

    if ( insert_slash )
    {
        dst[len_dirname - 1] = '/';
    }

    strncpy( dst + len_dirname, basename, len_basename );

    return dst;
}

String*
String_new( const char* ch )
{
    String* self = New( sizeof( String ) );
    if ( self )
    {
        self->length = strlen( ch );
        self->data   = NewArray( sizeof( char ), self->length + 1 );

        strcpy( self->data, ch );
    }

    return self;
}

String*
String_free( String** self )
{
    if ( *self )
    {
        (*self)->length = 0;

        CharString_free( &(*self)->data );
    }
    return Delete( self );
}

const char*
String_getChars( const String*  self )
{
    return self->data;
}

int
String_getLength( const String*  self )
{
    return self->length;
}

bool
String_contains( const String*  self, const char* substring )
{
    return (NULL != strstr( self->data, substring ));
}

bool
String_contentEquals( const String* self, const char* string )
{
    return (0 == strcmp( self->data, string ));
}

bool
String_endsWith( const String* self, const char* suffix )
{
    const char* aString = self->data;
    
    int len1 = strlen( suffix );

    if ( 0 == len1 )
    {
        return 0;
    }
    else
    {
        int len2 = strlen( aString );

        if ( len1 > len2 )
        {
            return 0;
        }
        else
        {
            const char* _suffix = &(suffix [len1]);
            const char* _string = &(aString[len2]);

            do
            {
                _suffix--;
                _string--;

                if ( _suffix == suffix )
                {
                    return (*_suffix == *_string);
                }
            }
            while ( *_suffix == *_string );

            return 0;
        }
    }
}

String*
String_extension( const String* self, const char separator )
{
    int index = self->length - 1;

    while ( (0 <= index) && (separator != self->data[index]) ) index--;

    if ( 0 <= index )
    {
        return String_substring_index( self, index );
    }
    else
    {
        return String_new( "" );
    }
}

bool
String_isNumeric( const String*  self )
{
    int n = String_getLength( self );

    for ( int i=0; i < n; i++ )
    {
        char ch = self->data[i];

        if ( (ch < '0') || ('9' < ch) ) return false;
    }
    return true;
}

bool
String_startsWith( const String* self, const char* prefix )
{
    return (0 == strncmp( self->data, prefix, strlen( prefix ) ));
}

String*
String_substring_index( const String* self, int index )
{
    return String_new( &self->data[index] );
}

String*
String_substring_index_length( const String* self, int index, int length )
{
    String* substring = String_substring_index( self, index );
    {
        for ( int i=length; i < substring->length; i++ )
        {
            substring->data[i] = '\0';
            substring->length = length;
        }
    }
    return substring;
}

int
String_toNumber( const String* self )
{
    return atoi( self->data );
}

int
String_indexOf_ch_skip( const String* self, char ch, int skip )
{
    const char* data = self->data;
    int         len  = self->length;

    for ( int i=0; i < len; i++ )
    {
        if ( ch == data[i] )
        {
            if ( 0 == skip-- ) return i;
        }
    }
    return -1;
}

String*
String_deroot( String* self )
{
    while ( '/' == self->data[0] )
    {
        char* tmp = CharString_new( &self->data[1] );

        CharString_free( &self->data );

        self->data   = tmp;
        self->length = strlen( tmp );
    }
    return self;
}

String*
String_trimEnd( String* self )
{
    while ( (0 < self->length) && IsWhitespace( self->data[self->length-1] ) )
    {
        self->length--;
        self->data[self->length] = '\0';
    }
    return self;
}

Array*
String_toArray_separator( const String* self, char separator )
{
    Array* parts = Array_new_free( (Free) String_free );

    int loop  = 1;
    int start = 0;
    int skip  = 0;

    do
    {
        int index = String_indexOf_ch_skip( self, separator, skip );

        if ( -1 != index )
        {
            int len = index - start;

            if ( len > 0 )
            {
                String* part = String_substring_index_length( self, start, len );
                Array_append_element( parts, (void**) &part );
            }

            start = index + 1;
            skip++;
        }
        else
        {
            String* part = String_substring_index( self, start );
            if ( String_getLength( part ) > 0 )
            {
                Array_append_element( parts, (void**) &part );
            }
            else
            {
                String_free( &part );
            }
            loop = 0;
        }
    }
    while ( loop );

    return parts;
}

String*
String_reverseParts_separator( const String* self, char separator )
{
    String*       reversed = null;
    StringBuffer* buffer   = StringBuffer_new();
    {
        Array* parts = String_toArray_separator( self, separator );
        char sep[2] = { separator, '\0' };
        int count = Array_count( parts );

        if ( count )
        {
            for ( int i=count-1; i > -1; i-- )
            {
                StringBuffer_append_string( buffer, (String*) Array_get_index( parts, i ) );
                if ( i )
                {
                    StringBuffer_append_chars( buffer, sep );
                }
            }
        }
        Array_free_destructor( &parts, (void *(*)(void **)) String_free );
    }
    reversed = String_new( StringBuffer_getChars( buffer ) );

    StringBuffer_free( &buffer );

    return reversed;
}

String*
FGetLine( FILE* stream, size_t* len )
{
    String* line = NULL;
    {
        size_t sz     = 1024;
        char*  buffer = NewArray( sizeof( char ), sz );
        size_t i      = 0;
        bool   loop   = true;

        do
        {
            if ( (sz - 1) == i )
            {
                sz *= 2;
                buffer = realloc( buffer, sz * sizeof( char ) );
            }

            int ch = fgetc( stream );

            switch ( ch )
            {
            case EOF:
                loop = false;
                break;

            case '\n':
                buffer[i++] = ch;
                loop        = false;

            default:
                buffer[i++] = ch;
            }

        } while ( loop );

        line = String_new    ( buffer );
        line = String_trimEnd( line   );

        if ( len ) *len = i;

        DeleteArray( &buffer );

    }
    return line;
}

StringBuffer*
StringBuffer_new()
{
    StringBuffer* self = New( sizeof( StringBuffer ) );
    if ( self )
    {
        self->capacity = 1;
        self->length   = 0;
        self->data     = NewArray( sizeof( char ), self->capacity );
    }

    return self;
}

StringBuffer*
StringBuffer_free( StringBuffer** self )
{
    if ( *self )
    {
        (*self)->capacity = 0;
        (*self)->length   = 0;

        CharString_free( &(*self)->data );
    }
    return Delete( self );
}

void
StringBuffer_append_chars( StringBuffer* self, const char* chars )
{
    //  self->capacity = 16;
    //  self->length   = 8;
    //  self->data     = "Original00000000"
    //  chars          = "NewString"

    int charlen = CharString_length( chars );       // 9
    int newlen  = self->length + charlen;       // 17 = 8 + 9
    int oldcap  = self->capacity;           // 16

    while ( newlen > (self->capacity - 1) )
    {
        self->capacity *= 2;                // 32
    }

    if ( oldcap != self->capacity )
    {
                                        //  0123456789X123456789X123456789X12
        self->data = realloc( self->data, self->capacity ); // "Original00000000################"

        for ( int i=oldcap; i < self->capacity; i++ )
        {
            self->data[i] = '\0';
        }
                                        //                  |<------------>|
                                        //  0123456789X123456789X123456789X12
                                        // "Original000000000000000000000000"
    }

    for ( int i=0; i < charlen; i++ )
    {
        self->data[self->length + i] = chars[i];
    }

                                        //          |<----->|
                                        //  0123456789X123456789X123456789X12
                                        // "OriginalNewString000000000000000"

    self->length = newlen;
}

void
StringBuffer_append_string( StringBuffer* self, const String* string )
{
    StringBuffer_append_chars( self, String_getChars( string ) );
}

void
StringBuffer_append_number( StringBuffer*  self, int number )
{
    int digits = abs(number / 10) + 1;

    char* buffer = NewArray( sizeof( char ), digits + 1 );

    sprintf( buffer, "%i", number );

    StringBuffer_append_chars( self, buffer );

    DeleteArray( &buffer );
}

const char*
StringBuffer_getChars( const StringBuffer* self )
{
    return self->data;
}

void Swap( void* _one, void* _two )
{
    void** one = _one;
    void** two = _two;
    void*  tmp;

    tmp  = *one;
    *one = *two;
    *two =  tmp;
}

void* TakeElement( void** given )
{
    void* keeper = *given; *given = null;

    return keeper;
}
