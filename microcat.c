#include <unistd.h>
#include <fcntl.h>
// #include <fstream>
// #include <iostream>

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
