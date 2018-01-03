#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2b_1.png ... threads vs. throughput
#	lab2b_2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2b_3.png ... threads and iterations that run (protected) w/o failure
#	lab2b_4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "List-1: Throughput vs. number of threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

# grep out all threaded, un-protected, non-yield results
plot \
     "< grep 'list-none-m,' lab2_list.csv" using ($2):(1000000000/$7) \
	title 'mutex-sync' with linespoints lc rgb 'red', \
     "< grep 'list-none-s' lab2_list.csv" using ($2):(1000000000/$7) \
	title 'spin-sync' with linespoints lc rgb 'green'


set title "List-2: Time per mutex wait aand time per operation "

set xlabel "Threads"
set logscale x 2
set ylabel "Average wait-time for lock/operation"
set logscale y 10
set output 'lab2b_2.png'
plot \
     "< grep 'list-none-m,' lab2_list.csv" using ($2):($7) \
        title 'per op' with linespoints lc rgb 'red', \
     "< grep 'list-none-m' lab2_list.csv" using ($2):($8) \
        title 'wait-for-lock' with linespoints lc rgb 'green'

set title "List-3: Successful iterations vs. threads"

set xlabel "Threads"
set logscale x 2
set ylabel "Successful iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
     "< grep 'list-id-none,' lab2_list.csv" using ($2):($3) \
        title 'no sync' with points lc rgb 'red', \
     "< grep 'list-id-m,' lab2_list.csv" using ($2):($3) \
        title 'mutex sync ' with points lc rgb 'blue', \
     "< grep 'list-id-s' lab2_list.csv" using ($2):($3) \
        title 'spin sync' with points lc rgb 'green'
	
     


set title "List-4: Throughput vs. number of threads"

set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'

plot \
     "< grep 'list-none-m' lab2_list.csv | grep ',1000,1'" using ($2):(1000000000/$7) \
        title 'Mutex-sync, 1 list' with linespoint lc rgb 'red', \
     "< grep 'list-none-m' lab2_list.csv | grep ',1000,4'" using ($2):(1000000000/$7) \
        title 'mutex sync, 4 lists ' with linespoint lc rgb 'blue', \
     "< grep 'list-none-m' lab2_list.csv | grep ',1000,8'" using ($2):(1000000000/$7) \
        title 'mutex sync, 8 lists ' with linespoint lc rgb 'purple', \
     "< grep 'list-none-m' lab2_list.csv | grep ',1000,16'" using ($2):(100000000/$7) \
        title 'mutex sync, 16 lists' with linespoint lc rgb 'green'


set title "List-5: Throughtput vs. number of threads"

set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_5.png'

plot \
     "< grep 'list-none-s' lab2_list.csv | grep '1000,1'" using ($2):(1000000000/$7) \
        title 'Spin-sync, 1 list' with linespoint lc rgb 'red', \
     "< grep 'list-none-s' lab2_list.csv | grep '1000,4'" using ($2):(1000000000/$7) \
        title 'Spin sync, 4 lists ' with linespoint lc rgb 'blue', \
     "< grep 'list-none-s' lab2_list.csv | grep '1000,8'" using ($2):(1000000000/$7) \
        title 'Spin sync, 8 lists ' with linespoint lc rgb 'purple', \
     "< grep 'list-none-s' lab2_list.csv | grep '1000,16'" using ($2):(100000000/$7) \
        title 'Spin sync, 16 lists' with linespoint lc rgb 'green'