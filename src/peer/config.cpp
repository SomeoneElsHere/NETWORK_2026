#include "config.hpp"
#include "../util/networking.hpp"

#include <fstream>

using namespace std;

namespace peer_to_peer::peer
{
    /// Parses clientThreadConfig.cfg: tracker port, tracker IP, updatetracker interval (seconds)
    optional<PeerClientConfig> try_parse_peer_client_config(const string& filename)
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
        catch(...)
        {
            // One of the options could not be parsed correctly
            fileStream.close();
            return nullopt;
        }

        sockaddr_in tracker_addr;
        tracker_addr.sin_family = AF_INET;
        tracker_addr.sin_port = htons((uint16_t)intPort); // was ignoring the configured port (fixed)
        tracker_addr.sin_addr.s_addr = (in_addr_t)convertedAddr;

        return PeerClientConfig{
            tracker_addr,
            intUpdateInterval};
    }

    /// Parses serverThreadConfig.cfg: this peer's listen port and shared folder name
    optional<PeerServerConfig> try_parse_peer_server_config(const string& filename)
    {
        ifstream fileStream(filename);
        if(!fileStream.is_open())
        {
            // Failed to open the specified file for reading
            return nullopt;
        }
        int intPort;
        string strDirectory;
        try
        {
            string strPort;

            // Port this peer listens on for incoming peer connections
            getline(fileStream, strPort);
            intPort = stoi(strPort);
            if(intPort <= 0 || intPort > 65535)
                throw 0;

            // Shared folder name
            getline(fileStream, strDirectory);
            if(strDirectory.empty())
                throw 0;
            fileStream.close();
        }
        catch(...)
        {
            // One of the options could not be parsed correctly
            fileStream.close();
            return nullopt;
        }

        return PeerServerConfig{
            (int16_t)intPort,
            strDirectory};
    }
} // namespace peer_to_peer::peer
