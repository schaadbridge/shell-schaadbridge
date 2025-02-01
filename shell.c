/**
 * CMSC B355 Assignment 2, Part 3: Simple Shell
 * 
 * Implements a cat-themed shell using readline() for user input, fork(), and 
 * execvp() for child processes. The shell quits when the user types "exit".
 * 
 * @author: Bridge Schaad
 * @version: February 1, 2025
 */

#include "libparser.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>

#define ANSI_COLOR_PURPLE "\x1b[35m"
#define ANSI_COLOR_RESET  "\x1b[0m"

int main()
{
  printf("    /\\_/\\ \n"
         "= ( • . • ) =\n"
         "   /      \\     \n\n");

  char hostName[32];
  char cwdName[128];
  char* pwname = getpwuid(geteuid())->pw_name;
  int host_ret = gethostname(hostName, 32);
  getcwd(cwdName, 128);

  char* prompt;

  if (host_ret == 0 || cwdName == NULL || pwname == NULL) {
    prompt = (char*) malloc((strlen(ANSI_COLOR_PURPLE) + strlen(ANSI_COLOR_RESET) + strlen(hostName) +
       strlen(cwdName) + strlen(pwname) + 20) * sizeof(char));
    sprintf( prompt, ANSI_COLOR_PURPLE "%s@%s:%s$ meow <3 -- " ANSI_COLOR_RESET, pwname, hostName, cwdName);
  } else {
    printf("host_ret (0 for success): %d\n" 
            "cwdName (non-null for success): %s\n"
            "pwname (non-null for success): %s\n", host_ret, cwdName, pwname);
    exit(1);
  }
  while (1) {
    // read input in using readline
    char *line = readline( prompt );
    add_history(line);

    // Quit if the user types "exit" 
    if (strcmp("exit", line) == 0) {
      printf(ANSI_COLOR_PURPLE "purrrrrrrrrrr\n" ANSI_COLOR_RESET);
      exit(1);
    }

    // split command into arguments w/ execvp
    struct Cmd cmd;
    get_command(line, &cmd);

    // fork off child process
    if (cmd.cmd1_argv != NULL) {
      int status;
      if (fork() != 0) { // Parent node: wait for child to exit
        waitpid(-1, &status, 0);

        if (WIFSIGNALED(status)) {
          int signal = WTERMSIG(status);
          printf("Eek! %s (%d)\n", strsignal(signal), signal);
        } else if (WIFSTOPPED(status)) {
          printf("Stopped signal number = %d\n", WSTOPSIG(status));
        }
      } else { // Child node: execute command
        execvp(cmd.cmd1_argv[0], cmd.cmd1_argv);
      }
    }

    free_command(&cmd);
    free(line);
  }
}

