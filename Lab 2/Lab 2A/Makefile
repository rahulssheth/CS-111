#!/bin/bash
#NAME: Rahul Sheth
#EMAIL: rahulssheth@g.ucla.edu
#ID: 304779669

CC=gcc
build: all  
all: 
	$(CC) -pthread -o lab2_add lab2_add.c -Wall -Wextra
	$(CC) -pthread -o lab2_list lab2_list.c SortedList.c -Wall -Wextra 
lab2_add: lab2_add.c
	gcc -pthread -o lab2_add lab2_add.c -Wall -Wextra
lab2_list: lab2_list.c SortedList.c
	gcc -pthread -o lab2_list lab2_list.c SortedList.c -Wall -Wextra 
dist: lab2_add lab2_list
	make tests
	make graphs
	tar -czf  lab2a-304779669.tar.gz $(source) 
lab2a-304779669.tar.gz: $(source) 
	tar -czf  lab2a-304779669.tar.gz $(source)
	
source = Makefile lab2_add.c  README lab2_add.gp lab2_list.gp sample.sh lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png SortedList.h SortedList.c lab2_list.c

 
clean:
	rm -rf lab2a-304779669.tar.gz
	rm -rf lab2_list
	rm -rf lab2_add
	
tests:
	
	-./sample.sh
graphs:
	-./lab2_add.gp
	-./lab2_list.gp
	
