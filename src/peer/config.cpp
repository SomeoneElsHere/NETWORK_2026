#include "../constants.hpp"
#include "config.hpp"

#include <fstream>

using namespace std;

namespace peer_to_peer::peer
{
    ///
    optional<PeerClientConfig> try_parse_peer_client_config(string filename)
    {
        ifstream fileStream(filename);
        if(!fileStream.is_open())
        {
            // Failed to open the specified file for reading
            return nullopt;
        }
        int intPort, intUpdateInterval;
        in_addr_t convertedAddr;
        try
        {
            string strPort, strAddr, strUpdateInterval;
        
            // Port Number
            getline(fileStream, strPort);
            intPort = stoi(strPort);
        
            // IP Address
            getline(fileStream, strAddr);
            convertedAddr = inet_addr(strAddr.c_str());
            if(convertedAddr == -1)
                throw 0;
        
            // Update Interval
            getline(fileStream, strUpdateInterval);
            intUpdateInterval = stoi(strUpdateInterval);
            fileStream.close();
        }
        catch (...)
        {
            // One of the options could not be parsed correctly
            fileStream.close();
            return nullopt;
        }
    
        sockaddr_in tracker_addr;
        tracker_addr.sin_family = AF_INET;
        tracker_addr.sin_port = htons(TRACKER_PORT);
        tracker_addr.sin_addr.s_addr = (in_addr_t)convertedAddr;
        
        return PeerClientConfig{
            tracker_addr,
            intUpdateInterval
        };
    }
}