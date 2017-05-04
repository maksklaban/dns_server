/*
** showip.c -- show IP addresses for a host given on the command line
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main()
{
    const char *buff[3] = {"Hellosssssssssssssssssssssssssssssssssssss", "World", "ololo"};
    // int i = 42;

    // if ( i = 43 == 1) {
    //     printf("ololo\n");
    // }
    // printf("%d\n", i);
    printf("%s\n", buff[0]);
    return 0;
}
