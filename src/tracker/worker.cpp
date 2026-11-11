#include "worker.hpp"
#include "../util/commands.hpp"
#include "../util/md5.hpp"
#include "../util/misc.hpp"
#include "../util/networking.hpp"
#include "../util/strings.hpp"
#include "files.hpp"

// C++ Standard Library
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <string>

// Unix Specification
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

namespace peer_to_peer::tracker
{
    /// Converts a peer's IPv4 socket address to a dotted-quad string, e.g. "127.0.0.1"
    static string ip_addr_str(const sockaddr_in& peer_ipv4)
    {
        char ipaddr[INET_ADDRSTRLEN];
        inet_ntop(
            AF_INET,
            &(peer_ipv4.sin_addr),
            ipaddr,
            INET_ADDRSTRLEN); // get ip as char*

        return string(ipaddr);
    }

    /// (constructor) Records the peer connection, then spawns the work() thread serving it
    Worker::Worker(int peer_fd, const sockaddr_in& peer_ipv4, const TrackerServerConfig& tracker_config, bool close_fd_on_completion) : peer_fd(peer_fd),
                                                                                                                                        peer_address(ip_addr_str(peer_ipv4)),
                                                                                                                                        close_fd_on_completion(close_fd_on_completion)
    {
        this -> work_thread = new thread(&Worker::work, this);
        // NOTE: the thread is NOT detached; wait_and_dispose_thread() joins it
        // during teardown. Detaching caused a use-after-free race where this
        // Worker could be deleted while work() was still running on it.
        this -> lock = new mutex();
        this -> completed = false;
        this -> killswitch = false;

        this -> tracker_config = tracker_config;
    }

    /// (destructor) Stops the work thread if still running, joins it, and frees the lock
    Worker::~Worker()
    {
        if(!this -> completed)
        {
            this -> signal_to_stop();
        }
        this -> wait_for_completion();

        delete this -> lock;
    }

    /// Returns whether this worker's thread has finished with its assigned peer
    bool Worker::is_complete()
    {
        return this -> completed;
    }

    /// Raises the killswitch and shutdown()s the socket's read side so a blocking recv() returns
    void Worker::signal_to_stop()
    {
        int peer_fd = this -> peer_fd.load();
        if(peer_fd != -1)
        {
            if(shutdown(peer_fd, SHUT_RD) == 0)
                this -> peer_fd.store(-1);
        }

        if(!this -> killswitch.load() && !this -> completed)
            this -> killswitch.store(true);
    }

    /// Joins and deletes the work thread, then resets the killswitch
    void Worker::wait_for_completion()
    {
        wait_and_dispose_thread(this -> work_thread);
        this -> killswitch.store(false);
    }

    constexpr int MAX_MSG_SIZE = 4096;

    /// Thread main: recv peer bytes, reassemble "\n"-framed messages (survives TCP splitting/coalescing), dispatch each complete command
    void Worker::work()
    {
        char buf[MAX_MSG_SIZE];
        string pending; // received bytes not yet terminated by a newline

        while(!(this -> killswitch.load()))
        {
            this_thread::sleep_for(WORK_DELAY);

            fill_n(buf, MAX_MSG_SIZE, '\0');
            int recvBytes = recv(this -> peer_fd, buf, sizeof(buf) - 1, 0);
            if(recvBytes <= 0) // if failure to recieve stop. Store data at buf
            {
                break;
            }

            pending.append(buf, recvBytes);

            // Protocol messages are framed as "<...>\n" per the project specification.
            // Process one complete line at a time so that commands received
            // back-to-back in a single TCP segment are all handled, and a
            // command split across multiple segments is reassembled first.
            size_t newline;
            while((newline = pending.find('\n')) != string::npos)
            {
                string message = pending.substr(0, newline);
                pending.erase(0, newline + 1);
                if(!message.empty() && message.back() == '\r')
                    message.pop_back();
                if(message.empty())
                    continue;

                _debug_print("Received message: " + message);

                if(message[0] == CH_COMMAND_OPEN)
                {
                    _debug_print("Received message may be in command format.");
                    string command_str;
                    // Received data is a command
                    if(try_get_command_str(message.data(), message.size(), &command_str))
                    {
                        // Command was formatted properly
                        _debug_print("Command string: '" + command_str + "'.");
                        if(!this -> try_handle_command(command_str))
                        {
                            // Command was not successfully handled
                            send_string(this -> peer_fd, "Command failed.");
                            _debug_print("The command was not successfully handled.");
                        }
                    }
                    else
                    {
                        // Command was not formatted properly
                        send_string(this -> peer_fd, "Improper format");
                        _debug_print("Received message was improperly formatted as command.");
                    }
                }
                else
                {
                    // Received data is part of a file
                    send_string(this -> peer_fd, "Not a command.");
                    _debug_print("Received message was not in command format.");
                }
            }
        }

        if(this -> close_fd_on_completion)
        {
            // Close peer connection before end of thread
            int peer_fd = this -> peer_fd.load();
            if(peer_fd != -1)
            {
                close(peer_fd);
                this -> peer_fd.store(-1);
            }
        }

        this -> completed = true;
    }

