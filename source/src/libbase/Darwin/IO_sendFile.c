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
#include <uuid/uuid.h>

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
