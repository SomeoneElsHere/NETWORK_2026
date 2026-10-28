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
#include <map>
#include <arpa/inet.h>
#include "lib/util.hpp"
// TOTAL TODOS
// Line 21: Verify peersockets and peerinput has proper string data
// Line 52: Add Multithreading to process multiple peers at the same time

class tracker
{
public:
    sockaddr_in TrackerAddr;
    int TrackerSocket = 0;
    // insert an empty string in the socket/ array if they leave pls :D - Natalie
    

    std::map<std::string, std::string> PeerInputStore = std::map<std::string, std::string>();
    /*
    PeerInputStore stores the data of the file per IP with a IP->File lookup
    */
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
    
    struct acceptArgs
    {
        std::string IPADDR;
        int index;
    };

    void CreateTracker(const char* port) // tracker profile (see peer)
    {
        // while going through the connection list
        struct addrinfo* addrList = make_connection_list(port); 
        for(struct addrinfo* addr = addrList; addr; addr = addrList->ai_next)
        {
            // try the socket
            TrackerSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if(TrackerSocket < 0) continue;
          
            // get around the "bind address already in use"
            int optval = 1;
            setsockopt(TrackerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
            setsockopt(TrackerSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

            // try the bind
            int bind_status = bind(TrackerSocket, addr->ai_addr, addr->ai_addrlen);
            if (bind_status >= 0) // success, save and stop going through the list
            { 
                freeaddrinfo(addrList);
                return;
            }
            close(TrackerSocket); // failure, clean the socket and continue
        } // end for(...)
        fprintf(stderr, "ERROR %d - could not create and bind to any network sockets -> %s:%d\n", errno, __FILE__, __LINE__);
    }

    void Create()
    {
        for(int i = 0; i< 256; i++)
        {
            PeerSockets[i] = 0;
            PeerInput[i] = "";
        }
        CreateTracker("10101"); // Create tracker socket to recieve (IPv4,TCP,automatic protocol)
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
            
            
            sockaddr_in sockaddr_client; //where the addr of an IP from a client is stored
            sockaddr_client.sin_family = AF_INET;
            in_addr addr;
            addr.s_addr = INADDR_ANY;
            sockaddr_client.sin_addr = addr;
            sockaddr_client.sin_port = TrackerAddr.sin_port;
            socklen_t sockklen;
            sockklen = 4;
            
            

            int q = accept(TrackerSocket,(struct sockaddr*)&sockaddr_client, &sockklen); // accept any addr,  q is peer fd
            if (q == -1)
            {
                std::cout << "Accept failure: " << errno << std::endl;
                exit(-2);
            }

            
            sockaddr_in IPV4 = (sockaddr_in)sockaddr_client;
            char ipaddr[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &(IPV4.sin_addr), ipaddr, INET_ADDRSTRLEN ); //get ip as char*
            
            std::string Ipaddrstring(ipaddr); 
            try
            {
                PeerInputStore.at(Ipaddrstring); //does the IP already exist?
                
            }
            catch(...)
            {
                PeerInputStore[Ipaddrstring] = ""; //if not, create new handle for IP
            }
            

            
            // i = 0; //debug for 1 peer
            PeerSockets[index] = q; // insert the ith peer fd at the last index.
            acceptArgs arg;
            arg.index = index;
            arg.IPADDR = Ipaddrstring;
            std::thread t = std::thread(&tracker::Accept,this,arg);
            
            t.detach(); //let it do its thang
            
        }
    }
    void Accept(acceptArgs arg)
    {
        int n = arg.index;
        char buf[1024];
        while (true)
        {
            int recvBytes = recv(PeerSockets[n], buf, sizeof(buf), 0);
            if (recvBytes <= 0) // if failure to recieve stop. Store data at buf
            {
                close(PeerSockets[n]);
                
                PeerInputStore[arg.IPADDR] = PeerInputStore[arg.IPADDR] + PeerInput[n];
                PeerInput[n] = "";
                return;
            }
            
            std::string str(buf); // convert  buf to a string
            PeerInput[n] = PeerInput[n]+str;
            // output string (debug)
            std::cout << "["<< n << "]" << "["<< recvBytes << "]"<< str << " : " << PeerInputStore[arg.IPADDR] << std::endl;
            
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
