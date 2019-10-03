#include "../include/shell.h"

pid_t* job_arr = NULL;
int job_arr_sz = 0;

const string_t builtin_str[] = {
  "cd",
  "exit",
  "jobs",
  "fg",
  "bg",
  NULL
};

const void* builtin_func[] = {
  &cd,
  &exit_sh,
  &jobs,
  &fg,
  &bg
};

int check_children()
{
  int status;
  int ret;
  int i;

  do {
    ret = waitpid(-1, &status, WNOHANG);

    if (ret > 0) {
      for(i = 0; i < job_arr_sz; i++) {
        if (job_arr[i] == ret) {
          job_arr[i] = -1;
        }
      }

      printf("[%d]+ Concluído\t%d\n", 1, ret);
    }
  } while (ret > 0);

  return status;
}

void sh_loop()
{
  string_t line;
  string_t* args;
  string_t* tmp_arg;
  cmd_t* cmd;
  int status;

  for (;;) {
    printf("$ ");
    line = get_line();
    args = split_line(line);
    cmd = parse_args(args);

    status = check_children();
    status = run_cmd(cmd);

    free(line);
    free(args);
  }
}

int run_cmd(cmd_t* cmd)
{
  switch (cmd->type) {
    case EXEC:
      return run_exec_cmd((exec_cmd_t*) cmd);
    case FORK:
      return run_fork_cmd((fork_cmd_t*) cmd);
    case RINP:
    case ROUT:
      return run_redi_cmd((redi_cmd_t*) cmd);
    default:
      return EXIT_FAILURE;
  }
}

int run_exec_cmd(exec_cmd_t* cmd)
{
  pid_t pid;
  int status;
  int (*f)(string_t*);
  int i = check_builtins(cmd->argv[0]);

  if (i >= 0) {
    f = builtin_func[i];
    return f(cmd->argv);
  }

  pid = fork();

  if (pid < 0) {
    return EXIT_FAILURE;
  } else if (pid == 0) {

    if (execvp(cmd->argv[0], cmd->argv) != 0) {
      return EXIT_FAILURE;
    }
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return status;
  }
}

int run_fork_cmd(fork_cmd_t* cmd)
{
  pid_t pid;
  int status;
  int job_index;
  exec_cmd_t* ecmd = (exec_cmd_t*) cmd->left;

  pid = fork();

  if (pid < 0) {
    return EXIT_FAILURE;
  } else if (pid == 0) {
    if (execvp(ecmd->argv[0], ecmd->argv) != 0) {
      return EXIT_FAILURE;
    }
  } else {
    job_index = add_to_jobs(pid);

    printf("[%d] %d\n", job_index + 1, pid);

    return EXIT_SUCCESS;
  }
}

int run_redi_cmd(redi_cmd_t* cmd)
{
  
}

int add_to_jobs(pid_t pid)
{
  int i;
  for(i = 0; i < job_arr_sz; i++) {
    if (job_arr[i] < 0) {
      job_arr[i] = pid;
      return i;
    }
  }

  job_arr_sz++;
  job_arr = (pid_t*) realloc(job_arr, sizeof(pid_t) * job_arr_sz);
  job_arr[job_arr_sz - 1] = pid;

  return job_arr_sz - 1;
}

int check_builtins(string_t cmd)
{
  int i = 0;
  while (builtin_str[i] != NULL) {
    if (strcmp(cmd, builtin_str[i]) == 0) {
      return i;
    }

    i++;
  }

  return -1;
}

int cd(string_t* args)
{
  return chdir(args[1]);
}

int exit_sh(string_t* args)
{
  exit(atoi(args[1]));
}

int jobs(string_t* args)
{
  int i = 0;

  for (i = 0; i < job_arr_sz; i++) {
    if (job_arr[i] > 0) {
      printf("[%d] %d\n", i + 1, job_arr[i]);
    }
  }

  return 0;
}

int fg(string_t* args)
{
  pid_t pid;
  int status;
  int i = atoi(args[1]);

  if (i <= job_arr_sz) {
    pid = job_arr[i - 1];


    if (pid > 0) {
      do {
        waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));

      return status;
    }
  }

  printf("Job doesn't exist\n");
  return EXIT_FAILURE;
}

int bg(string_t* args)
{

}
