#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ftw.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "shelldata.h"
#include "jobctrl.h"
#include "jobhandler.h"
#include "vector.h"
#include "vecutils.h"
#include "mystring.h"
#include "utils.h"
#include "prompt.h"
#include "argparse.h"
#include "shellcmdutils.h"
#include "parser.h"

#include "shellcmds.h"

// List of shell builtins
const st_shellcmd shellcmd_list[] = {{cmd_hop, "hop"},
                                     {cmd_reveal, "reveal"},
                                     {cmd_log, "log"},
                                     {cmd_proclore, "proclore"},
                                     {cmd_seek, "seek"},
                                     {cmd_activities, "activities"},
                                     {cmd_ping, "ping"},
                                     {cmd_fg, "fg"},
                                     {cmd_bg, "bg"},
                                     {cmd_neonate, "neonate"},
                                     {cmd_iMan, "iMan"}};
const size_t num_shellcmds = sizeof(shellcmd_list)/sizeof(st_shellcmd);
// List of shell builtins which should not be run in a subshell
// if they are not background processes.
const st_shellcmd forced_shellcmd_list[] = {{cmd_hop, "hop"},
                                            {cmd_log, "log"},
                                            {cmd_seek, "seek"},
                                            {cmd_ping, "ping"},
                                            {cmd_fg, "fg"},
                                            {cmd_bg, "bg"}};
const size_t num_forced_shellcmds = sizeof(forced_shellcmd_list)/sizeof(st_shellcmd);

shellcmd_func is_shellcmd(Process p) {
    for (size_t i = 0; i < num_shellcmds; i++)
    {
        if (strcmp(p->argv->data[0], shellcmd_list[i].cmd_name) == 0)
            return shellcmd_list[i].cmd_func;
    }
    return NULL;
}

shellcmd_func is_forced_shellcmd(Process p) {
    for (size_t i = 0; i < num_forced_shellcmds; i++)
    {
        if (strcmp(p->argv->data[0], forced_shellcmd_list[i].cmd_name) == 0)
            return forced_shellcmd_list[i].cmd_func;
    }
    return NULL;
}

void cmd_hop(ShellData sd, Process p) {
    char** paths = (char**)p->argv->data;
    if (p->argv->len == 2)
    {
        String curr_path = get_path();
        if (chdir(string_get_cstr(sd->home_dir_path)) >= 0)
        {
            printf("%s\n", string_get_cstr(sd->home_dir_path));
            string_set_equal(sd->prev_path, curr_path);
        }
        else
            warn_failure(-1, "%s", "chdir");
        string_delete(curr_path);
        return;
    }

    for (size_t i = 1; i < p->argv->len-1; i++)
    {
        String curr_path = get_path();
        String parsed_path = parse_path(sd, paths[i]);
        if (chdir(string_get_cstr(parsed_path)) >= 0)
        {
            String new_path = get_path();
            printf("%s\n", string_get_cstr(new_path));
            string_set_equal(sd->prev_path, curr_path);
            string_delete(new_path);
        }
        else
            warn_failure(-1, "%s", "chdir");
        string_delete(curr_path);
        string_delete(parsed_path);
    }
}

