#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>

#include "structs.c"
#include "dns_server.h"

unsigned char dns_ip[20];
unsigned char error_res[100];


int count_lines(unsigned char* name) {
    int line_count = 0;
    char* buff;
    size_t len = 0;
    FILE* file;

    if ( (file = fopen(name, "r")) == NULL ) {
        error("ERROR opening blacklist file");
    }

    for ( ; (getline(&buff, &len, file)) != -1; line_count++ );
    
    fclose(file);
    
    return line_count;
}

int check_hostname(unsigned char* hostname, char blacklist[][MAXDATASIZE], int len) {
    for ( int i = 0; i < len; i++ ) {
        if ( strcmp(blacklist[i], hostname) == 0 ) {
            return 1;
        }
    }

    return 0;
}

void load_settings() {
    FILE* settings_file;

    char* buffer;
    char stop = ' ';
    char buff_ip[20];
    char buff_err_res[100];

    if ((settings_file = fopen(SETTINGS_FILENAME, "r")) == NULL) {
        error("ERROR opening settings file");
    }
    
    fgets(buff_ip, 50, settings_file);
    
    if ((buffer = strchr(buff_ip, stop)) == NULL) {
        error("ERROR missed settings");
    }

    buffer++;
    strtok(buffer, "\n");
    strcpy(dns_ip, buffer);
    
    fgets(buff_err_res, 100, settings_file);
    if ((buffer = strchr(buff_err_res, stop)) == NULL) {
        error("ERROR missed settings");
    }

    buffer++;
    strtok(buffer, "\n");
    strcpy(error_res, buffer);

    fclose(settings_file);
}

void load_blacklist(char blacklist[][MAXDATASIZE]) {
    FILE* blacklst_file;
    char* buffer;
    size_t len = 0;

    blacklst_file = fopen(BLACKLIST_FILENAME, "r");
    
    for (int i = 0; getline(&buffer, &len, blacklst_file) != -1; i++) {
        strtok(buffer, "\n");
        strcpy(blacklist[i], buffer);
    }

    free(buffer);
    fclose(blacklst_file);
}

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


void udp_handler(int sock, char* hostname, char blacklist[][MAXDATASIZE], int len) {
    int numbytes;
    unsigned char dns_request[MAX_DNS_REQUST_SIZE];
    int status;

    if ((status = check_hostname(hostname, blacklist, len)) == 1) {
        if (send(sock, error_res, MAXDATASIZE-1, 0) < 0) {
            error("ERROR writing to socket");
        }
    }
    printf("%s\n", hostname);
    get_dns(hostname, dns_request);

    if (send(sock, dns_request, MAX_DNS_REQUST_SIZE-1, 0) < 0) {
        error("ERROR writing to socket");
    }
}


void send_dns_request(unsigned char* dns_request, unsigned long len) {
    int sock_udp;
    int udp_status;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    int recv_size;

    bzero((char *) &hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((udp_status = getaddrinfo(dns_ip, DNS_PORT, &hints, &servinfo)) != 0) {
        error(gai_strerror(udp_status));
    }

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
}

void start_udp_server() {
    int numbytes;
    int udp_socket, new_udpsock, status, cli_status;
    socklen_t cli_len;
    struct addrinfo serv_addr, *udp_info;
    struct sockaddr_storage cli_addr;


    int list_len = count_lines(BLACKLIST_FILENAME);
    char black_list[list_len][MAXDATASIZE];

    bzero((char *) &serv_addr, sizeof(serv_addr));

    load_blacklist(black_list);

    serv_addr.ai_family = AF_UNSPEC;
    serv_addr.ai_socktype = SOCK_DGRAM;
    serv_addr.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORTNUMB, &serv_addr, &udp_info)) != 0) {
        error(gai_strerror(status));
    }

    udp_socket = socket(udp_info->ai_family, udp_info->ai_socktype, udp_info->ai_protocol);

    if (udp_socket < 0) {
        error("ERROR opening udp_socket");
    }

    if (bind(udp_socket, udp_info->ai_addr, udp_info->ai_addrlen) < 0) {
        error("ERROR on binding");
    }
    
    freeaddrinfo(udp_info);
    
    printf("server: waiting for connections...\n");
    
    while (1) {
        char buf[MAXDATASIZE];
        cli_len = sizeof(cli_addr);

        if ((numbytes = recvfrom(udp_socket, buf, MAXDATASIZE-1 , 0, (struct sockaddr *)&cli_addr, &cli_len)) < 0) {
            perror("ERROR on recive");
            continue;
        }
        buf[numbytes-1] = '\0';

        if ((new_udpsock = socket(cli_addr.ss_family, SOCK_DGRAM, 0)) < 0) {
            perror("ERROR on socket");
            continue;
        }
        if ((connect(new_udpsock, (struct sockaddr*)&cli_addr, cli_len)) < 0) {
            perror("ERROR on connect");
            continue;
        }

        if (!fork()) {
            close(udp_socket);

            udp_handler(new_udpsock, buf, black_list, list_len);

            close(new_udpsock);
            exit(0);
        }
        close(new_udpsock);
    }
    
    close(udp_socket);
}

void get_dns(unsigned char* hostname, unsigned char* dns_request) {
    unsigned char *qname;
    struct dns_header* dns = NULL;
    struct question* quest_info = NULL;
    unsigned long dns_request_size;
    const unsigned long DNS_HEADER_SIZE = sizeof(struct dns_header); 
    
    dns = (struct dns_header*)dns_request;

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
    dns_request_size = DNS_HEADER_SIZE + (strlen((const char*)qname) + 1) + sizeof(struct question);
    send_dns_request(dns_request, dns_request_size);
}
