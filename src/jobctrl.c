#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>

#include "jobctrl.h"
#include "utils.h"
#include "shelldata.h"
#include "vector.h"
#include "mystring.h"

#define PATH_MAX 4096

Process process_create(Vector argv) {
    Process p = malloc(sizeof(st_Process));
    p->argv = argv;
    p->is_done = false;
    p->is_stopped = false;
    p->pid = -1;
    p->status = -1;
    p->in = NULL;
    p->out = NULL;
    p->err = NULL;
    return p;
}

Job job_create(String command, Vector procs) {
    Job j = malloc(sizeof(st_Job));
    j->tmodes = NULL;
    j->command = command;
    j->procs = procs;
    j->pgid = -1;
    j->have_notified = false;
    j->is_bg = false;
    return j;
}

JobList joblist_create() {
    JobList jl = malloc(sizeof(st_JobList));
    jl->sentinel = malloc(sizeof(st_JobListNode));
    jl->sentinel->job = NULL;
    jl->sentinel->prev = NULL;
    jl->sentinel->next = NULL;
    jl->size = 0;
    return jl;
}

void process_delete(Process p) {
    char** args = (char**)p->argv->data;
    for (int i = 0; i < p->argv->len; i++)
        free(args[i]);
    if (p->in)
        free(p->in);
    if (p->out)
        free(p->out);
    if (p->err)
        free(p->err);
    vector_delete(p->argv);
    free(p);
}

void job_delete(Job j) {
    Process* procs = (Process*)j->procs->data;
    for (int i = 0; i < j->procs->len; i++)
        process_delete(procs[i]);
    string_delete(j->command);
    vector_delete(j->procs);
    free(j->tmodes);
    free(j);
}

void joblistnode_delete(JobListNode node, bool_t deljob) {
    if (deljob)
        job_delete(node->job);
    free(node);
}

void joblist_delete(JobList jl, bool_t deljobs) {
    JobListNode node = jl->sentinel->next;
    if (node)
        while (node != jl->sentinel)
        {
            JobListNode temp = node;
            node = node->next;
            joblistnode_delete(temp, deljobs);
        }
    free(jl->sentinel);
    free(jl);
}

void joblist_add_job(JobList jl, Job j) {
    JobListNode node = malloc(sizeof(st_JobListNode));
    node->job = j;
    if (jl->size == 0) {
        node->next = jl->sentinel;
        node->prev = jl->sentinel;
        jl->sentinel->next = node;
        jl->sentinel->prev = node;
    } else {
        jl->sentinel->prev->next = node;
        node->prev = jl->sentinel->prev;
        node->next = jl->sentinel;
        jl->sentinel->prev = node;
    }
    jl->size++;
}

void joblist_pop_node(JobList jl, JobListNode node) {
    if (jl->size == 0 || !node)
        return;
    
    if (jl->size == 1)
    {
        jl->sentinel->next = NULL;
        jl->sentinel->prev = NULL;
    }
    else
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    
    jl->size--;
}

void joblist_delete_node(JobList jl, JobListNode node) {
    joblist_pop_node(jl, node);
    joblistnode_delete(node, true);
}

Job joblist_find_job(JobList jl, pid_t pgid) {
    JobListNode node = jl->sentinel->next;
    if (!node)
        return NULL;
    for (; node != jl->sentinel; node = node->next)
        if (node->job->pgid == pgid)
            return node->job;
    return NULL;
}

bool_t joblist_check_cmd(JobList jl, char* cmd) {
    JobListNode node = jl->sentinel->next;
    if (!node)
        return false;
    for (; node != jl->sentinel; node = node->next)
        for (int i = 0; i < node->job->procs->len; i++)
            if (strcmp(cmd, ((Process)node->job->procs->data[i])->argv->data[0]) == 0)
                return true;
    return false;
}

bool_t job_is_stopped(Job j) {
    Process* proclist = (Process*)j->procs->data;
    for (int i = 0; i < j->procs->len; i++)
        if (!proclist[i]->is_done && !proclist[i]->is_stopped)
            return false;
    return true;
}

bool_t job_is_done(Job j) {
    Process* proclist = (Process*)j->procs->data;
    for (int i = 0; i < j->procs->len; i++)
        if (!proclist[i]->is_done)
            return false;
    return true;
}

void process_print(Process p) {
    print_err("Debug info for Process struct at: %p\n", p);

    print_err("pid: %d\n", p->pid);

    print_err("argv (Vector at %p):\n", p->argv);
    char** argv = (char**)p->argv->data;
    for (int i = 0; i < p->argv->len; i++)
        print_err("%s (char* at %p)\n", argv[i], argv[i]);

    print_err("is_done: %s\n", (p->is_done ? "true" : "false"));
    print_err("is_stopped: %s\n", (p->is_stopped ? "true" : "false"));
    print_err("append: %s\n", (p->append ? "true" : "false"));
    print_err("status: %d\n", p->status);
    if (p->in)
        print_err("in: %s\n", p->in);
    else
        print_err("in: NULL\n");
    if (p->out)
        print_err("out: %s\n", p->out);
    else
        print_err("out: NULL\n");
    if (p->err)
        print_err("err: %s\n", p->err);
    else
        print_err("err: NULL\n");
}

void job_print(Job j) {
    print_err("Debug info for Job struct at: %p\n", j);
    
    print_err("pgid: %d\n", j->pgid);
    print_err("command: %s\n", string_get_cstr(j->command));

    print_err("procs (Vector at %p):\n", j->procs);
    Process* procs = (Process*)j->procs->data;
    for (int i = 0; i < j->procs->len; i++)
        process_print(procs[i]);
    
    print_err("have_notified: %s\n", (j->have_notified ? "true" : "false"));
    print_err("is_bg: %s\n", (j->is_bg ? "true" : "false"));
    print_err("tmodes: struct termios at %p\n", j->tmodes);
}

