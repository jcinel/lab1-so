#include "../include/parser.h"

static string_t* old_args = NULL;

token_t* _next_token(const bool pop) {
  token_t* tkn = (token_t*) malloc(sizeof(token_t));
  int sz = 0;
  string_t* args = old_args;
  string_t arg;

  if ((arg = args[sz++]) == NULL) {
    return (token_t*) NULL;
  } else if (strcmp(PIPE, arg) == 0) {
    tkn->type = PIPE_TOKEN;
  } else if(strcmp(FORK, arg) == 0) {
    tkn->type = FORK_TOKEN;
  } else {
    tkn->type = ARG_TOKEN;

    while ((arg = args[sz++]) != NULL
      && strcmp(PIPE, arg) != 0
      && strcmp(FORK, arg) != 0);
    sz--;
  }

  tkn->args = (string_t*) malloc(sizeof(string_t) * (sz + 1));
  memcpy(tkn->args, args, sizeof(string_t) * sz);
  tkn->args[sz] = NULL;

  if (pop == TRUE) {
    old_args += sz;
  }

  return tkn;
}

token_t* get_next_token() {
  return _next_token(TRUE);
}

token_t* peek_next_token() {
  return _next_token(FALSE);
}

void set_arg_list(string_t* args) {
  old_args = args;
}

cmd_t* parse_fork() {
  cmd_t* cmd = (cmd_t*) malloc(sizeof(cmd_t));
  exec_cmd_t* ecmd = (exec_cmd_t*) malloc(sizeof(exec_cmd_t));
  fork_cmd_t* fcmd;
  token_t* token;
  
  if ((token = get_next_token()) == NULL || token->type != ARG_TOKEN) {
    return NULL;
  }

  ecmd->left = token->args;
  ecmd->type = SINGLE_CMD;
  cmd = (cmd_t*) ecmd;

  if((token = peek_next_token()) != NULL && token->type == FORK_TOKEN) {
    fcmd = (fork_cmd_t*) malloc(sizeof(fork_cmd_t));
    fcmd->left = ecmd;
    fcmd->type = FORK_CMD;
    fcmd->right = NULL;

    if((token = get_next_token()) != NULL && token->type == ARG_TOKEN) {
      fcmd->right = parse_fork();
    }

    cmd = (cmd_t*) fcmd;
  }

  return cmd;
}

cmd_t* parse(const string_t* args) {
  cmd_t* cmd;

  set_arg_list(args);
  cmd = parse_fork();

  return cmd;
}
