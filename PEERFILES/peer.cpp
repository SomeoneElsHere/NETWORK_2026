#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cerrno>

class peer
{
    public:
    sockaddr_in TrackerAddr;
    int PeerSocket;
    void CreateTracker()
    {
        TrackerAddr.sin_family = AF_INET;
        TrackerAddr.sin_port = htons(7879);
        TrackerAddr.sin_addr.s_addr = INADDR_ANY;
        
    }
    void Create()
    {
        PeerSocket = 0;
        PeerSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (PeerSocket < 0)
        {
            std::cout << "No sock juj: " << errno;
            exit(-2);
        }
        CreateTracker();
    }
    void Connect()
    {
       //std::cout << PeerSocket;
       int q = connect(PeerSocket, (struct sockaddr*)&TrackerAddr, sizeof(TrackerAddr));
       if(q == -1)
       {
        std::cout << "Failure to connect: " << errno << std::endl << PeerSocket;
        exit(-1);
       }
    }
        
    void Send(std::string str)
    {
        const char *c = str.c_str();
        send(PeerSocket,c, str.length(),0);
    }
};

int main()
{
    peer p;
    p.Create();
    p.Connect();
    p.Send("Hello!");
    return 0;
}