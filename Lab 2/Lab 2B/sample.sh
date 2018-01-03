#!/bin/bash


rm lab2b_list.csv

#Script for testing for part 1
#echo "PART 1 TESTING" 
for threads in 1 2 4 8 12 16 24; do
    #echo "lab2_list with mutex sync"
    ./lab2_list --threads=$threads --iterations=1000 --sync=m >> lab2b_list.csv
done

for threads in 1 2 4 8 12 16 24; do
    #echo "lab2_list with spin lock sync"
    ./lab2_list --threads=$threads --iterations=1000 --sync=s >> lab2b_list.csv

done

#Script for Testing list implementations:

#echo "TESTING MUTEX TIMING"

for threads in 1 2 4 8 12 16 24; do
    #echo "lab2_list with mutex sync"
    ./lab2_list --threads=$threads --iterations=1000 --sync=m >> lab2b_list.csv
done

for threads in 1 2 4 8 12 16 24; do
    #echo "lab2_list with spin lock sync"
    ./lab2_list --threads=$threads --iterations=1000 --sync=s >> lab2b_list.csv

done



#echo "LIST CHECK"

for threads in 1 4 8 12 16; do
    for iterations in 1 2 4 8 16; do
	./lab2_list --threads=$threads --iterations=$iterations --yield=id --lists=4 >> lab2b_list.csv
    done
done

#echo "LIST SYNC CHECK"

for threads in 1 4 8 12	16; do
    for iterations in 10 20 40 80; do
	#echo "List check sync=m"
        ./lab2_list --threads=$threads --iterations=$iterations	--yield=id --lists=4 --sync=m >>	lab2b_list.csv
    done
done

for threads in 1 4 8 12	16; do
    for iterations in 10 20 40 80; do
	#echo "List Check sync=s" 
       ./lab2_list --threads=$threads --iterations=$iterations	--yield=id --lists=4 --sync=s  >>	lab2b_list.csv
    done
done
    


#echo "LIST PROPER. TIME TO CHECK THE REST"

for threads in 1 2 4 8 12; do
    for lists in 1 4 8 16; do
	#echo "Test List with spin"
	./lab2_list --threads=$threads --lists=$lists --iterations=1000 --sync=s >> lab2b_list.csv
    done
done

for threads in 1 2 4 8 12; do
    for lists in 1 4 8 16; do
	#echo "Test List with mutex" 
	./lab2_list --threads=$threads --lists=$lists --iterations=1000	--sync=m >> lab2b_list.csv
    done
done	
