#include <stdlib.h>

#include "vector.h"
#include "mystring.h"

Vector vector_create(size_t buflen) {
    Vector v = malloc(sizeof(st_Vector));
    v->buflen = buflen > 2 ? buflen : 2;
    v->data = malloc(sizeof(void*)*v->buflen);
    v->len = 0;
    return v;
}

void vector_append(Vector v, void* data) {
    if (v->len == v->buflen)
    {
        v->buflen *= 2;
        v->data = realloc(v->data, sizeof(void*)*v->buflen);
    }

    v->data[v->len++] = data;
}

void vector_delete(Vector v) {
    free(v->data);
    free(v);
}