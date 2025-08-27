#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mystring.h"
#include "vector.h"
#include "vecutils.h"
#include "parser.h"
#include "argparse.h"

#define AT_INVALID_FLAG_ARG -1
#define AT_NO_MORE_POSIT_ARGS -2
#define AT_POSIT_ARG_NA_IN_MULTIFLAG_ARG -3
#define AT_INC_POSIT_COUNT -4
#define AT_EXPECTED_ADD -5

/*
options:
Of the form "(-|+)arg1,...".
('-'|'+') indicates that an argument is a flag argument.
'-' indicates that it is merely a boolean flag argument, passed to signify true and omitted to signify false.
'+' indicates that the flag argument requires an additional argument, which is assumed to be the very next argument.
No ('-'|'+') indicates that an argument is positional.
The first non-flag argument will be set to the first positional argument, the second to the second, and so on.
*/

ArgTable argtable_create(String options) {
    Vector args = vector_create(0);
    bool_t syntax_error = false;
    
    size_t iter = 0;
    while (iter < string_get_strlen(options))
    {
        if (string_cmp_idx(options, iter, '-') || string_cmp_idx(options, iter, '+'))
        {
            iter++;
            if (iter == string_get_strlen(options) || is_whitespace(string_get_idx(options, iter))\
                                                   || string_cmp_idx(options, iter, '-')\
                                                   || string_cmp_idx(options, iter, '+')\
                                                   || string_cmp_idx(options, iter, ','))
            {
                syntax_error = true;
                break;
            }
            iter++;
            if (iter < string_get_strlen(options) && !string_cmp_idx(options, iter, ','))
            {
                syntax_error = true;
                break;
            }
            vector_append(args, string_substr_abs(options, iter-2, 2));
        }
        else
        {
            size_t start = iter;
            while (iter < string_get_strlen(options) && !string_cmp_idx(options, iter, ',')\
                                                     && !string_cmp_idx(options, iter, '-')\
                                                     && !string_cmp_idx(options, iter, '+')\
                                                     && !is_whitespace(string_get_idx(options, iter)))
                iter++;
            if (iter < string_get_strlen(options) && !string_cmp_idx(options, iter, ','))
            {
                syntax_error = true;
                break;
            }
            if (iter-start == 0)
            {
                syntax_error = true;
                break;
            }
            vector_append(args, string_substr_abs(options, start, iter-start));
        }
        iter++;
    }

    if (syntax_error || args->len == 0)
    {
        vector_free_str(args);
        vector_delete(args);
        return NULL;
    }

    Vector values = vector_create(args->len);
    values->len = args->len;
    for (int i = 0; i < values->len; i++)
        values->data[i] = NULL;

    ArgTable argtab = malloc(sizeof(st_ArgTable));
    argtab->args = args;
    argtab->values = values;
    return argtab;
}

void argtable_delete(ArgTable argtab) {
    vector_free_str(argtab->args);
    vector_delete(argtab->args);
    vector_free_str(argtab->values);
    vector_delete(argtab->values);
    free(argtab);
}

int argtable_get_flag_pos(ArgTable argtab, char flag) {
    String* args = (String*)argtab->args->data;
    for (size_t i = 0; i < argtab->args->len; i++)
        if (string_get_strlen(args[i]) == 2 && string_cmp_idx(args[i], 1, flag))
            return i;
    return AT_INVALID_FLAG_ARG;
}

int argtable_get_posit_pos(ArgTable argtab, char* arg, size_t offt) {
    String* args = (String*)argtab->args->data;
    size_t num_posits = 0;
    for (size_t i = 0; i < argtab->args->len; i++)
    {
        if (string_cmp_idx(args[i], 0, '-') || string_cmp_idx(args[i], 0, '+'))
            continue;
        if (num_posits < offt)
        {
            num_posits++;
            continue;
        }
        return i;
    }
    return AT_NO_MORE_POSIT_ARGS;
}

