#include "../include/shell.h"

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

  while (1) {
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
