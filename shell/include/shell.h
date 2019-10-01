#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/parser.h"

#include <termios.h>
#include <unistd.h>

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

#define FOREGROUND 0
#define BACKGROUND 1

const string_t op_str[];

int         num_ops();

void        sh_loop();
string_t    read_line();
string_t*   split_line(string_t);
int         sh_exec(cmd_t*);

typedef struct process
{
  struct process *next;       /* next process in pipeline */
  char **argv;                /* for exec */
  pid_t pid;                  /* process ID */
  int completed;             /* true if process has completed */
  int stopped;               /* true if process has stopped */
  int status;                 /* reported status value */
} process;

typedef struct job
{
  struct job *next;           /* next active job */
  char *command;              /* command line, used for messages */
  process *first_process;     /* list of processes in this job */
  pid_t pgid;                 /* process group ID */
  char notified;
  struct termios tmodes;               /* true if user told about stopped job */
  int stdin, stdout, stderr;
} job;
