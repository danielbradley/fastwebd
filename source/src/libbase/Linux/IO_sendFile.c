#include "libbase.h"

#include <sys/sendfile.h>

struct _IO
{
    FD    descriptor;
    FILE* stream;
};

int
IO_sendFile( IO* self, IO* file )
{
    off_t  offset = 0;
    size_t count  = 0;

    int result = sendfile( file->descriptor, self->descriptor, NULL, count );

    if ( -1 == result )
    {
        IO_PrintError( stdout );
    }
    return result;
}
