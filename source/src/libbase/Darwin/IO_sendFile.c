#include "libbase.h"
#include "libbase.private.h"

#include <netinet/ip.h>

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
