# 20cs01065_20cs01002_CCproject

Lamport’s Distributed Mutual Exclusion Algorithm

## Distributed Mutual Exclusion Algorithm

This code implements a distributed mutual exclusion algorithm using socket programming in C++. The algorithm allows multiple processes running on different ports to coordinate access to a critical section.

### Features

- **Distributed Coordination**: Processes communicate over sockets to coordinate access to the critical section.
- **Request-Reply Mechanism**: Processes send request and reply messages to acquire and release access to the critical section.
- **Priority Queue**: A priority queue is used to manage incoming requests, ensuring fairness in granting access.

### Implementation Overview

The code consists of a `ProcessUnit` class representing each process. Here's an overview of the key components:

1. **Initialization**: Each process is initialized with a listening port and a list of ports of other processes.

2. **Socket Communication**: Processes communicate using TCP sockets. They send messages of different types (`'r'` for request, `'o'` for open acknowledgment, `'q'` for release) to coordinate access.

3. **Request Handling**: Processes listen for incoming messages and handle them appropriately. When receiving a request, a process updates its priority queue with the request and sends acknowledgment to the requester.

4. **CriticalSection Entry**: When a process has received acknowledgment from all other processes for its request and its request is at the top of the priority queue, it enters the critical section. After executing the critical section, it sends release messages to other processes.

5. **Event Generation**: Processes periodically generate events, such as requesting access to the critical section or printing the current queue.

6. **Concurrency**: Threads are used to handle socket communication and event generation concurrently, allowing processes to perform multiple tasks simultaneously.

### Usage

1. **Compilation**: Compile the code using a C++ compiler, such as `g++`.

2. **Execution**: Execute the compiled binary, providing the listening port for each process and the ports of other processes as input.

3. **Interaction**: During execution, processes can request access to the critical section, print the current queue, or exit the program.

### Example Output

```
Enter the listening port for this process: 5000
Enter the number of peers (excluding this process): 2
Enter the port for peer 1: 5001
Enter the port for peer 2: 5002

Enter 1 to request Critical Section, 2 to print current queue, or 3 to exit: 1
Entering critical section!

Got a Request from 5001
Reply sent to 5001

Got a Reply from 5002
Entering critical section!

Got a Release message from 5000

Enter 1 to request Critical Section, 2 to print current queue, or 3 to exit: 2
Timestamp: 1, Port: 5001
Timestamp: 2, Port: 5002
Timestamp: 3, Port: 5000

Enter 1 to request Critical Section, 2 to print current queue, or 3 to exit: 3
```

### Dependencies

- C++ Compiler (e.g., `g++`)
- Standard C++ Library
