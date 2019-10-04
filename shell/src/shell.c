#include "../include/shell.h"

pid_t* job_arr = NULL;
int job_arr_sz = 0;

const string_t builtin_str[] = { //Array com os comandos Built in que devem ser executados de maneira diferente com prioridade
  "cd",
  "exit",
  "jobs",
  "fg",
  "bg",
  NULL
};

const void* builtin_func[] = { //Array com ponteiros para as funções, cada função está em uma posição correspondente ao seu comando no array de Built ins
  &cd,
  &exit_sh,
  &jobs,
  &fg,
  &bg
};

int check_children() //Função para coletar os processos filhos que terminarem sua execução sem deixá-los como zumbi
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

void sh_loop() //Loop principal de funcionamento que executa o ciclo de vida do shell
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

    //Libera a memória dos ponteiros
    free(line);
    free(args);
  }
}

int run_cmd(cmd_t* cmd) //Função para rodar o comando a depender do seu tempo
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

int run_exec_cmd(exec_cmd_t* cmd) //Roda um comando do tipo normal
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

int run_fork_cmd(fork_cmd_t* cmd) //Roda comandos do tipo prog &
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

int run_redi_cmd(redi_cmd_t* cmd) //Roda comandos do tipo prog1 > arquivo
{
  
}

int add_to_jobs(pid_t pid) //Adiciona ao array de jobs do shell
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

int check_builtins(string_t cmd) //Verifica se o comando executado é um dos comandos built in
{
  int i = 0;
  while (builtin_str[i] != NULL) { //Percorre o vetor de built ins verificando se há match com algum
    if (strcmp(cmd, builtin_str[i]) == 0) {
      return i;
    }

    i++;
  }

  return -1;
}

int cd(string_t* args) //Built in cd
{
  return chdir(args[1]);
}

int exit_sh(string_t* args) //Built in exit
{
  int exit_code = 0;
  if (args[1] != NULL) {
    exit_code = atoi(args[1]);  
  }
  exit(exit_code);
}

int jobs(string_t* args) //Built in jobs para mostrar os jobs desse shell
{
  int i = 0;

  for (i = 0; i < job_arr_sz; i++) {
    if (job_arr[i] > 0) {
      printf("[%d] %d\n", i + 1, job_arr[i]);
    }
  }

  return 0;
}

int fg(string_t* args) //Built in fg que joga o job partir do seu job number para foreground
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

int bg(string_t* args) //Built in bg que resume a execução de um job anteriormente em estado de STOP
{
  pid_t pid;
  int i = atoi(args[1]);

  if (i <= job_arr_sz) {
    pid = job_arr[i-1];

    if (pid > 0) {
      int ret = kill(pid, SIGCONT);
      if (ret) {
        printf("Processo não encontrado");
        return EXIT_FAILURE;
      }
      else {
        printf("%d retomado\n", pid);
        return EXIT_SUCCESS;
      }
    }
    printf("PID não encontrado");
    return EXIT_FAILURE;
  }
  return EXIT_FAILURE;  
}
