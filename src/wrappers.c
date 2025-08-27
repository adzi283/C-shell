#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mytypes.h"

errcode_t wrap_getname(char* buf, size_t buflen) {
    struct passwd *pw = getpwuid(getuid());
    
    if (pw) {
        size_t namelen = strlen(pw->pw_name);
        if (namelen > buflen)
            return -1;
        strncpy(buf, pw->pw_name, namelen+1);
    } else {
        if (buflen == 0)
            return -1;
        buf[0] = 0;
    }

    return 0;
}

errcode_t wrap_getcwd(char* buf, size_t buflen) {
    char* ret = getcwd(buf, buflen);
    if (!ret)
        return -1;
    return 0;
}

errcode_t wrap_fgets(char* buf, size_t buflen) {
    char* ret = fgets(buf, buflen, stdin);
    if (!ret){
        if (feof(stdin))
            return -2;
        else
            return -1;
    }

    return 0;
}