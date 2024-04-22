# 20cs01065_20cs01002_CCproject

Lamport’s Distributed Mutual Exclusion Algorithm
Overview
This project demonstrates a distributed mutual exclusion system implemented using Lamport's logical clock algorithm. The system allows multiple processes running on different nodes to access a critical section in a coordinated manner, ensuring mutual exclusion and preventing race conditions.

Features
Distributed Architecture: The system consists of multiple processes running on different nodes, communicating over TCP/IP sockets.
Lamport's Logical Clock: Lamport's logical clock algorithm is used to establish a partial ordering of events in the distributed system, ensuring consistency.
CriticalSection Entry: Processes can request access to the critical section, and entry is granted based on predefined rules, ensuring fairness and mutual exclusion.
Dynamic Peer Configuration: The system allows dynamic addition and removal of peers, adapting to changes in the network topology.
Interactive Control: Users can interactively control the system by requesting critical section access, printing the current queue status, or exiting the system.

Usage
Compile the Code: Compile the C++ source files using a C++ compiler such as g++.
g++ -o distributed_mutex main.cpp -lpthread

Run the Executable: Execute the compiled program specifying the listening port for the current process and providing port numbers for the other peers in the distributed system.
./distributed_mutex


Interact with the System: Follow the on-screen prompts to interact with the system. You can request access to the critical section, print the current queue status, or exit the system.
