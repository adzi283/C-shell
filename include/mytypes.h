#ifndef __MYTYPES__
#define __MYTYPES__

#define false 0
#define true  1

#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"
#define CRESET "\e[0m"

typedef int errcode_t;
typedef int fd_t;
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long unsigned int size_t;
typedef char bool_t;

typedef struct st_ShellData st_ShellData;
typedef struct st_Vector st_Vector;
typedef struct st_String st_String;
typedef struct st_Process st_Process;
typedef struct st_Job st_Job;
typedef struct st_JobListNode st_JobListNode;
typedef struct st_JobList st_JobList;

typedef st_ShellData* ShellData;
typedef st_Vector* Vector;
typedef st_String* String;
typedef st_Process* Process;
typedef st_Job* Job;
typedef st_JobListNode* JobListNode;
typedef st_JobList* JobList;

struct termios;

#endif