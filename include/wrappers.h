#ifndef __WRAPPERS__
#define __WRAPPERS__

#include "mytypes.h"

errcode_t wrap_getname(char* buf, size_t buflen);
errcode_t wrap_getcwd(char* buf, size_t buflen);
errcode_t wrap_fgets(char* buf, size_t buflen);

#endif