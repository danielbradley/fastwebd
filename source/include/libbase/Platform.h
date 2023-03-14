#ifndef LIBBASE_PLATFORM_H
#define LIBBASE_PLATFORM_H

//void*  New        ( int    size            );
void*  NewArray   ( int    size, int count );
void*  Del        ( void** ptrptr          );
void*  DelArray   ( void** ptrptr          );
int    Exit       ( int    exit            );
void** Give       ( void*  pointer         );
void   Swap       ( void*  one, void* two  );
void*  TakeElement( void** given           );
void   MemInfo    (                        );

typedef void*(*Free)(void**);

#define         New( size        )         Platform_New(  size,  0     )
#define    NewArray( size, count )         Platform_New(  size,  count )
#define      Delete( self        )      Platform_Delete( (void**) self )
#define DeleteArray( self        ) Platform_DeleteArray( (void**) self )
#define        Take( self        )          TakeElement( (void**) self )

void* Platform_New        ( int size, int count );
bool  Platform_ChangeDir  ( const Path* path    );
void* Platform_Delete     ( void** element      );
int   Platform_Fork       (                     );
void  Platform_MicroSleep ( int microseconds    );
void  Platform_MilliSleep ( int milliseconds    );
void  Platform_SecondSleep( int      seconds    );
void  Platform_Wait       (                     );

#endif
