#define __USE_POSIX

#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "utils.h"
#include "shelldata.h"
#include "prompt.h"
#include "parser.h"
#include "jobctrl.h"
#include "jobhandler.h"

int main() {
    ShellData sd = init_shell();

    while (true)
    {
        display_prompt(sd);
        JobList newjobs = parse_input(sd, NULL);
        // debug_do(joblist_print(sd->jobs));
        joblist_update(sd->jobs);
        run_jobs(sd, newjobs);
        joblist_update(sd->jobs);
    }

    shelldata_delete(sd);
}