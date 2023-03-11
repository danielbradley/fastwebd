
#ifndef LIBBASE_PRIVATE_H
#define LIBBASE_PRIVATE_H

struct _IO
{
    Object super;
    FD     descriptor;
    FILE*  stream;
};

#endif