void cmd_reveal(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("-l,-a,path"), p->argv);
    if (!argtab)
        return;
    
    String path = argtable_get_pos_arg(argtab, "path");
    String parsedpath;
    if (!path)
        parsedpath = string_create_copyc(".");
    else
        parsedpath = parse_path(sd, string_get_cstr(path));
    bool_t is_lflag = argtable_is_flag_set(argtab, 'l');
    bool_t is_aflag = argtable_is_flag_set(argtab, 'a');

    struct dirent** namelist;
    errcode_t numdirs = scandir(string_get_cstr(parsedpath), &namelist, NULL, alphasort);
    if (numdirs == -1)
    {
        if (errno == ENOENT)
            fprintf(stderr, "reveal: No such file/directory.\n");
        else if (errno == ENOTDIR)
        {
            struct stat* fileinfo = malloc(sizeof(struct stat));
            int ret = stat(string_get_cstr(parsedpath), fileinfo);
            warn_failure(ret, "%s", "stat");
            if (ret == 0)
                print_file_data(string_get_cstr(parsedpath), fileinfo, true, is_lflag, -1, -1, -1, -1, -1);
            free(fileinfo);
        }
        argtable_delete(argtab);
        string_delete(parsedpath);
        return;
    }

    blkcnt_t total_blocks = 0;
    if (numdirs > 0)
    {
        fd_t parentdir = open(string_get_cstr(parsedpath), O_RDONLY);
        warn_failure(parentdir, "%s", "open");
        if (parentdir != -1)
        {
            nlink_t max_nlink = 0;
            int max_len_uname = 0;
            int max_len_gname = 0;
            off_t max_size = 0;
            int max_len_time = 8;

            struct stat fileinfo;
            for (int i = 0; i < numdirs; i++) {
                int ret = fstatat(parentdir, namelist[i]->d_name, &fileinfo, AT_SYMLINK_NOFOLLOW);
                warn_failure(ret, "%s", "stat");
                if (ret == 0)
                {
                    if (!is_aflag && namelist[i]->d_name[0] == '.')
                        continue;
                    total_blocks += fileinfo.st_blocks/2;
                    int len_uname = strlen(get_username_uid(fileinfo.st_uid));
                    max_len_uname = (max_len_uname > len_uname) ? max_len_uname : len_uname;
                    int len_gname = strlen(get_grpname_gid(fileinfo.st_gid));
                    max_len_gname = (max_len_gname > len_gname) ? max_len_gname : len_gname;
                    max_nlink = (max_nlink > fileinfo.st_nlink) ? max_nlink : fileinfo.st_nlink;
                    max_size = (max_size > fileinfo.st_size) ? max_size : fileinfo.st_size;
                    if ((time(NULL) - 15778476) < fileinfo.st_mtim.tv_sec)
                        max_len_time = 12;
                }
            }

            int max_len_nlink = 0;
            while (max_nlink != 0)
            {
                max_len_nlink++;
                max_nlink /= 10;
            }
            
            int max_len_size = 0;
            while (max_size != 0)
            {
                max_len_size++;
                max_size /= 10;
            }

            if (is_lflag)
                printf("total %ld\n", total_blocks);

            for (int i = 0; i < numdirs; i++) {
                int ret = fstatat(parentdir, namelist[i]->d_name, &fileinfo, AT_SYMLINK_NOFOLLOW);
                warn_failure(ret, "%s", "stat");
                if (ret == 0)
                    print_file_data(namelist[i]->d_name, &fileinfo, is_aflag, is_lflag, max_len_nlink,
                                                                                        max_len_size,
                                                                                        max_len_uname,
                                                                                        max_len_gname,
                                                                                        max_len_time);
                free(namelist[i]);
            }

            close(parentdir);
        }
    }
    else if (is_lflag)
        printf("total 0\n");
    argtable_delete(argtab);
    string_delete(parsedpath);
    free(namelist);
}

void cmd_log(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("command,index"), p->argv);
    if (!argtab)
        return;
    
    bool_t purge = false, execute = false;
    int index = 0;
    String command = argtable_get_pos_arg(argtab, "command");
    if (command)
    {
        if (string_is_equalc(command, "purge"))
            purge = true;
        else if (string_is_equalc(command, "execute"))
            execute = true;
        else
        {
            fprintf(stderr, "log: Unknown argument %s.\n", string_get_cstr(command));
            argtable_delete(argtab);
            return;
        }
        String indexstr = argtable_get_pos_arg(argtab, "index");
        if (indexstr)
        {
            if (purge || !execute)
            {
                fprintf(stderr, "log: Unexpected argument %s.\n", string_get_cstr(indexstr));
                argtable_delete(argtab);
                return;
            }
            if (str2int(&index, string_get_cstr(indexstr), 10) != STR2INT_SUCCESS || index < 1 || index > 15)
            {
                fprintf(stderr, "log: index must be a positive integer between 1 and 15.\n");
                argtable_delete(argtab);
                return;
            }
        }
        else if (execute)
        {
            fprintf(stderr, "log: Expected argument \"index\" after execute.\n");
            argtable_delete(argtab);
            return;
        } 
    }

    if (purge)
    {
        log_purge(sd);
        argtable_delete(argtab);
        return;
    }
    Vector logcmds = log_read(sd);

    if (execute)
    {
        if (index > logcmds->len)
        {
            fprintf(stderr, "log: index (%d) greater than current log size (%ld).\n", index, logcmds->len);
            vector_free_str(logcmds);
            vector_delete(logcmds);
            argtable_delete(argtab);
        }

        JobList jl = parse_input(sd, logcmds->data[logcmds->len-index]);
        run_jobs(sd, jl);
        vector_free_str(logcmds);
        vector_delete(logcmds);
        argtable_delete(argtab);
        return;
    }

    for (int i = 0; i < logcmds->len; i++)
        printf("%s", string_get_cstr(logcmds->data[i]));
    vector_free_str(logcmds);
    vector_delete(logcmds);
    argtable_delete(argtab);
}

