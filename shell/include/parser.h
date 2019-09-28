#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define PIPE "|"
#define FORK "&"

#define ARG_TOKEN   0
#define FORK_TOKEN  1
#define PIPE_TOKEN  2

#define SINGLE_CMD  0
#define FORK_CMD    1
#define PIPE_CMD    2

typedef int       bool;
typedef char*     string_t;
typedef string_t* args_t;
typedef int       token_type_t;
typedef int       cmd_type_t;

typedef struct Token {
  args_t args;
  token_type_t type;
} token_t;

typedef struct Cmd {
  cmd_type_t     type;
} cmd_t;

typedef struct ExecCmd {
  cmd_type_t  type;
  args_t*     left;
} exec_cmd_t;

typedef struct ForkCmd {
  cmd_type_t  type;
  exec_cmd_t* left;
  cmd_t*      right;
} fork_cmd_t;

token_t*  _next_token(const bool pop);
token_t*  get_next_token();
token_t*  peek_next_token();
void      set_args_list(string_t*);
cmd_t*    parse_fork();
cmd_t*    parse(const string_t*);
