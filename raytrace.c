#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Raycaster.h"

int main(int argc, char **argv)
{
  if (argc != 5) {
    printf("Error: not enough arguments.\n");
    exit(1);
  }

  generate_image(atoi(argv[1]), atoi(argv[2]), argv[3], argv[4]);

  return 0;
}