void cmd_proclore(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("pid"), p->argv);
    if (!argtab)
        return;
    
    pid_t pid = -1;
    String pidstr = argtable_get_pos_arg(argtab, "pid");
    if (pidstr)
    {
        if (str2int(&pid, string_get_cstr(pidstr), 10) != STR2INT_SUCCESS || pid < 0)
        {
            fprintf(stderr, "proclore: pid must be a positive integer.\n");
            argtable_delete(argtab);
            return;
        }
    }
    else
        pid = sd->shell_pgid;
    
    char procname[32], temp_str_garbage[32], state;
    snprintf(procname, sizeof(procname), "/proc/%u/stat", pid);

    int garbage_int, pgrp, tpgid;
    unsigned int garbage_uint;
    long unsigned int garbage_luint, vmmem;
    long int garbage_lint;
    long long unsigned int garbage_lluint;
    
    FILE* fd_stat = fopen(procname, "r");
    if (!fd_stat)
    {
        warn_failure(-1, "%s", "fopen");
        return;
    }

    fscanf(fd_stat, "%d %s %c %d %d %d %d %u", &pid, temp_str_garbage, &state, &garbage_int, &pgrp, &garbage_int, &garbage_uint, &tpgid);
    fscanf(fd_stat, "%lu %lu %lu %lu %lu %lu", &garbage_luint, &garbage_luint, &garbage_luint, &garbage_luint, &garbage_luint, &garbage_luint);
    fscanf(fd_stat, "%ld %ld %ld %ld %ld %ld", &garbage_lint, &garbage_lint, &garbage_lint, &garbage_lint, &garbage_lint, &garbage_lint);
    fscanf(fd_stat, "%llu %lu", &garbage_lluint, &vmmem);
    fclose(fd_stat);

    printf("pid : %d\n", pid);
    if (!pidstr)
        printf("process status : R+\n");
    else if (pgrp == tpgid)
        printf("process status : %c+\n", state);
    else
        printf("process status : %c\n", state);
    printf("process group : %d\n", pgrp);
    printf("virtual memory : %lu\n", vmmem);

    procname[strlen(procname)-1] = 0;
    procname[strlen(procname)-1] = 'e';
    procname[strlen(procname)-2] = 'x';
    procname[strlen(procname)-3] = 'e';

    String exec_path = string_create(realpath(procname, NULL), 0);
    if (!string_get_cstr(exec_path))
    {
        free(exec_path);
        exec_path = string_create_copyc("?");
    }
    if (string_is_prefix(exec_path, sd->home_dir_path)) {
        warn_failure(string_set_offset(exec_path, string_get_strlen(sd->home_dir_path)-1), "%s", "string_set_offset");
        warn_failure(string_modify(exec_path, 0, '~'), "%s", "string_modify");
    }
    printf("executable path : %s\n", string_get_cstr(exec_path));

    string_delete(exec_path);
    argtable_delete(argtab);
}

