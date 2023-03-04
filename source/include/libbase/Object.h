#ifndef LIBBASE_OBJECT_H
#define LIBBASE_OBJECT_H

void Object_init    ( Object* self, Destructor destruct );
void Object_destruct( Object* self );

#endif
