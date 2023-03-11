#ifndef LIBBASE_ADDRESS_H
#define LIBBASE_ADDRESS_H

Address* Address_new_port( short port );

String*  Address_origin( const Address* self );

#endif