    /// Routes a parsed command (REQ LIST / GET / createtracker / updatetracker) to its handler, per the project protocol
    bool Worker::try_handle_command(const string& command_str)
    {
        // Determine the name of the command to run
        size_t args_ind = 0;
        switch(command_str[0])
        {
        case CMDCH_REQ_LIST:
            if(command_compare(command_str, COMMAND_REQ_LIST, nullptr))
            {
                this -> handle_req_list();
                return true;
            }
            break;
        case CMDCH_GET:
            if(command_compare(command_str, COMMAND_GET, &args_ind))
            {
                this -> handle_get_tracker(command_str.substr(args_ind));
                return true;
            }
            break;
        case CMDCH_TRACKER_CREATE:
            if(command_compare(command_str, COMMAND_TRACKER_CREATE, &args_ind))
            {
                vector<string> arguments;
                if(str_to_args(command_str.substr(args_ind), arguments))
                {
                    this -> handle_create_tracker(arguments);
                    return true;
                }
            }
            break;
        case CMDCH_TRACKER_UPDATE:
            if(command_compare(command_str, COMMAND_TRACKER_UPDATE, &args_ind))
            {
                vector<string> arguments;
                if(str_to_args(command_str.substr(args_ind), arguments))
                {
                    this -> handle_update_tracker(arguments);
                    return true;
                }
            }
            break;
        }

        // Command string did not start with a valid command
        return false;
    }

    /// Replies with the listing block: <REP LIST X>, one <i name size md5> per .track file, then <REP LIST END>
    void Worker::handle_req_list()
    {
        list<TrackerFile> files = produce_file_list(this -> tracker_config.shared_directory);

        // Reply structure per the project specification:
        // <REP LIST X>\n<I name size MD5>\n...\n<REP LIST END>\n
        // "filename.track" is displayed as "filename" in the listing
        string reply = "<REP LIST " + to_string(files.size()) + ">\n";
        int entry = 1;
        for(TrackerFile& file : files)
        {
            reply += "<" + to_string(entry) + " " + file.filename + " " + to_string(file.filesize) + " " + file.md5 + ">\n";
            entry++;
        }
        reply += "<REP LIST END>\n";

        send_string(this -> peer_fd, reply);
        cout << "Tracker file list requested by " << this -> peer_address
             << " (" << files.size() << " files sent)" << endl;
    }

