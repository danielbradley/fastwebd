#include "libbase.h"
#include "libbase.private.h"

#include <sys/sendfile.h>

int
IO_sendFile( IO* self, IO* file )
{
    size_t count  = 0x7ffff000;

    int result = sendfile( self->descriptor, file->descriptor, NULL, count );

    if ( -1 == result )
    {
        IO_PrintError( stdout );
    }
    return result;
}
