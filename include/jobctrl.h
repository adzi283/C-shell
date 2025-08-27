#ifndef __COMMAND__
#define __COMMAND__

#include "mytypes.h"

typedef struct st_Process
{
    Vector argv;
    pid_t pid;
    bool_t is_done, is_stopped, append;
    int status;
    char *in, *out, *err;
} st_Process;

typedef st_Process* Process;

typedef struct st_Job
{
    String command;
    Vector procs;
    pid_t pgid;
    bool_t have_notified, is_bg;
    struct termios* tmodes;
} st_Job;

typedef st_Job* Job;

typedef struct st_JobListNode
{
    Job job;
    struct st_JobListNode *next, *prev;
} st_JobListNode;

typedef st_JobListNode* JobListNode;

typedef struct st_JobList
{
    size_t size;
    JobListNode sentinel;
} st_JobList;

typedef st_JobList* JobList;

Process process_create(Vector argv);
Job job_create(String command, Vector procs);
JobList joblist_create();

void process_delete(Process p);
void job_delete(Job j);
void joblist_delete(JobList jl, bool_t deljobs);
void joblistnode_delete(JobListNode node, bool_t deljob);

void joblist_add_job(JobList jl, Job j);
Job joblist_find_job(JobList jl, pid_t pgid);
void joblist_pop_node(JobList jl, JobListNode node);
void joblist_delete_node(JobList jl, JobListNode node);
bool_t joblist_check_cmd(JobList jl, char* cmd);

void job_mv_to_bg(Job j, bool_t cont);
void job_mv_to_fg(ShellData sd, Job j, bool_t cont);
bool_t job_continue(ShellData sd, pid_t pgid, bool_t isfg);

bool_t job_is_stopped(Job j);
bool_t job_is_done(Job j);

void job_update(Job j);
void job_wait(Job j);
void joblist_update(JobList jl);
void joblist_kill_all(JobList jl);

void process_print(Process p);
void job_print(Job j);
void joblist_print(JobList jl);

#endif