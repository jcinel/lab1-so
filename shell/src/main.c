#include <stdio.h>
#include <stdlib.h>

#include "../include/shell.h"

int main(int argc, char** argv) {
  sh_loop(); //O shell roda em loop infinito
  return EXIT_SUCCESS;
}
