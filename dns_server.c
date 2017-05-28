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
#define MAX_DNS_REQUST_SIZE 10000
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
void get_hostname(char* hostname, char* dns_request);
int udp_handler(int sock, unsigned char* cli_reqv, char blacklist[][MAXDATASIZE], int len);
int send_dns_request(unsigned char* dns_request, long len);
void start_udp_server();

char dns_ip[20];
char error_res[100];

// TODO: Delete getline fun
int count_lines(char* name) {
    int line_count = 0;
    char* buff;
    size_t len = 0;
    FILE* file;

    if ( (file = fopen(name, "r")) == NULL ) {
        error("[count_lines]ERROR opening blacklist file");
    }

    for ( ;(getline(&buff, &len, file)) != -1; line_count++);
    
    fclose(file);
    
    return line_count;
}

int check_hostname(char* hostname, char blacklist[][MAXDATASIZE], int len) {
    for ( int i = 0; i < len; i++ ) {
        if ( strcmp(blacklist[i], hostname) == 0 ) {
            return -1;
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
        error("[load_settings]ERROR opening settings file");
    }
    
    fgets(buff_ip, 50, settings_file);
    
    if ((buffer = strchr(buff_ip, stop)) == NULL) {
        error("[load_settings]ERROR missed settings");
    }

    buffer++;
    strtok(buffer, "\n");
    strcpy(dns_ip, buffer);
    
    fgets(buff_err_res, 100, settings_file);
    
    if ((buffer = strchr(buff_err_res, stop)) == NULL) {
        error("[load_settings]ERROR missed settings");
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

void get_hostname(char* hostname, char* dns_request) {
    for ( int i = 0, j = 1; i < strlen((char*)dns_request); i++, j++) {
        int label_size = dns_request[i];

        for( int x = 0; x < label_size; x++, i++, j++ ) {
            hostname[i] = dns_request[j];
        }
        hostname[i] = '.';
    }

    hostname[strlen(dns_request) - 1] = '\0';
}


int udp_handler(int sock, unsigned char* cli_reqv, char blacklist[][MAXDATASIZE], int len) {
    int mult_rr;
    int status;
    char* host;
    char hostname[100];
    long dns_request_size;
    struct dns_header* head = NULL;
    struct question* quest = NULL;


    head = (struct dns_header*)cli_reqv;
    host = (char*)&cli_reqv[sizeof(struct dns_header)];

    get_hostname(hostname, host);

    mult_rr = (ntohs(head->ancount) + ntohs(head->nscount) + ntohs(head->arcount));
    dns_request_size = (sizeof(struct dns_header) + (strlen((const char*)host) + 1)
    + (ntohs(head->qdcount) * sizeof(struct question)) + (mult_rr * sizeof(struct resource_rec)));
    // printf("size %ld\n", dns_request_size);
    // printf("id - %d\n", head->id);
    // printf("rd - %d\n", head->rd);
    // printf("tc - %d\n", head->tc);
    // printf("aa - %d\n", head->aa);
    // printf("opcode - %d\n", head->opcode);
    // printf("qr - %d\n", head->qr);
    // printf("rcode - %d\n", head->rcode);
    // printf("cd - %d\n", head->cd);
    // printf("ad - %d\n", head->ad);
    // printf("z - %d\n", head->z);
    // printf("ra - %d\n", head->ra);
    // printf("qdcount - %d\n", ntohs(head->qdcount));
    // printf("ancount - %d\n", head->ancount);
    // printf("nscount - %d\n", head->nscount);
    // printf("arcount - %d\n", ntohs(head->arcount));
    
    // printf("host %s\n", host);

    // printf("qtype %d\n", ntohs(quest->qtype));
    // printf("qclass %d\n", ntohs(quest->qclass));
    if ((status = check_hostname(hostname, blacklist, len)) < 0 ) {
        return status;
    }

    return send_dns_request(cli_reqv, dns_request_size);
}
//     int numbytes;
//     char hostname[MAXNAMESIZE];
//     char dns_request[MAX_DNS_REQUST_SIZE];
//     int status;
//     char* quest_info;

//     bzero(hostname,MAXDATASIZE);
    
//     while (1) {
//         if ((numbytes = recv(sock, hostname, MAXDATASIZE-1, 0)) < 0) {
//             perror("[udp_handler]ERROR reading from socket");
//             break;
//         } else if ( numbytes == 0 ) {
//             perror("[udp_handler]Client close connection");
//             break;
//         }

//         // hostname[numbytes-2] = '\0';
//         hostname[numbytes] = '\0';
//         // head = (struct dns_header*)hostname;
//         quest_info = (char*)&hostname[sizeof(struct dns_header) + 2];
//         // printf("%s\n", quest_info);
//         for ( int i = 0; i < strlen(quest_info); i++ ) {
//             printf("Numb %d; val %c; int_val %d\n",i, quest_info[i], quest_info[i] );
//         }

//         // printf("ID %d\n", htons(head->id));
//         // printf("RD %d\n", htons(head->rd));
//         // for ( int i = 0; i < (numbytes - 1); i++ ) {
//         //     printf("Numer %d - value %d - size - %zu\n", i, hostname[i], sizeof hostname[i]);
//         // }
//         // printf("hostname %d\n", numbytes);

//         if ((status = check_hostname(hostname, blacklist, len)) == 1) {
//             if (send(sock, error_res, MAXDATASIZE-1, 0) < 0) {
//                 error("[udp_handler]ERROR writing to socket");
//             }
//             continue;
//         }

//         get_dns(hostname, dns_request);

//         if (send(sock, dns_request, MAX_DNS_REQUST_SIZE-1, 0) < 0) {
//             perror("[udp_handler]ERROR writing to socket");
//             break;
//         }
//     }
// }


int send_dns_request(unsigned char* dns_request, long len) {
    int sock_udp;
    int udp_status;
    int numbytes;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    socklen_t recv_size;

    memset(&hints, 0, sizeof(hints));
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((udp_status = getaddrinfo(dns_ip, DNS_PORT, &hints, &servinfo)) != 0) {
        error(gai_strerror(udp_status));
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock_udp = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }
        break;
    }

    if (p == NULL) {
        error("[send_dns]ERROR opening udp_socket");
    }

    printf("[send_dns]Socket open\n");

    printf("[send_dns]Sending Packet...\n");
    if ((sendto(sock_udp, dns_request, len, 0, p->ai_addr, p->ai_addrlen)) < 0) {
        error("[send_dns]ERROR sending dns_request");
    }
    printf("[send_dns]Done\n");

    freeaddrinfo(servinfo);

    recv_size = sizeof(their_addr);

    printf("[send_dns]Recieving Packet...\n");
    if ((numbytes = recvfrom(sock_udp, dns_request, MAX_DNS_REQUST_SIZE-1, 0, (struct sockaddr *)&their_addr, &recv_size)) < 0) {
        error("[send_dns]ERROR recive dns_request");
    }
    printf("[send_dns]Done\n");

    close(sock_udp);

    return numbytes;
}

void start_udp_server() {
    int udp_socket, new_udpsock;
    int status, numbytes;
    socklen_t clilen;
    struct addrinfo serv_addr, *tcp_info, *p;
    struct sockaddr_storage cli_addr;

    int list_len = count_lines(BLACKLIST_FILENAME);
    char black_list[list_len][MAXDATASIZE];

    memset(&serv_addr, 0, sizeof(serv_addr));

    load_blacklist(black_list);

    serv_addr.ai_family = AF_UNSPEC;
    serv_addr.ai_socktype = SOCK_DGRAM;
    serv_addr.ai_flags = AI_PASSIVE; // set my ip

    if ((status = getaddrinfo(NULL, PORTNUMB, &serv_addr, &tcp_info)) != 0) {
        error(gai_strerror(status));
    }

    for (p = tcp_info; p != NULL; p = p->ai_next) {
        if ((udp_socket = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) < -1) {
            continue;
        }

        if (bind(udp_socket, tcp_info->ai_addr, tcp_info->ai_addrlen) < 0) {
            close(udp_socket);
            perror("[udp_server] ERROR on binding");
            continue;
        }
        break;
    }

    if ( p == NULL ) {
        error("[udp_server] ERROR opening udp_socket");
    }

    freeaddrinfo(tcp_info);
    
    printf("[udp_server]: waiting for connections...\n");
    
    while (1) {
        char client_reqv[MAX_DNS_REQUST_SIZE];
        memset(&client_reqv, 0, sizeof(client_reqv));

        clilen = sizeof(cli_addr);
        numbytes = recvfrom(udp_socket, client_reqv, MAX_DNS_REQUST_SIZE-1, 0, (struct sockaddr*)&cli_addr, &clilen);


        if (numbytes < 0) {
            error("[udp_server] ERROR on rcv");
            // continue;
        }
        // printf("before - %d\n", strlen((char*)client_reqv));
        // client_reqv[numbytes-1] = '\0';
        // printf("after - %d\n", strlen((char*)client_reqv));
        // printf("bytes %d\n", numbytes);

        numbytes = udp_handler(udp_socket, client_reqv, black_list, list_len);

        if (numbytes < 0) {
            if (sendto(udp_socket, error_res, sizeof(error_res), 0, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
                error("[udp_server] ERROR on send");
            }
        } else {
            if (sendto(udp_socket, client_reqv, numbytes, 0, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
                error("[udp_server] ERROR on send");
            }
        }
        // if ((new_udpsock = socket(cli_addr.ss_family, SOCK_DGRAM, 0)) < 0) {
        //     perror("[udp_server] ERROR on client UDP socket");
        //     continue;
        // }

        // if (numbytes < 0) {
        //     error("[udp_server] ERROR on send");
        //     // continue;
        // }
        
        // if ((connect(new_udpsock, (struct sockaddr*)&cli_addr, clilen)) < 0) {
        //     perror("[udp_server] ERROR on connect client");
        //     continue;
        // }

        // if (!fork()) {
        //     close(udp_socket);

        //     udp_handler(new_udpsock, client_reqv, black_list, list_len);

        //     close(new_udpsock);
        //     exit(0);
        // }

        // close(new_udpsock);
    }
    
    close(udp_socket);
}


int main() {
    load_settings();
    start_udp_server();

    return 0;   
}
