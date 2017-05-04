#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "structs.c"

#define PORTNUMB "50000"
#define BACKLOG 20
#define MAXDATASIZE 65536



void error(const char *msg) {
    perror(msg);
    exit(1);
}

void tcp_handler(int sock) {
    int numbytes;
    char buffer[MAXDATASIZE];

    bzero(buffer,MAXDATASIZE);

    while (1) {
        if ((numbytes = recv(sock, buffer, MAXDATASIZE-1, 0)) < 0) {
            error("ERROR reading from socket");
        }

        buffer[numbytes] = '\0';
        
        printf("Here is the message: %s",buffer);

        if (send(sock, buffer, MAXDATASIZE-1, 0) < 0) {
            error("ERROR writing to socket");
        }
    }
}


void start_tcp_server() {
    int sockfd, newsockfd, pid, status;
    socklen_t clilen;
    struct addrinfo serv_addr, cli_addr, *ip_info;
    char s[INET6_ADDRSTRLEN];

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.ai_family = AF_UNSPEC;
    serv_addr.ai_socktype = SOCK_STREAM;
    serv_addr.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORTNUMB, &serv_addr, &ip_info)) != 0) {
        error(gai_strerror(status));
    }

    sockfd = socket(ip_info->ai_family, ip_info->ai_socktype, ip_info->ai_protocol);


    if (sockfd < 0) {
        error("ERROR opening socket");
    }


    if (bind(sockfd, ip_info->ai_addr, ip_info->ai_addrlen) < 0) {
        error("ERROR on binding");
    }
    
    if (listen(sockfd,BACKLOG) < 0) {
        error("ERROR listen");
    }
    freeaddrinfo(ip_info);
    
    printf("server: waiting for connections...\n");
    
    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        if (!fork()) {
            close(sockfd);

            tcp_handler(newsockfd);

            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    
    close(sockfd);
}

char* get_dns(char name[], int len) {
    
}

int main() {
    const *char blacklist[20];
    char er_response[20];
    char dns_ip[15];

    start_tcp_server();

    return 0;
}
