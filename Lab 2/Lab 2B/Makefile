#!/bin/bash
#NAME: Rahul Sheth
#EMAIL: rahulssheth@g.ucla.edu
#ID: 304779669

CC=gcc
build: all  
all: 
	
	$(CC) -pthread -o lab2_list lab2_list.c SortedList.c -Wall -Wextra -lprofiler -g 
gdb:
	$(CC) -pthread -o lab2_list lab2_list.c SortedList.c -Wall -Wextra -g
lab2_list: lab2_list.c SortedList.c
	gcc -pthread -o lab2_list lab2_list.c SortedList.c -Wall -Wextra -lprofiler -g 
dist:  lab2_list
	make tests
	make graphs
	make profile
	-tar -czf  lab2b-304779669.tar.gz $(source) 
lab2b-304779669.tar.gz: $(source) 
	tar -czf  lab2a-304779669.tar.gz $(source)
	
source = Makefile  README lab2_list.gp sample.sh lab2b_list.csv SortedList.h SortedList.c lab2_list.c $(PNG) profile.out
PNG = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png 
 
clean:
	rm -rf lab2b-304779669.tar.gz
	rm -rf lab2_list
	rm -rf lab2.prof 
	
	
tests:
	
	-./sample.sh
graphs:
	-gnuplot lab2_list.gp
profile:
	CPUPROFILE=lab2.prof ./lab2_list --threads=12 --iterations=1000 --sync=s
	-pprof --text lab2_list lab2.prof >> profile.out
	-pprof --list=insertionSpin lab2_list lab2.prof >> profile.out
	
   
