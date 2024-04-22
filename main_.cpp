#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <stack>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <queue>
#include <vector>
using namespace std;

int thisPort;
vector<int> otherPorts;
int replyCount = 0;
priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> requestQueue;

// Class representing a process
class ProcessUnit
{
public:
  ProcessUnit(int port, int peerCount, int index)
  {

    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    processIndex = index;
    for (int i = 0; i < peerCount; ++i)
    {
      processTimestamp = 0;
    }

    if (socketDesc < 0)
    {
      cerr << "ERROR opening socket\n";
      exit(1);
    }

    // Binding socket to the given port
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(port);

    if (bind(socketDesc, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
      perror("ERROR on binding \n");
      exit(1);
    }
  }
  
  // Function to check if it's allowed to enter the critical section
  bool canEnterCriticalSection()
  {
    if (replyCount == otherPorts.size() - 1 && requestQueue.top().second == thisPort)
      return true;
    return false;
  }

  // Function representing the critical section
  void static criticalSection()
  {
    cout << "ENTERING CRITICAL SECTION!" << endl;
    usleep(8000000); // sleep for 8 seconds
    for (int port : otherPorts)
    {
      if (port == thisPort)
        continue;
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cerr << "Error opening socket.\n";
        return;
      }
      struct sockaddr_in servAddr;
      bzero((char *)&servAddr, sizeof(servAddr));
      servAddr.sin_family = AF_INET;
      servAddr.sin_port = htons(port);
      servAddr.sin_addr.s_addr = INADDR_ANY;

      // Connect to the server
      if (connect(sockfd, (struct sockaddr *)&servAddr,
                  sizeof(servAddr)) < 0)
      {
        cerr << "Error connecting to the server.\n";
        return;
      }

      string timestampStr = "o\n" + to_string(0) + "\n" + to_string(thisPort) + "\n";

      send(sockfd, timestampStr.c_str(), timestampStr.size(), 0);
      // Close the socket
      cout << "Release message sent to " << port << endl;
      close(sockfd);
    }
  }

