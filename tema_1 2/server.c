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
#include "map.h"
#include "common.h"

#define MAX_CONNECTIONS 32
#define UDP_MES_SIZE 1569

int create_tcp_listener(int port) {
    // create TCP socket
    const int listenfd_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_TCP < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        exit(1);
    }

    // make socket reusable
    int option = 1;
    int rc;
	rc = setsockopt(listenfd_TCP, SOL_SOCKET, SO_REUSEADDR,
				&option, sizeof(int));
    
    if (rc < 0) {
        fprintf(stderr, "Failed to make socket address reusable\n");
        exit(1);
    }

    rc = setsockopt(listenfd_TCP, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
    if (rc < 0) {
        fprintf(stderr, "Failed to disable Nagle's algorithm\n");
        exit(1);
    }

    // set everything in socket address
    struct sockaddr_in TCP_addr;

    memset(&TCP_addr, 0, sizeof(TCP_addr));
    TCP_addr.sin_family = AF_INET;
    TCP_addr.sin_addr.s_addr = INADDR_ANY;
    TCP_addr.sin_port = htons(port);
 
    // bind sockets to the addresses
    rc = bind(listenfd_TCP, (const struct sockaddr *)&TCP_addr, sizeof(TCP_addr));
    if (rc < 0) {
        fprintf(stderr, "Bind failed \n");
        exit(1);
    }

    // create listener
    rc = listen(listenfd_TCP, MAX_CONNECTIONS);
    if (rc < 0) {
        fprintf(stderr, "Listen failed\n");
        exit(1);
    }


    return listenfd_TCP;
}

int create_udp_listener(int port) {
    // create UDP socket
    const int listenfd_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd_UDP < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        exit(1);
    }

    // make sockets reusable
    int option = 1;
    int rc;
    rc = setsockopt(listenfd_UDP, SOL_SOCKET, SO_REUSEADDR,
        &option, sizeof(int));

    if (rc < 0) {
        fprintf(stderr, "Failed to make socket address reusable\n");
        exit(1);
    }
 
    // set everything in socket addresses

    struct sockaddr_in UDP_addr;
    memset(&UDP_addr, 0, sizeof(UDP_addr));
    UDP_addr.sin_family = AF_INET;
    UDP_addr.sin_addr.s_addr = INADDR_ANY;
    UDP_addr.sin_port = htons(port);

    // bind socket to the addresses
    rc = bind(listenfd_UDP, (const struct sockaddr *)&UDP_addr, sizeof(UDP_addr));
    if (rc < 0) {
        fprintf(stderr, "Bind failed \n");
        exit(1);
    }

    return listenfd_UDP;
}

struct udp_message *create_udp_message (char* buf) {
    struct udp_message *mes = malloc(sizeof(struct udp_message));
    strncpy(mes->topic, buf, 50);
    mes->topic[50] = '\0';
    mes->data_type = *(buf + 50); 
    memcpy(mes->message, buf + 51, 1500);
    return mes;
}

