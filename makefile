# all: dns_server main 
# 	gcc dns_server.o main.o && ./a.out

# dns_server:
# 	gcc -c -Wall dns_server.c 

# main:
# 	gcc -c -Wall main.c


CC = gcc
CFLAGS = -Wall -std=gnu11
OUTFILE = dns_server
OBJS = dns_server.o
SRCS = dns_server.c

$(OUTFILE): $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTFILE) $(OBJS)
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)
clean:
	rm -rf *.o *.out
