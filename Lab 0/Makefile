#Name: Rahul Sheth
#Email: rahulssheth@g.ucla.edu
#ID: 304779669
#!/bin/bash

Input=test.txt
Output=test2.txt
Check=newTest.txt
error=$$?




CC=gcc
all: 
	$(CC) -o lab0 lab0.c -Wall -Wextra
lab0: lab0.c
	$(CC) -o $@ lab0.c -Wall -Wextra
dist: lab0  lab0-304779669.tar.gz 
source = Makefile lab0.c README backtrace.png breakpoint.png
lab0-304779669.tar.gz: $(source)
	tar -czf $@ $(source) 
clean:
	rm -rf lab0-304779669.tar.gz lab0
check: clean  dist
	echo "hello" > test.txt   
	 ./lab0 --input=$(Input) --output=$(Output); test $$? -eq 0 && echo "Success! Exited Well" || echo "Failure! Did not exit Well"

	cmp --silent $(Input) $(Output); test $$? -eq 0 || echo "Input and Output are not Identical" 
	 ./lab0 <$(Input) >$(Output); test $$? -eq 0 && echo "Success! Exited Well" || echo "Failure! Did not exit Well"
	cmp --silent $(Input) $(Output); test $$? -eq 0 || echo "Input and Output are not Identical"

	-./lab0 --segfault --catch; test $$? -eq 4 && echo "Success! Caught Segmentation Fault" || echo "Failure! Did not catch Segmentation Fault" 

	-./lab0 --segfault; test $$? -eq 139 && echo "Success! Segmentation Fault Occurred" || echo "Failure! Did not execute Segmentation Fault" 

	-./lab0 --input=NOTREAL.txt --output=MaybeItsReal.txt; test $$? -eq 2 && echo "Success! Input File not Found and exited" || echo "Failure! Did not Exit Well"

	-touch File.txt | chmod -w File.txt | ./lab0 --input=$(Input) --output=File.txt; test $$? -eq 3 && echo "Success! File should not be opened" || echo "Failure! File was opened" | rm -rf File.txt && rm -rf test.txt && rm -rf test2.txt  
