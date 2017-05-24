#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>

#include "structs.c"

#define PORTNUMB "50000"
#define BACKLOG 20
#define MAX_DNS_REQUST_SIZE 65536
#define MAXDATASIZE 100
#define MAXNAMESIZE 255
#define BLACKLIST_FILENAME "blacklist.ini"
#define SETTINGS_FILENAME "settings.ini"
#define DNS_PORT "53"
#define QTYPE_A 1
#define QCLASS_IN 1

int count_lines(char* name);
int check_hostname(char* hostname, char blacklist[][MAXDATASIZE], int len);
void load_settings();
void load_blacklist(char blacklist[][MAXDATASIZE]);
void error(const char *msg);
void add_dns_name(char* dns,char* host);
void tcp_handler(int sock, char blacklist[][MAXDATASIZE], int len);
void send_dns_request(char* dns_request, long len);
void start_tcp_server();
void get_dns(char* hostname, char* dns_request);


char dns_ip[20];
char error_res[100];


int count_lines(char* name) {
    int line_count = 0;
    char* buff;
    size_t len = 0;
    FILE* file;

    if ( (file = fopen(name, "r")) == NULL ) {
        error("ERROR opening blacklist file");
    }

    while ((getline(&buff, &len, file)) != -1) {
        line_count++;
    }
    
    fclose(file);
    
    return line_count;
}

int check_hostname(char* hostname, char blacklist[][MAXDATASIZE], int len) {
    int i = 0;

    while ( i < len ) {
        if ( strcmp(blacklist[i], hostname) == 0 ) {
            return 1;
        }
        i++;
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
    int i = 0;
    size_t len = 0;


    blacklst_file = fopen(BLACKLIST_FILENAME, "r");
    
    while (getline(&buffer, &len, blacklst_file) != -1) {
        strtok(buffer, "\n");
        strcpy(blacklist[i], buffer);
        i++;
    }

    free(buffer);
    fclose(blacklst_file);
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void add_dns_name(char* dns,char* host) {
    strcat((char*)host,".");
    int i = 0;
    int host_i = 0;
     
    while (i < strlen((char*)host)) {
        if(host[i]=='.') {
            *dns++ = i-host_i;

            while (host_i < i) {
                *dns++=host[host_i];
                host_i++;
            }

            host_i++;
        }
        i++;
    }
    *dns++='\0';
}


void tcp_handler(int sock, char blacklist[][MAXDATASIZE], int len) {
    int numbytes;
    char hostname[MAXNAMESIZE];
    char dns_request[MAX_DNS_REQUST_SIZE];
    int status;

    bzero(hostname,MAXDATASIZE);
    
    while (1) {
        if ((numbytes = recv(sock, hostname, MAXDATASIZE-1, 0)) < 0) {
            error("ERROR reading from socket");
        }

        hostname[numbytes-2] = '\0';

        if ((status = check_hostname(hostname, blacklist, len)) == 1) {
            if (send(sock, error_res, MAXDATASIZE-1, 0) < 0) {
                error("ERROR writing to socket");
            }
            continue;
        }

        get_dns(hostname, dns_request);

        if (send(sock, dns_request, MAX_DNS_REQUST_SIZE-1, 0) < 0) {
            error("ERROR writing to socket");
        }
    }
}


void send_dns_request(char* dns_request, long len) {
    int sock_udp;
    int udp_status;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    socklen_t recv_size;

    memset(&hints, 0, sizeof(hints));
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((udp_status = getaddrinfo(dns_ip, DNS_PORT, &hints, &servinfo)) != 0) {
        error(gai_strerror(udp_status));
    }

    p = servinfo;

    while (p != NULL) {
        if ((sock_udp = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            p = p->ai_next;
            continue;
        }
        break;
    }

    printf("Socket open\n");
    if (p == NULL) {
        error("ERROR opening udp_socket");
    }

    printf("Sending Packet...\n");
    if ((sendto(sock_udp, dns_request, len, 0, p->ai_addr, p->ai_addrlen)) < 0) {
        error("ERROR sending dns_request");
    }
    printf("Done\n");

    freeaddrinfo(servinfo);

    recv_size = sizeof(their_addr);
    printf("Recieving Packet...\n");
    if ((recvfrom(sock_udp, dns_request, MAX_DNS_REQUST_SIZE-1, 0, (struct sockaddr *)&their_addr, &recv_size)) < 0) {
        error("ERROR recive dns_request");
    }
    printf("Done\n");

    close(sock_udp);
}

void start_tcp_server() {
    int tcp_socket, new_tcpsock, status, yes;
    socklen_t clilen;
    struct addrinfo serv_addr, cli_addr, *tcp_info;

    int list_len = count_lines(BLACKLIST_FILENAME);
    char black_list[list_len][MAXDATASIZE];

    memset(&serv_addr, 0, sizeof(serv_addr));

    load_blacklist(black_list);

    serv_addr.ai_family = AF_UNSPEC;
    serv_addr.ai_socktype = SOCK_STREAM;
    serv_addr.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORTNUMB, &serv_addr, &tcp_info)) != 0) {
        error(gai_strerror(status));
    }

    tcp_socket = socket(tcp_info->ai_family, tcp_info->ai_socktype, tcp_info->ai_protocol);

    if (tcp_socket < 0) {
        error("ERROR opening tcp_socket");
    }

// handle "address already in use" error
    if (setsockopt(tcp_socket, SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1) {
        error("ERROR on setsockopt");
    } 

    if (bind(tcp_socket, tcp_info->ai_addr, tcp_info->ai_addrlen) < 0) {
        error("ERROR on binding");
    }
    
    if (listen(tcp_socket,BACKLOG) < 0) {
        error("ERROR listen");
    }

    freeaddrinfo(tcp_info);
    
    printf("server: waiting for connections...\n");
    
    while (1) {
        clilen = sizeof(cli_addr);
        new_tcpsock = accept(tcp_socket, (struct sockaddr *) &cli_addr, &clilen);

        if (new_tcpsock < 0) {
            perror("ERROR on accept");
            continue;
        }

        if (!fork()) {
            close(tcp_socket);

            tcp_handler(new_tcpsock, black_list, list_len);

            close(new_tcpsock);
            exit(0);
        }
        close(new_tcpsock);
    }
    
    close(tcp_socket);
}

void get_dns(char* hostname, char* dns_request) {
    char *qname;
    struct dns_header* dns = NULL;
    struct question* quest_info = NULL;
    long dns_request_size;
    const long DNS_HEADER_SIZE = sizeof(struct dns_header); 
    
    dns = (struct dns_header*)dns_request;

    dns->id = (short) htons(getpid());
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

    qname =(char*)&dns_request[DNS_HEADER_SIZE];
    add_dns_name(qname , hostname);
    quest_info = (struct question*)&dns_request[DNS_HEADER_SIZE + (strlen((const char*)qname) + 1)];
    quest_info->qtype = htons(QTYPE_A);
    quest_info->qclass = htons(QCLASS_IN);
    dns_request_size = DNS_HEADER_SIZE + (strlen((const char*)qname) + 1) + sizeof(struct question);
    send_dns_request(dns_request, dns_request_size);
}

int main() {
    load_settings();
    start_tcp_server();

    return 0;
}
