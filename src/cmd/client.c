#include "onSignal.h"
#include "utils.h"
#include <bits/pthreadtypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int tcpSocket = 0;
pthread_t transmit = 0, display = 0;

void onInt(int signal) {
  (void)signal;
  pthread_cancel(transmit);
  pthread_cancel(display);
}

void *displayAll(void *arg) {
  (void)arg;
  while (1) {
    char buffer[1024] = {0};
    int got = read(tcpSocket, buffer, sizeof(buffer) - 1);
    if (got == -1) {
      perror("read");
      break;
    }
    if (got == 1 && buffer[0] == 0) break;
    buffer[got] = '\0';
    printf("%s", buffer);
    fflush(stdout);
  }
  pthread_cancel(transmit);
  return NULL;
}

void *transmitAll(void *arg) {
  (void)arg;
  while (1) {
    char buffer[1024] = {0};
    fgets(buffer, sizeof(buffer), stdin);
    int sent = write(tcpSocket, buffer, strlen(buffer));
    if (sent == -1) {
      perror("write");
      break;
    }
  }
  pthread_cancel(display);
  return NULL;
}

int main() {
  onSignal(SIGINT, onInt);
  tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (connect(tcpSocket, &ADDRESS, sizeof(ADDRESS)) == -1)
    ER("Failed to connect");

  pthread_create(&display, NULL, displayAll, NULL);
  pthread_create(&transmit, NULL, transmitAll, NULL);
  pthread_join(transmit, NULL);
  pthread_join(display, NULL);

  const char quit[] = "QUIT\n";
  write(tcpSocket, quit, sizeof(quit) - 1);
  close(tcpSocket);
  printf("Bye, bye!\n");
}
