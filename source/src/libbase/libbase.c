#include "libbase.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define NOT_FOUND -1

#define MIME_UNKNOWN        "application/binary"
#define MIME_APP_JS         "application/javascript"
#define MIME_IMAGE_GIF      "image/gif"
#define MIME_IMAGE_JPG      "image/jpeg"
#define MIME_IMAGE_PNG      "image/png"
#define MIME_TEXT_PLAIN     "text/plain"
#define MIME_TEXT_HTML      "text/html"
#define MIME_TEXT_HTML_UTF8 "text/html;charset=UTF-8"

static const char*   File_DetermineMimeType( const String* extension );
static       void    IO_PrintError( FILE* out );
static       String* FGetLine( FILE* stream, size_t* len );

//static
       char* Join( const char* dirname, const char* basename );

static int Objects = 0;
static int Arrays  = 0;

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

File* File_new ( const char* filepath )
{
	File* self = New( sizeof( File ) );
	if ( self )
	{
		self->filepath  = String_new( filepath );
		self->extension = String_extension( self->filepath, '.' );
		self->mimeType  = File_DetermineMimeType( self->extension );
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
		self->io       = IO_new( &fd );
		self->byteSize = buf.st_size;
	}

	return self;
}

static const char* File_DetermineMimeType( const String* extension )
{
	if ( String_startsWith( extension, ".gif" ) )
	{
		return MIME_IMAGE_GIF;
	}
	else
	if ( String_startsWith( extension, ".png" ) )
	{
		return MIME_IMAGE_PNG;
	}
	else
	if ( String_startsWith( extension, ".jpg" ) )
	{
		return MIME_IMAGE_JPG;
	}
	else
	if ( String_startsWith( extension, ".jpeg" ) )
	{
		return MIME_IMAGE_JPG;
	}
	else
	if ( String_startsWith( extension, ".txt" ) )
	{
		return MIME_TEXT_PLAIN;
	}
	else
	if ( String_startsWith( extension, ".htm" ) )
	{
		return MIME_TEXT_HTML;
	}
	else
	if ( String_startsWith( extension, ".html" ) )
	{
		return MIME_TEXT_HTML;
	}
	else
	if ( String_startsWith( extension, ".css" ) )
	{
		return MIME_TEXT_HTML;
	}
	else
	if ( String_startsWith( extension, ".js" ) )
	{
		return MIME_APP_JS;
	}
	else
	if ( String_startsWith( extension, ".mjs" ) )
	{
		return MIME_APP_JS;
	}
	else
	{
		return MIME_UNKNOWN;
	}
}

IO* IO_new( FD* descriptor )
{
	IO* self = New( sizeof( IO ) );
	if ( self )
	{
		self->descriptor = *descriptor; *descriptor = 0;
		self->stream     = fdopen( self->descriptor, "r" );
	}
	return self;
}

IO* IO_free( IO** self )
{
	if ( *self )
	{
		fclose( (*self)->stream ); (*self)->stream = NULL;
		(*self)->descriptor = 0;

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
			// Ignore as expected if CTRL + C is received.
			break;

		default:
			fprintf( stdout, "IO_accept: error: %i - %s\n", errno, strerror( errno ) );
		}
		return false;
	}
	else
	{
		*connection = IO_new( &fd );

		return true;
	}
}

int
IO_sendFile( IO* self, IO* file )
{
	off_t           offset = 0;
	off_t           len    = 0;
	struct sf_hdtr* hdtr   = NULL;
	int             flags  = 0;

	int result = sendfile( file->descriptor, self->descriptor, offset, &len, hdtr, flags );

	if ( -1 == result )
	{
		IO_PrintError( stdout );
	}
	return result;
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
	fclose( self->stream );
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
