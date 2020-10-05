#include "../include/shell.h"

pid_t* job_arr = NULL;
int job_arr_sz = 0;

// Array com os comandos Built in que devem ser executados de maneira diferente
// com prioridade
const string_t builtin_str[] = {
  "cd",
  "exit",
  "jobs",
  "fg",
  "bg",
  NULL
};

// Array com ponteiros para as funções, cada função está em uma posição 
// correspondente ao seu comando no array de Builtins
const void* builtin_func[] = {
  &cd,
  &exit_sh,
  &jobs,
  &fg,
  &bg
};

// Função para coletar os processos filhos que terminarem sua execução sem 
// deixá-los como zumbi
int check_children()
{
  int status;
  int ret;
  int i;

  do 
  {
    ret = waitpid(-1, &status, WNOHANG);

    if (ret > 0) 
    {
      for(i = 0; i < job_arr_sz; i++) 
      {
        if (job_arr[i] == ret) 
        {
          job_arr[i] = -1;
        }
      }
      printf("[%d]+ Concluído\t%d\n", 1, ret);
    }
  } while (ret > 0);

  return status;
}

// Loop principal de funcionamento que executa o ciclo de vida do shell
void sh_loop()
{
  string_t line;
  string_t* args;
  string_t* tmp_arg;
  cmd_t* cmd;
  int status;

  for (;;) 
  {
    printf("$ ");
    line = get_line();
    args = split_line(line);
    cmd = parse_args(args);

    status = check_children();
    if(strlen(line) > 0)
      status = run_cmd(cmd);

    //Libera a memória dos ponteiros
    free(line);
    free(args);
  }
}

// Função para rodar o comando a depender do seu tipo
int run_cmd(cmd_t* cmd)
{
  switch (cmd->type)
  {
    case EXEC:
      return run_exec_cmd((exec_cmd_t*) cmd);
    case FORK:
      return run_fork_cmd((fork_cmd_t*) cmd);
    case RINP:
      return run_redi_cmd((redi_cmd_t*) cmd);
    case ROUT:
      return run_redi_cmd((redi_cmd_t*) cmd);
    case ROUTAPP:
      return run_redi_app_cmd((redi_cmd_app_t*) cmd);
    default:
      return EXIT_FAILURE;
  }
}

// Roda um comando do tipo normal
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

  if (pid < 0) 
  {
    return EXIT_FAILURE;
  } else if (pid == 0) 
  {
    if (execvp(cmd->argv[0], cmd->argv) != 0) 
    {
      _exit(-1);
      return EXIT_FAILURE;
    }
  } else 
  {
    do 
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return status;
  }
}

// Roda comandos do tipo prog &
int run_fork_cmd(fork_cmd_t* cmd)
{
  pid_t pid;
  int status;
  int job_index;
  exec_cmd_t* ecmd = (exec_cmd_t*) cmd->left;

  pid = fork();

  if (pid < 0) 
  {
    return EXIT_FAILURE;
  } else if (pid == 0) 
  {
    if (execvp(ecmd->argv[0], ecmd->argv) != 0) 
    {
      _exit(-1);
      return EXIT_FAILURE;
    }
  } else 
  {
    job_index = add_to_jobs(pid);
    printf("[%d] %d\n", job_index + 1, pid);

    return EXIT_SUCCESS;
  }
}

