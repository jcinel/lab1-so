#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/parser.h"

const string_t op_str[];

int         num_ops();

void        sh_loop();
string_t    read_line();
string_t*   split_line(string_t);
int         sh_exec(cmd_t*);