void cmd_seek(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("-d,-f,-e,target,searchdir"), p->argv);
    if (!argtab)
        return;
    
    String target = argtable_get_pos_arg(argtab, "target");
    if (!target)
    {
        fprintf(stderr, "seek: Expected argument \"target\".\n");
        argtable_delete(argtab);
        return;
    }
    String searchdir = argtable_get_pos_arg(argtab, "searchdir");
    String parsed_path;
    if (!searchdir)
        parsed_path = string_create_copyc(".");
    else
        parsed_path = parse_path(sd, string_get_cstr(searchdir));
    bool_t is_dflag = argtable_is_flag_set(argtab, 'd');
    bool_t is_eflag = argtable_is_flag_set(argtab, 'e');
    bool_t is_fflag = argtable_is_flag_set(argtab, 'f');
    if (is_dflag && is_fflag)
    {
        fprintf(stderr, "seek: Options \"-f\" and \"-d\" cannot be set at the same time.\n");
        string_delete(parsed_path);
        argtable_delete(argtab);
        return;
    }

    find_init(target, parsed_path);
    if (is_dflag)
        warn_failure(nftw(string_get_cstr(parsed_path), find_dirs, 2000, FTW_ACTIONRETVAL | FTW_PHYS), "%s", "nftw");
    else if (is_fflag)
        warn_failure(nftw(string_get_cstr(parsed_path), find_files, 2000, FTW_ACTIONRETVAL | FTW_PHYS), "%s", "nftw");
    else
        warn_failure(nftw(string_get_cstr(parsed_path), find, 2000, FTW_ACTIONRETVAL | FTW_PHYS), "%s", "nftw");
    
    if (is_eflag && (find_get_num_results() == 1))
    {
        String respath = find_get_last_found();
        struct stat* resultstats = malloc(sizeof(struct stat));
        warn_failure(lstat(string_get_cstr(respath), resultstats), "%s", "lstat");

        if (S_ISDIR(resultstats->st_mode))
        {
            String prevpath = get_path();
            int ret = chdir(string_get_cstr(respath));
            if (ret == -1)
            {
                if (errno == EACCES)
                    fprintf(stderr, "Missing permissions for task!\n");
            }
            else
                string_set_equal(sd->prev_path, prevpath);
            string_delete(prevpath);
        }
        else
        {
            FILE* temp_file_fd = fopen(string_get_cstr(respath), "r");
            if (temp_file_fd == NULL)
                if (errno == EACCES)
                    fprintf(stderr, "Missing permissions for task!\n");

            char* temp_buff = calloc(4096, 1);
            while (fgets(temp_buff, 4096, temp_file_fd) != NULL)
                printf("%s", temp_buff);
            free(temp_buff);
        }
        free(resultstats);
        string_delete(respath);
    }

    find_clean();
    argtable_delete(argtab);
    string_delete(parsed_path);
}

int strsort_cmp(const void * a, const void * b ) {
    const char * stra = (const char *) a;
    const char * strb = (const char *) b;

    return strcmp(stra,strb);
}

void cmd_activities(ShellData sd, Process p) {
    if (!sd->jobs || sd->jobs->size == 0)
        return;
    Vector job_update_strings = vector_create(0);
    for (JobListNode node = sd->jobs->sentinel->next; node != sd->jobs->sentinel; )
    {
        JobListNode next = NULL;

        if (job_is_done(node->job))
            continue;
        ssize_t bufsz = snprintf(NULL, 0, "%d : %s - Stopped\n", node->job->pgid, string_get_cstr(node->job->command));
        char* buf = malloc(bufsz + 1);
        if (job_is_stopped(node->job))
            snprintf(buf, bufsz + 1, "%d : %s - Stopped\n", node->job->pgid, string_get_cstr(node->job->command));
        else
            snprintf(buf, bufsz + 1, "%d : %s - Running\n", node->job->pgid, string_get_cstr(node->job->command));
        vector_append(job_update_strings, buf);

        if (!next)
            next = node->next;
        node = next;
    }

    if (job_update_strings->len == 0)
    {
        vector_free_cstr(job_update_strings);
        vector_delete(job_update_strings);
        return;
    }

    qsort(job_update_strings->data, job_update_strings->len, sizeof(char*), strsort_cmp);

    for (int i = 0; i < job_update_strings->len; i++)
        printf("%s", (char*)job_update_strings->data[i]);
    
    vector_free_cstr(job_update_strings);
    vector_delete(job_update_strings);
}

