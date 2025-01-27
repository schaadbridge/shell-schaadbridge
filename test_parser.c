#include "libparser.h"
#include <string.h>

void print_cmd(struct Cmd* cmd)
{
  // implement me
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

