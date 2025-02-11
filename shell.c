/**
 * CMSC B355 Assignment 2, Part 3: Simple Shell 
 * -- Extended for Assignment 3: Better Shell
 * 
 * Implements a cat-themed shell using readline() for user input, fork(), and 
 * execvp() for child processes. The shell quits when the user types "exit".
 * 
 * Better Shell supports file redirection and piping.
 * 
 * @author: Bridge Schaad
 * @version: February 10, 2025
 */

#include <err.h>
#include "libparser.h"
#include <fcntl.h>
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
    if (cmd->cmd1_fds[0] != NULL) { // input redirection
      int fd_in = open(cmd->cmd1_fds[0], O_RDONLY);
      if (fd_in == -1) {
        check_err("open");
      }
      if (dup2(fd_in, STDIN_FILENO) == -1) {
        check_err("dup2");
      }
      close(fd_in);
    }
    if (cmd->cmd1_fds[1] != NULL) { // output redirection
      int fd_out = open(cmd->cmd1_fds[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd_out == -1) {
        check_err("open");
      }
      if (dup2(fd_out, STDOUT_FILENO) == -1) {
        check_err("dup2");
      }
      close(fd_out);
    }
    if (cmd->cmd1_fds[2] != NULL) { // error redirection
      int fd_err = open(cmd->cmd1_fds[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd_err == -1) {
        check_err("open");
      }
      if (dup2(fd_err, STDERR_FILENO) == -1) {
        check_err("dup2");
      }
      close(fd_err);
    }
    if (execvp(cmd->cmd1_argv[0], cmd->cmd1_argv) == -1) {
      check_err("execvp");
    }

    free_command(cmd);
  } else {
    if (waitpid(pid, &status, 0) == -1) {
      check_err("waitpid");
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
    if (cmd->cmd1_fds[0] != NULL) { // input redirection
      int fd_in = open(cmd->cmd1_fds[0], O_RDONLY);
      if (fd_in == -1) {
        check_err("open");
      }
      if (dup2(fd_in, STDIN_FILENO) == -1) {
        check_err("dup2");
      }
      if (close(fd_in) == -1) {
        check_err("close");
      }
    }
    // Skip std_out redirection for first command in fork, 
    // must pipe it to second command
    if (cmd->cmd1_fds[2] != NULL) { // error redirection
      int fd_err = open(cmd->cmd1_fds[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd_err == -1) {
        check_err("88");
      }
      if (dup2(fd_err, STDERR_FILENO) == -1) {
        check_err("dup2");
      }
      close(fd_err);
    }

    if (close(pipefd[0]) == -1)
      check_err("pipe");
    int dup1_ret = dup2(pipefd[1], STDOUT_FILENO);
    if (dup1_ret == -1) {
      check_err("dup");
    }
    // child1 close output on pipe
    if (close(pipefd[1]) == -1)
      check_err("pipe");
    if (execvp(cmd->cmd1_argv[0], cmd->cmd1_argv) == -1) {
      check_err("exec");
    }

    free_command(cmd);
  } else { // ends child1 body
    cpid2 = fork();

    if (cpid2 == -1) 
      err(EXIT_FAILURE, "fork");

    if (cpid2 == 0) { // child 2 body: 
      // skip std_in redirection on second command in pipe
      // must pipe from first command
      if (cmd->cmd2_fds[1] != NULL) { // output redirection
        int fd_out = open(cmd->cmd2_fds[1], O_WRONLY | O_CREAT | O_TRUNC);
        if (fd_out == -1) {
          check_err("open");
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
          check_err("dup2");
        }
        close(fd_out);
      }
      if (cmd->cmd2_fds[2] != NULL) { // error redirection
        int fd_err = open(cmd->cmd2_fds[2], O_WRONLY | O_CREAT | O_TRUNC);
        if (fd_err == -1) {
          check_err("open");
        }
        if (dup2(fd_err, STDERR_FILENO) == -1) {
          check_err("dup2");
        }
        if (close(fd_err) == -1) {
          check_err("close");
        }
      }

      // child2 map to standard output
      if (close(pipefd[1]) == -1)
        check_err("close");
      int dup2_ret = dup2(pipefd[0], STDIN_FILENO);
      if (dup2_ret == -1) {
        check_err("dup2");
      }
      // child2 close input on pipe
      if (close(pipefd[0]) == -1)
        check_err("close");
      if (execvp(cmd->cmd2_argv[0], cmd->cmd2_argv) == -1) {
        check_err("execvp");
      }
      free_command(cmd);
    } else { // ends child2 body, only the parent reaches here!!
      if (close(pipefd[0]) == -1)
        check_err( "close");
      if (close(pipefd[1]) == -1)
        err(EXIT_FAILURE, "close");
      if (waitpid(cpid1, &status1, 0) == -1) {
        check_err("waitpid child 1");
      }
      if (waitpid(cpid2, &status2, 0) == -1) {
        check_err("waitpid child 2");
      }
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
          check_err("chdir");
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
    if (&cmd != NULL) {
      free_command(&cmd);
    }
    free(line);
  }
}

