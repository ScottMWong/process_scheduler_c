Assignment for Computer Systems (COMP30023), University of Melbourne, Semester 1 2021
Created March 2021
Relevant documentation below:

Single processor scheduler
Processes will be allocated CPU time according to shortest-time-remaining algorithm, a preemptive
scheduling algorithm. When a new process arrives, it is scheduled if there is no other process running.
If, when process j arrives, there is a process i running, then i is postponed and added to a queue if and
only if j has a shorter execution time than the remaining time of i. Process i is resumed where it left
off, if it is the process with the shortest remaining time left among all other processes in the queue (i.e.,
including those that may have arrived after j). Process ids are used to break ties, giving preference to
processes with smaller ids. Since there is only one CPU, you can ignore the information on whether a
process is parallelisable or not.
The simulation should terminate once all processes have run to completion.

Two processor scheduler
In this section you are asked to extend your scheduler to work with 2 processors: processors numbered 0
and 1. This will simulate (1) a situation where processes can run truly in parallel and (2) a process that
can run faster with more computational resources (e.g., by creating child/sub processes).
If the process that arrives is not parallelisable, it is assigned to the queue of the CPU with the smallest
amount of remaining execution time for all processes and subprocesses (defined below) allocated to it,
using CPU id to break ties, giving preference to CPU 0. After that the simulation for this process
behaves same as in Section 1. In this project, once a process is assigned to a CPU it cannot be moved
to another CPU. (Think of why it may not be a good idea to migrate processes between CPUs.)
If arriving process with id i and execution time x is parallelisable, two subprocesses are created, i.0
and i.1, each with execution time dx=2e + 1. The extra time simulates the cost of synchronisation. For
example, a parallelisable process that runs on a single CPU in 6 seconds, can finish within 4 seconds if
both CPUs are idle when it arrives.

Once subprocesses are created, subprocess i.0 is added to the queue of CPU 0 and i.1 is added to CPU 1.
After that they are treated as regular processes when scheduling them on each CPU. A parallelisable
process is considered finished when both of its subprocesses have finished.

N processor scheduler
In this task we generalise the 2 processor setting to N >= 3 processors. Similar to above, a nonparallelisable
process is assigned to a CPU that has the shortest remaining time to complete all processes
and subprocesses assigned to it so far.
A parallelisable process is split into k <= N subproceses, where k is the largest value for which x=k >= 1.
Each subprocess is then assigned execution time of dx=ke + 1. Subprocesses follow a similar naming
convention as above: a process with id i is split into subprocesses with id’s i.0, i.1, : : :, i.k0, where
k0 = k 􀀀 1. Subprocesses are then assigned to k processors with shortest time left to complete such
that subprocess i:0 is assigned to the CPU that can finish its processes the fastest, i:1 is assigned to the
second fastest processor and so on. Use CPU ids to break ties, giving preference to smaller processor
numbers.
Example: consider a scenario with 4 CPUs, each with one process left to complete; processes on CPU
0,1,2,3 have remaining time of 30,20,20,10, respectively. Then for a process i with execution time of 2,
k is set to 2. Its subprocess i:0 and i:1 will be assigned to CPU 3 and 1 respectively.
Once a process or subprocess is assigned to a CPU it cannot migrate to another CPU. A parallelisable
process is considered finished when all of its subprocesses have finished.

Program Specification
Your program must be called allocate and take the following command line arguments. The arguments
can be passed in any order but you can assume that they will be passed exactly once.
-f filename will specify the name of the file describing the processes.
-p processors where processors is one of {1,2,N}, N <= 1024.
The filename contains the processes to be executed and has the following format. Each line of the file
corresponds to a process. The first line refers to the first process that needs to be executed, and the last
line refers to the last process to be executed. Each line consists of a space-separated tuple (time-arrived,
process-id, execution-time, parallelisable). You can assume that the file will be sorted by time-arrived
which is an integer in [0; 232) indicating seconds; all process-ids will be distinct integers in the domain
of [0; 232) and the first process will always have time-arrived set to 0; execution-time will be an integer
in [1; 232) indicating seconds; parallelisable is either n or p. If it is p, then the corresponding process is
parallelisable; if it is n, it is not. You can ignore n/p when -p is 1. More than one process can arrive at
the same time.
Example: ./allocate -f processes.txt -p 1.
The allocation program is required to simulate execution of processes in the file processes.txt on a
single CPU.
Given processes.txt with the following information:
0 4 30 n
3 2 40 n
5 1 20 n
20 3 30 n
The program should simulate execution of 4 processes where process 4 arrives at time 0, needs 30 seconds
running time to finish; process 2 arrives at time 3 and needs 40 seconds of time to complete.
Each line (including the last) will be terminated with a LF (ASCII 0x0a) control character.
You can read the whole file before starting the simulation or read one line at a time. We will not give
malformed input (e.g., negative number of processors after -p or more than 4 columns in the process
description file)

Expected Output
In order for us to verify that your code meets the above specification, it should print to standard output
(stderr will be ignored) information regarding the states of the system and statistics of its performance.
All times are to be printed in seconds.

Execution transcript
For the following events the code should print out a line in the following format:
• When a (sub)process starts and every time it resumes its execution:
<current-time>,RUNNING,pid=<process-id>,remaining_time=<T>,cpu=<cpu-id>\n
where:
‘current-time’ refers to the time at which CPU is given to a process;
‘process-id’ refers to the id of the process that is about to be run;
‘T’ refers to the remaining execution time for this process;
‘cpu-id’ refers to the processor where the process is scheduled on. It can be 0; 1; 2 : : : ;N - 1 when
-p N for N >= 1;
Sample output could be:
20,RUNNING,pid=15,remaining_time=10,cpu=0
• Every time a process finishes:
<current-time>,FINISHED,pid=<process-id>,proc_remaining=<num-proc-left>\n
where:
– ‘<current-time>’ is as above for the RUNNING event;
– ‘process-id’ refers to the id of the process that has just been completed;
– ‘num-proc-left’ refers to the number of processes that are waiting to be executed over all
processors (i.e., those that have already arrived but not yet completed, including those that
have unfinished subprocesses).
If there is more than one event to be printed at the same time: print FINISHED before RUNNING and print
events for smaller CPU ids first.
Example: Consider the last remaining process which has 10 seconds left to completion. Then the following
lines may be printed:
20,RUNNING,pid=15,remaining_time=10,cpu=1
30,FINISHED,pid=15,proc_remaining=0

Performance statistics
When the simulation is completed, 3 lines with the following performance statistics about your simulation
performance should be printed:

• Turnaround time: average time (in seconds, rounded up to an integer) between the time when the
process completed and when it arrived;
• Time overhead: maximum and average time overhead when running a process, both rounded to the
first two decimal points, where overhead is defined as the turnaround time of the process divided
by its total execution time (i.e., the one specified in the process description file).
• Makespan: the time in seconds when your simulation ended.
Example: For the invocation with arguments -p 1 and the processes file as described above, the simulation
would print
Turnaround time 62
Time overhead 2.93 1.9
Makespan 120
