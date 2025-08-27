#ifndef __PARSER__
#define __PARSER__

#include "mytypes.h"

typedef struct st_parser_ret {
    errcode_t err;
    void* data;
} st_parser_ret;

bool_t is_whitespace(char c);

String get_input();
void parse_whitespace(String input);
st_parser_ret parse_process(String input);
st_parser_ret parse_job(String input);
JobList parse_input(ShellData sd, String input);

#endif