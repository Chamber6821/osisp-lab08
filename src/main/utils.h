#pragma once

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#define PORT 3636

#define ADDRESS                                                                \
  ((struct sockaddr_in){                                                       \
      .sin_family = AF_INET,                                                   \
      .sin_addr = {inet_addr("127.0.0.1")},                                    \
      .sin_port = htons(PORT),                                                 \
  })

#define ER(msg)                                                                \
  do {                                                                         \
    perror(msg);                                                               \
    exit(__LINE__);                                                            \
  } while (0);

#define RUN_FORKED(expr)                                                       \
  ({                                                                           \
    int childPid = fork();                                                     \
    if (childPid == 0) {                                                       \
      exit(expr);                                                              \
    }                                                                          \
    childPid;                                                                  \
  })
