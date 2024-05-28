#include "onSignal.h"
#include "utils.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define RUN_FORKED(expr)                                                       \
  ({                                                                           \
    int childPid = fork();                                                     \
    if (childPid == 0) {                                                       \
      exit(expr);                                                              \
    }                                                                          \
    childPid;                                                                  \
  })

int work = 1;

void onInt(int signal) {
  (void)signal;
  work = 0;
}

int runShell(int connection) {
  if (dup2(connection, STDIN_FILENO) == -1)
    ER("Failed to redirect connection to stdin");
  if (dup2(connection, STDOUT_FILENO) == -1)
    ER("Failed to redirect stdout to connection");
  if (execl("/bin/sh", "/bin/sh", NULL) == -1) ER("Failed to run /bin/sh");
  return -1;
}

int serve(int connection) {
  waitpid(RUN_FORKED(runShell(connection)), NULL, 0);
  printf("Close connection: %d\n", connection);
  close(connection);
  return 0;
}

int listener(int socket) {
  while (work) {
    int connection = accept(socket, NULL, NULL);
    if (connection == -1) {
      perror("Failed to accept socket");
      continue;
    }
    printf("Accepted connection: %d\n", connection);
    RUN_FORKED(serve(connection));
  }
  printf("Stop listen\n");
  return 0;
}

int main() {
  onSignal(SIGINT, onInt);
  int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (tcpSocket == -1) ER("Failed to open socket");
  struct sockaddr_in sockerAddress = {
      .sin_family = AF_INET,
      .sin_port = PORT,
  };
  if (bind(tcpSocket, &sockerAddress, sizeof(sockerAddress)) == -1)
    ER("Failed to bind socket");
  if (listen(tcpSocket, 42) == -1) ER("Failed to listen socket");

  int listenerProcess = RUN_FORKED(listener(tcpSocket));
  printf("Start listener: %d\n", listenerProcess);
  waitpid(listenerProcess, NULL, 0);

  printf("Close socket\n");
  close(tcpSocket);
}
