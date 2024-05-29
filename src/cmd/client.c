#include "utils.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (connect(tcpSocket, &ADDRESS, sizeof(ADDRESS)) == -1)
    ER("Failed to connect");

  char buffer[1024] = {0};
  ssize_t bytes_read, bytes_written;

  while (1) {
    printf("> ");
    fflush(stdout);
    fgets(buffer, sizeof(buffer), stdin);
    if (strlen(buffer) == 1) continue;

    bytes_written = write(tcpSocket, buffer, strlen(buffer));
    if (bytes_written == -1) {
      perror("write");
      break;
    }

    bytes_read = read(tcpSocket, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
      perror("read");
      break;
    }
    if (bytes_read == 1 && buffer[0] == 0) break;
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
  }

  close(tcpSocket);
  printf("Bye, bye!\n");
}
