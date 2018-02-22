//
//  player.c
//  HW3
//
//  Created by Yifan Li on 2/19/18.
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

int playerSocket, sockfd_PandM;
int port_num, num_players, num_hops;
struct sockaddr_in player;
int comm_status;
struct hostent * player_hostent;
int playerID;
int prevPlayerSocket, nextPlayerSocket;

void printErrorMsg(const char *str) {
    fprintf(stderr, "Error: %s", str);
    exit(EXIT_FAILURE);
}

void data_connect_read(char *machine_name) {
    sockfd_PandM = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_PandM < 0) {
        printErrorMsg("Cannot open socket for client\n");
    }
    
    player_hostent = gethostbyname(machine_name);
    if (player_hostent == NULL) {
        printErrorMsg("Invalid host name\n");
    }
    
    memset(&player, 0, sizeof(player));
    player.sin_family = AF_INET;
    player.sin_port = htons(port_num);
    memmove(&player.sin_addr.s_addr, player_hostent->h_addr_list[0], player_hostent->h_length); 

    int ifConnected = connect(sockfd_PandM, (struct sockaddr *)&player, sizeof(player));
    if (ifConnected < 0) {
        printErrorMsg("Cannot connect to ringmaster\n");
    }
    // read player ID, # players and # hops
    comm_status = (int)recv(sockfd_PandM, &playerID, sizeof(int), 0);
    if (comm_status < 0) {
        printErrorMsg("Cannot receive player ID\n");
    }
    comm_status = (int)recv(sockfd_PandM, &num_players, sizeof(int), 0);
    if (comm_status < 0) {
        printErrorMsg("Cannot receive total # of players\n");
    }
    comm_status = (int)recv(sockfd_PandM, &num_hops, sizeof(int), 0);
    if (comm_status < 0) {
        printErrorMsg("Cannot receive total # of hops\n");
    }
}

int findPort() {
    int sockfd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_client < 0) {
        fprintf(stderr, "Error creating socket for client\n");
        return -1;
    }
    struct sockaddr_in master_addr;
    memset(&master_addr, 0, sizeof(master_addr));
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr.s_addr = INADDR_ANY;

    int port = -1;
    for (int p = 1024; p <= 65535; p++) {
        master_addr.sin_port = htons(p);
        int isBinded = bind(sockfd_client, (struct sockaddr *)&master_addr, sizeof(master_addr));
        if (isBinded < 0)
            continue;
        else {
            port = p;
            break;
        }
    }
    playerSocket = sockfd_client;
    return port;
}

