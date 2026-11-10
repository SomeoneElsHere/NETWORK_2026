#pragma once

#include <optional>
#include <string>

using namespace std;

namespace peer_to_peer::tracker
{
    /**
     * A set of configuration values relating to a peer's server thread
     */
    typedef struct {
        /// The port this tracker server will listen on
        int16_t port;

        /// The name of the shared folder
        string shared_directory;
    } TrackerServerConfig;

    /**
     * Attempts to parse a tracker server configuration file
     * @param filename The name of the file to parse
     * @return The parsed tracker server configuration values, if successful; otherwise, nullopt
     */
    optional<TrackerServerConfig> try_parse_sconfig(const string& filename);
}