#include "../include/shell.h"
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


job *first_job = NULL;

void sig_stop(int sig){
  job *j;
  for(j = first_job; j != NULL && j->status == FOREGROUND ; j++);
  put_job_in_background(j, 0);
  pause();
}

void init_shell(){

  /* See if we are running interactively.  */
  shell_terminal = STDIN_FILENO;
  shell_is_interactive = isatty (shell_terminal);

  if (shell_is_interactive)
    {
      /* Loop until we are in the foreground.  */
      while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
        kill (- shell_pgid, SIGTTIN);

      /* Ignore interactive and job-control signals.  */
      signal (SIGINT, SIG_IGN);
      signal (SIGQUIT, SIG_IGN);
      signal (SIGTSTP, SIG_IGN);
      signal (SIGTTIN, SIG_IGN);
      signal (SIGTTOU, SIG_IGN);
      signal (SIGCHLD, SIG_IGN);

      /* Put ourselves in our own process group.  */
      shell_pgid = getpid ();
      if (setpgid (shell_pgid, shell_pgid) < 0)
        {
          perror ("Couldn't put the shell in its own process group");
          exit (1);
        }

      /* Grab control of the terminal.  */
      tcsetpgrp (shell_terminal, shell_pgid);

      /* Save default terminal attributes for shell.  */
      tcgetattr (shell_terminal, &shell_tmodes);
    }
}

void launch_process(process *p, pid_t pgid,
                int infile, int outfile, int errfile,
                int foreground){
  pid_t pid;

  if (shell_is_interactive)
    {
      /* Put the process into the process group and give the process group
         the terminal, if appropriate.
         This has to be done both by the shell and in the individual
         child processes because of potential race conditions.  */
      pid = getpid ();
      if (pgid == 0) pgid = pid;
      setpgid (pid, pgid);
      if (foreground)
        tcsetpgrp (shell_terminal, pgid);

      /* Set the handling for job control signals back to the default.  */
      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      signal (SIGTTIN, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);
      signal (SIGCHLD, SIG_DFL);
    }

  /* Set the standard input/output channels of the new process.  */
  if (infile != STDIN_FILENO)
    {
      dup2 (infile, STDIN_FILENO);
      close (infile);
    }
  if (outfile != STDOUT_FILENO)
    {
      dup2 (outfile, STDOUT_FILENO);
      close (outfile);
    }
  if (errfile != STDERR_FILENO)
    {
      dup2 (errfile, STDERR_FILENO);
      close (errfile);
    }

  /* Exec the new process.  Make sure we exit.  */
  execvp (p->argv[0], p->argv);
  perror ("execvp");
  exit (1);
}

int mark_process_status (pid_t pid, int status){
  job *j;
  process *p;


  if (pid > 0)
    {
      /* Update the record for the process.  */
      for (j = first_job; j; j = j->next)
        for (p = j->first_process; p; p = p->next)
          if (p->pid == pid)
            {
              p->status = status;
              if (WIFSTOPPED (status))
                p->stopped = 1;
              else
                {
                  p->completed = 1;
                  if (WIFSIGNALED (status))
                    fprintf (stderr, "%d: Terminated by signal %d.\n",
                             (int) pid, WTERMSIG (p->status));
                }
              return 0;
             }
      fprintf (stderr, "No child process %d.\n", pid);
      return -1;
    }
}

void wait_for_job(job *j)
{
  int status;
  pid_t pid;

  do
    pid = waitpid (WAIT_ANY, &status, WUNTRACED);
  while (!mark_process_status (pid, status)
         && !job_is_stopped (j)
         && !job_is_completed (j));
}

void put_job_in_foreground (job *j, int cont) {
  /* Put the job into the foreground. */

  /* Send the job a continue signal, if necessary.  */
  if (cont)
    {
      tcsetattr (shell_terminal, TCSADRAIN, &j->tmodes);
      if (kill (- j->pgid, SIGCONT) < 0)
        perror ("kill (SIGCONT)");
    }


  /* Wait for it to report.  */
  wait_for_job (j);

  /* Put the shell back in the foreground.  */
  tcsetpgrp (shell_terminal, shell_pgid);

  /* Restore the shell’s terminal modes.  */
  tcgetattr (shell_terminal, &j->tmodes);
  tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
}


void put_job_in_background (job *j, int cont){
  /* Send the job a continue signal, if necessary.  */
  if (cont)
    if (kill (-j->pgid, SIGCONT) < 0)
      perror ("kill (SIGCONT)");
  //j->status = BACKGROUND;
}

