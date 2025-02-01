/**
 * CMSC B355 Assignment 2, Part 2: Parser
 * 
 * The main driver for part 2, exercising the functionality in libparser.c and 
 * printing the contents of the struct Cmd.
 * 
 * @author: Bridge Schaad
 * @version: February 1, 2025
 */

#include "libparser.h"
#include <string.h>


void print_cmd(struct Cmd* cmd)
{
  // implement me
  printf("cmd1_args: [");
  int i;
  for (i = 0; i < cmd->max_argv; i++) {
    printf("%s ", cmd->cmd1_argv[i]);
    if (cmd->cmd1_argv[i] == NULL) {
      break;
    }
  }
  printf("]\ncmd2_args: [");
  for (i = 0; i < cmd->max_argv; i++) {
    printf("%s ", cmd->cmd2_argv[i]);
    if (cmd->cmd2_argv[i] == NULL) {
      break;
    }
  }
  printf("]\n\n");
  for (int j = 0; j < 3; j++) {
    printf("cmd1_fds[%d]: %s\n", j, cmd->cmd1_fds[j]);
  }
  printf("\n");
  for (int j = 0; j < 3; j++) {
    printf("cmd2_fds[%d]: %s\n", j, cmd->cmd2_fds[j]);
  }
  printf("\n\n");
}

int main()
{
  char line[2048];
  struct Cmd cmd;

  strncpy(line, "ls -l", 2048);      
  get_command(line, &cmd);
  print_cmd(&cmd);

  strncpy(line, "ls | sort", 2048);      
  get_command(line, &cmd);
  print_cmd(&cmd);

  strncpy(line, "ls -l 1> out.txt 2> error.txt", 2048);      
  get_command(line, &cmd);
  print_cmd(&cmd);

  strncpy(line, "grep -i blah < input.txt | sort 1> output.txt", 2048);      
  get_command(line, &cmd);
  print_cmd(&cmd);

  return 0;
}

