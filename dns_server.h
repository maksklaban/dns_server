#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>

#define PORTNUMB "50000"
#define BACKLOG 20
#define MAX_DNS_REQUST_SIZE 65536
#define MAXDATASIZE 100
#define MAXNAMESIZE 255
#define DNS_PORT "53"
#define QTYPE_A 1
#define QCLASS_IN 1
#define BLACKLIST_FILENAME "blacklist.ini"
#define SETTINGS_FILENAME "settings.ini"


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
