#ifndef LIBBASE_ADDRESS_H
#define LIBBASE_ADDRESS_H

Address* Address_new_port( short port );

Address* Address_free( Address** self );

#endif