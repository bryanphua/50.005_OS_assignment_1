# 50.005 OS Programming Assignment 1

## Purpose of Program
Transverse a directed acyclic graph of user commands and executing them while maintaing both control and data dependencies(The child program can only start executing when its parent(s) have finished or only after recieving input from its parent(s)

## What the program does
It takes in a graph file name as an argument. It parses the graph file and converts it into a DAG data structure. It first runs programs that have no dependencies. When all of the parent program have finished, it raises the READY flag of the child program to indicate to the process manager that it is ready to run.

## How to compile
gcc -std=c99 -o processmgt processmgt.c

## How to run
./processmgt `<graph filename>`

## Programming Assignment 1
Author 1 :Shun Git Kwok

Author 2: Bryan Phua

Date :10 Mar 2017

