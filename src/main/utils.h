#pragma once

#include <stdio.h>
#include <stdlib.h>

#define PORT 3636

#define ER(msg)                                                                \
  do {                                                                         \
    perror(msg);                                                               \
    exit(__LINE__);                                                            \
  } while (0);
