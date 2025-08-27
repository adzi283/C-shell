#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "mystring.h"
#include "wrappers.h"
#include "utils.h"
#include "jobctrl.h"
#include "parser.h"
#include "vector.h"
#include "vecutils.h"
#include "shellcmdutils.h"
#include "shelldata.h"

#define INPUT_MAX 4096

void exit_shell(ShellData sd) {
    joblist_kill_all(sd->jobs);
    shelldata_delete(sd);
    exit(EXIT_SUCCESS);
}

String get_input(ShellData sd) {
    String input = string_create(NULL, INPUT_MAX+1);
    int retval = string_fetch(input, wrap_fgets);
    if (retval == -1)
        warn_failure(-1, "%s", "wrap_fgets");
    else if (retval == -2)
    {
        string_delete(input);
        exit_shell(sd);
    }
    return input;
}

bool_t is_whitespace(char c) {
    return (c == ' ' ||\
            c == '\n'||\
            c == '\t'||\
            c == '\r'||\
            c == '\v');
}

void parse_whitespace(String input) {
    size_t iter = input->offset;
    size_t stop = input->offset + string_get_strlen(input);
    while (iter < stop && is_whitespace(input->cstr[iter]))
        iter++;
    string_set_offset(input, iter);
}

st_parser_ret parse_process(String input) {
    Vector argv = vector_create(0);
    size_t iter = input->offset;
    size_t stop = input->offset + string_get_strlen(input);
    bool_t is_in = false;
    bool_t is_out = false;
    bool_t is_append = false;
    errcode_t err = 0;
    char *fin = NULL, *fout = NULL, *ferr = NULL;
    while (iter < stop)
    {
        parse_whitespace(input);
        iter = input->offset;
        if (iter == stop)
            break;
        
        size_t start = iter;
        while (iter < stop && input->cstr[iter] != '|' &&\
                               input->cstr[iter] != ';' &&\
                               input->cstr[iter] != '&' &&\
                               input->cstr[iter] != '>' &&\
                               input->cstr[iter] != '<' &&\
                               !is_whitespace(input->cstr[iter]))
            iter++;
        
        if (iter-start > 0)
        {
            char* tok = string_subcstr_abs(input, start, iter-start);
            if (is_in)
            {
                fin = tok;
                is_in = false;
            }
            else if (is_out)
            {
                fout = tok;
                is_out = false;
                is_append = false;
            }
            else
                vector_append(argv, tok);
        }
        else if (iter < stop && input->cstr[iter] == '<')
        {
            if (is_in || is_out || argv->len == 0)
            {
                err = 1;
                break;
            }
            iter++;
            is_in = true;
        }
        else if (iter < stop && input->cstr[iter] == '>')
        {
            if (is_in || is_out || argv->len == 0)
            {
                err = 1;
                break;
            }
            iter++;
            is_out = true;
            if (iter < stop && input->cstr[iter] == '>')
            {
                iter++;
                is_append = true;
            }
        }
        else
            break;
        string_set_offset(input, iter);
    }

    if (err != 0 || argv->len == 0)
    {
        vector_free_cstr(argv);
        vector_delete(argv);
        st_parser_ret ret;
        ret.data = NULL;
        ret.err = err;
        return ret;
    }

    st_parser_ret ret;
    ret.data = process_create(argv);
    ((Process)ret.data)->in = fin;
    ((Process)ret.data)->out = fout;
    ((Process)ret.data)->err = ferr;
    ((Process)ret.data)->append = is_append;
    ret.err = err;
    return ret;
}

st_parser_ret parse_job(String input) {
    Vector procs = vector_create(0);
    size_t iter = input->offset;
    size_t cmdstart = iter;
    size_t stop = input->offset + string_get_strlen(input);
    bool_t need_job = false;
    bool_t is_bg = false;
    errcode_t err = 0;
    while (iter < stop)
    {
        st_parser_ret ret = parse_process(input);
        if (!ret.data)
        {
            if (ret.err != 0)
            {
                err = ret.err;
                break;
            }
            else if (need_job)
            {
                err = 1;
                break;
            }
            else
            {
                iter = input->offset;
                if (iter < stop && (input->cstr[iter] == '|'))
                {
                    need_job = true;
                    iter++;
                    string_set_offset(input, iter);
                    continue;
                }
                else if (iter < stop && (input->cstr[iter] == '&'))
                {
                    is_bg = true;
                    iter++;
                    string_set_offset(input, iter);
                    break;
                }
                else
                    break;
            }
        }
        if (need_job)
            need_job = false;
        vector_append(((Process)ret.data)->argv, NULL);
        vector_append(procs, ret.data);
    }

    if (err != 0 || procs->len == 0)
    {
        vector_free_process(procs);
        vector_delete(procs);
        st_parser_ret ret;
        ret.data = NULL;
        ret.err = err;
        return ret;
    }

    st_parser_ret ret;
    ret.data = job_create(string_substr_abs(input, cmdstart, iter-cmdstart), procs);
    ((Job)ret.data)->is_bg = is_bg;
    ret.err = 0;
    return ret;
}

JobList parse_input(ShellData sd, String input) {
    bool_t freeinput = false;
    if (!input)
    {
        input = get_input(sd);
        freeinput = true;
    }
    else
        string_resize(input, 4097);
    string_modify(input, string_get_strlen(input)-1, 0);
    
    JobList jl = joblist_create();
    errcode_t err = 0;
    while (string_get_strlen(input) > 0)
    {
        st_parser_ret ret = parse_job(input);
        if (!ret.data)
        {
            if (ret.err != 0)
            {
                err = ret.err;
                break;
            }
            else if (input->cstr[input->offset] == ';')
            {
                input->offset++;
                continue;
            }
            else
                break;
        }
        joblist_add_job(jl, ret.data);
    }

    if (err != 0 || jl->size == 0) {
        joblist_delete(jl, true);
        if (err == 1)
            print_err("Syntax Error: found unexpected token at position %ld\n", input->offset);
        string_delete(input);
        return NULL;
    }

    string_set_offset(input, 0);
    log_update(sd, input, jl);

    if (freeinput)
        string_delete(input);

    return jl;
}