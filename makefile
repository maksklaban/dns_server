all: dns_server main 
	gcc dns_server.o main.o && ./a.out

dns_server:
	gcc -c dns_server.c 

main:
	gcc -c main.c

clean:
	rm -rf *.o *.out