// Roda comandos do tipo prog1 > arquivo e prog1 < arquivo
int run_redi_cmd(redi_cmd_t* cmd)
{
  int status;
  pid_t pid = fork();

  if (pid < 0) 
  {
    return EXIT_FAILURE;

  } else if (pid == 0) 
  {
    string_t path = cmd->file;

    if(cmd->type == ROUT) 
    {
      if(path != NULL && strlen(path) > 1) 
      {
        FILE * fd = fopen(path ,"w");
        int fd_n = fileno(fd);
        dup2(fd_n, 1);
        exec_cmd_t* ecmd = (exec_cmd_t*) cmd->left;
        if (execvp(ecmd->argv[0], ecmd->argv) != 0) 
        {
          _exit(-1);
          return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;

      }
      printf("Arquivo especificado inválido");
      return EXIT_FAILURE;

    } else if(cmd->type == RINP) 
    {
      if(path != NULL && strlen(path) > 1)
      {
        int fd_in = open(path, O_RDONLY);
        close(0);
        dup(fd_in);
        close(fd_in);
        exec_cmd_t* ecmd = (exec_cmd_t*) cmd->left;
        if (execvp(ecmd->argv[0], ecmd->argv) != 0)
        {
          _exit(-1);
          return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
  }

  } else
  {
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return status;
  }

}

// Roda comandos do tipo prog1 >> arquivo
int run_redi_app_cmd(redi_cmd_app_t* cmd)
{
  int status;
  pid_t pid = fork();

  if (pid < 0) 
  {
    return EXIT_FAILURE;

  } else if (pid == 0) 
  {
    string_t path = cmd->file;

    if(cmd->type == ROUTAPP)
    {
      if(path != NULL && strlen(path) > 1)
      {
        FILE * fd = fopen(path ,"a");
        int fd_n = fileno(fd);
        dup2(fd_n, 1);
        exec_cmd_t* ecmd = (exec_cmd_t*) cmd->left;
        if (execvp(ecmd->argv[0], ecmd->argv) != 0)
        {
          _exit(-1);
          return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;

      }
      printf("Arquivo especificado inválido");
      return EXIT_FAILURE;
    }

  } else
  {
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return status;
  }
  
}

// Adiciona ao array de jobs do shell
int add_to_jobs(pid_t pid)
{
  int i;
  for(i = 0; i < job_arr_sz; i++)
  {
    if (job_arr[i] < 0)
    {
      job_arr[i] = pid;
      return i;
    }
  }

  job_arr_sz++;
  job_arr = (pid_t*) realloc(job_arr, sizeof(pid_t) * job_arr_sz);
  job_arr[job_arr_sz - 1] = pid;

  return job_arr_sz - 1;
}

// Verifica se o comando executado é um dos comandos built in
int check_builtins(string_t cmd)
{
  int i = 0;
  // Percorre o vetor de built ins verificando se há match com algum
  while (builtin_str[i] != NULL)
  {
    if (strcmp(cmd, builtin_str[i]) == 0)
      return i;
    i++;
  }

  return -1;
}

// Built in cd
int cd(string_t* args)
{
  return chdir(args[1]);
}

// Built in exit
int exit_sh(string_t* args)
{
  int exit_code = 0;
  if (args[1] != NULL) 
  {
    exit_code = atoi(args[1]);
  }
  exit(exit_code);
}

// Built in jobs para mostrar os jobs desse shell
int jobs(string_t* args)
{
  int i = 0;

  for (i = 0; i < job_arr_sz; i++)
  {
    if (job_arr[i] > 0)
      printf("[%d] %d\n", i + 1, job_arr[i]);
  }

  return 0;
}

// Built in fg que joga o job partir do seu job number para foreground
int fg(string_t* args)
{
  pid_t pid;
  int status;
  int i = atoi(args[1]);

  if (i <= job_arr_sz) 
  {
    pid = job_arr[i - 1];

    if (pid > 0) 
    {
      do 
      {
        waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));

      return status;
    }
  }

  printf("Job não existe\n");
  return EXIT_FAILURE;
}

// Built in bg que resume a execução de um job anteriormente em estado de STOP
int bg(string_t* args)
{
  pid_t pid;
  int i = atoi(args[1]);

  if (i <= job_arr_sz)
  {
    pid = job_arr[i-1];

    if (pid > 0)
    {
      int ret = kill(pid, SIGCONT);
      if (ret)
      {
        printf("Processo não encontrado");
        return EXIT_FAILURE;
      }
      else
      {
        printf("%d retomado\n", pid);
        return EXIT_SUCCESS;
      }
    }
    printf("PID não encontrado");
    return EXIT_FAILURE;
  }
  return EXIT_FAILURE;
}
