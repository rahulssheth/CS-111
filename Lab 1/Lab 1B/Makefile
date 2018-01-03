#NAME: Rahul Sheth
#EMAIL: rahulssheth@g.ucla.edu
#ID: 304779669
all:
	gcc -o lab1b-server lab1b-server.c -lpthread -lmcrypt
	gcc -o lab1b-client lab1b-client.c -lpthread -lmcrypt

clean:
	rm -rf lab1b-304779669.tar.gz lab1b-client lab1b-server

dist:
	tar -czf lab1b-304779669.tar.gz Makefile README lab1b-server.c lab1b-client.c my.key
