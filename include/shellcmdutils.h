#ifndef __SHELLCMDUTILS__
#define __SHELLCMDUTILS__

#include "mytypes.h"

typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

struct FTW;

char* get_username_uid(uid_t uid);
char* get_grpname_gid(gid_t gid);
String parse_path(ShellData sd, char* path);
void print_file_data(char* name, struct stat* info, bool_t print_hidden, bool_t print_extra, int pad_nlink, int pad_size, int pad_uname, int pad_gname, int pad_time);
str2int_errno str2int(int *out, char *s, int base);
void log_purge(ShellData sd);
Vector log_read(ShellData sd);
void log_update(ShellData sd, String cmd, JobList jl);
int find_files(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf);
int find_dirs(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf);
int find(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf);
void find_init(String target, String searchdir);
size_t find_get_num_results();
String find_get_last_found();
void find_clean();

#endif