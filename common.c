#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;
    while(bytes_remaining) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return bytes_received;
      size_t currb_recv = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
      if (currb_recv < 0) {
        printf("error with recv\n");
        exit(1);
      } else if (currb_recv == 0) {
        break;
      }
      bytes_remaining -= currb_recv;
      bytes_received += currb_recv;
    }

    return (int)bytes_received;
  }
  
int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;
    while(bytes_remaining) {
      size_t currb_sent = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
      if (currb_sent < 0) {
        printf("error with send\n");
        exit(1);
      } else if (currb_sent == 0) {
        break;
      }
      bytes_remaining -= currb_sent;
      bytes_sent += currb_sent;
    }

    return (int)bytes_sent;
  }
  