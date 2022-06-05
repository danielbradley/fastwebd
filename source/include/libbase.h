#ifndef LIBBASE_H
#define LIBBASE_H

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef null
#define null 0
#endif

#include "libbase/types.h"
#include "libbase/Address.h"
#include "libbase/CharString.h"
#include "libbase/File.h"
#include "libbase/IO.h"
#include "libbase/Path.h"
#include "libbase/String.h"

void* New     ( int    size            );
void* NewArray( int    size, int count );
void* Del     ( void** ptrptr          );
void* DelArray( void** ptrptr          );
int   Exit    ( int    exit            );

#define Delete( self )      Del( (void**) self )
#define DeleteArray( self ) DelArray( (void**) self )

#endif