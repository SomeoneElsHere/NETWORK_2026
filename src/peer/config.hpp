#pragma once

// C++ Standard Library
#include <optional>
#include <string>

// Unix Specification
#include <arpa/inet.h>

using namespace std;

namespace peer_to_peer::peer
{
    /**
     * A set of configuration values relating to a peer's client thread
     */
    typedef struct
    {
        /// The address of the tracker server to connect to
        sockaddr_in tracker_addr;

        /// The updatetracker interval, in seconds
        int update_interval;
    } PeerClientConfig;

    /**
     * Attempts to parse a client thread configuration file
     * @param filename The name of the file to parse
     * @return The parsed client thread configuration values, if successful; otherwise, nullopt
     */
    optional<PeerClientConfig> try_parse_peer_client_config(const string& filename);

    /**
     * A set of configuration values relating to a peer's server thread
     */
    typedef struct
    {
        /// The port this peer listens on for incoming peer connection requests
        int16_t port;

        /// The name of this peer's shared folder
        string shared_directory;
    } PeerServerConfig;

    /**
     * Attempts to parse a server thread configuration file (serverThreadConfig.cfg:
     * first line is the port this peer listens on, second line is the shared folder)
     * @param filename The name of the file to parse
     * @return The parsed peer server configuration values, if successful; otherwise, nullopt
     */
    optional<PeerServerConfig> try_parse_peer_server_config(const string& filename);
} // namespace peer_to_peer::peer
