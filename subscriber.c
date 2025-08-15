#include<string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
#include <math.h>

#define MAX_CONNECTIONS 2
const char DATA_TYPES[4][12] = {
    "INT",
    "SHORT_REAL",
    "FLOAT",
    "STRING"
};


int parse_input (char id[11], int listenfd) {
    struct tcp_message mes;
    strcpy(mes.id, id);
    strcpy(mes.command, "");
    strcpy(mes.message, "");
    char buf[100];
    if (fgets(buf, sizeof(buf), stdin)) {
        int i = 0;
        char *p = strtok(buf, " ");
        strcpy(mes.command, "");
        strcpy(mes.message, "");
        while(p) {
            if (i == 0) {
                strcpy(mes.command, p);
                i++;
            } else {
                strcpy(mes.message, p);
                mes.message[strlen(mes.message) - 1] = '\0';
            }
            p = strtok(NULL, " ");
        }
        mes.len = strlen(mes.command) + strlen(mes.message); 
        send_all(listenfd, &mes, sizeof(mes));
        if (strstr(mes.command, "exit") != 0) {
            mes.command[strlen(mes.command) - 1] = '\0';
            return 1;
        }
        if (strcmp(mes.command, "subscribe") == 0) {
            printf("Subscribed to topic %s\n", mes.message);
            return 0;
        }
        if (strcmp(mes.command, "unsubscribe") == 0) {
            printf("Unsubscribed from topic %s\n", mes.message);
            return 0;
        }
    } else {
        return 0;
    }
    return 0;
}

void print_message (char ip[12], int port, char topic[51], int data_type, char message[1500]) {
    printf("%s:%d - %s - %s - ", ip, port, topic, DATA_TYPES[data_type]);
    if (data_type == 0) {
        int sign = message[0];
        uint32_t value = 0;
        memcpy(&value, message + 1, sizeof(uint32_t));
        if (sign) {
            printf("-%d\n", ntohl(value));
        } else {
            printf("%d\n", ntohl(value));
        }
        return;
    }
    if (data_type == 1) {
        uint16_t value = 0;
        memcpy(&value, message, sizeof(uint16_t));
        value = ntohs(value);
        printf("%.2f\n", value / 100.0);
        return;
    }
    if (data_type == 2) {
        int sign = message[0];
        uint32_t value = 0;
        uint8_t powIndex = 0;
        memcpy(&value, message + 1, sizeof(uint32_t));
        memcpy(&powIndex, message + 5, sizeof(uint8_t));
        value = ntohl(value);
        double result = value / pow(10, powIndex);
        if (sign) {
            printf("-%.*g\n", 10, result);
        } else {
            printf("%.*g\n", 10, result);
        }
        return;
    }
    if (data_type == 3) {
        printf("%s\n", message);
    }
}
void run_client(int listenfd, char id[11]) {
    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        fprintf(stderr, "Failed to creade epoll\n");
        exit(1);
    }

    struct epoll_event events[2];
    events[0].events = EPOLLIN;
    events[0].data.fd = listenfd;
    int rc;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &events[0]);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    events[1].events = EPOLLIN;
    events[1].data.fd = STDIN_FILENO;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &events[1]);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    while(1) {
        int current_events = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1);
        if (current_events < 0) {
            fprintf(stderr, "Issue with waiting for events\n");
            exit(1);
        }
        for (int i = 0; i < current_events; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd ==  listenfd) {
                    struct udp_message mes;
                    int rc = recv_all(events[i].data.fd, &mes, sizeof(mes));
                    // client is already connected
                    if (rc <= 0) {
                        // epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        // close(listenfd);
                        close(epollfd);
                        return;
                    }
                    
                    print_message(mes.ip, mes.port, mes.topic, mes.data_type, mes.message);
                } else if (events[i].data.fd == STDIN_FILENO) {
                    int rc = parse_input(id, listenfd);
                    if (rc == 1) {
                        //close(listenfd);
                        close(epollfd);
                        return;
                    }
                }
            }
        }
    }
}
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }
    // deactivate buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int port = atoi(argv[3]);
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        exit(1);
    }

    // set everything in socket address
    struct sockaddr_in TCP_addr;

    memset(&TCP_addr, 0, sizeof(TCP_addr));
    TCP_addr.sin_family = AF_INET;
    TCP_addr.sin_port = htons(port);

    int rc;
    rc = inet_pton(AF_INET, argv[2], &TCP_addr.sin_addr.s_addr);
    if (rc < 0) {
        fprintf(stderr, "Failed to parse ip address\n");
        exit(1);
    }

    int option = 1;
    rc = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
    if (rc < 0) {
        fprintf(stderr, "Failed to disable Nagle's algorithm\n");
        exit(1);
    }

    // connect to server
    rc = connect(listenfd, (struct sockaddr *)&TCP_addr, sizeof(TCP_addr));
    if (rc < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(1);
    }

    // send message with only id to help server identity uniqueness
    struct tcp_message mes;
    strcpy(mes.id, argv[1]);
    mes.len = strlen(argv[1]);
    send_all(listenfd, &mes, sizeof(mes));

    run_client(listenfd, argv[1]);

    close(listenfd);

    return 0;
}