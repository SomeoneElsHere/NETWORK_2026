#pragma once

#include <list>
#include <optional>
#include <string>
#include <vector>

using namespace std;

namespace peer_to_peer::tracker
{
    /* Adapted from Jankatay's "files.hpp" */

    /// Information on a peer that can transmit a portion of an actual file
    struct PeerInfo
    {
        /// The IP address of this peer
        string addr;

        /// The port this peer is connecting through
        int16_t port; // size_t just in case
        
        /// This peer's start byte of the actual file
        size_t start_byte;

        /// This peer's end byte of the actual file
        size_t end_byte;

        /// The timestamp at which (from the tracker's perspective) the peer...
        time_t time_stamp;
    };

    // Library to make, read, and store the tracker file format

    /**
     * A tracker file stored by a tracker server
     */
    class TrackerFile
    {
        public:
            // data, public for quick read only.

            /// The name of the actual file
            string filename;

            /// The size of the actual file, in bytes
            size_t filesize;

            /// A description of the actual file, if one was provided; otherwise, nullopt
            optional<string> description;

            /// The actual file's computed MD5 hash
            string md5;

            /// The peers that may transmit portions of the actual file
            vector<PeerInfo> peers;

            /// A representational string that may be used for display purposes
            string quickstring; // Of form "file1name file1size file1MD5\n"

            /**
             * Checks if this file's filename matches another filename
             * @param filename The filename to compare with
             * @return true if the filenames match; otherwise, false
             */
            bool has_filename(string& filename);

            /**
             * Defines a new tracker file
             * @param filename The name of the actual file
             * @param filesize The size of the actual file, in bytes
             * @param description An optional peer-provided description of the actual file
             * @param md5 The actual file's computed MD5 hash
             */
            TrackerFile(string filename, string filesize, optional<string> description, string md5);
    };

    /**
     * Make a list of all current tracker files by reading the directory.
     * Creates the directory if it does not yet exist.
     * @return A list containing all tracker files found in the directory, if any were found; otherwise, returns an empty list.
     */
    list<TrackerFile> produce_file_list();
}

