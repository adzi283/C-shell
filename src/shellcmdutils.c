#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64 

#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <ftw.h>
#include <string.h>

#include "mystring.h"
#include "vector.h"
#include "vecutils.h"
#include "shelldata.h"
#include "jobctrl.h"
#include "shellcmdutils.h"

char* get_username_uid(uid_t uid) {
    struct passwd* pw = getpwuid(uid);
    if (pw)
        return pw->pw_name;
    return "?";
}

char* get_grpname_gid(gid_t gid) {
    struct group* gr = getgrgid(gid);
    if (gr)
        return gr->gr_name;
    return "?";
}

String parse_path(ShellData sd, char* path) {
    String pathstr = string_create(path, 0);
    String parsed_path = NULL;
    if (string_cmp_idx(pathstr, 0, '~'))
    {
        if (string_get_strlen(pathstr) == 1)
            parsed_path = string_create_copy(sd->home_dir_path);
        if (string_cmp_idx(pathstr, 1, '/'))
        {
            string_set_offset(pathstr, 1);
            parsed_path = string_add(sd->home_dir_path, pathstr);
        }
    }
    else if (string_cmp_idx(pathstr, 0, '-') && (string_get_strlen(pathstr) == 1))
        parsed_path = string_create_copy(sd->prev_path);
    
    if (!parsed_path)
        parsed_path = string_create_copy(pathstr);
    
    free(pathstr);
    return parsed_path;
}

void print_file_data(char* name, struct stat* info, bool_t print_hidden, bool_t print_extra, int pad_nlink, int pad_size, int pad_uname, int pad_gname, int pad_time) {
    if (!print_hidden && name[0] == '.')
        return;
    if (!print_extra)
    {
        printf("%s\n", name);
        return;
    }
    char* timestamp;
    bool_t free_timestamp = false;
    if ((time(NULL) - 15778476) < info->st_mtim.tv_sec)
    {
        timestamp = ctime(&info->st_mtim.tv_sec);
        if (!timestamp)
            timestamp = "????????????????????????\n";
        timestamp = &timestamp[4];
        timestamp[12] = 0;
    }
    else
    {
        struct tm* yeartime = localtime(&info->st_mtim.tv_sec);
        timestamp = calloc(13, 1);
        strftime(timestamp, 13, "%b %d  %Y", yeartime);
        free_timestamp = true;
    }

    String file_perms = string_create_copyc("----------");
    if (info->st_mode & S_IRUSR)
        string_modify(file_perms, 1, 'r');
    if (info->st_mode & S_IWUSR)
        string_modify(file_perms, 2, 'w');
    if (info->st_mode & S_IXUSR)
        string_modify(file_perms, 3, 'x');
    if (info->st_mode & S_IRGRP)
        string_modify(file_perms, 4, 'r');
    if (info->st_mode & S_IWGRP)
        string_modify(file_perms, 5, 'w');
    if (info->st_mode & S_IXGRP)
        string_modify(file_perms, 6, 'x');
    if (info->st_mode & S_IROTH)
        string_modify(file_perms, 7, 'r');
    if (info->st_mode & S_IWOTH)
        string_modify(file_perms, 8, 'w');
    if (info->st_mode & S_IXOTH)
        string_modify(file_perms, 9, 'x');

    if (info->st_mode & S_ISUID)
    {
        if (string_cmp_idx(file_perms, 3, 'x'))
            string_modify(file_perms, 3, 's');
        else
            string_modify(file_perms, 3, 'S');
    }

    if (info->st_mode & S_ISGID)
    {
        if (string_cmp_idx(file_perms, 6, 'x'))
            string_modify(file_perms, 6, 's');
        else
            string_modify(file_perms, 6, 'l');
    }

    if (info->st_mode & __S_ISVTX)
    {
        if (string_cmp_idx(file_perms, 9, 'x'))
            string_modify(file_perms, 9, 't');
        else
            string_modify(file_perms, 9, 'T');
    }

    if (S_ISDIR(info->st_mode))
        string_modify(file_perms, 0, 'd');
    else if (S_ISCHR(info->st_mode))
        string_modify(file_perms, 0, 'c');
    else if (S_ISBLK(info->st_mode))
        string_modify(file_perms, 0, 'b');
    else if (S_ISFIFO(info->st_mode))
        string_modify(file_perms, 0, 'p');
    else if (S_ISLNK(info->st_mode))
        string_modify(file_perms, 0, 'l');
    else if (!S_ISREG(info->st_mode))
        string_modify(file_perms, 0, '?');

    printf("%s %*ld %*s %*s %*ld %*s ", string_get_cstr(file_perms), pad_nlink, info->st_nlink, pad_uname, get_username_uid(info->st_uid), pad_gname, get_grpname_gid(info->st_gid), pad_size, info->st_size, pad_time, timestamp);
    if (S_ISDIR(info->st_mode))
        printf(BLU "%s" CRESET "\n", name);
    else if (info->st_mode & S_IXUSR)
        printf(GRN "%s" CRESET "\n", name);
    else
        printf("%s\n", name);
    if (free_timestamp)
        free(timestamp);
    string_delete(file_perms);
}

