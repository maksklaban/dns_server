all: dns_server main 
	gcc dns_server.o main.o && ./a.out

dns_server:
	gcc -c -Wall dns_server.c 

main:
	gcc -c -Wall main.c

clean:
	rm -rf *.o *.out
