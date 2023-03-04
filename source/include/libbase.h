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

typedef void*(*Destructor)(void*);

#include "libbase/types.h"
#include "libbase/Address.h"
#include "libbase/Arguments.h"
#include "libbase/Array.h"
#include "libbase/ArrayOfFile.h"
#include "libbase/CharString.h"
#include "libbase/File.h"
#include "libbase/IO.h"
#include "libbase/KeyValue.h"
#include "libbase/Object.h"
#include "libbase/Path.h"
#include "libbase/Platform.h"
#include "libbase/Security.h"
#include "libbase/String.h"
#include "libbase/StringBuffer.h"
#include "libbase/SysLog.h"

#endif
