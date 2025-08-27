#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#include "mytypes.h"
#include "utils.h"

void check_failure(errcode_t retcode, const char* errmsg) {
    if (retcode < 0){
        perror(errmsg);
        exit(EXIT_FAILURE);
    }
}

void set_jobctrl_signals(__sighandler_t sighandler) {
    struct sigaction sact = {0};
    sact.sa_handler = sighandler;
    
    warn_failure(sigaction(SIGINT, &sact, NULL), "%s", "sigaction");
    warn_failure(sigaction(SIGQUIT, &sact, NULL), "%s", "sigaction");
    warn_failure(sigaction(SIGTSTP, &sact, NULL), "%s", "sigaction");
    warn_failure(sigaction(SIGTTIN, &sact, NULL), "%s", "sigaction");
    warn_failure(sigaction(SIGTTOU, &sact, NULL), "%s", "sigaction");
}

void disable_jobctrl_signals() {
    set_jobctrl_signals(SIG_IGN);
}

void enable_jobctrl_signals() {
    set_jobctrl_signals(SIG_DFL);
}

pid_t get_terminal_pgrp(fd_t terminal) {
    return tcgetpgrp(terminal);
}

void set_terminal_pgrp(fd_t terminal, pid_t pgid) {
    warn_failure(tcsetpgrp(terminal, pgid), "%s", "tcsetpgrp");
}

void get_terminal_attr(fd_t terminal, struct termios *tmodes) {
    warn_failure(tcgetattr(terminal, tmodes), "%s", "tcgetattr");
}

void set_terminal_attr(fd_t terminal, struct termios *tmodes) {
    warn_failure(tcsetattr(terminal, TCSADRAIN, tmodes), "%s", "tcsetattr");
}