void joblist_print(JobList jl) {
    print_err("Debug info for JobList struct at: %p\n", jl);
    if (!jl)
        return;
    print_err("sentinel (JobListNode struct at %p):\n", jl->sentinel);
    print_err("prev:%p\n", jl->sentinel->prev);
    print_err("next:%p\n", jl->sentinel->next);
    JobListNode node = jl->sentinel->next;
    if (!node)
        return;
    print_err("nodes (JobListNode structs in linked list):\n");
    for (; node != jl->sentinel; node = node->next)
    {
        print_err("JobListNode struct at %p:\n", node);
        print_err("prev:%p\n", node->prev);
        print_err("next:%p\n", node->next);
        job_print(node->job);
    }
}

errcode_t job_update_status(Job j, pid_t pid, int status) {
    if (pid == 0 || errno == ECHILD)
        return -1;
    else if (pid < 0)
    {
        perror("waitpid");
        return -1;
    }

    Process* procs = (Process*)j->procs->data;

    for (int i = 0; i < j->procs->len; i++)
        if (procs[i]->pid == pid)
        {
            procs[i]->status = status;
            if (WIFSTOPPED(status))
            {
                procs[i]->is_stopped = true;
                if (j->procs->len == 1)
                {
                    print_err("(%d) %s: Stopped\n", pid, (char*)procs[i]->argv->data[0]);
                    j->have_notified = true;
                }
            }
            else
            {
                procs[i]->is_done = true;
                if (j->procs->len == 1 && WIFSIGNALED(status))
                {
                    print_err("(%d) %s: Terminated by signal %d\n", pid, (char*)procs[i]->argv->data[0], WTERMSIG(status));
                    j->have_notified = true;
                }
            }
            return 0;
        }
    print_err("job_update_status: No process with pid %d\n", pid);
    return -1;
}

void job_update(Job j) {
    int status;
    pid_t pid;
    
    do
        pid = waitpid(-j->pgid, &status, WUNTRACED|WNOHANG);
    while (!job_update_status(j, pid, status));
}

void job_wait(Job j) {
    int status;
    pid_t pid;
    
    do
        pid = waitpid(-j->pgid, &status, WUNTRACED);
    while (!job_update_status(j, pid, status) && !job_is_stopped(j) && !job_is_done(j));
}

void job_mv_to_bg(Job j, bool_t cont) {
    if (cont)
        warn_failure(kill(-j->pgid, SIGCONT), "%s", "kill");
}

void job_mv_to_fg(ShellData sd, Job j, bool_t cont) {
    struct timeval stop, start;
    gettimeofday(&start, NULL);

    set_terminal_pgrp(sd->shell_terminal, j->pgid);
    
    if (cont)
    {
        if (j->tmodes)
            set_terminal_attr(sd->shell_terminal, j->tmodes);
        warn_failure(kill(-j->pgid, SIGCONT), "%s", "kill");
    }

    j->have_notified = false;

    job_wait(j);

    gettimeofday(&stop, NULL);

    long timetaken = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec)/1000000;
    if (timetaken > 2)
    {
        sd->prev_command->cstr = calloc(4110, sizeof(char));
        snprintf(sd->prev_command->cstr, 4110,"%s : %lds", string_get_cstr(j->command), timetaken);
    }

    if (job_is_done(j))
        j->have_notified = true;

    set_terminal_pgrp(sd->shell_terminal, sd->shell_pgid);

    if (!j->tmodes)
        j->tmodes = malloc(sizeof(struct termios));
    get_terminal_attr(sd->shell_terminal, j->tmodes);
    set_terminal_attr(sd->shell_terminal, sd->shell_tmodes);
}

void joblist_update(JobList jl) {
    if (!jl || jl->size == 0)
        return;
    for (JobListNode node = jl->sentinel->next; node != jl->sentinel; )
    {
        JobListNode next = NULL;

        job_update(node->job);

        if (job_is_done(node->job))
        {
            if (!node->job->have_notified)
            {
                print_err("(%d) %s: Done\n", node->job->pgid, string_get_cstr(node->job->command));
                node->job->have_notified = true;
            }
            joblist_pop_node(jl, node);
            next = node->next;
            joblistnode_delete(node, true);
        }
        else if (job_is_stopped(node->job))
        {
            if (!node->job->have_notified)
            {
                print_err("(%d) %s: Stopped\n", node->job->pgid, string_get_cstr(node->job->command));
                node->job->have_notified = true;
            }
        }

        if (!next)
            next = node->next;
        node = next;
    }
}

void job_mark_running(Job j) {
    Process* procs = (Process*)j->procs->data;
    for (int i = 0; i < j->procs->len; i++)
        procs[i]->is_stopped = false;
    j->have_notified = false;
}

bool_t job_continue(ShellData sd, pid_t pgid, bool_t isfg) {
    Job j = joblist_find_job(sd->jobs, pgid);
    if (!j)
        return false;
    job_mark_running(j);
    if (isfg)
        job_mv_to_fg(sd, j, true);
    else
        job_mv_to_bg(j, true);
    return true;
}

void joblist_kill_all(JobList jl) {
        if (!jl || jl->size == 0)
        return;
    for (JobListNode node = jl->sentinel->next; node != jl->sentinel; )
    {
        JobListNode next = NULL;

        kill(-node->job->pgid, 9);

        if (!next)
            next = node->next;
        node = next;
    }
}