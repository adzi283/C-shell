#ifndef __ARGPARSE__
#define __ARGPARSE__

#include "mytypes.h"

typedef struct st_ArgTable{
    Vector args;
    Vector values;
} st_ArgTable;

typedef st_ArgTable* ArgTable;

ArgTable parse_args(String options, Vector argv);
bool_t argtable_is_flag_set(ArgTable argtab, char flag);
String argtable_get_add_arg(ArgTable argtab, char flag);
String argtable_get_pos_arg(ArgTable argtab, char* posarg);

void argtable_delete(ArgTable argtab);

#endif