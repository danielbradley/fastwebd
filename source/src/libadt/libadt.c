#include "libbase.h"
#include "libadt.h"

#define DEFAULT_CAPACITY 255
#define NOT_FOUND         -1

struct _Array
{
	int    count;
	int    capacity;
	void** elements;
};

static void Array_resize( Array* self, int index );

Array*
Array_new()
{
	Array* self = New( sizeof( Array ) );
	if ( self )
	{
		self->count    = 0;
		self->capacity = DEFAULT_CAPACITY;
		self->elements = NewArray( sizeof( void* ), self->capacity );
	}
	return self;
}

Array*
Array_free( Array** self )
{
	if ( *self )
	{
		(*self)->count    = 0;
		(*self)->capacity = DEFAULT_CAPACITY;
		Delete( &(*self)->elements );
	}
	return *self;
}

int
Array_count( const Array* self )
{
	return self->count;
}

int
Array_getFirstIndex( const Array* self )
{
	int index = 0;

	while ( (index < self->capacity) && !self->elements[index] ) index++;

	if ( index == self->capacity )
	{
		return NOT_FOUND;
	}
	else
	{
		return index;
	}
}

int
Array_getLastIndex( const Array* self )
{
	int index = self->capacity;

	while ( (0 < index) && !self->elements[index - 1] ) index--;

	if ( 0 == index )
	{
		return NOT_FOUND;
	}
	else
	{
		return index;
	}
}

const void*
Array_getFirst( const Array* self )
{
	int index = Array_getFirstIndex( self );

	return (NOT_FOUND == index) ? null : Array_get_index( self, index );
}

const void* Array_get_index( const Array* self, int index )
{
	return (0 <= index) && (index < self->capacity) ? self->elements[index] : null;
}

const void*
Array_getLast( const Array* self )
{
	int index = Array_getLastIndex( self );

	return (NOT_FOUND == index) ? null : Array_get_index( self, index );
}

void
Array_append_element( Array* self, void** element )
{
	int index = Array_getLastIndex( self );

	index = (NOT_FOUND == index) ? 0 : index + 1;

	Array_replace_element_index( self, index, element );
}

void*
Array_replace_element_index( Array* self, int index, void** element )
{
	Array_resize( self, index );

	void* ret = self->elements[index];

	if ( element && *element )
	{
		self->elements[index] = *element; *element = null;
	}

	return ret;
}

void*
Array_remove_index( Array* self, int index )
{
	void* ret = null;

	if ( (0 <= index) && (index < self->capacity) )
	{
		ret = self->elements[index]; self->elements[index] = null;
	}
	return null;
}

static void Array_resize( Array* self, int index )
{
	int new_size = self->capacity;

	while ( new_size < (index + 1) )
	{
		new_size = new_size + new_size;
	}

	if ( self->capacity != new_size )
	{
		void** elements = NewArray( sizeof(void*), new_size );

		for ( int i=0; i < self->capacity; i++ )
		{
			elements[i] = self->elements[i]; self->elements[i] = 0;
		}

		Delete( &self->elements );

		self->elements = elements;
		self->capacity = new_size;
	}
}

