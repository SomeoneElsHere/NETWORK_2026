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
    void CreateTracker() //Creates the socket profile at 9999
    {
        TrackerAddr.sin_family = AF_INET; //Family is IPv4
        TrackerAddr.sin_port = htons(9999); //Socket is 9999
        TrackerAddr.sin_addr.s_addr = INADDR_ANY;// IP addr is any ip addr
        
    }
    void Create() //Sets the Peersocket and creates the socket profile for tracker
    {
        PeerSocket = 0;
        PeerSocket = socket(AF_INET, SOCK_STREAM, 0); //Socket: same family, TCP, and 0 for automatic protocol
        if (PeerSocket < 0)
        {
            std::cout << "No sock juj: " << errno;
            exit(-2);
        }
        CreateTracker();
    }
    void Connect() //Connects to the socket.
    {
       //std::cout << PeerSocket;
       int q = connect(PeerSocket, (struct sockaddr*)&TrackerAddr, sizeof(TrackerAddr));  //Connect: FD for sender, casted sockaddr of reciver, size of sockaddr_in reciver
       if(q == -1)
       {
        std::cout << "Failure to connect: " << errno << std::endl << PeerSocket;
        exit(-1);
       }
    }
        
    void Send(std::string str)
    {
        const char *c = str.c_str(); //convert to byte
        send(PeerSocket,c, str.length(),0); //send bytes (fd, pointer, length, no flags)
        close(PeerSocket); //close fd. Remove this if you want to send something else.
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