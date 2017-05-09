#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTNUMB "50000"
#define BACKLOG 20
#define MAX_DNS_REQUST_SIZE 65000
#define MAXDATASIZE 100
#define MAXNAMESIZE 255
#define DNS_PORT "53"
#define QTYPE_A 1
#define QCLASS_IN 1
#define BLACKLIST_FILENAME "blacklist.ini"
#define SETTINGS_FILENAME "settings.ini"


int count_lines(unsigned char* name);
int check_hostname(unsigned char* hostname, char blacklist[][MAXDATASIZE], int len);
void load_settings();
void load_blacklist(char blacklist[][MAXDATASIZE]);
void error(const char *msg);
void add_dns_name(unsigned char* dns,unsigned char* host);
void udp_handler(int sock, char* hostname, char blacklist[][MAXDATASIZE], int len);
void send_dns_request(unsigned char* dns_request, unsigned long len);
void start_udp_server();
void get_dns(unsigned char* hostname, unsigned char* dns_request);
