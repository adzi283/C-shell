#include <stdlib.h>

#include "vector.h"
#include "mystring.h"
#include "jobctrl.h"

void vector_free_cstr(Vector v) {
    char** strs = (char**)v->data;
    for (int i = 0; i < v->len; i++)
        free(strs[i]);
}

void vector_free_str(Vector v) {
    String* strs = (String*)v->data;
    for (int i = 0; i < v->len; i++)
        string_delete(strs[i]);
}

void vector_free_process(Vector v) {
    Process* procs = (Process*)v->data;
    for (int i = 0; i < v->len; i++)
        process_delete(procs[i]);
}