    /// Sends the requested .track file inside a <REP GET BEGIN>/<REP GET END md5> envelope (md5 = checksum of the bytes sent)
    void Worker::handle_get_tracker(const string& filename)
    {
        // The specification requests "<GET filename.track>"; accept the filename
        // with or without the .track extension for convenience
        string tracker_name = filename;
        const string ext = ".track";
        if(tracker_name.size() < ext.size() || tracker_name.compare(tracker_name.size() - ext.size(), ext.size(), ext) != 0)
            tracker_name += ext;

        filesystem::path path = filesystem::path(this -> tracker_config.shared_directory) / tracker_name;
        ifstream file(path, ios::binary);
        if(!file.is_open())
        {
            // The specification defines no reply for a failed GET; "<GET fail>"
            // mirrors the createtracker/updatetracker failure replies (documented assumption)
            send_string(this -> peer_fd, "<GET fail>\n");
            cout << "Tracker file \"" << tracker_name << "\" requested by " << this -> peer_address
                 << " but could not be found" << endl;
            return;
        }

        // Read the entire tracker file into memory (tracker files are small)
        string contents((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        // The specification's "FileMD5" is taken to mean the MD5 checksum of the
        // tracker file content sent, so the peer can verify the reply's integrity
        // (realistic assumption; the specification is ambiguous here)
        char checksum[MD5_DIGEST_STRING_LENGTH];
        MD5Data((const uint8_t*)contents.data(), contents.size(), checksum);

        string reply = "<REP GET BEGIN>\n" + contents + (contents.empty() || contents.back() != '\n' ? "\n" : "") + "<REP GET END " + string(checksum) + ">\n";
        send_string(this -> peer_fd, reply);

        cout << "Tracker file \"" << tracker_name << "\" sent to " << this -> peer_address
             << " (" << contents.size() << " bytes)" << endl;
    }

    /// Validates args, refuses duplicates (ferr), writes a new .track file with the seeding peer's full-range (0:filesize) entry
    void Worker::handle_create_tracker(const vector<string>& ct_args)
    {
        // Specification: <createtracker filename filesize description md5 ip-address port-number>
        unsigned long long filesize, port;
        if(ct_args.size() != 6 || !str_to_u64(ct_args[1], filesize) || !str_to_u64(ct_args[5], port) || port == 0 || port > 65535)
        {
            send_string(this -> peer_fd, "<createtracker fail>\n");
            cout << "Malformed createtracker command from " << this -> peer_address << endl;
            return;
        }

        const string& filename = ct_args[0];
        const string& file_desc = ct_args[2];
        const string& md5 = ct_args[3];
        const string& ip = ct_args[4];

        // Refuse to create a tracker file that already exists
        filesystem::path path = filesystem::path(this -> tracker_config.shared_directory) / (filename + ".track");
        if(filesystem::exists(path))
        {
            send_string(this -> peer_fd, "<createtracker ferr>\n");
            cout << "createtracker for \"" << filename << "\" rejected: tracker file already exists" << endl;
            return;
        }

        // Build the tracker file; the requesting peer is a seed with the full file,
        // so its entry spans bytes 0 through filesize with the current timestamp
        TrackerFile tracker_file(filename, ct_args[1], file_desc.empty() ? nullopt : optional<string>(file_desc), md5);
        tracker_file.peers.push_back(PeerInfo{
            ip,
            (int16_t)port,
            0,
            (size_t)filesize,
            time(nullptr)});

        if(!tracker_file.save_to(this -> tracker_config.shared_directory))
        {
            send_string(this -> peer_fd, "<createtracker fail>\n");
            cout << "Failed to write tracker file \"" << filename << ".track\"" << endl;
            return;
        }

        send_string(this -> peer_fd, "<createtracker succ>\n");
        cout << "Tracker file \"" << filename << ".track\" created for " << ip
             << ":" << port << " (requested by " << this -> peer_address << ")" << endl;
    }

    /// Refreshes or inserts the peer's entry with the current timestamp; prunes dead peers (no update within the timeout)
    void Worker::handle_update_tracker(const vector<string>& ut_args)
    {
        // Specification: <updatetracker filename start_bytes end_bytes ip-address port-number>
        unsigned long long start_byte, end_byte, port;
        if(ut_args.size() != 5 || !str_to_u64(ut_args[1], start_byte) || !str_to_u64(ut_args[2], end_byte) || !str_to_u64(ut_args[4], port) || port == 0 || port > 65535)
        {
            string fail_name = ut_args.empty() ? "?" : ut_args[0];
            send_string(this -> peer_fd, "<updatetracker " + fail_name + " fail>\n");
            cout << "Malformed updatetracker command from " << this -> peer_address << endl;
            return;
        }

        const string& filename = ut_args[0];
        const string& ip = ut_args[3];

        // The tracker file must already exist
        filesystem::path path = filesystem::path(this -> tracker_config.shared_directory) / (filename + ".track");
        optional<TrackerFile> loaded = TrackerFile::try_load(path.string());
        if(!loaded.has_value())
        {
            send_string(this -> peer_fd, "<updatetracker " + filename + " ferr>\n");
            cout << "updatetracker for \"" << filename << "\" rejected: no such tracker file" << endl;
            return;
        }
        TrackerFile tracker_file = loaded.value();
        time_t now = time(nullptr);

        // Remove entries of dead peers (no update received within the timeout),
        // per the specification's requirement on updatetracker handling
        int timeout = this -> tracker_config.dead_peer_timeout_seconds;
        tracker_file.peers.erase(
            remove_if(tracker_file.peers.begin(), tracker_file.peers.end(),
                      [&](const PeerInfo& peer)
                      {
                          return (now - peer.time_stamp) > timeout;
                      }),
            tracker_file.peers.end());

        // Update the existing entry if this peer is already listed;
        // otherwise, add a new entry for it
        bool found = false;
        for(PeerInfo& peer : tracker_file.peers)
        {
            if(peer.addr == ip && peer.port == (int16_t)port)
            {
                peer.start_byte = (size_t)start_byte;
                peer.end_byte = (size_t)end_byte;
                peer.time_stamp = now; // must be refreshed with the current system timestamp
                found = true;
                break;
            }
        }
        if(!found)
        {
            tracker_file.peers.push_back(PeerInfo{
                ip,
                (int16_t)port,
                (size_t)start_byte,
                (size_t)end_byte,
                now});
        }

        if(!tracker_file.save_to(this -> tracker_config.shared_directory))
        {
            send_string(this -> peer_fd, "<updatetracker " + filename + " fail>\n");
            cout << "Failed to write tracker file \"" << filename << ".track\"" << endl;
            return;
        }

        send_string(this -> peer_fd, "<updatetracker " + filename + " succ>\n");
        cout << "Tracker file \"" << filename << ".track\" updated by " << ip << ":" << port
             << " (bytes " << start_byte << "-" << end_byte << ")" << endl;
    }
} // namespace peer_to_peer::tracker
