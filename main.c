#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>

#include "structs.c"
#include "dns_server.h"


int main() {
    load_settings();
    start_tcp_server();

    return 0;
}
