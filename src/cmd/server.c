#include "onSignal.h"
#include "utils.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int work = 1;

void onInt(int signal) {
  (void)signal;
  work = 0;
}

int handleCommand(const char *command, char *buffer) {
#define PRINT(...) buffer += sprintf(buffer, __VA_ARGS__)
  if (strcmp(command, "HI\n") == 0) {
    PRINT("Hi!\n");
    return 0;
  }
  if (strcmp(command, "QUIT\n") == 0) {
    PRINT("Bye-bye\n");
    return -1;
  }
  PRINT("Unknown command: %s", command);
  return 0;
}

int runShell(int connection) {
  while (1) {
    char command[1024] = {0};
    if (read(connection, command, sizeof(command)) == -1) ER("Failed to read");
    char output[1024] = {0};
    if (handleCommand(command, output) == -1) break;
    write(connection, output, strlen(output));
  }
  return 0;
}

int serve(int connection) {
  waitpid(RUN_FORKED(runShell(connection)), NULL, 0);
  const char buf[] = "";
  write(connection, buf, sizeof(buf));
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
  int tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (tcpSocket == -1) ER("Failed to open socket");
  if (bind(tcpSocket, &ADDRESS, sizeof(ADDRESS)) == -1)
    ER("Failed to bind socket");
  if (listen(tcpSocket, 42) == -1) ER("Failed to listen socket");

  int listenerProcess = RUN_FORKED(listener(tcpSocket));
  printf("Start listener: %d\n", listenerProcess);
  waitpid(listenerProcess, NULL, 0);

  printf("Close socket\n");
  close(tcpSocket);
}
