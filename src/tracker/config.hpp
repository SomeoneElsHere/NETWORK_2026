#pragma once

#include <optional>
#include <string>

using namespace std;

namespace peer_to_peer::tracker
{
    /**
     * A set of configuration values relating to a peer's server thread
     */
    typedef struct
    {
        /// The port this tracker server will listen on
        int16_t port;

        /// The name of the shared folder
        string shared_directory;

        /**
         * Seconds after which a peer that has not sent an updatetracker message is
         * considered dead and removed from tracker files. Optional third line of
         * the sconfig file; the project specification leaves tracker-side timing
         * unspecified, so this defaults to the peer's default refresh interval
         * (15 minutes = 900 seconds).
         */
        int dead_peer_timeout_seconds = 900;
    } TrackerServerConfig;

    /**
     * Attempts to parse a tracker server configuration file
     * @param filename The name of the file to parse
     * @return The parsed tracker server configuration values, if successful; otherwise, nullopt
     */
    optional<TrackerServerConfig> try_parse_sconfig(const string& filename);
} // namespace peer_to_peer::tracker