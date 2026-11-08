#include "config.hpp"

#include <fstream>

using namespace std;

namespace peer_to_peer::tracker
{
    ///
    optional<TrackerServerConfig> try_parse_sconfig(string filename)
    {
        ifstream fileStream(filename);
        if(!fileStream.is_open())
        {
            // Failed to open the specified file for reading
            return nullopt;
        }

        int16_t intPort;
        string strDirectory;
        try
        {
            string strPort;

            // Port Number
            getline(fileStream, strPort);
            intPort = stoi(strPort);

            // Shared Directory
            getline(fileStream, strDirectory);
        }
        catch (...) 
        {
            // One of the options could not be parsed correctly
            fileStream.close();
            return nullopt;
        }

        fileStream.close();
        return TrackerServerConfig{
            intPort,
            strDirectory
        };
    }
}