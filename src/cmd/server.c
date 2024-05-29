#include "onSignal.h"
#include "utils.h"
#include <linux/limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int work = 1;
char clientPath[PATH_MAX] = {0};

void onInt(int signal) {
  (void)signal;
  work = 0;
}

int startsWith(const char *prefix, const char *str) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

const char *tail(const char *command) {
  char *offset = strchr(command, ' ');
  if (offset == NULL) return command + strlen(command) - 1;
  return offset + 1;
}

int handleCommand(const char *command) {
  if (strcmp("\n", command) == 0) {
    /* nothing */
  } else if (startsWith("ECHO", command)) {
    printf("%s", tail(command));
  } else if (startsWith("INFO", command) == 0) {
    printf("Hello, I am naive server with simple custom shell\n"
           "Command list:\n"
           "ECHO [args...] - print args to console\n"
           "INFO           - show this page\n"
           "CD   path      - change current directory\n"
           "LIST           - show file list in current directory\n"
           "QUIT           - quit\n");
  } else if (startsWith("CD", command) == 0) {
    if (strcmp("/", tail(command)) == 0) {
      getcwd(clientPath, sizeof(clientPath));
    } else {
      char absolutePath[PATH_MAX], currentPath[PATH_MAX];
      realpath(tail(command), absolutePath);
      getcwd(currentPath, sizeof(currentPath));
      if (startsWith(currentPath, absolutePath))
        strcpy(clientPath, absolutePath);
    }
  } else if (startsWith("QUIT", command) == 0) {
    return -1;
  } else {
    printf("Unknown command: %s", command);
  }
  return 0;
}

int runShell(int connection) {
  dup2(connection, STDOUT_FILENO);
  getcwd(clientPath, sizeof(clientPath));
  while (1) {
    const char prefix[] = "> ";
    write(connection, prefix, sizeof(prefix) - 1);
    char command[1024] = {0};
    if (read(connection, command, sizeof(command)) == -1) ER("Failed to read");
    fprintf(stderr, "Connection: %d Command: %s", connection, command);
    if (handleCommand(command) == -1) break;
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
