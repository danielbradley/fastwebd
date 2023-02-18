#include "libbase.h"

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
#include <unistd.h>
#include <grp.h>

#define NOT_FOUND -1

#define MIME_UNKNOWN        "application/binary"
#define MIME_APP_JS         "application/javascript"
#define MIME_IMAGE_GIF      "image/gif"
#define MIME_IMAGE_JPG      "image/jpeg"
#define MIME_IMAGE_PNG      "image/png"
#define MIME_TEXT_CSS       "text/css"
#define MIME_TEXT_PLAIN     "text/plain"
#define MIME_TEXT_HTML      "text/html"
#define MIME_TEXT_HTML_UTF8 "text/html;charset=UTF-8"
#define DEFAULT_CAPACITY    255
#define NOT_FOUND          -1

static const char*   File_DetermineMimeType( const String* extension );
static       String* FGetLine( FILE* stream, size_t* len );

//static
       char* Join( const char* dirname, const char* basename );

static int Objects = 0;
static int Arrays  = 0;

struct _Array
{
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

void*
New( int size )
{
	Objects++;
	return calloc( 1, size );
}

void*
NewArray( int size, int count )
{
	Arrays++;
	return calloc( count, size );
}

void*
Del( void** self )
{
	if ( *self )
	{
		free( *self ); *self = 0;
		Objects--;
	}
	return *self;
}

void*
DelArray( void** self )
{
	if ( *self )
	{
		free( *self ); *self = 0;
		Arrays--;
	}
	return *self;
}

int Exit( int exit )
{
	if ( Objects + Arrays ) printf( "Objects: %i, Arrays: %i\n", Objects, Arrays );

	return exit;
}

Array*
Array_new()
{
	Array* self = New( sizeof( Array ) );
	if ( self )
	{
		self->count    = 0;
		self->capacity = DEFAULT_CAPACITY;
		self->elements = NewArray( sizeof( void* ), self->capacity );
	}
	return self;
}

Array*
Array_free( Array** self )
{
	if ( *self )
	{
		(*self)->count    = 0;
		(*self)->capacity = DEFAULT_CAPACITY;
		DeleteArray( &(*self)->elements );
	}
	return Delete( self );
}

Array*
Array_free_destructor( Array** self, void* (*free)( void** ) )
{
	if ( *self )
	{
		Array* _self = *self;

		for ( int i=0; i < _self->capacity; i++ )
		{
			if ( _self->elements[i] ) _self->elements[i] = free( &_self->elements[i] );
		}
	}
	return Array_free( self );
}

int
Array_count( const Array* self )
{
	return self->count;
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

                    if ( i < (count-1) )
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
		inner->sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	}
	return address;
}

Address*
Address_free( Address** self )
{
	return Delete( self );
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

	//		start = "GET /index.html 1.1\r\n"  len = 21
	//		copy  = "                       0" len = 22

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
		self->io       = IO_open_mode( IO_new( &fd ), "r" );
		self->byteSize = buf.st_size;
	}

	return self;
}

File* File_close( File* self )
{
	IO_free( &self->io );

	return self;
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

static const char* File_DetermineMimeType( const String* extension )
{
	if ( String_endsWith( extension, ".gif" ) )
	{
		return MIME_IMAGE_GIF;
	}
	else
	if ( String_endsWith( extension, ".png" ) )
	{
		return MIME_IMAGE_PNG;
	}
	else
	if ( String_endsWith( extension, ".jpg" ) )
	{
		return MIME_IMAGE_JPG;
	}
	else
	if ( String_endsWith( extension, ".jpeg" ) )
	{
		return MIME_IMAGE_JPG;
	}
	else
	if ( String_endsWith( extension, ".txt" ) )
	{
		return MIME_TEXT_PLAIN;
	}
	else
	if ( String_endsWith( extension, ".htm" ) )
	{
		return MIME_TEXT_HTML;
	}
	else
	if ( String_endsWith( extension, ".html" ) )
	{
		return MIME_TEXT_HTML;
	}
	else
	if ( String_endsWith( extension, ".css" ) )
	{
		return MIME_TEXT_CSS;
	}
	else
	if ( String_endsWith( extension, ".js" ) )
	{
		return MIME_APP_JS;
	}
	else
	if ( String_endsWith( extension, ".mjs" ) )
	{
		return MIME_APP_JS;
	}
	else
	{
		return MIME_UNKNOWN;
	}
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
		close ( (*self)->descriptor  ); (*self)->descriptor = 0;
	}
	Delete( self );
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
			// Ignore as expected if CTRL + C is received.
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
		}
	}
	return substring;
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
    Array* parts = Array_new();

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
	//	self->capacity = 16;
	//	self->length   = 8;
	//	self->data     = "Original00000000"
	//	chars          = "NewString"

	int charlen = CharString_length( chars );		// 9
	int newlen  = self->length + charlen;		// 17 = 8 + 9
	int oldcap  = self->capacity;			// 16

	while ( newlen > (self->capacity - 1) )
	{
		self->capacity *= 2;				// 32
	}

	if ( oldcap != self->capacity )
	{
										//  0123456789X123456789X123456789X12
		self->data = realloc( self->data, self->capacity );	// "Original00000000################"

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
