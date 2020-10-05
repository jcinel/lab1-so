#include "../include/parser.h"

string_t _old_line = NULL;

string_t get_line()
{
  string_t buf = NULL;
  int ch;
  int sz = 0;
  int pos = 0;

  //Lê cada caractere da linha entrada
  while ((ch = getchar()) != EOF && ch != '\n') 
  {
    if (pos + 1 >= sz) 
    {
      sz = sz * 2 + 1;
      buf = (string_t) realloc(buf, sizeof(char) * sz);
    }

    buf[pos++] = ch;
  }

  sz = pos + 1;
  buf = (string_t) realloc(buf, sizeof(char) * sz);
  buf[pos] = '\0';

  return buf;
}
// Divide uma string de uma linha toda em várias strings cada qual com um 
// token (parcela do comando)
string_t* split_line(string_t line)
{
  string_t* tokens = NULL;
  string_t token;
  string_t delim = " \0";
  int sz = 0;
  int pos = 0;

  token = strtok(line, delim);

  while (token != NULL) 
  {
    if (pos + 1 >= sz) 
    {
      sz = sz * 2 + 1;
      tokens = (string_t*) realloc(tokens, sizeof(string_t) * sz);
    }

    tokens[pos++] = token;
    token = strtok(NULL, delim);
  }

  sz = pos + 1;
  tokens = (string_t*) realloc(tokens, sizeof(string_t) * sz);
  tokens[pos] = NULL;

  return tokens;
}

/* Essa função chama uma outra função de parse, que chama outra, e assim 
   sucessivamente e recebe um ponteiro para uma estrutura generica que teve 
   seu tipo de comando modificado por essas funções. Posteriormente (na função
   run_cmd) esse ponteiro genérico será convertido a um ponteiro de uma 
   estrutura especifica do tipo do comando
   Divide os argumentos dos comandos a depender do tipo de comando nos campos 
   da estrutura (A estrutura depende do tipo de comando) */
cmd_t* parse_args(string_t* args)
{
  cmd_t* cmd = NULL;
  cmd = parse_pipe(&args);

  return cmd;
}

// Função que modifica tipo da estrutura para o tipo PIPE de comando
// Caso seja um comando com pipe (|)
cmd_t* parse_pipe(string_t** args)
{
  cmd_t* cmd = NULL;
  pipe_cmd_t* pcmd = NULL;

  cmd = parse_redi(args);
  if (**args != NULL && strcmp(**args, PIPE_STR) == 0)
  {
    (*args)++;
    pcmd = (pipe_cmd_t*) malloc(sizeof(pipe_cmd_t));
    pcmd->type = PIPE;
    pcmd->left = cmd;
    pcmd->right = parse_pipe(args);

    cmd = (cmd_t*) pcmd;
  }

  return cmd;
}

// Função que modifica tipo da estrutura para os tipos ROUT ou RINP de comando
// (redirecionamentos de entrada e saída), caso seja um comando com 
// redirecionador < ou >
cmd_t* parse_redi(string_t** args)
{
  cmd_t* cmd = NULL;
  redi_cmd_t* rcmd = NULL;

  cmd = parse_fork(args);
  if (**args != NULL 
    && (strcmp(**args, ROUT_STR) == 0 || strcmp(**args, RINP_STR) == 0)) 
    {
    rcmd = (redi_cmd_t*) malloc(sizeof(redi_cmd_t));

    if (strcmp(**args, ROUT_STR) == 0)
    {
      rcmd->type = ROUT;
    } else {
      rcmd->type = RINP;
    }

    (*args)++;
    rcmd->left = cmd;
    rcmd->file = **args;

    cmd = (cmd_t*) rcmd;
  }

  return cmd;
}


// Caso seja um comando com >>
cmd_t* parse_redi_app(string_t** args)
{
  cmd_t* cmd = NULL;
  redi_cmd_app_t* rcmd = NULL;
  cmd = parse_fork(args);
  if (**args != NULL 
    && strcmp(**args, ROUT_STR_APP) == 0)
    {
    rcmd = (redi_cmd_app_t*) malloc(sizeof(redi_cmd_app_t));
    rcmd->type = ROUTAPP;

    (*args)++;
    rcmd->left = cmd;
    rcmd->file = **args;

    cmd = (cmd_t*) rcmd;
  }

  return cmd;
}


// Função que modifica tipo da estrutura para o tipo FORK de comando, caso 
// seja um comando com &
cmd_t* parse_fork(string_t** args)
{
  cmd_t* cmd = NULL;
  fork_cmd_t* fcmd = NULL;

  cmd = parse_exec(args);
  if (**args != NULL && strcmp(**args, FORK_STR) == 0)
  {
    fcmd = (fork_cmd_t*) malloc(sizeof(fork_cmd_t));
    fcmd->type = FORK;
    fcmd->left = cmd;

    cmd = (cmd_t*) fcmd;
  }

  return cmd;
}

// Função que modifica tipo da estrutura para o tipo EXEC de comando
// (comando comum)
cmd_t* parse_exec(string_t** args)
{
  cmd_t* cmd = NULL;
  exec_cmd_t* ecmd;
  string_t* argv = NULL;
  string_t arg;
  int sz = 0;
  int pos = 0;

  if (**args != NULL
    && strcmp(**args, FORK_STR) != 0
    && strcmp(**args, PIPE_STR) != 0
    && strcmp(**args, ROUT_STR) != 0
    && strcmp(**args, RINP_STR) != 0) {
    ecmd = (exec_cmd_t*) malloc(sizeof(exec_cmd_t));
    ecmd->type = EXEC;

    while ((arg = **args) != NULL
      && strcmp(arg, FORK_STR) != 0
      && strcmp(arg, PIPE_STR) != 0
      && strcmp(arg, ROUT_STR) != 0
      && strcmp(arg, RINP_STR) != 0) 
      {
      if (pos + 1 >= sz) {
        sz = sz * 2 + 1;
        argv = (string_t*) realloc(argv, sizeof(string_t) * sz);
      }

      argv[pos++] = arg;
      (*args)++;
    }

    sz = pos + 1;
    argv = (string_t*) realloc(argv, sizeof(string_t) * sz);
    argv[pos] = NULL;

    ecmd->argv = argv;
    cmd = (cmd_t*) ecmd;
  }

  return cmd;
}
