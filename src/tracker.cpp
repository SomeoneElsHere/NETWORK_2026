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
    int TrackerSocket = 0;
    // insert an empty string in the socket/ array if they leave pls :D - Natalie
    int PeerSockets[256];
    /*
    Peersockets is a vector with every single peer fd that is connected. For ease of tracking in the forloops,
    change the disconeccted client from a fd to 0
    */
    std::string  PeerInput[256];
    /*
    PeerInput is a vector with strings. The nth string is connected to the nth peer and so on. The nth peer will
    add onto the nth string. If the nth peer disconnects, put the string as empty so that a new peersocket can use it.
    Can change to a binary format if needed.
    */
    // Needlessly adding to the peerinput and peersocket without trying to use empty slots like shown will add more and more to the
    // vector untill it cant hold anymore. Doing it this way conserves memory.

    //TODO: Add a place to store input based on IP+MAC
    /*
    Peer input right now only saves the input temporarily based on buffer.
    We need to add a dictionary to check the IP+MAC addr and save the current file there.
    Program will change this dictionary when  recvBytes <= 0 (Around line 140)
    */

    void CreateTracker() // tracker profile (see peer)
    {
        
        TrackerAddr.sin_family = AF_INET; // tracker profile (see peer)
        TrackerAddr.sin_port = htons(10101);
        TrackerAddr.sin_addr.s_addr = INADDR_ANY;
        int o = 1;
        if(setsockopt(TrackerSocket,SOL_SOCKET,SO_REUSEADDR,&o,sizeof(int))<0)
        {
            std::cout << "Opt failure: " << errno << std::endl;
            exit(-1);
        }
        if(bind(TrackerSocket, (struct sockaddr *)&TrackerAddr, sizeof(TrackerAddr))<0)
        {
            std::cout << "Bind failure: " << errno << std::endl;
            close(TrackerSocket);
            exit(-1);
        }  
        
    }
    void Create()
    {
        TrackerSocket = socket(AF_INET, SOCK_STREAM, 0); // Create tracker socket to recieve (IPv4,TCP,automatic protocol)
        for(int i = 0; i< 256; i++)
        {
            PeerSockets[i] = 0;
            PeerInput[i] = "";
        }
        CreateTracker();
    }
    void Listen() 
    {
        
        
        int p = listen(TrackerSocket, 5); // listen for max 5 clients in que. This prepares the FD to accept after connect from peer
        
        if (p == -1)
        {
            std::cout << "Failure to listen: " << errno << std::endl;
            exit(-1);
        }
        while (true)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(TrackerSocket, &fds);

            //timeout (needed or select will stall)
            struct timeval tm;
            tm.tv_usec = 100;
            tm.tv_sec = 0;

            int sel = select(TrackerSocket+1,&fds, NULL,NULL,&tm); //put timeout in final para
            if(sel <= 0)
            {
                
                continue;
            }
           
            
            int index = 0;
            for (index = 0; index < 256; index++) // get first empty peersocket
            {
                
                if (PeerSockets[index] == 0)
                {
                    
                    break;
                }
            }
            if(index == 256)
            {
                
                continue;
            }
            
            int q = accept(TrackerSocket, 0, 0); // accept any addr, no restrictions (FD,no restrict, no restrict), q is peer fd
            
            if (q == -1)
            {
                std::cout << "Accept failure: " << errno << std::endl;
                exit(-2);
            }
            // i = 0; //debug for 1 peer
            PeerSockets[index] = q; // insert the ith peer fd at the last index.
            std::thread t = std::thread(&tracker::Accept,this,std::ref(index));
            
            t.detach();
            
        }
    }
    void Accept(int n)
    {
        char buf[1024];
        while (true)
        {
            int recvBytes = recv(PeerSockets[n], buf, sizeof(buf), 0);
            if (recvBytes <= 0) // if failure to recieve stop. Store data at buf
            {
                close(PeerSockets[n]);
                PeerSockets[n] = 0;
                //PeerInput[n] = "";
                //std::cout << "abort" << std::endl;
                //fflush(NULL);
                return;
            }
            
            std::string str(buf); // convert  buf to a string
            PeerInput[n] = PeerInput[n]+str;
            // output string (debug)
            std::cout << "["<< n << "]" << "["<< recvBytes << "]"<< str << " : " << PeerInput[n] << std::endl;
            
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