int main (int argc, char * argv[]) {
    if (argc != 3) {
        printErrorMsg("Invalid input argument: Usage: ./player <machine_name> <port_num>\n");
    }

    char *endptr;
    port_num = (int)strtol(argv[2], &endptr, 10);
    if (port_num > 51097 || port_num < 51015  || *endptr != '\0') {
        printErrorMsg("Invalid port number. Valid range: 51015 ~ 51097\n");
    }

    data_connect_read(argv[1]);
    
    printf("Connected as player %d out of %d total players\n", playerID, num_players);

    int port = findPort();
    if (port == -1) {
        printErrorMsg("All ports unavailable\n");
    }
    comm_status = (int)send(sockfd_PandM, &port, sizeof(int), 0);
    if (comm_status < 0) {
        printErrorMsg("Cannot send port info to ringmaster\n");
    }
    
    char message[256];
    for (int i = 0; i < 2; i++) {
        memset(message, 0, 256);
        comm_status = (int)recv(sockfd_PandM, &message, sizeof(message), 0);
        if (comm_status < 0) {
            printErrorMsg("Cannot receive message from ringmaster\n");
        }
        int recv_buf = 0;
	    int received;
        if (strcmp(message, "gameover") == 0) {
            return EXIT_SUCCESS;
        } else if (strcmp(message, "ready") == 0) {
            struct sockaddr_in neighbor;
	        memset(&neighbor, 0, sizeof(neighbor));
            int ifListening = listen(playerSocket, 2);
            if (ifListening < 0) {
                printErrorMsg("Cannot listen to socket\n");
            }
            socklen_t len = sizeof(neighbor);
            received = (int)send(sockfd_PandM, &recv_buf, sizeof(int), 0);
            nextPlayerSocket = accept(playerSocket, (struct sockaddr *)&neighbor, &len);
            if (nextPlayerSocket < 0) {
                printErrorMsg("Cannot accept connection\n");
            }
        } else {
            struct sockaddr_in neighbor;
            char *tmp = strtok(message, ":");
            char *host;
            int port;
            if (tmp)
                host = tmp;
            tmp = strtok(NULL, ":");
            if (tmp)
                port = atoi(tmp);
            
            prevPlayerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (prevPlayerSocket < 0) {
                printErrorMsg("Cannot crate socket for player\n");
            }
            player_hostent = gethostbyname(host);
            if (player_hostent == NULL) {
                printErrorMsg("Invalid host name\n");
            }
	        memset(&neighbor, 0, sizeof(neighbor));
            neighbor.sin_family = AF_INET;
            neighbor.sin_port = htons(port);
	        memmove(&player.sin_addr.s_addr, player_hostent->h_addr_list[0], player_hostent->h_length); 
	        recv_buf = 0;
            received = (int)send(sockfd_PandM, &recv_buf, sizeof(int), 0);
            int ifConnected = connect(prevPlayerSocket, (struct sockaddr *)&neighbor, sizeof(neighbor));
            if (ifConnected < 0) {
                printErrorMsg("Cannot conntect to neighbor\n");
            }
        }
    }


    srand((unsigned int) time(NULL) + playerID);
    fd_set rfd;
    struct timeval tv;
    int maxfd;
    char *potato = malloc(sizeof(char) * 10 * num_hops);
    int leng = num_hops*10;
    while(1) {
        leng = 10*num_hops;
        memset(potato, 0, leng);
        FD_ZERO(&rfd);
        FD_SET(sockfd_PandM, &rfd);
        maxfd = sockfd_PandM;
        FD_SET(prevPlayerSocket, &rfd);
        if (maxfd < prevPlayerSocket)
            maxfd = prevPlayerSocket;
        FD_SET(nextPlayerSocket, &rfd);
        if (maxfd < nextPlayerSocket)
            maxfd = nextPlayerSocket;
        // set a 5 sec timeout
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        comm_status = select(maxfd+1, &rfd, NULL, NULL, &tv);
        if (comm_status < 0) {
            printErrorMsg("Select\n");
        } else if (comm_status == 0) {
            printErrorMsg("Select timed out\n");
        }
        
        if (FD_ISSET(sockfd_PandM, &rfd)) {
            comm_status = recv(sockfd_PandM, potato, leng, 0);
            if (comm_status < 0) {
                printErrorMsg("Cannot receive potato\n");
            }
        } else if (FD_ISSET(prevPlayerSocket, &rfd)) {
            comm_status = recv(prevPlayerSocket, potato, leng, 0);
            if (comm_status < 0) {
                printErrorMsg("Cannot receive potato\n");
            }
        } else if (FD_ISSET(nextPlayerSocket, &rfd)) {
            comm_status = recv(nextPlayerSocket, potato, leng, 0);
            if (comm_status < 0) {
                printErrorMsg("Cannot receive potato\n");
            }
        }
        leng = strlen(potato);
        char ptt[leng];
        strcpy(ptt, potato);
        char * tmp = strtok(ptt, "|");
        char *trace;
        int num_hops;
        if (tmp)
            num_hops = atoi(tmp);
        tmp = strtok(NULL, "|");
        if (tmp)
            trace = tmp;
        if (num_hops > 1) {
            num_hops--;
            if (strcmp(trace, "Trace: ") == 0)
                sprintf(trace, "%s%d", trace, playerID);
            else
                sprintf(trace, "%s,%d", trace, playerID);
            sprintf(potato, "%d|%s", num_hops, trace);
            leng = strlen(potato);
            int random_neighbor = rand() % 2;
	        int id;
            if (random_neighbor == 0) {
                if (playerID == 0)
                    id = num_players - 1;
                else
                    id = playerID - 1;
                printf("Sending potato to %d\n", id);
                comm_status = (int)send(prevPlayerSocket, potato, leng, 0);
                if (comm_status < 0) {
 		            printErrorMsg("Cannot send potato to left neighbor\n");
                }
            } else if (random_neighbor == 1) {
                if (playerID == num_players-1)
                    id = 0;
                else
                    id = playerID+1;
                printf("Sending potato to %d\n", id);
                comm_status = (int)send(nextPlayerSocket, potato, leng, 0);
                if (comm_status < 0) {
  		            printErrorMsg("Cannot send potato to right neighbor\n");
                }
            }
        } else if (num_hops == 1) {
            num_hops--;
            if (strcmp(trace, "Trace: ") == 0)
                sprintf(trace, "%s%d", trace, playerID);
            else
                sprintf(trace, "%s,%d", trace, playerID);
            
            sprintf(potato, "%d|%s", num_hops, trace);
            
	        printf("I'm it\n");
	        leng = strlen(potato);
	        comm_status = (int)send(sockfd_PandM, potato, leng, 0);
            if (comm_status < 0) {
	            printErrorMsg("Cannot send potato to master\n");
            }
        } else if (num_hops == 0) {
            free(potato);
            close(sockfd_PandM);
            close(playerSocket);
            close(nextPlayerSocket);
            close(prevPlayerSocket);
            break;
        }
        
        
    }
    
    return EXIT_SUCCESS;
}
