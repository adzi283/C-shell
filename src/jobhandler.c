#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "shelldata.h"
#include "utils.h"
#include "mystring.h"
#include "jobctrl.h"
#include "vector.h"
#include "shellcmds.h"

void set_io(fd_t infd, fd_t outfd, fd_t errfd) {
    if (infd != STDIN_FILENO)
    {
        dup2(infd, STDIN_FILENO);
        close(infd);
    }
    if (outfd != STDOUT_FILENO)
    {
        dup2(outfd, STDOUT_FILENO);
        close(outfd);
    }
    if (errfd != STDERR_FILENO)
    {
        dup2(errfd, STDERR_FILENO);
        close(errfd);
    }
}

void run_process(ShellData sd, Process p, pid_t pgid, fd_t infd, fd_t outfd, fd_t errfd, bool_t is_bg) {
    pid_t pid = getpid();
    if (pgid == -1)
        pgid = pid;
    
    setpgid(pid, pgid);
    if (!is_bg)
        set_terminal_pgrp(sd->shell_terminal, pgid);
    
    enable_jobctrl_signals();

    set_io(infd, outfd, errfd);

    shellcmd_func runshellcmd;
    if ((runshellcmd = is_shellcmd(p)))
    {
        runshellcmd(sd, p);
        exit(EXIT_SUCCESS);
    }

    execvp((char*)p->argv->data[0], (char**)p->argv->data);
    if (errno == ENOENT)
        print_err("%s: command not found\n", (char*)p->argv->data[0]);
    exit(EXIT_FAILURE);
}

void run_forced_shellcmd(ShellData sd, Process p, fd_t infd, fd_t outfd, fd_t errfd, shellcmd_func forced_shellcmd) {
    fd_t shell_infd = dup(STDIN_FILENO);
    fd_t shell_outfd = dup(STDOUT_FILENO);
    fd_t shell_errfd = dup(STDERR_FILENO);

    set_io(infd, outfd, errfd);

    forced_shellcmd(sd, p);

    set_io(shell_infd, shell_outfd, shell_errfd);
    
    p->is_done = true;
}

void run_job(ShellData sd, Job j) {
    fd_t pipefds[2];
    fd_t infd = STDIN_FILENO;
    fd_t outfd = STDOUT_FILENO;
    fd_t errfd = STDERR_FILENO;

    Process* procs = (Process*)j->procs->data;
    for (int procnum = 0; procnum < j->procs->len; procnum++) {
        if (procnum + 1 != j->procs->len)
        {
            warn_failure(pipe(pipefds), "%s", "pipe");
            outfd = pipefds[1];
        }
        else
            outfd = STDOUT_FILENO;
        
        bool_t skip_process = false;
        if (procs[procnum]->in)
        {
            fd_t newinfd = open(procs[procnum]->in, O_RDONLY);
            if (newinfd < 0)
            {
                warn_failure(newinfd, "-yash: %s:", procs[procnum]->in);
                skip_process = true;
            }
            else
            {
                if (infd != STDIN_FILENO)
                    close(infd);
                infd = newinfd;
            }
        }
        if (procs[procnum]->out)
        {
            int flags = O_WRONLY | O_CREAT;
            if (procs[procnum]->append)
                flags |= O_APPEND;
            fd_t newoutfd = open(procs[procnum]->out, flags, 0644);
            if (newoutfd < 0)
            {
                warn_failure(newoutfd, "-yash: %s:", procs[procnum]->out);
                skip_process = true;
            }
            else
            {
                if (outfd != STDOUT_FILENO)
                    close(outfd);
                outfd = newoutfd;
            }
        }
        if (procs[procnum]->err)
        {
            fd_t newerrfd = open(procs[procnum]->err, O_WRONLY | O_CREAT, 0644);
            if (newerrfd < 0)
            {
                warn_failure(newerrfd, "-yash: %s:", procs[procnum]->err);
                skip_process = true;
            }
            else
                errfd = newerrfd;
        }

        if (!j->is_bg)
        {
            shellcmd_func forced_shellcmd = is_forced_shellcmd(procs[procnum]);
            if (forced_shellcmd)
            {
                run_forced_shellcmd(sd, procs[procnum], infd, outfd, errfd, forced_shellcmd);
                skip_process = true;
            }
        }

        if (!skip_process)
        {
            pid_t pid = fork();
            if (pid == 0)
                run_process(sd, procs[procnum], j->pgid, infd, outfd, errfd, j->is_bg);
            else if (pid < 0)
                warn_failure(-1, "%s", "pid");
            
            procs[procnum]->pid = pid;
            if (j->pgid == -1)
                j->pgid = pid;
            setpgid(pid, j->pgid);
        }

        if (infd != STDIN_FILENO)
            close(infd);
        if (outfd != STDOUT_FILENO)
            close(outfd);
        if (errfd != STDERR_FILENO)
        {
            close(errfd);
            errfd = STDERR_FILENO;
        }
        infd = pipefds[0];
    }

    if (j->pgid != -1)
    {
        if (j->is_bg)
        {
            job_mv_to_bg(j, false);
            print_err("%d\n", j->pgid);
        }
        else
            job_mv_to_fg(sd, j, false);
    }
    else
        j->have_notified = true;
}

void run_jobs(ShellData sd, JobList newjobs) {
    if (!newjobs)
        return;

    for (JobListNode node = newjobs->sentinel->next; node != newjobs->sentinel; node = node->next)
    {
        run_job(sd, node->job);
        joblist_add_job(sd->jobs, node->job);
    }

    joblist_delete(newjobs, false);
}