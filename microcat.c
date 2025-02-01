/**
 * CMSC B355 Assignment 2, Part 1: Microcat
 * 
 * Implements a simple Unix "cat" command with no flags. Concatenates and 
 * prints files to standard output. If no arguments are given, takes input 
 * from stdin. 
 * 
 * @author: Bridge Schaad
 * @version: February 1, 2025
 */

#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
  if (argc == 1) {
    char buffer[1];
    int ret = read(STDIN_FILENO, buffer, 1);
    while (ret > 0) {
      write(STDOUT_FILENO, buffer, 1);
      ret = read(STDIN_FILENO, buffer, 1);
    }
  }
  
  // loop through each arg
  for (int i = 1; i < argc; i++) {
    int infile = open(argv[i], O_RDONLY);
    // read one byte at a time
    char buffer[1];
    int ret = read(infile, buffer, 1);
    // confirm not EOF or error
    while (ret > 0) {
      write(STDOUT_FILENO, buffer, 1);
      ret = read(infile, buffer, 1);
    }
    close(infile);
  }
}
