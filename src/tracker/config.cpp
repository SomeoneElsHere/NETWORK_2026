#include "config.hpp"

#include <fstream>

using namespace std;

namespace peer_to_peer::tracker
{
    /// Parses sconfig: port, shared directory, and an optional 3rd line = dead-peer timeout seconds (default 900) 
    optional<TrackerServerConfig> try_parse_sconfig(const string& filename)
    {
        ifstream fileStream(filename);
        if(!fileStream.is_open()) {
            // Failed to open the specified file for reading
            return nullopt;
        }

        int16_t intPort;
        string strDirectory;
        try {
            string strPort;

            // Port Number
            getline(fileStream, strPort);
            intPort = stoi(strPort);

            // Shared Directory
            getline(fileStream, strDirectory);
        }
        catch(...)
        {
            // One of the options could not be parsed correctly
            fileStream.close();
            return nullopt;
        }

        // Optional third line: dead peer timeout (seconds before a silent peer's
        // entries are removed from tracker files). Defaults to 900s (15 minutes,
        // matching the specification's default peer refresh interval) when absent
        // or unparseable.
        int dead_peer_timeout = 900;
        try
        {
            string strTimeout;
            if(getline(fileStream, strTimeout) && !strTimeout.empty())
            {
                int value = stoi(strTimeout);
                if(value > 0)
                    dead_peer_timeout = value;
            }
        }
        catch(...)
        { /* keep the default */
        }

        fileStream.close();
        return TrackerServerConfig {
            intPort,
            strDirectory,
            dead_peer_timeout};
    }
} // namespace peer_to_peer::tracker