str2int_errno str2int(int *out, char *s, int base) {
    char *end;
    if (s[0] == '\0' || isspace(s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}

void log_purge(ShellData sd) {
    String logfile = string_create_copyc("/.yash_log");
    String logfile_path = string_add(sd->home_dir_path, logfile);
    fclose(fopen(string_get_cstr(logfile_path), "w"));
    string_delete(logfile);
    string_delete(logfile_path);
}

Vector log_read(ShellData sd) {
    String logfile = string_create_copyc("/.yash_log");
    String logfile_path = string_add(sd->home_dir_path, logfile);
    Vector logcmds = vector_create(0);
    FILE* logfilef = fopen(string_get_cstr(logfile_path), "a+");

    char* templinebuf = NULL;
    size_t sz = 0;
    ssize_t len = 0;
    while ((len = getline(&templinebuf, &sz, logfilef)) > 0)
    {
        vector_append(logcmds, string_create_copyc(templinebuf));
        // printf("[%s]\n", templinebuf);
    }
    free(templinebuf);
    fclose(logfilef);
    string_delete(logfile);
    string_delete(logfile_path);

    return logcmds;
}

void log_update(ShellData sd, String cmd, JobList jl) {
    if (joblist_check_cmd(jl, "log"))
        return;
    
    String cmdnewl = string_addc(cmd, "\n");

    Vector logcmds = log_read(sd);
    if (logcmds->len > 0 && string_is_equal(logcmds->data[logcmds->len-1], cmdnewl))
    {
        vector_free_str(logcmds);
        vector_delete(logcmds);
        string_delete(cmdnewl);
        return;   
    }
    vector_append(logcmds, cmdnewl);

    String logfile = string_create_copyc("/.yash_log");
    String logfile_path = string_add(sd->home_dir_path, logfile);
    FILE* logf = fopen(string_get_cstr(logfile_path), "w");

    int start = (logcmds->len == 16) ? 1 : 0;
    for (; start< logcmds->len; start++)
        fprintf(logf, "%s", string_get_cstr(logcmds->data[start]));
    
    fclose(logf);
    
    string_delete(logfile);
    string_delete(logfile_path);
    vector_free_str(logcmds);
    vector_delete(logcmds);
}

char* seek_searchname, *seek_last_found;
size_t seek_searchname_len, seek_num_results, seek_searchdir_len;

int find_files(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    if (ftwbuf->level == 0)
        return FTW_CONTINUE;

    if (typeflag == FTW_D || typeflag == FTW_DNR)
        return FTW_CONTINUE;
    
    if (strncmp(&fpath[ftwbuf->base], seek_searchname, seek_searchname_len) == 0)
    {
        if (S_ISDIR(sb->st_mode))
            printf(BLU "./%s" CRESET "\n", &fpath[seek_searchdir_len+1]);
        else if (sb->st_mode & S_IXUSR)
            printf(GRN "./%s" CRESET "\n", &fpath[seek_searchdir_len+1]);
        else
            printf("./%s\n", &fpath[seek_searchdir_len+1]);
        seek_num_results += 1;
        if (seek_last_found == NULL)
            seek_last_found = strdup(fpath);
    }
    return FTW_CONTINUE;
}

int find_dirs(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    if (ftwbuf->level == 0)
        return FTW_CONTINUE;

    if (typeflag != FTW_D || typeflag == FTW_DNR)
        return FTW_CONTINUE;

    if (strncmp(&fpath[ftwbuf->base], seek_searchname, seek_searchname_len) == 0)
    {
        if (S_ISDIR(sb->st_mode))
            printf(BLU "./%s" CRESET "\n", &fpath[seek_searchdir_len+1]);
        else if (sb->st_mode & S_IXUSR)
            printf(GRN "./%s" CRESET "\n", &fpath[seek_searchdir_len+1]);
        else
            printf("./%s\n", &fpath[seek_searchdir_len+1]);
        seek_num_results += 1;
        if (seek_last_found == NULL)
            seek_last_found = strdup(fpath);
    }
    return FTW_CONTINUE;
}

int find(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    if (ftwbuf->level == 0)
        return FTW_CONTINUE;

    if (strncmp(&fpath[ftwbuf->base], seek_searchname, seek_searchname_len) == 0)
    {
        if (S_ISDIR(sb->st_mode))
            printf(BLU "./%s" CRESET "\n", &fpath[seek_searchdir_len+1]);
        else if (sb->st_mode & S_IXUSR)
            printf(GRN "./%s" CRESET "\n", &fpath[seek_searchdir_len+1]);
        else
            printf("./%s\n", &fpath[seek_searchdir_len+1]);
        seek_num_results += 1;
        if (seek_last_found == NULL)
            seek_last_found = strdup(fpath);
    }
    return FTW_CONTINUE;
}

void find_init(String target, String searchdir) {
    seek_num_results = 0;
    seek_last_found = NULL;
    seek_searchname = target->cstr;
    seek_searchname_len = string_get_strlen(target);
    seek_searchdir_len = string_get_strlen(searchdir);
}

size_t find_get_num_results() {
    return seek_num_results;
}

String find_get_last_found() {
    return string_create_copyc(seek_last_found);
}

void find_clean() {
    seek_num_results = 0;
    free(seek_last_found);
    seek_searchname = NULL;
    seek_searchname_len = 0;
    seek_searchdir_len = 0;
}