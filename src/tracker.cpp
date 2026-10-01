#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cerrno>
#include <functional>

// TOTAL TODOS
// Line 21: Verify peersockets and peerinput has proper string data
// Line 52: Add Multithreading to process multiple peers at the same time

class tracker
{
public:
    sockaddr_in TrackerAddr;
    int TrackerSocket;
    // insert an empty string in the socket/ array if they leave pls :D - Natalie
    // TODO- Verify peersockets and peerinput is working
    std::vector<int> PeerSockets;
    /*
    Peersockets is a vector with every single peer fd that is connected. For ease of tracking in the forloops,
    change the disconeccted client from a fd to 0
    */
    std::vector<std::string> PeerInput;
    /*
    PeerInput is a vector with strings. The nth string is connected to the nth peer and so on. The nth peer will
    add onto the nth string. If the nth peer disconnects, put the string as empty so that a new peersocket can use it.
    Can change to a binary format if needed.
    */
    // Needlessly adding to the peerinput and peersocket without trying to use empty slots like shown will add more and more to the
    // vector untill it cant hold anymore. Doing it this way conserves memory.

    void CreateTracker() // tracker profile (see peer)
    {
        TrackerAddr.sin_family = AF_INET; // tracker profile (see peer)
        TrackerAddr.sin_port = htons(9999);
        TrackerAddr.sin_addr.s_addr = INADDR_ANY;
        bind(TrackerSocket, (struct sockaddr *)&TrackerAddr, sizeof(TrackerAddr)); // only the program that has the physical socket binds
    }
    void Create()
    {
        TrackerSocket = socket(AF_INET, SOCK_STREAM, 0); // Create tracker socket to recieve (IPv4,TCP,automatic protocol)

        CreateTracker();
    }
    void Listen() // TODO make this a loop and use multithreading to create threads of method Accept
    {
        int p = listen(TrackerSocket, 5); // listen for max 5 clients in que. This prepares the FD to accept after connect from peer
        if (p == -1)
        {
            std::cout << "Failure to listen: " << errno;
            exit(-1);
        }
        while (true)
        {
            int index = 0;
            for (index = 0; index < 0; index++) // get first empty peersocket and set it to i
            {
                try
                {
                    if (PeerSockets.at(index) == 0)
                    {
                        PeerSockets.erase(PeerSockets.begin() + index);
                        break;
                    }
                }
                catch (const std::exception e)
                {
                    break;
                }
            }
            int q = accept(TrackerSocket, nullptr, nullptr); // accept any addr, no restrictions (FD,no restrict, no restrict), q is peer fd
            if (q == -1)
            {
                std::cout << "Accept failure: " << errno;
                exit(-2);
            }
            // i = 0; //debug for 1 peer
            PeerSockets.insert(PeerSockets.begin() + index, q); // insert the ith peer fd at the last index.
            std::thread t = std::thread(&tracker::Accept,this,std::ref(index));
            t.detach();
        }
    }
    void Accept(int n)
    {
        char buf[1024];
        while (true)
        {
            if (recv(PeerSockets.at(n), buf, sizeof(buf), 0) <= 0) // if failure to recieve stop. Store data at buf
            {
                close(PeerSockets.at(n));
                PeerSockets.erase(PeerSockets.begin()+n);
                PeerSockets.insert(PeerSockets.begin()+n,0);
                bind(TrackerSocket, (struct sockaddr *)&TrackerAddr, sizeof(TrackerAddr)); 
                return;
            }
            std::string str(buf); // convert  buf to a string
            try                   // try to add string from peer i to a string vector Tracker saves at Peerinput
            {
                PeerInput.at(n); // check if the nth peer string exists
                PeerInput.erase(PeerInput.begin() + n);
                PeerInput.insert(PeerInput.begin() + n, str);
            }
            catch (const std::exception &e)
            {
                try
                {
                    PeerInput.insert(PeerInput.begin() + n, str); // if nplace in peerinput does not exist
                }
                catch (const std::exception &e)
                {
                    PeerInput.insert(PeerInput.begin() + n - 1, str); // if weird stuff happens (this shouldnt call)
                }
            }
            // output string
            std::cout << str << std::endl;
        }
    }
};

int main()
{
    tracker t;
    t.Create(); // create the tracker socket
    t.Listen(); // listen for the peer connection and recieve data
    // t.Accept(0); debug
    return 0;
}