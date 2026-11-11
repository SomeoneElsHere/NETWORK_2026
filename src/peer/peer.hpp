#pragma once

#include "config.hpp"

// C++ Standard Library
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// Unix Specification
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace peer_to_peer::peer;

namespace peer_to_peer
{
    class Peer
    {
      public:
        /// The time between iterations of the tracker listening loop
        static constexpr duration TRACKER_LISTEN_DELAY = 10ms;

      private:
        /// The configuration values for the client thread of this peer
        PeerClientConfig client_config;

        /// The socket used for communicating with the tracker server
        int tracker_socket_fd;

        /// The thread used to listen for responses sent by the tracker server
        thread* tracker_listener;

        /// Whether this peer's tracker listening thread is still active
        bool actively_listening;

        /// A kill switch used to terminate the connection to the tracker server and clean up resources
        atomic<bool> killswitch;

        // -- Server thread (serves file chunks to other peers) --

        /// This peer's server thread configuration (serverThreadConfig.cfg), if one was loaded
        optional<PeerServerConfig> server_config;

        /// The socket used for listening for other peers' connections
        int server_socket_fd;

        /// The thread used to listen for other peers' connections
        thread* server_listener;

        /// Threads currently serving connected peers (joined when the server shuts down)
        vector<thread*> serving_threads;

        /// A kill switch used to terminate this peer's server thread
        atomic<bool> server_killswitch;

        // -- Tracker reply processing / file download --

        /// Whether the tracker listener is currently collecting a multi-line "REP GET" reply
        bool collecting_rep_get;

        /// The accumulated lines of the tracker file content inside a "REP GET" reply
        string rep_get_content;

        /// This peer's own IP address on the tracker-facing interface (auto-detected on connect)
        string own_ip;

        /// Serializes sends on the tracker socket (main thread and listener both send)
        std::mutex* send_lock;

      public:
        /**
         * Creates a new peer with the default client thread configuration
         */
        Peer();

        /**
         * Creates a new peer with a custom client thread configuration
         * @param clientConfig The configuration values for the client thread
         */
        Peer(PeerClientConfig clientConfig);

        /**
         * Clean up resources used by this peer
         */
        ~Peer();

      private:
        /**
         * Receives messages from a tracker server until its connection closes
         */
        void listen_to_tracker();

      public:
        /**
         * Determines whether this peer is still listening for messages from the tracker
         * @return true if the tracker listening thread is still active; otherwise, false
         */
        bool is_listening();

        /**
         * Attempts to initialize the socket and tracker address this peer will use
         * @param error_code The memory location to store the error code, if creating the socket fails
         * @return true if creation of the socket was successful; otherwise, false
         */
        bool try_init_socket(int* error_code);

        /**
         * Attempts to connect to the tracker server using the created socket
         * @param error_code The memory location to store the error code, if the connection attempt fails
         * @return true if connection was successful; otherwise, false
         */
        bool try_connect(int* error_code);

        /**
         * Attempts to send a string message to the tracker
         * @param message The message to be sent
         * @param len The length of the message
         * @param error_code The memory location to store the error code, if the send attempt fails
         * @return true if the message was successfully sent; otherwise, false
         */
        bool try_send_to_tracker(std::string message, int len, int* error_code);

        /**
         * Closes the connection with the tracker server
         */
        void close_tracker_connection();

        /**
         * Sets this peer's server thread configuration. Call before try_start_server.
         * @param serverConfig The configuration values for this peer's server thread
         */
        void set_server_config(PeerServerConfig serverConfig);

        /**
         * Attempts to open this peer's server socket and start listening for other peers
         * @param error_code The memory location to store the error code, if starting fails
         * @return true if the server thread started successfully; otherwise, false
         */
        bool try_start_server(int* error_code);

        /**
         * Stops this peer's server thread and closes the server socket
         */
        void close_server();

      private:
        /**
         * The server thread's accept loop: accepts other peers and spawns a serving thread per connection
         */
        void server_loop();

        /**
         * Handles one connected peer's chunk requests until that peer disconnects.
         * Request format: "<GET filename start_byte end_byte>\n"; byte ranges are
         * half-open [start, end), matching the specification's seed entry "0:filesize".
         * The end_byte - start_byte difference must not exceed MAX_CHUNK_SIZE (1024 bytes);
         * larger requests are refused with "<GET invalid>\n" as the specification requires.
         * @param client_fd The socket file descriptor of the connected peer
         * @param client_addr The IPv4 address of the connected peer
         */
        void serve_connection(int client_fd, sockaddr_in client_addr);

        /**
         * Processes one complete line received from the tracker server
         * (collects REP GET blocks; prints everything else to the screen)
         * @param line The received line, without its trailing newline
         */
        void handle_tracker_line(const string& line);

        /**
         * Verifies, caches, and begins downloading the file described by a received tracker file
         * @param content The tracker file content received inside the REP GET reply
         * @param claimed_md5 The MD5 checksum attached to the REP GET END line
         */
        void process_tracker_file(const string& content, const string& claimed_md5);

        /**
         * Downloads a file chunk-by-chunk from the peers listed in a tracker file,
         * resuming from any partially downloaded copy found in the shared folder.
         * Runs on the tracker listener thread (kept there for the midterm demo;
         * should move to dedicated download threads with parallel chunk fetch
         * for the final demo).
         * @param filename The name of the file to download
         * @param filesize The full size of the file in bytes
         * @param expected_md5 The file's MD5 checksum from the tracker file
         * @param peers The list of (address, port, timestamp) sharing candidates
         */
        void download_file(const string& filename, unsigned long long filesize,
                           const string& expected_md5,
                           const vector<std::pair<string, int16_t>>& peers);

        /**
         * Sends a string to the tracker server in a thread-safe manner
         * @param message The message to send
         */
        void send_to_tracker_locked(const string& message);
    };
} // namespace peer_to_peer