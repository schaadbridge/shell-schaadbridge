#ifndef parser_H_
#define parser_H_

#include <string>
#include <sys/types.h>

struct Cmd {
  bool foreground;
  char *job_str;
  char **cmd1_argv;
  char **cmd2_argv;
  char *cmd1_fds[3];
  char *cmd2_fds[3];
  int max_argv;
  pid_t pgrp;
};

extern void get_command(char* line, struct Cmd* cmdline);

extern void free_command(struct Cmd* cmdline);

#endif

