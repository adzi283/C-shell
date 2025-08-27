#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "mystring.h"
#include "utils.h"
#include "wrappers.h"
#include "shelldata.h"

#define PATH_MAX 4096
#define NAME_MAX 32

String get_username() {
    String username = string_create(NULL, NAME_MAX);
    warn_failure(string_fetch(username, wrap_getname), "%s", "wrap_getname");
    return username;
}

String get_hostname() {
    String hostname = string_create(NULL, _SC_HOST_NAME_MAX+1);
    warn_failure(string_fetch(hostname, gethostname), "%s", "gethostname");
    return hostname;
}

String get_path() {
    String path = string_create(NULL, PATH_MAX+1);
    warn_failure(string_fetch(path, wrap_getcwd), "%s", "wrap_getcwd");
    return path;
}

void display_prompt(ShellData sd) {
    String username = get_username();
    String hostname = get_hostname();
    String path = get_path();
    
    if (string_is_prefix(path, sd->home_dir_path)) {
        warn_failure(string_set_offset(path, string_get_strlen(sd->home_dir_path)-1), "%s", "string_set_offset");
        warn_failure(string_modify(path, 0, '~'), "%s", "string_modify");
    }

    if (string_get_cstr(sd->prev_command) != NULL) {
        printf("<" GRN "%s@%s" CRESET ":" BLU "%s" CRESET " %s> ", string_get_cstr(username),\
                                                                   string_get_cstr(hostname),\
                                                                   string_get_cstr(path),\
                                                                   string_get_cstr(sd->prev_command));
        free(sd->prev_command->cstr);
        sd->prev_command->cstr = NULL;
    } else {
        printf("<" GRN "%s@%s" CRESET ":" BLU "%s" CRESET "> ", string_get_cstr(username),\
                                                                string_get_cstr(hostname),\
                                                                string_get_cstr(path));
    }

    string_delete(username);
    string_delete(hostname);
    string_delete(path);
}