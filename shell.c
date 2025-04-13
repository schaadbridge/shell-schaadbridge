/**
 * CMSC B355 Assignment 2, Part 3: Simple Shell 
 * -- Extended for Assignment 3: Better Shell
 * -- Extended for Assignment 4: Trust the Process (Part 3)
 * 
 * Implements a cat-themed shell using readline() for user input, fork(), and 
 * execvp() for child processes. The shell quits when the user types "exit".
 * 
 * Better Shell supports file redirection and piping.
 * Trust the Process (Part 3) implements foregrounding and backgrounding.
 * 
 * Known issues: resetting terminal after ctrl-c or ctrl-p in child process
 * 
 * @author: Bridge Schaad
 * @version: February 10, 2025
 */

#include <err.h>
#include <list>
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
#define TRUE 1
#define FALSE 0

int child_pid;
int parent_pid;

std::list<struct Cmd*> jobs;

void printJobs() {
  int i = 1;
  for (struct Cmd* job: jobs) {
    char c;
    printf("#%d %s PGID: %d\n", i, job->job_str, job->pgrp);
    i++;
  }
}

#define check_err(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Signal handler */
void handler(int signo) {
  int wstatus;
  int pid;
  while (pid = waitpid(child_pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED) > 0) {
    if (WIFEXITED(wstatus)) {
      printf(" exited, status=%d\n", WEXITSTATUS(wstatus));
    } else if (WIFSIGNALED(wstatus)) {
      printf(" killed by signal %d\n", WTERMSIG(wstatus));
    } else if (WIFSTOPPED(wstatus)) {
      printf(" stopped by signal %d\n", WSTOPSIG(wstatus));
    } else if (WIFCONTINUED(wstatus)) {
      printf(" continued\n");
    }
  }
}

/* Kill all child processes on ctrl-C*/
void c_handler(int signo) {
  kill(-child_pid, SIGTERM);
  for (struct Cmd* job: jobs) {
    free_command(job);
  }
  exit(0);
}

// void babyC_handler(int signo) {
//   printf("babyC handler reached\n");
//   tcsetpgrp(STDIN_FILENO, parent_pid);
//   exit(0);
// }

// void z_handler(int signo) {
//   printf("zhandler reached\n");
//   tcsetpgrp(STDIN_FILENO, parent_pid);
//   kill(-child_pid, SIGSTOP);
// }

/* Generate shell prompt based on cwd*/
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