void run_server(int tcp_listerfd, int udp_listenerfd) {
    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        fprintf(stderr, "Failed to creade epoll\n");
        exit(1);
    }

    int flags = fcntl(tcp_listerfd, F_GETFL, 0);
    fcntl(tcp_listerfd, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(udp_listenerfd, F_GETFL, 0);
    fcntl(udp_listenerfd, F_SETFL, flags | O_NONBLOCK);

    struct epoll_event events[MAX_CONNECTIONS];
    int events_cnt = 0;
    // add sockets to epoll
    events[0].events = EPOLLIN;
    events[0].data.fd = tcp_listerfd;
    events_cnt++;

    int rc;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, tcp_listerfd, &events[0]);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    events[1].events = EPOLLIN;
    events[1].data.fd = udp_listenerfd;
    events_cnt++;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, udp_listenerfd, &events[1]);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }
    
    events[2].events = EPOLLIN;
    events[2].data.fd = STDIN_FILENO;
    events_cnt++;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &events[2]);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }
    // map to keep subscribers unique
    struct mapStruct *subscribers_map = map_init();

    // map which holds the topics each client is subscirbed to
    struct mapStruct *subscribers_topics = map_init();

    while(1) {
        int current_events = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1);
        if (current_events < 0) {
            fprintf(stderr, "Issue with waiting for events\n");
            exit(1);
        }
        for (int i = 0; i < current_events; i++) {
            if (events[i].events & EPOLLIN) {
                // new tcp subscriber connection
                if (events[i].data.fd == tcp_listerfd) {
                    // variable to hold subscriber's details
                    struct sockaddr_in subscriber_addr;
                    socklen_t addrlen = sizeof(subscriber_addr);
                    // accept connection
                    const int sub_sockfd = accept (tcp_listerfd, 
                        (struct sockaddr *)&subscriber_addr, 
                        &addrlen);
                    
                    // get the credentials message
                    struct tcp_message recv_mes;
                    recv_all(sub_sockfd, &recv_mes, sizeof(recv_mes));

                    // check if user already exists or not
                    struct pairStruct* pair = map_get(subscribers_map, recv_mes.id);
                    if (pair != NULL) { // user alreadye exists
                        printf("Client %s already connected.\n", recv_mes.id);
                        close(sub_sockfd);
                    } else {
                        // if the user is new, add it to epoll
                        events[events_cnt].data.fd = sub_sockfd;
                        events[events_cnt].events = EPOLLIN;
                        
                        rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, sub_sockfd, &events[events_cnt]);
                        if (rc < 0) {
                            fprintf(stderr, "Failed to add socket to epoll\n");
                            exit(1);
                        }

                        events_cnt++;
                        printf("New client %s connected from %s:%d.\n", recv_mes.id, 
                            inet_ntoa(subscriber_addr.sin_addr), 
                            ntohs(subscriber_addr.sin_port));
                        // add user to the map
                        map_add_int(subscribers_map, recv_mes.id, sub_sockfd);
                    }
                } else if (events[i].data.fd == udp_listenerfd) {
                    // recieve content from udp client
                    char buf[UDP_MES_SIZE];
                    memset(buf, 0, UDP_MES_SIZE);
                    struct sockaddr_in udp_client_addr;
                    socklen_t addrlen = sizeof(udp_client_addr);

                    int rc = recvfrom(udp_listenerfd, &buf, sizeof(buf) - 1, 0, 
                            (struct sockaddr *)&udp_client_addr, &addrlen);

                    if (rc < 0) {
                        fprintf(stderr, "Error with recieving data\n");
                        exit(1);
                    }

                    // create udp message structure
                    struct udp_message *mes = create_udp_message(buf);

                    // add info about ip and port
                    strcpy(mes->ip ,inet_ntoa(udp_client_addr.sin_addr));
                    mes->port = ntohs(udp_client_addr.sin_port);

                    // find the users subscribed to topic
                    for (int i = 0; i < subscribers_topics->size; i++) {
                        struct pairStruct* pair = &subscribers_topics->data[i];
                        if (is_subscribed(pair, mes->topic) == 1) {
                            struct pairStruct* id_socket_pair = map_get(subscribers_map, pair->key);
                            if (id_socket_pair != NULL) {
                                send_all(*((int *)id_socket_pair->value), mes, sizeof(struct udp_message));
                            }
                        }
                    }
                    
                } else if (events[i].data.fd == STDIN_FILENO) {
                    char buf[10];
                    if (fgets(buf, sizeof(buf), stdin)) {
                        buf[strlen(buf) - 1] = '\0';
                        if (strcmp(buf, "exit") == 0) {
                            close(epollfd);
                            map_free(subscribers_map); 
                            map_free(subscribers_topics);
                            return;
                        }
                    } else {
                        fprintf(stderr, "Error with inputing info\n");
                        exit(1);
                    }
                }
                else { // updates from the already connected users
                    struct tcp_message recv_mes;
                    recv_all(events[i].data.fd, &recv_mes, sizeof(recv_mes));
                    // check what user wants to do
                    if (strcmp(recv_mes.command, "subscribe") == 0) {
                        // printf("Subscribed to topic %s\n", recv_mes.message);
                        // add topic to the topics array of current user
                        map_add_string_to_array(subscribers_topics, recv_mes.id, (char *)recv_mes.message);
                    } else if (strcmp(recv_mes.command, "unsubscribe") == 0) {
                        // remove topic from the topics array of current user
                        // printf("Unsubscribed from topic %s\n", recv_mes.message);
                        remove_topic(subscribers_topics, recv_mes.id, (char *)recv_mes.message);
                    } else if (strstr(recv_mes.command, "exit") != 0) {
                        // end connection
                        map_remove(subscribers_map, recv_mes.id);
                        // map_remove(subscribers_topics, recv_mes.id);
                        int rc = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        if (rc < 0) {
                            fprintf(stderr, "Error with deleting socket\n");
                            exit(1);
                        }

                        close(events[i].data.fd);
                        // remove also from events array
                        for (int j = i; j < current_events - 1; j++) {
                            events[j] = events[j + 1];
                        }

                        current_events--;
                
                        printf("Client %s disconnected.\n", recv_mes.id);
                    }
                    
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }

    // deactivate buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int port = atoi(argv[1]);

    int tcp_listener = create_tcp_listener(port);

    int udp_listener = create_udp_listener(port);

    run_server(tcp_listener, udp_listener);
    

    close(tcp_listener);
    close(udp_listener);

    return 0;
}