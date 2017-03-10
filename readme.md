 /* Programming Assignment 1
 * Author 1 :Shun Git Kwok
 * ID 1: 1001557
 * Author 2: Bryan Phua
 * ID 2: 1001550
 * Date :10 Mar 2017



#50.005 OS Programming Assignment 1

##Purpose of Program
Transverse a directed acyclic graph of user commands and executing them while maintaing both control and data dependencies(The child program can only start executing when its parent(s) have finished or only after recieving input from its parent(s)

##What the program does
It takes in a graph file name as an argument. It parses the graph file and converts it into a DAG data structure. It first runs programs that have no dependencies. When a program has finished, it raises the READY flag of its children programs to indicate to the process manager that they are ready to run.

##How to compile
gcc -std=c99 -o processmgt processmgt.c

##How to run
./processmgt `<graph filename>`


