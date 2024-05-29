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
char root[PATH_MAX] = {0};

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
  } else if (startsWith("INFO", command)) {
    printf("Hello, I am naive server with simple custom shell\n"
           "Command list:\n"
           "ECHO [args...] - print args to console\n"
           "INFO           - show this page\n"
           "CD   path      - change current directory\n"
           "LIST           - show file list in current directory\n"
           "QUIT           - quit\n");
  } else if (startsWith("CD", command)) {
    if (startsWith("/", tail(command))) {
      chdir(root);
    } else {
      char copy[strlen(command) + 1];
      strcpy(copy, tail(command));
      copy[strlen(copy) - 1] = 0;
      char absolutePath[PATH_MAX];
      realpath(copy, absolutePath);
      if (startsWith(root, absolutePath)) chdir(absolutePath);
    }
  } else if (startsWith("LIST", command)) {
    waitpid(
        RUN_FORKED(execl("/bin/ls", "ls", "-la", "--color=force", NULL)),
        NULL,
        0
    );
  } else if (startsWith("QUIT", command)) {
    return -1;
  } else if (startsWith("@", command)) {
    char filename[strlen(command) + 1];
    strcpy(filename, command + 1);
    filename[strlen(filename) - 1] = 0;
    FILE *file = fopen(filename, "r");
    printf("Opened: %s\n", filename);
    if (file == NULL) {
      printf("Failed to open file: %s\n", filename);
    } else {
      ungetc(fgetc(file), file);
      while (!feof(file)) {
        char buffer[1024] = {0};
        fgets(buffer, sizeof(buffer), file);
        if (handleCommand(buffer) == -1) break;
      }
      fclose(file);
    }
  } else {
    printf("Unknown command: %s", command);
  }
  return 0;
}

int runShell(int connection) {
  close(STDOUT_FILENO);
  dup(connection);
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
  getcwd(root, sizeof(root));
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
