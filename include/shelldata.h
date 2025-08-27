#ifndef __SHELLDATA__
#define __SHELLDATA__

#include "mytypes.h"

typedef struct st_ShellData
{
    String home_dir_path, prev_command, prev_path;
    struct termios* shell_tmodes;
    fd_t shell_terminal;
    pid_t shell_pgid;
    JobList jobs;
} st_ShellData;

typedef st_ShellData* ShellData;

ShellData shelldata_create();

void shelldata_delete(ShellData sd);

ShellData init_shell();

#endif