#include "libbase.h"

#include <netinet/ip.h>

struct _IO
{
    FD    descriptor;
    FILE* stream;
};

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