int argtable_set_arg(ArgTable argtab, char* arg, size_t offset, bool_t next_is_additional) {
    String argstr = string_create_copyc(arg);
    if (string_cmp_idx(argstr, 0, '-') && string_get_strlen(argstr) > 1)
    {
        if (next_is_additional)
            return AT_EXPECTED_ADD;
        
        if (string_get_strlen(argstr) == 2)
        {
            int pos = argtable_get_flag_pos(argtab, string_get_idx(argstr, 1));
            string_delete(argstr);
            if (pos < 0)
                return pos;
            if (string_cmp_idx(argtab->args->data[pos], 0, '+'))
                return pos+1;
            argtab->values->data[pos] = string_create(NULL, 0);
            return 0;
        }
        for (int i = 1; i < argstr->len; i++)
        {
            int pos = argtable_get_flag_pos(argtab, string_get_idx(argstr, i));
            if (pos < 0)
            {
                string_delete(argstr);
                return pos;
            }
            if (string_cmp_idx(argtab->args->data[pos], 0, '+'))
                return AT_POSIT_ARG_NA_IN_MULTIFLAG_ARG;
            if (!argtab->values->data[pos])
                argtab->values->data[pos] = string_create(NULL, 0);
        }
        string_delete(argstr);
        return 0;
    }

    string_delete(argstr);
    if (next_is_additional)
        return 0;

    int pos = argtable_get_posit_pos(argtab, arg, offset);
    if (pos < 0)
        return pos;
    argtab->values->data[pos] = string_create_copyc(arg);
    return AT_INC_POSIT_COUNT;
}

ArgTable parse_args(String options, Vector argv) {
    ArgTable argtab = argtable_create(options);
    string_delete(options);
    if (!argtab)
        return NULL;

    char** args = (char**)argv->data;

    bool_t is_err = false;

    bool_t next_is_additional = false;
    size_t additional_idx;

    size_t posit_offset = 0;
    int i;
    for (i = 1; i < argv->len-1; i++)
    {
        int ret = argtable_set_arg(argtab, args[i], posit_offset, next_is_additional);
        if (ret == AT_EXPECTED_ADD)
        {
            fprintf(stderr, "%s: Flag %s requires an argument, found flag.\n", args[0], args[i-1]);
            is_err = true;
            break;
        }
        else if (ret == AT_INVALID_FLAG_ARG)
        {
            fprintf(stderr, "%s: Invalid flag %s.\n", args[0], args[i]);
            is_err = true;
            break;
        }
        else if (ret == AT_NO_MORE_POSIT_ARGS)
        {
            fprintf(stderr, "%s: Expected maximum %ld positional arguments, found %ld.\n", args[0], posit_offset, posit_offset+1);
            is_err = true;
            break;
        }
        else if (ret == AT_POSIT_ARG_NA_IN_MULTIFLAG_ARG)
        {
            fprintf(stderr, "%s: Invalid multi-flag %s. Contains flag which requires an argument.\n", args[0], args[i]);
            is_err = true;
            break;
        }
        else if (ret > 0)
        {
            next_is_additional = true;
            additional_idx = ret-1;
            continue;
        }
        if (next_is_additional)
        {
            argtab->values->data[additional_idx] = string_create_copyc(args[i]);
            next_is_additional = false;
            continue;
        }
        if (ret == AT_INC_POSIT_COUNT)
            posit_offset++;
    }

    if (is_err)
    {
        argtable_delete(argtab);
        return NULL;
    }

    return argtab;
}

bool_t argtable_is_flag_set(ArgTable argtab, char flag) {
    int pos = argtable_get_flag_pos(argtab, flag);
    if (pos < 0 || !argtab->values->data[pos])
        return false;
    return true;
}

String argtable_get_add_arg(ArgTable argtab, char flag) {
    int pos = argtable_get_flag_pos(argtab, flag);
    if (pos < 0)
        return NULL;
    return argtab->values->data[pos];
}

String argtable_get_pos_arg(ArgTable argtab, char* posarg) {
    String* args = (String*)argtab->args->data;
    for (int i = 0; i < argtab->args->len; i++)
        if (string_is_equalc(args[i], posarg))
            return argtab->values->data[i];
    return NULL;
}