#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

#include "shelldata.h"
#include "utils.h"
#include "prompt.h"
#include "jobctrl.h"
#include "mystring.h"

ShellData shelldata_create() {
    ShellData sd = malloc(sizeof(st_ShellData));
    sd->home_dir_path = NULL;
    sd->prev_command = NULL;
    sd->prev_path = NULL;
    sd->shell_tmodes = malloc(sizeof(struct termios));
    sd->shell_terminal = -1;
    sd->shell_pgid = -1;
    sd->jobs = joblist_create();
    return sd;
}

void shelldata_delete(ShellData sd) {
    if (sd->home_dir_path)
        string_delete(sd->home_dir_path);
    if (sd->prev_command)
        string_delete(sd->prev_command);
    if (sd->prev_path)
        string_delete(sd->prev_path);
    free(sd->shell_tmodes);
    joblist_delete(sd->jobs, true);
    free(sd);
}

ShellData init_shell() {
    ShellData sd = shelldata_create();
    sd->home_dir_path = get_path();
    sd->prev_path = get_path();
    sd->prev_command = string_create(NULL, 0);
    sd->shell_terminal = STDIN_FILENO;

    while (get_terminal_pgrp(sd->shell_terminal) != (sd->shell_pgid = getpgrp()))
        kill(-sd->shell_pgid, SIGTTIN);

    disable_jobctrl_signals();

    sd->shell_pgid = getpid();
    warn_failure(setpgid(sd->shell_pgid, sd->shell_pgid), "%s", "setpgid");

    set_terminal_pgrp(sd->shell_terminal, sd->shell_pgid);
    get_terminal_attr(sd->shell_terminal, sd->shell_tmodes);

    return sd;
}