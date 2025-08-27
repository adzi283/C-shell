#ifndef __MYSTRING__
#define __MYSTRING__

#include "mytypes.h"

typedef struct st_String
{
    char* cstr;
    size_t buflen;
    size_t len;
    size_t offset;
    bool_t dirty;
} st_String;

typedef st_String* String;

String string_create(char* cstr, size_t buflen);
String string_create_copy(String s);
String string_create_copyc(char* s);

errcode_t string_set_offset(String s, size_t off);
void string_set_equal(String s1, String s2);
void string_resize(String s, size_t newsize);

char* string_get_cstr(String s);
size_t string_get_strlen(String s);
char string_get_idx(String s, size_t idx);

errcode_t string_fetch(String s, errcode_t (*funcPtr)(char*, size_t));
errcode_t string_modify(String s, size_t pos, char val);

bool_t string_is_prefix(String s, String prefix);
bool_t string_is_prefixc(String s, char* prefix);
bool_t string_cmp_idx(String s, size_t idx, char c);
bool_t string_is_equal(String s1, String s2);
bool_t string_is_equalc(String s1, char* s2);
bool_t string_has_csubstr(String s, char* sub);

String string_substr_abs(String s, size_t start, size_t offt);
char* string_subcstr_abs(String s, size_t start, size_t offt);

String string_add(String s1, String s2);
String string_addc(String s1, char* s2);

void string_delete(String s);

#endif