  // Function to start accepting requests
  void startAccept(int currentIndex)
  {

    clilen = sizeof(cliAddr);
    newsockfd = accept(socketDesc, (struct sockaddr *)&cliAddr, &clilen);
    if (newsockfd < 0)
    {
      cerr << "ERROR on accept\n";
      exit(1);
    }

    char buffer[1024];
    bzero(buffer, 1024);
    int n = recv(newsockfd, buffer, 1024, 0);
    if (n < 0)
    {
      cerr << "ERROR reading from socket\n";
      exit(1);
    }
    // emulate delay
    usleep(500000);

    int receivedTimestamp = -1;
    stringstream ss(buffer);
    string temp;
    int port;
    bool isReply = false, isReleased = false;
    while (getline(ss, temp, '\n'))
    {
      if (!temp.empty() && temp[0] == 'r') // reply
        isReply = true;
      else if (!temp.empty() && temp[0] == 'o') // open
        isReleased = true;
      else if (!temp.empty() && temp[0] == 'q') // request
        continue;
      else if (!temp.empty() && receivedTimestamp == -1)
        receivedTimestamp = (stoi(temp));
      else if (!temp.empty())
        port = stoi(temp);
    }

    // Compare the received timestamps with the current timestamps
    if (isReply)
    {
      if (port == thisPort)
      {
        replyCount += 1;
        cout << "Got a Reply!" << endl;
        if (canEnterCriticalSection())
        {
          replyCount = 0;
          requestQueue.pop();
          thread cs(criticalSection);
          cs.detach();
        }
      }
    }
    else if (isReleased)
    {
      requestQueue.pop();
      cout << "Got a Release message!" << endl;
      if (canEnterCriticalSection())
      {
        replyCount = 0;
        requestQueue.pop();
        thread cs(criticalSection);
        cs.detach();
      }
    }
    else
    {
      int currentTimestamp = processTimestamp;
      processTimestamp = max(currentTimestamp, receivedTimestamp) + 1;

      cout << "Got a Request from " << port << endl;
      requestQueue.push(pair<int, int>(receivedTimestamp, port));
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cerr << "Error opening socket.\n";
        return;
      }
      struct sockaddr_in servAddr;
      bzero((char *)&servAddr, sizeof(servAddr));
      servAddr.sin_family = AF_INET;
      servAddr.sin_port = htons(port);
      servAddr.sin_addr.s_addr = INADDR_ANY;

      // Connect to the server
      if (connect(sockfd, (struct sockaddr *)&servAddr,
                  sizeof(servAddr)) < 0)
      {
        cerr << "Error connecting to the server.\n";
        return;
      }

      string timestampStr = "r\n" + to_string(processTimestamp) + "\n" + to_string(port) + "\n";

      send(sockfd, timestampStr.c_str(), timestampStr.size(), 0);
      // Close the socket
      cout << " Reply sent to " << port << endl;
      close(sockfd);
    }
    close(newsockfd);
  }

  // Function to start listening for requests
  void startListening()
  {

    int index = processIndex;
    listen(socketDesc, 5);
    threads.push_back(thread([this, index]()
                             {
      
      while (true) {
        startAccept(index);
      } }));
  }

  // Function to start event generation
  void startEventGeneration()
  {
    int index = processIndex;
    threads.push_back(thread([this, index]()
                             {
      while (true) {
        int option;
        cout << "Enter 1 to request Critical Section, or \n2 to print current queue or \n3 to exit the code.\n";

        cin >> option;
        if (option == 1) {
          processTimestamp++;
          requestQueue.push(pair<int,int>(processTimestamp,thisPort));
          
          for(int port:otherPorts)
          {
            if(port==thisPort)
              continue;
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
              cerr << "Error opening socket.\n";
              return;
            }
            struct sockaddr_in servAddr;
            bzero((char *)&servAddr, sizeof(servAddr));
            servAddr.sin_family = AF_INET;
            servAddr.sin_port = htons(port);
            servAddr.sin_addr.s_addr = INADDR_ANY;

            // Connect to the server
            if (connect(sockfd, (struct sockaddr *)&servAddr,
                        sizeof(servAddr)) < 0) {
              cerr << "Error connecting to the server.\n";
            return;
            }

            string timestampStr="q\n"+to_string(processTimestamp)+"\n"+to_string(thisPort)+"\n";
            
            send(sockfd, timestampStr.c_str(), timestampStr.size(), 0);
            // Close the socket
            cout  << "Request sent to "<<port<<endl;
            close(sockfd);
            
          }
          
          
        } else if (option == 2) {
          vector<pair<int,int>>temp;
          while(!requestQueue.empty())
          {
            temp.push_back(requestQueue.top());
            cout<<requestQueue.top().first<<":"<<requestQueue.top().second<<endl;
            requestQueue.pop();
          }
          for(int i=0;i<temp.size();i++)
            requestQueue.push(temp[i]);
        } else if (option == 3) {
          
          exit(0);
        }
      } }));
  }

  // Function to wait for all threads to finish
  void waitForThreads()
  {
    for (auto &thread : threads)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
  }

private:
  int socketDesc, newsockfd;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in servAddr, cliAddr;
  atomic<int> processTimestamp;
  int processIndex;
  vector<thread> threads;
};

int main()
{

  cout << "Enter the listening port for this process: ";
  cin >> thisPort;
  int numPeers;
  cout << "Enter the number of peers(excluding this): ";
  cin >> numPeers;

  if (numPeers < 1)
  {
    cerr << "Number of peers must be at least 1.\n";
    return 1;
  }

  for (int i = 0; i < numPeers; ++i)
  {
    cout << "Enter the port for peer " << i + 1 << ": ";
    int t;
    cin >> t;
    otherPorts.push_back(t);
  }

  // Add the listening port to the vector
  otherPorts.push_back(thisPort);

  // Sort the vector
  sort(otherPorts.begin(), otherPorts.end());

  // Find the index of the listening port
  int currentIndex =
      find(otherPorts.begin(), otherPorts.end(), thisPort) -
      otherPorts.begin();

  ProcessUnit p(thisPort, numPeers, currentIndex);
  p.startListening();
  p.startEventGeneration();
  p.waitForThreads();

  return 0;
}

