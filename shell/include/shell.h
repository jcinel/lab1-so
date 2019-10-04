#ifndef SHELL_PUBLIC
#define SHELL_PUBLIC

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "./parser.h"
#include <fcntl.h>

int  check_children();
void  sh_loop();
int   run_cmd(cmd_t*);
int   run_exec_cmd(exec_cmd_t*);
int   run_fork_cmd(fork_cmd_t*);
int   run_redi_cmd(redi_cmd_t*);
int   run_redi_app_cmd(redi_cmd_app_t*);
int   add_to_jobs(pid_t);
int   check_builtins(string_t);
int   cd(string_t*);
int   exit_sh(string_t*);
int   jobs(string_t*);
int   fg(string_t*);
int   bg(string_t*);

#endif
