#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void print_buf(char *buf) {
  for (int i = 0; i<BUFSIZ; ++i) {
    printf("%c", buf[i]);
  }
  printf("\n");
}

const char *request = "GET /ws HTTP/1.1\r\nHost: localhost:3000\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: 7Wrl5Wp3kkEaYOVhio4o6w==\r\nSec-WebSocket-Version: 13\r\n\r\n";

int main (int argc, char **argv) {
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(3000);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  int sock = socket(serv_addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    fprintf(stderr, "failed to connect\n");
    return 1;
  }
  char buffer[BUFSIZ] = {0};
  size_t n = send(sock, request, strlen(request), 0);
  if (n == 0) {
    fprintf(stderr, "message wasn't sent\n");
    return 1;
  }
  while (1) {
    n = recv(sock, buffer, BUFSIZ, 0);
    if (n > 0) {
      print_buf(buffer);
    } else {
      fprintf(stderr, "buffer read zero\n");
    }
  }
  return 0;
}
