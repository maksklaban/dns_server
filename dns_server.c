#include "dns_server.h"

// store the values from conf file
char dns_ip[INET6_ADDRSTRLEN];
char error_res[MAX_ERR_RESP_LEN];

// counts the number of banned domain in blacklist file and return counter
int count_lines(char* name) {
    FILE* file;
    char buff[MAXDATASIZE];
    int line_count = 0;

    if ( (file = fopen(name, "r")) == NULL ) {
        error("[count_lines]ERROR opening blacklist file");
    }

    for ( ;(fgets(buff, MAXDATASIZE, file)) != NULL; ) {
        // check if commended
        if ( buff[0] == '/') {
            continue;
        }
        line_count++;
    }
    
    fclose(file);

    return line_count;
}

// take needle hostname and looking for match in the blacklist. 
// If match - return -1, else - return zero
int check_hostname(char* hostname, char blacklist[][MAXDATASIZE], int len) {
    for ( int i = 0; i < len; i++ ) {
        if ( strcmp(blacklist[i], hostname) == 0 ) {
            return -1;
        }
    }

    return 0;
}

// load settings from config file, store value in global variable
// return zero if success, if error - return negative value
int load_settings() {
    FILE* settings_file;
    char buff[MAX_ERR_RESP_LEN];
    // this flag check for all settings
    int flag = -2;

    if ((settings_file = fopen(SETTINGS_FILENAME, "r")) == NULL) {
        error("[config parse] Missing conf file");
    }

    for ( ; fgets(buff, MAX_ERR_RESP_LEN, settings_file) != NULL; ) {
        if (buff[0] == '/') {
            continue;
        }
        if (strcmp(buff, "[server]\n") == 0) {
            flag += 1;
            
            // if miss server sett value or commended, return error
            if (fgets(dns_ip, INET6_ADDRSTRLEN, settings_file) == NULL || dns_ip[0] == '\n' || dns_ip[0] == '/') {
                fprintf(stderr, "[config parse] Missing server setting\n");
                exit(1);
            }

            // delete new line character
            dns_ip[strlen(dns_ip) - 1] = 0;
        } else if (strcmp(buff, "[error_response]\n") == 0) {
            flag += 1;

            // same about error_res
            if (fgets(error_res, MAX_ERR_RESP_LEN, settings_file) == NULL || error_res[0] == '\n' || error_res[0] == '/') {
                fprintf(stderr, "[config parse] Missing error_res setting\n");
                exit(1);
            }
            error_res[strlen(error_res) - 1] = 0;
        }
    }

    fclose(settings_file);

    return flag;
}

// load all blacked domain from file into double-dimension array
void load_blacklist(char blacklist[][MAXDATASIZE]) {
    FILE* blacklst_file;
    char buffer[MAXDATASIZE];

    blacklst_file = fopen(BLACKLIST_FILENAME, "r");
    
    for (int i = 0; fgets(buffer, MAXDATASIZE, blacklst_file) != NULL; ) {
        if (buffer[0] == '/') {
            continue;
        }
        buffer[strlen(buffer) - 1] = 0;
        strcpy(blacklist[i], buffer);
        i++;
    }

    fclose(blacklst_file);
}

// error handler
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// change hostname format from 3www6google3com0 to www.google.com
// store new value in given var
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

// handle every incoming reqv from dns_cli. 
// check reqv for match in blacklist, send dns reqv to parent server(set in conf file)
// return dns response size if success, if cli reqv has banned domain - return -1
int udp_handler(int sock, char* cli_reqv, char blacklist[][MAXDATASIZE], int len) {
    // calc quantity of Resource Records(RR)
    int mult_rr;
    int status;
    char* host;
    char hostname[100];
    long dns_request_size;
    // used for calc quantity of RRs
    struct dns_header* head = NULL;


    head = (struct dns_header*)cli_reqv;
    host = (char*)&cli_reqv[sizeof(struct dns_header)];

    get_hostname(hostname, host);

    mult_rr = (ntohs(head->ancount) + ntohs(head->nscount) + ntohs(head->arcount));

    dns_request_size = (sizeof(struct dns_header) + (strlen((const char*)host) + 1)
    + (ntohs(head->qdcount) * sizeof(struct question)) + (mult_rr * sizeof(struct resource_rec)));

    if ((status = check_hostname(hostname, blacklist, len)) < 0 ) {
        return status;
    }

    return send_dns_request(cli_reqv, dns_request_size);
}

// open connection with parent dns server, send reqv and recieve response; handle all sockets error.
// return size of response
int send_dns_request(char* dns_request, long len) {
    int sock_udp;
    int udp_status;
    int numbytes;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    socklen_t recv_size;

    memset(&hints, 0, sizeof(hints));
    
    // no matter IPv4 or IPv6
    hints.ai_family = AF_UNSPEC;
    // UDP
    hints.ai_socktype = SOCK_DGRAM;

// take info about parent server
    if ((udp_status = getaddrinfo(dns_ip, DNS_PORT, &hints, &servinfo)) != 0) {
        error(gai_strerror(udp_status));
    }

// looking arount addrinfo linked list and open first availble socket
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

// we no longer need this list
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

// open UDP socket, handle all request from dns clients, load blacklist from file
// if success - send response to client, else - send current error message 
void start_udp_server() {
    int udp_socket;
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
// set my ip
    serv_addr.ai_flags = AI_PASSIVE;

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
        // store client reqv
        char client_reqv[MAX_DNS_REQUST_SIZE];

        memset(&client_reqv, 0, sizeof(client_reqv));

        clilen = sizeof(cli_addr);
        numbytes = recvfrom(udp_socket, client_reqv, MAX_DNS_REQUST_SIZE-1, 0, (struct sockaddr*)&cli_addr, &clilen);


        if (numbytes < 0) {
            error("[udp_server] ERROR on rcv");
        }

        numbytes = udp_handler(udp_socket, client_reqv, black_list, list_len);

        if (numbytes < 0) {
            // this doamin in blacklist. sends error message to client (err message set by conf file)
            if (sendto(udp_socket, error_res, sizeof(error_res), 0, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
                error("[udp_server] ERROR on send");
            }
        } else {
            if (sendto(udp_socket, client_reqv, numbytes, 0, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
                error("[udp_server] ERROR on send");
            }
        }
    }
    
    close(udp_socket);
}


int main() {
    // check if all settings available
    if (load_settings() < 0) {
        fprintf(stderr, "[config parse] Invalid config file format\n");
        exit(1);
    }
    start_udp_server();

    return 0;   
}
