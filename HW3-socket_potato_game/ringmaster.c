//
//  ringmaster.c
//  HW3
//
//  Created by Yifan Li on 2/17/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include "potato.h"

int sockfd_master;
int comm_status;
int i = 0;
int port_num, num_players, num_hops;

void printErrorMsg(const char *str) {
    perror(str);
    exit(EXIT_FAILURE);
}

void master_setup() {
    sockfd_master = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_master == -1) {
        printErrorMsg("Cannot open socket for ringmaster ");
    }
    
    int yes = 1;
    int ifSetsockopt = setsockopt(sockfd_master, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &yes, sizeof(yes));
    if (ifSetsockopt == -1) {
        printErrorMsg("Cannot setsockopt ");
    }
    
    struct sockaddr_in master;
    
    memset(&master, 0, sizeof(master));
    master.sin_family = AF_INET;
    master.sin_port = htons(port_num);
    master.sin_addr.s_addr = INADDR_ANY;
    
    int ifBinded = bind(sockfd_master, (struct sockaddr *)&master, sizeof(master));
    if (ifBinded == -1) {
        printErrorMsg("Cannot bind master socket ");
    }
    
    int ifListening = listen(sockfd_master, num_players);
    if (ifListening == -1) {
        printErrorMsg("Cannot listen to master socket ");
    }
}

void connection_init(struct client * players) {
    while (i < num_players) {
        struct sockaddr_in player;
        socklen_t addrlen = sizeof(player);
        int sockfd_PandM = accept(sockfd_master, (struct sockaddr *)&player, &addrlen);
        if (sockfd_PandM < 0) {
            printErrorMsg("Cannot accept connection ");
        }
        
        comm_status = (int)send(sockfd_PandM, &i, sizeof(int), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot send player ID ");
        }
        
        comm_status = (int)send(sockfd_PandM, &num_players, sizeof(int), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot send total # of players ");
        }
        
        comm_status = (int)send(sockfd_PandM, &num_hops, sizeof(int), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot send total # of hops ");
        }

        struct hostent * host_player = gethostbyaddr((char *)&player.sin_addr, sizeof(struct in_addr), AF_INET);        
        int playerPort;
        comm_status = (int)recv(sockfd_PandM, &playerPort, sizeof(int), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot receive port info from player ");
        }

        players[i].playerID = i;
        players[i].masterSocket = sockfd_PandM;
        players[i].listeningPort = playerPort;
        strcpy(players[i].hostname, host_player->h_name);
        
        i++;
    }
}

int data_trasfer(struct client *players) {
    char message[256];
    if (num_hops == 0) {
        i = 0;
        strcpy(message, "gameover");
        while (i < num_players) {
            comm_status = (int)send(players[i].masterSocket, message, sizeof(message), 0);
            close(players[i].masterSocket);
            printf("Player %d is ready to play\n", i);
            i++;
        }
        return 1;
    }
    
    //
    for (i = 0; i < num_players; i++) {
        memset(message, '\0', 256);
        strcpy(message, "ready");
        comm_status = (int)send(players[i].masterSocket, message, sizeof(message), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot send message to player ");
        }
        
        int recv_buf = 0;
        int received = (int)recv(players[i].masterSocket, &recv_buf, sizeof(int), 0);
        memset(message, '\0', 256);
        sprintf(message, "%s:%d", players[i].hostname, players[i].listeningPort);
        if (i != num_players-1) {
            comm_status = (int)send(players[i+1].masterSocket, message, sizeof(message), 0);
            if (comm_status < 0) {
                printErrorMsg("Cannot send message to next player ");
            }
            received = (int)recv(players[i+1].masterSocket, &recv_buf, sizeof(int), 0);
        } else {
            comm_status = (int)send(players[0].masterSocket, message, sizeof(message), 0);
            if (comm_status < 0) {
                printErrorMsg("Cannot send message to next player ");
            }
            received = (int)recv(players[0].masterSocket, &recv_buf, sizeof(recv_buf), 0);
        }
        if (received == -1) {
            printErrorMsg("Cannot receive acknowledgement from player ");
        }
        printf("Player %d is ready to play\n", i);
    }
    return 0;
}

int main (int argc, char * argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Invalid input arguments. Usage: ./ringmaster <port_num> <num_players> <num_hops>\n");
        exit(EXIT_FAILURE);
    }
    
    char *endptr1 = NULL;
    char *endptr2 = NULL;
    char *endptr3 = NULL;
    port_num = (int)strtol(argv[1], &endptr1, 10);
    if (port_num > 51097 || port_num < 51015 || *endptr1!='\0') {
        fprintf(stderr, "Invalid port number. Valid range: 51015 ~ 51097\n");
        exit(EXIT_FAILURE);
    }
    num_players = (int)strtol(argv[2], &endptr2, 10);
    if (num_players < 2 || *endptr2!='\0') {
        fprintf(stderr, "Invalid number of players. Valid range: 1 ~ INT_MAX\n");
        exit(EXIT_FAILURE);
    }
    num_hops = (int)strtol(argv[3], &endptr3, 10);
    if (num_hops > 512 || num_hops < 0 || *endptr3!='\0') {
        fprintf(stderr, "Invalid number of hops. Valid range: 0 ~ 512\n");
        exit(EXIT_FAILURE);
    }
    

    printf("Potato Ringmaster\n");
    printf("Players = %d\n", num_players);
    printf("Hops = %d\n", num_hops);

    struct client players[num_players];
    master_setup();
    connection_init(players);
    int shutdown = data_trasfer(players);
    if (shutdown == 1)
        return EXIT_SUCCESS;
    
   
    srand((unsigned int) time(NULL) + num_players);
    int theChosenOne = rand() % num_players;
    printf("Ready to start the game, sending potato to player %d\n", theChosenOne);
    
    char *potato = (char *)malloc(sizeof(char) * num_hops * 10);
    sprintf(potato, "%d|%s", num_hops, "Trace: ");
    int potato_len = (int)strlen(potato);
    comm_status = (int)send(players[theChosenOne].masterSocket, potato, potato_len, 0);
    if (comm_status < 0) {
        printErrorMsg("Cannot send the very first hot potato to the chosen one");
    }
    
    memset(potato, 0, num_hops*10);
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(players[0].masterSocket, &rfd);
    int maxfd = players[0].masterSocket;
    for (i = 1; i < num_players; i++) {
        FD_SET(players[i].masterSocket, &rfd);
        if (maxfd < players[i].masterSocket)
            maxfd = players[i].masterSocket;   
    }
    // set a 5 sec timeout
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    comm_status = select(maxfd+1, &rfd, NULL, NULL, &tv);
    if (comm_status < 0) {
        printErrorMsg("select failed");
    } else if (comm_status == 0) {
        printErrorMsg("select timed out");
    }
    
    for (i = 0; i < num_players; i++) {
        if (FD_ISSET(players[i].masterSocket, &rfd)) {
            comm_status = (int)recv(players[i].masterSocket, potato, num_hops*10, 0);
            if (comm_status < 0) {
                printErrorMsg("Cannot receive potato");
            }
            break;
        }
    }
    
    char *tmp = strtok(potato, ":");
    tmp = strtok(NULL, ":");
    char * trace = NULL;
    if (tmp)
        trace = tmp;
    
    printf("Trace of potato: \n%s\n", trace+1);
    for (i = 0; i < num_players; i++) {
        comm_status = (int)send(players[i].masterSocket, potato, strlen(potato), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot send potato to player");
        }
        close(players[i].masterSocket);
    }
    free(potato);
    return EXIT_SUCCESS;
}