void cmd_ping(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("pid,signal_number"), p->argv);
    if (!argtab)
        return;
    
    pid_t pid = -1;
    String pidstr = argtable_get_pos_arg(argtab, "pid");
    if (pidstr)
    {
        if (str2int(&pid, string_get_cstr(pidstr), 10) != STR2INT_SUCCESS || pid < 0)
        {
            fprintf(stderr, "ping: pid must be a positive integer.\n");
            argtable_delete(argtab);
            return;
        }
    }
    else
    {
        fprintf(stderr, "ping: Expected argument \"pid\".\n");
        argtable_delete(argtab);
        return;
    }

    int sigid = -1;
    String sigstr = argtable_get_pos_arg(argtab, "signal_number");
    if (sigstr)
    {
        if (str2int(&sigid, string_get_cstr(sigstr), 10) != STR2INT_SUCCESS || sigid < 0)
        {
            fprintf(stderr, "ping: signal_number must be a positive integer.\n");
            argtable_delete(argtab);
            return;
        }
    }
    else
    {
        fprintf(stderr, "ping: Expected argument \"signal_number\".\n");
        argtable_delete(argtab);
        return;
    }

    if (sigid%32 == 0)
    {
        fprintf(stderr, "ping: 0 (%d%%32) is an invalid signal_number.\n", sigid);
        argtable_delete(argtab);
        return;
    }

    int ret;
    warn_failure((ret = kill(pid, sigid)), "%s", "ping");

    if (ret == 0)
        printf("Sent signal %d to process with pid %d\n", sigid, pid);
    argtable_delete(argtab);
}

void cmd_fg(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("pid"), p->argv);
    if (!argtab)
        return;
    
    pid_t pid = -1;
    String pidstr = argtable_get_pos_arg(argtab, "pid");
    if (pidstr)
    {
        if (str2int(&pid, string_get_cstr(pidstr), 10) != STR2INT_SUCCESS || pid < 0)
        {
            fprintf(stderr, "fg: pid must be a positive integer.\n");
            argtable_delete(argtab);
            return;
        }
    }
    else
    {
        fprintf(stderr, "fg: Expected argument \"pid\".");
        argtable_delete(argtab);
        return;
    }

    if (!job_continue(sd, pid, true))
        fprintf(stderr, "fg: No such process found.\n");
    argtable_delete(argtab);
}

void cmd_bg(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("pid"), p->argv);
    if (!argtab)
        return;
    
    pid_t pid = -1;
    String pidstr = argtable_get_pos_arg(argtab, "pid");
    if (pidstr)
    {
        if (str2int(&pid, string_get_cstr(pidstr), 10) != STR2INT_SUCCESS || pid < 0)
        {
            fprintf(stderr, "bg: pid must be a positive integer.\n");
            argtable_delete(argtab);
            return;
        }
    }
    else
    {
        fprintf(stderr, "bg: Expected argument \"pid\".");
        argtable_delete(argtab);
        return;
    }

    if (!job_continue(sd, pid, false))
        fprintf(stderr, "bg: No such process found.\n");
    argtable_delete(argtab);
}

