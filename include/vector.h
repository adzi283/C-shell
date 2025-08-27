#ifndef __VECTOR__
#define __VECTOR__

#include "mytypes.h"

typedef struct st_Vector
{
    void** data;
    size_t buflen;
    size_t len;
} st_Vector;

typedef st_Vector* Vector;

Vector vector_create(size_t buflen);

void vector_append(Vector v, void* data);

void vector_delete(Vector v);

#endif