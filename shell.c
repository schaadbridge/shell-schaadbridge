/**
 * CMSC B355 Assignment 2, Part 3: Simple Shell 
 * -- Extended for Assignment 3: Better Shell
 * 
 * Implements a cat-themed shell using readline() for user input, fork(), and 
 * execvp() for child processes. The shell quits when the user types "exit".
 * 
 * Known issues:
 * leaves a child process hanging for commands not found, such as "test", or "exit ",
 * leading to the "exit" command required as many times as child processes are hanging
 * 
 * @author: Bridge Schaad
 * @version: February 7, 2025
 */

#include <err.h>
#include "libparser.h"
#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ANSI_COLOR_PURPLE "\x1b[35m"
#define ANSI_COLOR_RESET  "\x1b[0m"

void check_err(char* line) {
  perror(line);
  exit(0);
}

int get_prompt(char* buffer) {
  char hostName[32];
  char cwdName[128];
  char* pwname = getpwuid(geteuid())->pw_name;
  int host_ret = gethostname(hostName, 32);
  getcwd(cwdName, 128);

  if (host_ret == 0 || cwdName == NULL || pwname == NULL) {
    sprintf( buffer, ANSI_COLOR_PURPLE "%s@%s:%s$ meow <3 -- " ANSI_COLOR_RESET, pwname, hostName, cwdName);
  } else {
    printf("host_ret (0 for success): %d\n" 
            "cwdName (non-null for success): %s\n"
            "pwname (non-null for success): %s\n", host_ret, cwdName, pwname);
    exit(1);
  }
  return 0;
}

void single_command(struct Cmd* cmd) {
  int status;
  pid_t pid = fork();

  if (pid == -1) {
    err(EXIT_FAILURE, "fork");
  }

  if (pid == 0) { // Child -- execute command
    if (execvp(cmd->cmd1_argv[0], cmd->cmd1_argv) == -1) {
      check_err("63");
    }
  } else {
    if (waitpid(pid, &status, 0) == -1) {
      check_err("66");
    };
  }
}

void pipe_command(struct Cmd* cmd) {
  // create a pipe
  int    pipefd[2];
  pid_t  cpid1;
  pid_t  cpid2;
  int status1; int status2;

  if (pipe(pipefd) == -1){
    err(EXIT_FAILURE, "pipe");
  }

  // two child processes
  cpid1 = fork();
  if (cpid1 == -1){
    err(EXIT_FAILURE, "fork");
  }
  if (cpid1 == 0) { // child 1 body
    // child1 map to standard input
    if (close(pipefd[0]) == -1)
      check_err("91");
    int dup1_ret = dup2(pipefd[1], STDOUT_FILENO);
    if (dup1_ret == -1) {
      check_err("94");
    }
    // child1 close output on pipe
    if (close(pipefd[1]) == -1)
      check_err("98");
    execvp(cmd->cmd1_argv[0], cmd->cmd1_argv);

  } else { // ends child1 body
    cpid2 = fork();

    if (cpid2 == -1) 
      err(EXIT_FAILURE, "fork");

    if (cpid2 == 0) { // child 2 body: 
      // child2 map to standard output
      if (close(pipefd[1]) == -1)
        err(EXIT_FAILURE, "close");
      int dup2_ret = dup2(pipefd[0], STDIN_FILENO);
      if (dup2_ret == -1) {
        err(EXIT_FAILURE, "dup");
      }
      // child2 close input on pipe
      if (close(pipefd[0]) == -1)
        err(EXIT_FAILURE, "close");
      execvp(cmd->cmd2_argv[0], cmd->cmd2_argv);
    } else { // ends child2 body, only the parent reaches here!!
      if (close(pipefd[0]) == -1)
        err(EXIT_FAILURE, "close");
      if (close(pipefd[1]) == -1)
        err(EXIT_FAILURE, "close");
      waitpid(cpid1, &status1, 0);
      waitpid(cpid2, &status2, 0);
    }
  }
}

int main()
{
  printf("    /\\_/\\ \n"
         "= ( • . • ) =\n"
         "   /      \\     \n\n");

  char prompt[256];
  int prompt_ret = get_prompt(prompt);
  while (prompt_ret == 0) {
    // read input in using readline
    char *line = readline( prompt );
    add_history(line);

    // Quit if the user types "exit" 
    if (strcmp("exit", line) == 0) {
      printf(ANSI_COLOR_PURPLE "purrrrrrrrrrr\n" ANSI_COLOR_RESET);
      exit(0);
    }

    // split command into arguments w/ execvp
    struct Cmd cmd;
    get_command(line, &cmd);

    // fork off child process
    if (*cmd.cmd1_argv != NULL) {
      // change directory if user types "cd"
      if (strcmp(cmd.cmd1_argv[0], "cd") == 0) {
        int ret = chdir(cmd.cmd1_argv[1]);
        if (ret != 0) {
          check_err("156");
        } else {
          prompt_ret = get_prompt(prompt);
        }
      } else {
        // single command
        if (*cmd.cmd2_argv == NULL) {
          single_command(&cmd);
        } else { // pipe command
          pipe_command(&cmd);
        }
      }
    }

    free_command(&cmd);
    free(line);
  }
}