void cmd_neonate(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("+n"), p->argv);
    if (!argtab)
        return;
    
    int timeinterval = 0;
    String timestr = argtable_get_add_arg(argtab, 'n');
    if (timestr)
    {
        if (str2int(&timeinterval, string_get_cstr(timestr), 10) != STR2INT_SUCCESS || timeinterval < 0)
        {
            fprintf(stderr, "neonate: time_arg must be a positive integer.\n");
            argtable_delete(argtab);
            return;
        }
    }
    else
    {
        fprintf(stderr, "neonate: Expected flag \"-n\"\n");
        argtable_delete(argtab);
        return;
    }

    struct termios tmodes;
    get_terminal_attr(STDIN_FILENO, &tmodes);
    
    tmodes.c_lflag &= ~(ECHO | ICANON);
    tmodes.c_cc[VMIN] = 0;
    tmodes.c_cc[VTIME] = 0;

    set_terminal_attr(STDIN_FILENO, &tmodes);

    char inp;
    
    fd_t stdinfd = fileno(stdin);
    FILE* loadavg = fopen("/proc/loadavg", "r");
    
    struct timeval time_last_print, time_curr;
    gettimeofday(&time_last_print, NULL);
    
    bool_t printfirst = true;
    while (read(stdinfd, &inp, 1) != -1 && inp != 'x')
    {
        gettimeofday(&time_curr, NULL);
        double diff = (time_curr.tv_sec - time_last_print.tv_sec);
        diff += (time_curr.tv_usec - time_last_print.tv_usec)/1000000.0;

        if ((diff >= timeinterval) || printfirst)
        {
            double pad_double;
            int pad_int;
            pid_t latest_pid;
            fscanf(loadavg, "%lf %lf %lf %d/%d %d", &pad_double, &pad_double, &pad_double, &pad_int, &pad_int, &latest_pid);
            printf("%d\n", latest_pid);
            gettimeofday(&time_last_print, NULL);
            rewind(loadavg);
        }
        if (printfirst)
            printfirst = false;
    }

    argtable_delete(argtab);
}

void cmd_iMan(ShellData sd, Process p) {
    ArgTable argtab = parse_args(string_create_copyc("cmd,idiot,fool"), p->argv);
    if (!argtab)
        return;

    String searchstr = argtable_get_pos_arg(argtab, "cmd");
    if (!searchstr)
    {
        fprintf(stderr, "iMan: Expected argument \"cmd_name\".\n");
        argtable_delete(argtab);
        return;
    }

    struct addrinfo dns_hints = {0};
    dns_hints.ai_family = AF_UNSPEC;
    dns_hints.ai_socktype = SOCK_STREAM;
    dns_hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* dns;

    if (getaddrinfo("man.he.net", "http", &dns_hints, &dns) < 0)
    {
        warn_failure(-1, "%s", "getaddrinfo");
        argtable_delete(argtab);
        return;
    }

    fd_t sock = -1;
    for (struct addrinfo* curr = dns; curr != NULL; curr = curr->ai_next)
    {
        warn_failure((sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol)), "%s", "socket");
        warn_failure(connect(sock, curr->ai_addr, curr->ai_addrlen), "%s", "connect");
        break;
    }

    freeaddrinfo(dns);
    
    if (sock == -1)
    {
        fprintf(stderr, "iMan: Unable to connect to man.he.net.\n");
        argtable_delete(argtab);
        return;
    }
    
    char* getreqstr = malloc(60+string_get_strlen(searchstr));
    sprintf(getreqstr, "GET /?topic=%s&section=all HTTP/1.1\r\nHost: man.he.net\r\n\r\n", string_get_cstr(searchstr));
    if (write(sock, getreqstr, strlen(getreqstr)) < 0)
    {
        warn_failure(-1, "%s", "write");
        argtable_delete(argtab);
        free(getreqstr);
        return;
    }
    free(getreqstr);

    char* buf = malloc(1024);
    int buflen = 0;
    bool_t invalid = false;
    size_t numnewl = 0;
    while ((buflen = read(sock, buf, 1023)) > 0)
    {
        buf[buflen] = 0;
        size_t start = 0;
        if (numnewl < 8)
        {
            for (; start < buflen; start++)
            {
                if (numnewl == 8)
                    break;
                if (buf[start] == '\n')
                    numnewl++;
            }
        }
        if (strstr(buf, "No matches") != NULL)
            invalid = true;
        printf("%s", &buf[start]);
    }
    if (invalid)
        fprintf(stderr, "iMan: No matches for command \"%s\"\n", string_get_cstr(searchstr));        
    fflush(stdout);
    free(buf);
    close(sock);
    argtable_delete(argtab);
}