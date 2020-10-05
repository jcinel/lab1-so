#include <stdio.h>
#include <stdlib.h>

#include "../include/shell.h"

int main(int argc, char** argv) {
  // O shell vai rodar em loop infinito até se encerrar a execução
  sh_loop();
  return EXIT_SUCCESS;
}
