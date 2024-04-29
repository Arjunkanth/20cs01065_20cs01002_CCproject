#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cerrno>

using namespace std;

// Constants
const int MAX_BUFFER_SIZE = 1024;
const int SLEEP_TIME_CRITICAL_SECTION = 8; // seconds
const int SLEEP_TIME_EVENT_GENERATION = 0.5; // seconds

// Data structures
struct Message {
    char type; // 'r' for request, 'o' for open, 'q' for release
    int timestamp;
    int port;
};

// Process class
class Process {
private:
    int port;
    vector<int> otherPorts;
    atomic<int> replyCount;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> requestQueue;
    atomic<int> processTimestamp;
    mutex requestQueueMutex;

public:
    Process(int listeningPort, const vector<int>& peerPorts) : port(listeningPort), otherPorts(peerPorts), replyCount(0), processTimestamp(0) {}

    // Function to handle incoming requests
    void handleRequests() {
        int serverSocket;
        struct sockaddr_in servAddr, cliAddr;
        char buffer[MAX_BUFFER_SIZE];

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "Error opening socket." << endl;
            exit(1);
        }

        bzero((char *)&servAddr, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = INADDR_ANY;
        servAddr.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            cerr << "Error on binding." << endl;
            exit(1);
        }

        listen(serverSocket, 5);

        socklen_t cliLen = sizeof(cliAddr);
        int clientSocket;
        while (true) {
            clientSocket = accept(serverSocket, (struct sockaddr *)&cliAddr, &cliLen);
            if (clientSocket < 0) {
                cerr << "Error on accept." << endl;
                exit(1);
            }

            bzero(buffer, MAX_BUFFER_SIZE);
            int n = read(clientSocket, buffer, MAX_BUFFER_SIZE - 1);
            if (n < 0) {
                cerr << "Error reading from socket." << endl;
                exit(1);
            }

            Message message;
            sscanf(buffer, "%c %d %d", &message.type, &message.timestamp, &message.port);

            // Process the received message
            handleMessage(message);

            close(clientSocket);
        }

        close(serverSocket);
    }

    // Function to enter the critical section
    void enterCriticalSection() {
        cout << "Entering critical section!" << endl;
        usleep(SLEEP_TIME_CRITICAL_SECTION * 1000000); // Sleep for 8 seconds
        replyCount = 0;
        requestQueue.pop();
        // Send release messages to other processes
        for (int otherPort : otherPorts) {
            if (otherPort == port) continue;
            sendMessage('q', 0, otherPort);
        }
    }

    // Function to handle incoming messages
    void handleMessage(const Message& message) {
        requestQueueMutex.lock();
        if (message.type == 'r') { // Request message
            requestQueue.push({message.timestamp, message.port});
            sendMessage('o', 0, message.port); // Send open message as acknowledgment
        } else if (message.type == 'o') { // Open message (acknowledgment)
            replyCount++;
            if (replyCount == otherPorts.size() - 1 && requestQueue.top().second == port) {
                thread csThread(&Process::enterCriticalSection, this);
                csThread.detach();
            }
        } else if (message.type == 'q') { // Release message
            requestQueue.pop();
            if (replyCount == otherPorts.size() - 1 && !requestQueue.empty() && requestQueue.top().second == port) {
                thread csThread(&Process::enterCriticalSection, this);
                csThread.detach();
            }
        }
        requestQueueMutex.unlock();
    }

    // Function to send a message to a specific port
    void sendMessage(char type, int timestamp, int destinationPort) {
        int clientSocket;
        struct sockaddr_in servAddr;
        char buffer[MAX_BUFFER_SIZE];

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            cerr << "Error opening socket." << endl;
            exit(1);
        }

        bzero((char *)&servAddr, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(destinationPort);
        servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(clientSocket, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            cerr << "Error connecting to the server." << endl;
            exit(1);
        }

        sprintf(buffer, "%c %d %d", type, timestamp, port);
        int n = write(clientSocket, buffer, strlen(buffer));
        if (n < 0) {
            cerr << "Error writing to socket." << endl;
            exit(1);
        }

        close(clientSocket);
    }

    // Function to start accepting requests in a separate thread
    void startAcceptingRequests() {
        thread acceptThread(&Process::handleRequests, this);
        acceptThread.detach();
    }

    // Function to generate events (requesting critical section)
    void generateEvents() {
        while (true) {
            int option;
            cout << "Enter 1 to request Critical Section, 2 to print current queue, or 3 to exit: ";
            cin >> option;
            if (option == 1) {
                processTimestamp++;
                requestQueueMutex.lock();
                requestQueue.push({processTimestamp, port});
                requestQueueMutex.unlock();
                // Send request messages to other processes
                for (int otherPort : otherPorts) {
                    if (otherPort == port) continue;
                    sendMessage('r', processTimestamp, otherPort);
                }
            } else if (option == 2) {
                // Print current queue
                printQueue();
            } else if (option == 3) {
                exit(0);
            }
            usleep(SLEEP_TIME_EVENT_GENERATION * 1000000);
        }
    }

    // Function to print the current queue
    void printQueue() {
        requestQueueMutex.lock();
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> tempQueue = requestQueue;
        requestQueueMutex.unlock();
        while (!tempQueue.empty()) {
            cout << "Timestamp: " << tempQueue.top().first << ", Port: " << tempQueue.top().second << endl;
            tempQueue.pop();
        }
    }

    // Function to start event generation in a separate thread
    void startGeneratingEvents() {
        thread eventThread(&Process::generateEvents, this);
        eventThread.detach();
    }

    // Function to start the process
    void start() {
        startAcceptingRequests();
        startGeneratingEvents();
    }
};

int main() {
    int listeningPort, numPeers;
    cout << "Enter the listening port for this process: ";
    cin >> listeningPort;
    cout << "Enter the number of peers (excluding this process): ";
    cin >> numPeers;

    if (numPeers < 1) {
        cerr << "Number of peers must be at least 1.\n";
        return 1;
    }

    vector<int> peerPorts(numPeers);
    for (int i = 0; i < numPeers; ++i) {
        cout << "Enter the port for peer " << i + 1 << ": ";
        cin >> peerPorts[i];
    }

    // Create and start the process
    Process process(listeningPort, peerPorts);
    process.start();

    // Keep the main thread alive
    while (true) {
        // Implementation omitted for brevity
    }

    return 0;
}
