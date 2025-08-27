#ifndef __UTILS__
#define __UTILS__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "mytypes.h"

#ifndef DEBUG
#define DEBUG 0
#endif

void check_failure(errcode_t retcode, const char* errmsg);

void enable_jobctrl_signals();
void disable_jobctrl_signals();
pid_t get_terminal_pgrp(fd_t terminal);
void set_terminal_pgrp(fd_t terminal, pid_t pgid);
void get_terminal_attr(fd_t terminal, struct termios *tmodes);
void set_terminal_attr(fd_t terminal, struct termios *tmodes);

#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#define debug_do(func) \
        do { if (DEBUG) func;} while (0)

#define warn_failure(retcode,FMT,...)\
  do { if (retcode<0) fprintf(stderr, FMT ": %s\n", __VA_ARGS__, strerror(errno));} while (0) 

#define print_err(...) \
        fprintf(stderr, __VA_ARGS__)

#endif