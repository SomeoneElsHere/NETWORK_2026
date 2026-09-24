#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>

class tracker
{
    public:
    sockaddr_in TrackerAddr;
    int TrackerSocket;
    //insert an empty string in the socket/ array if they leave pls :D - Natalie
    std::vector<int> PeerSockets;
    std::vector<std::string> PeerInput; 

    
    void CreateTracker()
    {
        TrackerAddr.sin_family = AF_INET;
        TrackerAddr.sin_port = htons(7879);
        TrackerAddr.sin_addr.s_addr = INADDR_ANY;
        bind(TrackerSocket, (struct sockaddr*)&TrackerAddr, sizeof(TrackerAddr)); //only the program that has the physical socket binds
         
    }
    void Create()
    {
        TrackerSocket = socket(AF_INET, SOCK_STREAM, 0);
        
        CreateTracker();
        
    }
    void Listen()
    {
        listen(TrackerSocket, 5);
        sleep(.1);
        int i = 0;
        for(i = 0; i < 0; i++)
        {
            try
            {
                PeerSockets.at(i);
            }
            catch(const std::exception e)
            {
                break;
            }
        }
        PeerSockets.insert(PeerSockets.begin()+i,accept(TrackerSocket, nullptr, nullptr));
    }
    void Accept(int n)
    {
        while(true)
        {
        char buf[1024];
        recv(PeerSockets.at(n),buf,sizeof(buf),0);
        std::string str(buf);
        try
        {
            PeerInput.at(n);
            PeerInput.erase(PeerInput.begin()+n);
            PeerInput.insert(PeerInput.begin()+n,str);


        }
        catch(const std::exception& e)
        {
            try
            {
            PeerInput.insert(PeerInput.begin()+n,str);
            }
            catch(const std::exception& e)
            {
            PeerInput.insert(PeerInput.begin()+n-1,str);
            }
        }
        
        std::cout << str << std::endl;

        
        }
    }
    
};

int main()
{
    
    return 0;
}