#ifndef PARSER
#define PARSER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FORK_STR "&"
#define PIPE_STR "|"
#define ROUT_STR ">"
#define RINP_STR "<"
#define ROUT_STR_APP ">>"

typedef char* string_t;

typedef enum {
  TRUE,
  FALSE
} bool_t;

typedef enum {
  EXEC,
  FORK,
  PIPE,
  ROUT,
  RINP,
  ROUTAPP
} cmd_type_t;

typedef struct {
  cmd_type_t   type;
} cmd_t;

typedef struct {
  cmd_type_t  type;
  string_t*   argv;
} exec_cmd_t;

typedef struct {
  cmd_type_t  type;
  cmd_t*      left;
} fork_cmd_t;

typedef struct {
  cmd_type_t  type;
  cmd_t*      left;
  cmd_t*      right;
} pipe_cmd_t;

typedef struct {
  cmd_type_t  type;
  cmd_t*      left;
  string_t    file;
} redi_cmd_t;

typedef struct {
  cmd_type_t  type;
  cmd_t*      left;
  string_t    file;
} redi_cmd_app_t;


string_t  get_line();
string_t* split_line(string_t);

cmd_t*    parse_args(string_t*);
cmd_t*    parse_pipe(string_t**);
cmd_t*    parse_redi(string_t**);
cmd_t*    parse_redi_app(string_t**);
cmd_t*    parse_fork(string_t**);
cmd_t*    parse_exec(string_t**);

#endif
