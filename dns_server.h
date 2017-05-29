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
#define MAX_ERR_RESP_LEN 50
#define BLACKLIST_FILENAME "blacklist.ini"
#define SETTINGS_FILENAME "settings.ini"
#define DNS_PORT "53"
#define QTYPE_A 1
#define QCLASS_IN 1

int count_lines(char* name);
int check_hostname(char* hostname, char blacklist[][MAXDATASIZE], int len);
int load_settings();
void load_blacklist(char blacklist[][MAXDATASIZE]);
void error(const char *msg);
void get_hostname(char* hostname, char* dns_request);
int udp_handler(int sock, char* cli_reqv, char blacklist[][MAXDATASIZE], int len);
int send_dns_request(char* dns_request, long len);
void start_udp_server();
