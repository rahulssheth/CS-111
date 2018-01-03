#NAME: Rahul Sheth
#EMAIL: rahulssheth@g.ucla.edu
#ID: 304779669



 


all:
	gcc -o lab4c_tcp lab4c_tcp.c -Wall -Wextra -lm -lcrypto -lssl
	gcc -o lab4c_tls lab4c_tls.c -Wall -Wextra -lm -lcrypto -lssl

dist:
	tar -czf lab4c-304779669.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
clean:
	rm -rf lab4c_tcp
	rm -rf lab4c_tls
	rm -rf lab4c-304779669.tar.gz
