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
#define MAX_DNS_REQUST_SIZE 65536
#define MAXDATASIZE 100
#define MAXNAMESIZE 63
#define DNS_PORT "53"
#define QTYPE_A 1
#define QCLASS_IN 1

void error(const char *msg);
void add_dns_name(unsigned char* dns,unsigned char* host);
void tcp_handler(int sock);
void send_dns_request(unsigned char* dns_request, unsigned long len);
void start_tcp_server();
char* get_dns(unsigned char* hostname);

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void add_dns_name(unsigned char* dns,unsigned char* host) {
    strcat((char*)host,".");
     
    for(int i = 0, host_i = 0; i < strlen((char*)host) ; i++) {
        if(host[i]=='.') {
            *dns++ = i-host_i;

            for( ; host_i < i; host_i++) {
                *dns++=host[host_i];
            }

            host_i++;
        }
    }
    *dns++='\0';
}


void tcp_handler(int sock) {
    int numbytes;
    char hostname[MAXNAMESIZE];
    // char* ip;

    bzero(hostname,MAXDATASIZE);

    while (1) {
        if ((numbytes = recv(sock, hostname, MAXDATASIZE-1, 0)) < 0) {
            error("ERROR reading from socket");
        }

        hostname[numbytes] = '\0';
        
        printf("Here is the request: %s",hostname);

        get_dns(hostname);

        if (send(sock, hostname, MAXDATASIZE-1, 0) < 0) {
            error("ERROR writing to socket");
        }
    }
}


void send_dns_request(unsigned char* dns_request, unsigned long len) {
    int sock_udp;
    int udp_status;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    int recv_size;
    char dns_ip[15] = "8.8.8.8";

    bzero((char *) &hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((udp_status = getaddrinfo(dns_ip, DNS_PORT, &hints, &servinfo)) != 0) {
        error(gai_strerror(udp_status));
    }

    // sock_udp = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock_udp = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            continue;
        }

        break;
    }
    printf("Socket open\n");
    if (p == NULL) {
        error("ERROR opening udp_socket");
    }
    printf("Sending Packet...\n");
    if ((sendto(sock_udp, (char*)dns_request, len, 0, p->ai_addr, p->ai_addrlen)) < 0) {
        error("ERROR sending dns_request");
    }
    printf("Done\n");
    freeaddrinfo(servinfo);

    recv_size = sizeof(their_addr);
    printf("Recieving Packet...\n");
    if ((recvfrom(sock_udp, (char*)dns_request, MAX_DNS_REQUST_SIZE-1, 0, (struct sockaddr *)&their_addr, &recv_size)) < 0) {
        error("ERROR recive dns_request");
    }
    printf("Done\n");
    close(sock_udp);
    // return dns_request;
}

void start_tcp_server() {
    int sockfd, newsockfd, pid, status;
    socklen_t clilen;
    struct addrinfo serv_addr, cli_addr, *tcp_info;
    char s[INET6_ADDRSTRLEN];

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.ai_family = AF_UNSPEC;
    serv_addr.ai_socktype = SOCK_STREAM;
    serv_addr.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORTNUMB, &serv_addr, &tcp_info)) != 0) {
        error(gai_strerror(status));
    }

    sockfd = socket(tcp_info->ai_family, tcp_info->ai_socktype, tcp_info->ai_protocol);


    if (sockfd < 0) {
        error("ERROR opening tcp_socket");
    }


    if (bind(sockfd, tcp_info->ai_addr, tcp_info->ai_addrlen) < 0) {
        error("ERROR on binding");
    }
    
    if (listen(sockfd,BACKLOG) < 0) {
        error("ERROR listen");
    }
    freeaddrinfo(tcp_info);
    
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

char* get_dns(unsigned char* hostname) {
    char dns_request[MAX_DNS_REQUST_SIZE], *qname;
    struct dns_header* dns = NULL;
    struct question* quest_info = NULL;
    unsigned long dns_request_size;
    const unsigned long DNS_HEADER_SIZE = sizeof(struct dns_header); 
    

    dns = (struct dns_header*)&dns_request;

    dns->id = (unsigned short) htons(getpid());
    dns->rd = 1;
    dns->tc = 0;
    dns->aa = 0;
    dns->opcode = 0;
    dns->qr = 0;
    dns->rcode = 0;
    dns->cd = 0;
    dns->ad = 0;
    dns->z = 0;
    dns->ra = 0;
    dns->qdcount = htons(1);
    dns->ancount = 0;
    dns->nscount = 0;
    dns->arcount = 0;

    qname =(unsigned char*)&dns_request[DNS_HEADER_SIZE];
    add_dns_name(qname , hostname);
    quest_info = (struct question*)&dns_request[DNS_HEADER_SIZE + (strlen((const char*)qname) + 1)];
    quest_info->qtype = htons(QTYPE_A);
    quest_info->qclass = htons(QCLASS_IN);
    dns_request_size = DNS_HEADER_SIZE + (strlen((const char*)qname) + 1) + sizeof (struct question);
    printf("%d\n", (int)dns_request_size);
    send_dns_request(dns_request, dns_request_size);
}

int main() {
    char blacklist[20];
    char er_response[20];
    

    start_tcp_server();

    return 0;
}
