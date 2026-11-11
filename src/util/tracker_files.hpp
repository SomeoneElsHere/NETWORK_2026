#pragma once

#include <istream>
#include <ostream>
#include <optional>
#include <string>
#include <vector>
#include <list>

using namespace std;

namespace peer_to_peer
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

    /**
     * Go through and make a list of all the trackers inside a folder
     * Meant to be used at the beginning of the program.
     * @param target_folder The directory to search for tracker files
     * @return A list containing all found tracker files in the directory; if an error occurs, returns an empty list
     */
    list<string> list_trackers(string& target_folder);

    /* Alternate version of TrackerFile class */

    /**
     * A collection of metadata defining a file available for sharing between peers
     */
    class TrackerFile
    {
        public:
            /**
             * The file extension used by tracker files
             */
            static const string FILE_EXTENSION;

            /**
             * The character used to separate a property key (e.g. "Filename") from its value
             */
            static constexpr char CH_KEY_DELIMITER = ':';

            /**
             * The character used to denote a comment in a tracker file, signaling the parser to skip it
             */
            static constexpr char CH_COMMENT = '#';

        private:
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

        public:
            /**
             * Cannot create an empty tracker file
             */
            TrackerFile();

            /**
             * Creates a tracker file
             */
            TrackerFile(string filename, size_t filesize, optional<string> description, string md5, vector<PeerInfo> peers);

            /**
             * Moves the data from an existing tracker file object to this one
             */
            //TrackerFile(TrackerFile&& source);

            /**
             * Cleans up resources relating to this tracker file
             */
            //~TrackerFile();

            /**
             * Attempts to parse an input stream's contents as a tracker file
             * @param stream The stream to read the tracker file contents from
             * @param out_trackerfile The memory location to store the created TrackerFile object; pass nullptr to skip creation
             * @return true if a tracker file was correctly parsed from the specified input stream; otherwise, false
             */
            static bool try_parse_from_stream(istream& stream, TrackerFile* out_trackerfile);

            /**
             * Attempts to write this tracker file's contents (in format) to an output stream
             * @param stream The stream to write the tracker file contents to
             * @return true if a tracker file was able to be written to the specified output stream; otherwise, false
             */
            bool try_write_to_stream(ostream& stream);

            /**
             * Gets the filename of this tracker file's target file
             * @return The name of the actual file
             */
            string get_filename();

            /**
             * Gets the filesize of this tracker file's target file
             * @return The size of the actual file, in bytes
             */
            size_t get_filesize();

            /**
             * Gets the description assigned to this tracker file's target file
             * @return A description of the actual file, if one was provided; otherwise, nullopt
             */
            optional<string> get_description();

            /**
             * Retrieves the tracker file's stored MD5 hash for the target file
             * @return The actual file's computed MD5 hash
             */
            string get_md5();

            /**
             * Retrieves the tracker file's stored peers for the target file
             * @return The peers that may transmit portions of the actual file
             */
            vector<PeerInfo> get_peers();

            /**
             * Retrieves a string that can be used for display purposes
             * @return A representational string that may be used for display purposes
             */
            string get_quick_string();

            /**
             * Attempts to add a peer to this tracker file
             * @param peer The metadata of the peer to be added
             * @return true if the peer was not already present and successfully added to this tracker file; otherwise, false
             */
            bool try_add_peer(PeerInfo&& peer);

            /**
             * Attempts to remove a peer from this tracker file, using an address and port
             * @param peer_addr The IP address of the peer to be removed
             * @param peer_port The port of the peer to be removed
             * @return true if the peer was found and successfully removed from this tracker file; otherwise, false
             */
            bool try_remove_peer(string peer_addr, int16_t peer_port);
    };
}