/* Execute single command (no contents in args2)*/
void single_command(struct Cmd* cmd) {
  int status;

  pid_t pid = fork(); // Fork -- command in child process, parent does cleanup
  signal(SIGCHLD, handler);
  if (pid == -1) {
    err(EXIT_FAILURE, "fork");
  }

  if (pid == 0) { // Child process
    setpgid(0, 0); // Put child in new process group

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
    
    if (execvp(cmd->cmd1_argv[0], cmd->cmd1_argv) == -1) { // Execute with execvp
      check_err("execvp");
    }
  } else { // Parent process
    child_pid = pid;
    parent_pid = getpid();
    cmd->pgrp = child_pid; // Track job's new process group
    printf("cmd->pgrp: %d\n", cmd->pgrp);
    setpgid(pid, pid); // Parent process group
    signal(SIGINT, c_handler);

    int ret;
    if (cmd->foreground == TRUE) { // foreground job
      tcsetpgrp(STDIN_FILENO, pid);
      if (ret = waitpid(pid, &status, WUNTRACED) > 0){
        if (WIFSTOPPED(status)) {
          tcsetpgrp(STDIN_FILENO, getpgid(0));
          jobs.push_back(cmd);
          printf(" stopped by signal %d\n", WSTOPSIG(status));
        } else {
          if (WIFSIGNALED(status)) {
            // kill child if ctrl-c
            tcsetpgrp(STDIN_FILENO, getpgid(0));
            printf(" interrupted\n");
          } 
          printf("freeing\n");
          free_command(cmd); // free finished/killed command
        }
      } else {
        check_err("waitpid");
      }
      tcsetpgrp(STDIN_FILENO, getpgid(0));
    } else { // background job
      if (ret = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED)== -1) {
        // add to end of background jobs list
        check_err("waitpid");
      };
      jobs.push_back(cmd); // Add to joblist if interrupted
    }
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
        int fd_out = open(cmd->cmd2_fds[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
          check_err("open");
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
          check_err("dup2");
        }
        close(fd_out);
      }
      if (cmd->cmd2_fds[2] != NULL) { // error redirection
        int fd_err = open(cmd->cmd2_fds[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
  printf("%ld\n", sizeof(struct Cmd));
  printf("    /\\_/\\ \n"
         "= ( • . • ) =\n"
         "   /      \\     \n\n");

  char prompt[256];
  int prompt_ret = get_prompt(prompt);

  // initialize background job list and pointer to foreground job
  struct Cmd foregroundJob;
  // struct jobNode backgroundJobs;
  while (prompt_ret == 0) {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    // read input in using readline
    char *line = readline( prompt );
    add_history(line);

    // Quit if the user types "exit" 
    if (strcmp("exit", line) == 0) {
      printf(ANSI_COLOR_PURPLE "purrrrrrrrrrr\n" ANSI_COLOR_RESET);
      kill(-child_pid, SIGTERM); // Kill all child processes
      exit(0);
    }
    
    // Don't allocate or parse empty command
    if (strcmp("", line) == 0) {
      free(line);
      continue;
    }

    // split command into arguments w/ execvp
    struct Cmd* cmd = (struct Cmd*) malloc(sizeof(struct Cmd));
    get_command(line, cmd);
    printf("jobstr: %s\n", cmd->job_str);

    // fork off child process
    if (*cmd->cmd1_argv != NULL) {
      // change directory if user types "cd"
      // printf("reached1? %s\n", cmd.cmd1_argv[0]);
      if (strcmp(cmd->cmd1_argv[0], "cd") == 0) {
        int ret = chdir(cmd->cmd1_argv[1]);
        if (ret != 0) {
          perror("cd"); 
        } else {
          prompt_ret = get_prompt(prompt);
        }
      } else if (strcmp(cmd->cmd1_argv[0], "jobs") == 0) {
        printJobs();
      } else if (strcmp(cmd->cmd1_argv[0], "fg") == 0) { 
        // check for job #
        // or get last from job list
        pid_t new_fg = jobs.back()->pgrp;
        printf("newfg: %d\n", new_fg);
        int ret = tcsetpgrp(STDIN_FILENO, new_fg);
        printf("ret: %d\n", ret);
        if (ret == -1) {
          check_err("tcsetpgrp"); // b/c this is base image, this will crash shell
        }
        if (kill(-new_fg, SIGCONT)) {
          check_err("kill"); // this will crash shell
        }
        int wstatus;
        if (waitpid(child_pid, &wstatus, WUNTRACED) > 0) {
          if (WIFSTOPPED(wstatus)) {
            printf("pgid: %d\n", getpgid(0));
            tcsetpgrp(STDIN_FILENO, getpgid(0));
            printf(" stopped by signal %d\n", WSTOPSIG(wstatus));
          } else if (WIFSIGNALED(wstatus)) {
            // kill child if ctrl-c
            tcsetpgrp(STDIN_FILENO, getpgid(0));
            printf(" interrupted\n");
          }
        }
      } else if (strcmp(cmd->cmd1_argv[0], "bg") == 0) { 
        // check for job #
        // or get head from background list
        pid_t new_fg = jobs.back()->pgrp;
        kill(-new_fg, SIGCONT);
      }
        // single command+
      else if (*cmd->cmd2_argv == NULL) {
        // printf("reached");
        single_command(cmd);
      } else { // pipe command
        pipe_command(cmd);
      }
    }
    free(line);
  }
}

