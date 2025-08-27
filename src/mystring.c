#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mystring.h"

String string_create(char* cstr, size_t buflen) {
    String str = malloc(sizeof(st_String));
    
    if (!cstr) {
        if (buflen > 0)
            str->cstr = malloc(sizeof(char)*buflen);
        else
            str->cstr = NULL;
        str->buflen = buflen;
        str->len = 0;
    } else {
        str->cstr = cstr;
        str->len = strlen(str->cstr);
        if (buflen > 0)
            str->buflen = buflen;
        else
            str->buflen = str->len;
    }

    str->offset = 0;
    str->dirty = false;
    return str;
}

char* string_get_cstr(String s) {
    if (!s || !s->cstr)
        return NULL;
    return s->cstr+s->offset;
}

size_t string_get_strlen(String s) {
    if (!s->dirty)
        return s->len;
    s->dirty = false;
    if (!string_get_cstr(s))
        return s->len = 0;
    return s->len = strlen(string_get_cstr(s));
}

bool_t string_check_idx(String s, size_t idx) {
    return (s->buflen >= s->offset+idx);
}

char string_get_idx(String s, size_t idx) {
    if (!string_check_idx(s, idx))
        return -1;
    return (string_get_cstr(s))[idx];
}

bool_t string_cmp_idx(String s, size_t idx, char c) {
    return (string_get_idx(s, idx) == c);
}

errcode_t string_set_offset(String s, size_t off) {
    if (!string_check_idx(s, off))
        return -1;
    s->dirty = true;
    s->offset = off;
    return 0;
}

errcode_t string_modify(String s, size_t pos, char val) {
    if (!string_check_idx(s, pos))
        return -1;
    s->dirty = true;
    s->cstr[s->offset + pos] = val;
    return 0;
}

errcode_t string_fetch(String s, errcode_t (*funcPtr)(char*, size_t)) {
    s->dirty = true;
    return funcPtr(s->cstr, s->buflen);
}

bool_t string_is_prefix(String s, String prefix) {
    if (string_get_strlen(s) < string_get_strlen(prefix))
        return false;
    return (strncmp(string_get_cstr(s), string_get_cstr(prefix), string_get_strlen(prefix)) == 0);
}

bool_t string_is_equal(String s1, String s2) {
    return (strcmp(string_get_cstr(s1), string_get_cstr(s2)) == 0);
}

bool_t string_is_equalc(String s1, char* s2) {
    return (strcmp(string_get_cstr(s1), s2) == 0);
}

bool_t string_is_prefixc(String s, char* prefix) {
    String tempstr = string_create(prefix, 0);
    bool_t ret = string_is_prefix(s, tempstr);
    free(tempstr);
    return ret;
}

void string_copy(String s1, String s2) {
    if (!string_check_idx(s1, string_get_strlen(s2)))
        return;
    s1->dirty = true;
    strncpy(string_get_cstr(s1), string_get_cstr(s2), string_get_strlen(s2));
    string_modify(s1, string_get_strlen(s2), 0);
}

void string_copyn(String s1, String s2, size_t n) {
    if (!string_check_idx(s1, n) || !string_check_idx(s2, n-1))
        return;
    s1->dirty = true;
    strncpy(string_get_cstr(s1), string_get_cstr(s2), n);
    string_modify(s1, n, 0);
}

void string_resize(String s, size_t newsize) {
    s->buflen = newsize;
    s->cstr = realloc(s->cstr, sizeof(char)*newsize);
    s->dirty = true;
}

void string_set_equal(String s1, String s2) {
    if (!string_check_idx(s1, string_get_strlen(s2)))
        string_resize(s1, string_get_strlen(s2)+1);
    string_copy(s1, s2);
}

String string_create_copy(String s) {
    String new = string_create(NULL, string_get_strlen(s)+1);
    string_copy(new, s);
    return new;
}

String string_create_copyc(char* s) {
    String temp = string_create(s, 0);
    String ret = string_create_copy(temp);
    free(temp);
    return ret;
}

String string_substr_abs(String s, size_t start, size_t offt) {
    // printf("%ld - %ld - %ld - %s - %d - %ld - %ld\n", start, offt, s->buflen, s->cstr, s->dirty, s->len, s->offset);
    if (!string_check_idx(s, start+offt))
        return NULL;
    String sub = string_create(NULL, offt+1);
    size_t prevoff = s->offset;
    string_set_offset(s, start);
    string_copyn(sub, s, offt);
    string_set_offset(s, prevoff);
    return sub;
}

char* string_subcstr_abs(String s, size_t start, size_t offt) {
    String sub = string_substr_abs(s, start, offt);
    char* ret = sub->cstr;
    free(sub);
    return ret;
}

bool_t string_has_csubstr(String s, char* sub) {
    return (strstr(string_get_cstr(s), sub) != NULL);
}

String string_add(String s1, String s2) {
    String new = string_create(NULL, string_get_strlen(s1)+string_get_strlen(s2)+1);
    string_copy(new, s1);
    string_set_offset(new, string_get_strlen(s1));
    string_copy(new, s2);
    string_set_offset(new, 0);
    return new;
}

String string_addc(String s1, char* s2) {
    String tempstr = string_create_copyc(s2);
    String new = string_add(s1, tempstr);
    string_delete(tempstr);
    return new;
}

void string_delete(String s) {
    if (s)
        free(s->cstr);
    free(s);
}