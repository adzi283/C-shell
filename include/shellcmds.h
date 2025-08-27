#ifndef __SHELLCMDS__
#define __SHELLCMDS__

#include "mytypes.h"

typedef void (*shellcmd_func)(ShellData sd, Process p);

typedef struct st_shellcmd {
    shellcmd_func cmd_func;
    char* cmd_name;
} st_shellcmd;


shellcmd_func is_shellcmd(Process p);
shellcmd_func is_forced_shellcmd(Process p);

void cmd_hop(ShellData sd, Process p);
void cmd_reveal(ShellData sd, Process p);
void cmd_log(ShellData sd, Process p);
void cmd_proclore(ShellData sd, Process p);
void cmd_seek(ShellData sd, Process p);
void cmd_activities(ShellData sd, Process p);
void cmd_ping(ShellData sd, Process p);
void cmd_fg(ShellData sd, Process p);
void cmd_bg(ShellData sd, Process p);
void cmd_neonate(ShellData sd, Process p);
void cmd_iMan(ShellData sd, Process p);

#endif