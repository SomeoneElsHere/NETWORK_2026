#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>

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
        PeerSocket = socket(AF_INET, SOCK_STREAM, 0);
        CreateTracker();
    }
    void Connect()
    {
        sleep(.1);
        connect(PeerSocket, (struct sockaddr*)&TrackerAddr, sizeof(TrackerAddr));
    }
    void Send(std::string str)
    {
        char const *c = str.c_str();
        send(PeerSocket,c, strlen(c),0);
    }
};

int main()
{
    peer p;
    p.Connect();
    p.Create();
    p.Send("Hello!");
    return 0;
}