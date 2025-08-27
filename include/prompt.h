#ifndef __PROMPT__
#define __PROMPT__

#include "mytypes.h"

String get_username();
String get_hostname();
String get_path();
void display_prompt(ShellData sd);

#endif