process *get_fg_process(){
  process *p;
    for(p = first_job->first_process; p != NULL; p = p->next){
      if(p->status == FOREGROUND)
        return p;

    }
    return NULL;
}

void set_fg_process(process *p){
  get_fg_process()->status = 1;
  p->status = 0;
}

process *get_process_by_pid(pid_t pid){
  process *p;
  job *j;
  for(j = first_job; j != NULL ; j = j->next){
    for(p = first_job->first_process; p != NULL; p = p->next){
      if(pid == p->pid)
        return p;
    }
  }
  return NULL;
}

job *find_job (pid_t pgid){
  job *j;

  for (j = first_job; j; j = j->next)
    if (j->pgid == pgid)
      return j;
  return NULL;
}

int job_is_stopped (job *j){
  process *p;

  for (p = j->first_process; p; p = p->next)
    if (!p->completed && !p->stopped)
      return 0;
  return 1;
}

int job_is_completed (job *j){
  process *p;

  for (p = j->first_process; p; p = p->next)
    if (!p->completed)
      return 0;
  return 1;
}

const string_t op_str[] = {
  "&",
  "|",
  "&&"
};

int num_ops() {
  return sizeof(op_str) / sizeof(string_t);
}

void sh_loop() {
  string_t line;
  string_t* args;
  cmd_t* cmd;
  int status;


  init_shell();
  while(1) {
    printf("$ ");
    line = read_line();
    args = split_line(line);
    cmd = parse(args);
    status = sh_exec(cmd);

    free(line);
    free(args);
  }
}

string_t read_line() {
  string_t buf = (string_t) malloc(sizeof(char));
  int ch;
  int sz = 1;
  int pos = 0;

  while ((ch = getchar()) != EOF && ch != '\n') {
    if (pos + 1 >= sz) {
      sz = sz * 2;
      buf = (string_t) realloc(buf, sizeof(char) * sz);
    }

    buf[pos++] = ch;
  }

  buf[pos++] = '\0';

  return (string_t) realloc(buf, sizeof(char) * pos);
}

string_t* split_line(string_t line) {
  string_t* tokens = (string_t*) malloc(sizeof(string_t));
  string_t token;
  char delim[2] = " ";
  int sz = 1;
  int pos = 0;

  token = strtok(line, delim);

  while (token != NULL) {
    if (pos + 1 >= sz) {
      sz = sz * 2;
      tokens = (string_t*) realloc(tokens, sizeof(string_t) * sz);
    }

    tokens[pos++] = token;
    token = strtok(NULL, delim);
  }

  tokens[pos++] = NULL;

  return (string_t*) realloc(tokens, sizeof(string_t) * pos);
}

int sh_exec(cmd_t* cmd) {
  pid_t proc;
  int status;
  int fd[2];
  exec_cmd_t* ecmd = (exec_cmd_t*) cmd;
  fork_cmd_t* fcmd = (fork_cmd_t*) cmd;

  if (cmd == NULL) {
    return 0;
  }

  switch (cmd->type) {
    case SINGLE_CMD:
      proc = fork();
      //Sai da execução do shell
      if(!strcmp(ecmd->left[0], "exit")){
        exit(0);
      }
      if (proc < 0) {
        return -1;
      } else if (proc == 0) {
        if (execvp(ecmd->left[0], ecmd->left)) {
          if (!strcmp(ecmd->left[0], "cd")){
            char buf[1000], *gdir, *dir, *to;

            //Verifica se é caminho absoluto ou diretório imediatamente acima
            if(strcmp(ecmd->left[1], "/..") || strcmp(ecmd->left[1], "..") || ecmd->left[1][0] == '/'){
              chdir(ecmd->left[1]);
            }
            else{
              gdir = getcwd(buf, sizeof(buf));
              dir = strcat(gdir, "/");
              to = strcat(dir, ecmd->left[1]);
              chdir(to);
          }

        }
        else if(!strcmp(ecmd->left[0], "fg")){
          put_job_in_foreground(first_job, 1);
        }
          return -1;
        }
      } else {
        do {
          waitpid(proc, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      }
      break;

    case FORK_CMD:
      proc = fork();
      //Sai da execução do shell
      if(!strcmp(fcmd->left->left[0], "exit")){
        exit(0);
      }
      if (proc < 0) {
        return -1;
      } else if (proc == 0) {
        if (execvp(fcmd->left->left[0], fcmd->left->left)) {
          return -1;
        }
      } else {
        printf("[%d] %d", 1, proc);
        sh_exec(fcmd->right);
      }
      break;

    default:
      printf("Comando inválido");
      return -1;
  }
}
