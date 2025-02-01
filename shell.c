#include "libparser.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h>

int main()
{
  // starting implementation from class slides pseudocode
  // char* prompt, host, pwd;
  // int host_ret = gethostname(host, 10);
  while (1) {
    // read input in using readline
    // char *prompt = getpwid(geteuid()) + "@" + gethostname() + ":" getcwd() + "<3 -- ";
    char *line = readline("prompt <3 --  ");
    add_history(line);

    // Quit if the user types "exit" 
    if (strcmp("exit", line) == 0) {
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

