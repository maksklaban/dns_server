#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>

#include "structs.c"

#define PORTNUMB "50000"
#define MAX_DNS_REQUST_SIZE 10000
#define MAXDATASIZE 100
#define MAX_ERR_RESP_LEN 50
#define BLACKLIST_FILENAME "blacklist.ini"
#define SETTINGS_FILENAME "settings.ini"
#define DNS_PORT "53"

// counts the number of banned domain in blacklist file and return counter
int count_lines(char* name);

// take needle hostname and looking for match in the blacklist. 
// If match - return -1, else - return zero
int check_hostname(char* hostname, char blacklist[][MAXDATASIZE], int len);

// load settings from config file, store value in global variable
// return zero if success, if error - return negative value
int load_settings();

// load all blacked domain from file into double-dimension array
void load_blacklist(char blacklist[][MAXDATASIZE]);

// error handler
void error(const char *msg);

// change hostname format from 3www6google3com0 to www.google.com
// store new value in given var
void get_hostname(char* hostname, char* dns_request);

// handle every incoming reqv from dns_cli. 
// check reqv for match in blacklist, send dns reqv to parent server(set in conf file)
// return dns response size if success, if cli reqv has banned domain - return -1
int udp_handler(int sock, char* cli_reqv, char blacklist[][MAXDATASIZE], int len);

// open connection with parent dns server, send reqv and recieve response; handle all sockets error.
// return size of response
int send_dns_request(char* dns_request, long len);

// open UDP socket, handle all request from dns clients, load blacklist from file
// if success - send response to client, else - send current error message 
void start_udp_server();
