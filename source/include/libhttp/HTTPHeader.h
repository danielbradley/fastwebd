#ifndef LIBHTTP_HTTPHEADER_H
#define LIBHTTP_HTTPHEADER_H

HTTPHeader*  HTTPHeader_new ( String**     line );
HTTPHeader*  HTTPHeader_free( HTTPHeader** self );

#endif