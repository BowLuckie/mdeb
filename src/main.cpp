#include "linenoise.h"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"
#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Please enter a program to debug!";
    return EXIT_FAILURE;
  }

  char* prog = argv[1];

  int pid = fork();
  if (pid == 0) {
  }

  if (pid >= 1) {
  }

  return EXIT_SUCCESS;
}
