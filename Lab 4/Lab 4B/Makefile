#!/bin/bash
#NAME: Rahul Sheth
#EMAIL: rahulssheth@g.ucla.edu
#ID: 304779669



all:
	gcc -o lab4b lab4b.c -lmraa -lm -Wall -Wextra 
clean: 
	rm -rf lab4b
	rm -rf lab4b-304779669.tar.gz
dist:
	tar -czf lab4b-304779669.tar.gz lab4b.c Makefile README
check: 

	echo "SCALE=F\n PERIOD=10\n STOP\n START\n OFF\n" | ./lab4b --period=5 --scale=C --log=Logfile
	if [ -s "Logfile" ]; then
		echo "Logfile has data"
	        # do something as file has data
	else
		echo "Logfile has no data"

	fi
	rm -rf "Logfile"
 
