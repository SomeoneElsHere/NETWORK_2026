#pragma once

#include "util/tracker_files.hpp"

// C++ Standard Library
#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <thread>

// Unix Specification
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace peer_to_peer::tracker
{
    /// The command string for REQ LIST
    static const string COMMAND_REQ_LIST = "REQ LIST";
    static constexpr char CMDCH_REQ_LIST = 'R';

    /// The command string for GET
    static const string COMMAND_GET = "GET";
    static constexpr char CMDCH_GET = 'G';

    /// The command string for createtracker
    static const string COMMAND_TRACKER_CREATE = "createtracker";
    static constexpr char CMDCH_TRACKER_CREATE = 'c';

    /// The command string for updatetracker
    static const string COMMAND_TRACKER_UPDATE = "updatetracker";
    static constexpr char CMDCH_TRACKER_UPDATE = 'u';

    /**
     * A worker that is assigned by a tracker to handle a message from a peer
     */
    class Worker
    {
        public:
            /// The time between iterations of the work thread loop
            static constexpr duration WORK_DELAY = 1ms;
            
        private:
            /// A reference to the directory in which tracker files are stored
            const string shared_directory;

            /// The file descriptor corresponding to the socket this worker listens for messages from a peer on
            atomic<int> peer_fd;

            /// The thread this worker is using to communicate with its assigned peer
            thread* work_thread;

            /// A mutex lock used to keep resources utilized by this worker safe
            mutex* lock;

            /// Whether this worker has completed its work with its assigned peer and is ready to be disposed of
            bool completed;

            /// Whether this worker should terminate ASAP
            atomic<bool> killswitch;

        public:
            /// The IP address of the peer this worker is assigned to
            const string peer_address;

            /// Whether to close the socket file descriptor when this worker completes its work thread
            const bool close_fd_on_completion;

        private:
            /**
             * Creates a blank slate worker
             */
            Worker();

            ///**
            // * Creates a new worker
            // * @param peer_fd The file descriptor of the socket this worker will listen on
            // * @param peer_address The string representation of the peer's IP address
            // * @param tracker_files A list of tracker files
            // * @param close_fd_on_completion Whether to close the socket file descriptor when this worker completes its work thread
            // */
            //Worker(int peer_fd, string peer_address, list<TrackerFile> tracker_files, bool close_fd_on_completion);
            
        public:
            /**
             * Creates a new worker
             * @param shared_directory The directory to store tracker files in
             * @param peer_fd The file descriptor of the socket this worker will listen on
             * @param peer_ipv4 The assigned peer's IP address
             * @param close_fd_on_completion Whether to close the socket file descriptor when this worker completes its work thread
             */
            Worker(const string shared_directory, int peer_fd, const sockaddr_in& peer_ipv4, bool close_fd_on_completion);

            /**
             * Cleans up resources utilized by this worker
             */
            ~Worker();

            ///**
            // * Attempts to establish a connection with a peer and create a worker for it
            // * @param peer_fd The file descriptor of the socket assigned to the peer
            // * @param peer_ipv4 The IPv4 address of the peer
            // * @param close_fd_on_completion Whether to close the socket file descriptor when this worker completes its work thread
            // * @param out_worker The memory location to store the Worker object if successful; pass nullptr to skip creation of the worker
            // * @return true if creation of the Worker object was successful; otherwise, false
            // */
            //static bool try_create_worker_for(int peer_fd, sockaddr_in peer_ipv4, bool close_fd_on_completion, Worker* out_worker);

            /**
             * Checks whether this worker has completed its work with its assigned peer
             * @return true if this worker has completed its work with its assigned peer and is ready to be disposed of; otherwise, false
             */
            bool is_complete();

            /**
             * Signals to this worker that it should terminate ASAP. Shuts down the socket if it hasn't been already.
             */
            void signal_to_stop();

            /**
             * Blocks the remaining thread until this worker has completed
             */
            void wait_for_completion();

        private:
            /**
             * The function used by this worker's thread
             */
            void work();

            /**
             * Attempts to parse and handle a command string sent by the peer this worker is assigned to
             * @param command_str The command string sent by the peer
             * @return true if the command was recognized and handled; otherwise, false
             */
            bool try_handle_command(const string& command_str);

            /**
             * Sends a list containing all tracker files accessible by this worker
             */
            void handle_req_list();

            /**
             * Attempts to create a new tracker file using a set of parameters passed by this worker's assigned peer
             * @param ct_args The peer-provided arguments for the createtracker command
             */
            void handle_create_tracker(const vector<string>& ct_args);

            /**
             * Attempts to retrieve tracker file for the specified filename and send it to this worker's assigned peer
             * @param filename The peer-provided filename for the GET command
             */
            void handle_get_tracker(const string& filename);
    };
}