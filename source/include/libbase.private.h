
#ifndef LIBBASE_PRIVATE_H
#define LIBBASE_PRIVATE_H

struct _IO
{
    FD    descriptor;
    FILE* stream;
};

#endif
