/**
 * CMSC B355 Assignment 2, Part 2: Parser
 * 
 * Parses CLI input with at most 1 pipe into Cmd struct. Header file sets the
 * max number of arguments per process.
 * 
 * @author: Bridge Schaad
 * @version: February 1, 2025
 */

#include "libparser.h"
#include <stdlib.h>
#include <string.h>

void get_command(char* line, struct Cmd* cmdline)
{ 
  // strcpy(cmdline->job_str, line);
  cmdline->job_str = (char*) malloc(sizeof(char) * strlen(line) + 1);
  strcpy(cmdline->job_str, line);
  char* lastChar = &line[(int)strlen(line) - 1];
  if (*lastChar == '&') {
    cmdline->foreground = 0;
    *lastChar = '\0';
  } else {
    cmdline->foreground = 1;
  }
  // initialize input/output redirects to null
  for (int i = 0; i < 3; i++) {
    cmdline->cmd1_fds[i] = NULL;
    cmdline->cmd2_fds[i] = NULL;
  }

  char* tok = strtok(line, " ");

  cmdline->cmd1_argv = (char**) malloc(cmdline->max_argv * sizeof(char*));

  // get I/O redirect and argc
  int argv_idx = 0;
  while (tok != NULL && strcmp(tok, "|") != 0) {
    if (strcmp(tok, "<") == 0) {
      tok = strtok(NULL, " ");
      if (tok != NULL) {
        cmdline->cmd1_fds[0] = tok;
      }
    }
    else if (strcmp(tok, "1>") == 0 || strcmp(tok, ">") == 0) {
      tok = strtok(NULL, " ");
      if (tok != NULL) {
        cmdline->cmd1_fds[1] = tok;
      }
    }
    else if (strcmp(tok, "2>") == 0) {
      tok = strtok(NULL, " ");
      if (tok != NULL) {
        cmdline->cmd1_fds[2] = tok;
      }
    }
    else {
      cmdline->cmd1_argv[argv_idx] = tok;
      argv_idx++;
    }
    tok = strtok(NULL, " ");
  }
  cmdline->cmd1_argv[argv_idx] = NULL;

  // check for command 2!
  cmdline->cmd2_argv = (char**) malloc(cmdline->max_argv * sizeof(char*));
  
  // If the previous loop was exited but there are more tokens, it hit a pipe, skip over it now.
  if (tok != NULL) {
    tok = strtok(NULL, " ");
  }
  argv_idx = 0;

  while (tok != NULL) {
    if (strcmp(tok, "<") == 0) {
      tok = strtok(NULL, " ");
      if (tok != NULL) {
        cmdline->cmd2_fds[0] = tok;
      }
    }
    else if (strcmp(tok, "1>") == 0) {
      tok = strtok(NULL, " ");
      if (tok != NULL) {
        cmdline->cmd2_fds[1] = tok;
      }
    }
    else if (strcmp(tok, "2>") == 0) {
      tok = strtok(NULL, " ");
      if (tok != NULL) {
        cmdline->cmd2_fds[2] = tok;
      }
    }
    else {
      cmdline->cmd2_argv[argv_idx] = tok;
      argv_idx++;
    }
    tok = strtok(NULL, " ");
  }
  cmdline->cmd2_argv[argv_idx] = NULL;
}

/** 
TODO: ask why??? this doesn't seem to be necessary/isn't changing number of 
bytes in use after execution
*/ 
void free_command(struct Cmd* cmdline) {
  free(cmdline->cmd1_argv);
  free(cmdline->cmd2_argv);
  free(cmdline->job_str);
}
