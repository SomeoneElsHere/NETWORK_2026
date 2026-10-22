#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cerrno>
#include <string>

/**
 * A peer client
 */
class peer
{
    public:
    sockaddr_in TrackerAddr;
    int PeerSocket;

    /**
     * Creates the socket profile at 9999
     */
    void CreateTracker()
    {
        TrackerAddr.sin_family = AF_INET; //Family is IPv4
        TrackerAddr.sin_port = htons(10101); //Socket is 9999
        TrackerAddr.sin_addr.s_addr = INADDR_ANY;// IP addr is any ip addr
    }

    /**
     * Sets the PeerSocket and creates the socket profile for tracker
     */
    void Create()
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

    /**
     * Connects to the socket
     */
    void Connect()
    {
        //std::cout << PeerSocket;
        int q = connect(PeerSocket, (struct sockaddr*)&TrackerAddr, sizeof(TrackerAddr));  //Connect: FD for sender, casted sockaddr of reciver, size of sockaddr_in reciver
        if(q == -1)
        {
            std::cout << "Failure to connect: " << errno << std::endl << PeerSocket;
        }
    }
        
    /**
     * Sends a string message through the socket
     * @param str The message to be sent
     * @param len The length of the message in characters
     */
    void Send(std::string str, int len)
    {
        std::string s = (std::to_string(len) +":"+ str);
        const char *c = s.c_str(); //convert to byte
        //std::cout << "sent message" << std::endl;
        send(PeerSocket,c, s.length(),0); //send bytes (fd, pointer, length, no flags)
    }
};

int main()
{
    peer p;
    p.Create();
    p.Connect();
    //p.Send(" ",1);
    p.Send("Hiy",3);
    close(p.PeerSocket); //close fd. Remove this if you want to send something else.
    
    peer q;
    q.Create();
    q.Connect();
    q.Send("abc",3);
    q.Send("def",3);
    close(q.PeerSocket); //close fd. Remove this if you want to send something else.
    
    return 0;
}