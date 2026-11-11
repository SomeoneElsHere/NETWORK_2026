#include "peer.hpp"
#include "../util/commands.hpp"
#include "../util/md5.hpp"
#include "../util/misc.hpp"
#include "../util/networking.hpp"
#include "../util/strings.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace peer_to_peer::peer;

namespace peer_to_peer
{
    /// (Default Constructor) 
    Peer::Peer()
    {
        sockaddr_in default_addr;

        default_addr.sin_family = AF_INET;
        default_addr.sin_port = htons(TRACKER_PORT);
        default_addr.sin_addr.s_addr = INADDR_ANY;

        this -> client_config = PeerClientConfig{
            default_addr,
            1};
        this -> tracker_socket_fd = -1;

        this -> killswitch = false;
        this -> actively_listening = false;
        this -> tracker_listener = nullptr;
        this -> server_socket_fd = -1;
        this -> server_listener = nullptr;
        this -> server_killswitch = false;
        this -> collecting_rep_get = false;
        this -> send_lock = new std::mutex();
    }

    /// (constructor) Initializes with the given client configuration; no sockets or threads yet
    Peer::Peer(PeerClientConfig clientConfig)
    {
        this -> client_config = clientConfig;
        this -> tracker_socket_fd = -1;

        this -> killswitch = false;
        this -> actively_listening = false;
        this -> tracker_listener = nullptr;
        this -> server_socket_fd = -1;
        this -> server_listener = nullptr;
        this -> server_killswitch = false;
        this -> collecting_rep_get = false;
        this -> send_lock = new std::mutex();
    }

    /// (destructor) Stops the server thread and closes the tracker connection, then frees the send lock
    Peer::~Peer()
    {
        this -> close_server();
        if(this -> tracker_socket_fd >= 0)
            this -> close_tracker_connection();
        delete this -> send_lock;
    }

    constexpr int MAX_MSG_SIZE = 4096;

    /// The command name for peer-to-peer chunk requests: "<GET filename start end>"
    static const string COMMAND_PEER_GET = "GET";

    /// Thread main: recv tracker replies, reassemble "\n"-framed lines, hand each to handle_tracker_line
    void Peer::listen_to_tracker()
    {
        _debug_print("Tracker listening thread started.");
        this -> actively_listening = true;

        char buf[MAX_MSG_SIZE];
        string pending; // received bytes not yet terminated by a newline

        while(!(this -> killswitch.load()))
        {
            this_thread::sleep_for(TRACKER_LISTEN_DELAY);

            fill_n(buf, MAX_MSG_SIZE, '\0');
            int recvBytes = recv(this -> tracker_socket_fd, buf, sizeof(buf) - 1, 0);
            if(recvBytes <= 0) // if failure to recieve stop. Store data at buf
            {
                _debug_print("Connection suddenly terminated.");
                break;
            }

            pending.append(buf, recvBytes);

            // Messages are "<...>\n"-framed like the peer-to-tracker direction;
            // print one complete line at a time so multi-line replies (REP LIST,
            // REP GET) display cleanly even when a reply spans several TCP segments
            size_t newline;
            while((newline = pending.find('\n')) != string::npos)
            {
                string message = pending.substr(0, newline);
                pending.erase(0, newline + 1);
                if(!message.empty() && message.back() == '\r')
                    message.pop_back();
                if(message.empty())
                    continue;

                _debug_print("Received text '" + message + "'.");

                // Route the line through the reply processor (multi-line REP GET
                // replies are collected; everything else is printed to the screen,
                // per the specification's requirement of displaying necessary events)
                this -> handle_tracker_line(message);
            }
        }

        if(this -> killswitch.load())
            _debug_print("Killswitch was activated.");

        this -> actively_listening = false;
        _debug_print("Tracker listening thread stopped.");
    }

    /// Returns whether the tracker listener thread is currently active
    bool Peer::is_listening()
    {
        return this -> actively_listening;
    }

    /// Creates the TCP socket used for the tracker connection
    bool Peer::try_init_socket(int* error_code)
    {
        if(this -> tracker_socket_fd >= 0)
            return false;

        tracker_socket_fd = socket(
            AF_INET,
            SOCK_STREAM,
            0);

        return check_for_error(tracker_socket_fd < 0, error_code);
    }

    /// Connects to the tracker, auto-detects this peer's own IP via getsockname() (spec requirement), spawns the listener thread
    bool Peer::try_connect(int* error_code)
    {
        if(this -> tracker_socket_fd < 0)
            return false;

        sockaddr* tracker_socket_addr = (sockaddr*)(&this -> client_config.tracker_addr);

        int success = connect(
            this -> tracker_socket_fd,
            tracker_socket_addr,
            sizeof(this -> client_config.tracker_addr));

        if(success == 0)
        {
            // Auto-detect this peer's own IP address on the tracker-facing
            // interface (the specification requires peers to obtain their own
            // IP automatically rather than reading it from the config file)
            sockaddr_in local_addr;
            socklen_t local_len = sizeof(local_addr);
            if(getsockname(this -> tracker_socket_fd, (sockaddr*)&local_addr, &local_len) == 0)
            {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(local_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
                this -> own_ip = string(ip_str);
            }

            this -> tracker_listener = new thread(&Peer::listen_to_tracker, this);
            // NOT detached: close_tracker_connection() joins it via
            // wait_and_dispose_thread(); detaching risked use-after-free
        }
        return check_for_error(success == -1, error_code);
    }

    /// Sends a message to the tracker under a mutex (the main thread and listener thread can both send)
    bool Peer::try_send_to_tracker(std::string message, int len, int* error_code)
    {
        if(this -> tracker_socket_fd < 0)
            return false;

        lock_guard<std::mutex> guard(*(this -> send_lock));
        size_t sentBytes = send_string(
            this -> tracker_socket_fd,
            message);

        return check_for_error(sentBytes == -1, error_code);
    }

    /// Raises the killswitch, shutdown()s the socket to unblock the listener's recv(), joins the thread, closes the fd
    void Peer::close_tracker_connection()
    {
        if(this -> tracker_socket_fd < 0)
            return;

        _debug_print("Closing tracker server...");
        this -> killswitch.store(true);

        // Interrupt the listener thread's blocking recv() so it can terminate
        shutdown(this -> tracker_socket_fd, SHUT_RD);

        // Wait for peer listening thread to close
        wait_and_dispose_thread(this -> tracker_listener);

        this -> killswitch.store(false);

        close(this -> tracker_socket_fd);
        this -> tracker_socket_fd = -1;
        _debug_print("Tracking server closed.");
    }

    /// Stores the server thread configuration loaded from serverThreadConfig.cfg
    void Peer::set_server_config(PeerServerConfig serverConfig)
    {
        this -> server_config = serverConfig;
    }

    /// Binds/listens on the configured peer port, makes the shared folder if missing, spawns the accept loop
    bool Peer::try_start_server(int* error_code)
    {
        if(!this -> server_config.has_value() || this -> server_socket_fd >= 0)
            return false;

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0)
            return check_for_error(true, error_code);

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons((uint16_t)this -> server_config -> port);
        server_addr.sin_addr.s_addr = INADDR_ANY; // accept connections on any interface

        if(bind(fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            close(fd);
            return check_for_error(true, error_code);
        }
        if(listen(fd, 5) < 0)
        {
            close(fd);
            return check_for_error(true, error_code);
        }

        // Make sure the shared folder exists (the peer serves its files from there)
        std::error_code ec;
        filesystem::create_directories(this -> server_config -> shared_directory, ec);

        this -> server_socket_fd = fd;
        this -> server_listener = new thread(&Peer::server_loop, this);
        return true;
    }

    /// Stops the accept loop, joins all serving threads, closes the server socket
    void Peer::close_server()
    {
        if(this -> server_socket_fd < 0)
            return;

        this -> server_killswitch.store(true);
        shutdown(this -> server_socket_fd, SHUT_RDWR);

        wait_and_dispose_thread(this -> server_listener);
        for(thread* t : this -> serving_threads)
            wait_and_dispose_thread(t);
        this -> serving_threads.clear();

        close(this -> server_socket_fd);
        this -> server_socket_fd = -1;
        this -> server_killswitch.store(false);
        _debug_print("Peer server closed.");
    }

    /// Accept loop for incoming peers; spawns one serve_connection thread per connected peer
    void Peer::server_loop()
    {
        _debug_print("Peer server thread started.");

        while(!(this -> server_killswitch.load()))
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(this -> server_socket_fd, &fds);

            struct timeval tm;
            tm.tv_usec = 100000; // 100ms; lets the loop notice the killswitch
            tm.tv_sec = 0;

            int sel = select(this -> server_socket_fd + 1, &fds, nullptr, nullptr, &tm);
            if(sel <= 0)
            {
                if(sel < 0 && this -> server_killswitch.load())
                    break; // socket was shut down on purpose
                continue;
            }

            sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr); // MUST be the full struct size
            int client_fd = accept(this -> server_socket_fd, (sockaddr*)&client_addr, &addr_len);
            if(client_fd < 0)
            {
                if(this -> server_killswitch.load())
                    break;
                continue;
            }

            thread* serving = new thread(&Peer::serve_connection, this, client_fd, client_addr);
            this -> serving_threads.push_back(serving);
        }

        _debug_print("Peer server thread stopped.");
    }

    /// Handles a peer's chunk requests '<GET name start end>' -> raw bytes; strictly refuses (end-start) > 1024 with <GET invalid>
    void Peer::serve_connection(int client_fd, sockaddr_in client_addr)
    {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

        char buf[MAX_CHUNK_SIZE + 64];
        string pending; // received bytes not yet terminated by a newline

        while(!(this -> server_killswitch.load()))
        {
            int recvBytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
            if(recvBytes <= 0)
                break; // peer disconnected

            buf[recvBytes] = '\0';
            pending.append(buf, recvBytes);

            size_t newline;
            while((newline = pending.find('\n')) != string::npos)
            {
                string message = pending.substr(0, newline);
                pending.erase(0, newline + 1);
                if(!message.empty() && message.back() == '\r')
                    message.pop_back();
                if(message.empty())
                    continue;

                string command_str;
                if(!try_get_command_str(message.data(), message.size(), &command_str))
                {
                    send_string(client_fd, "<GET invalid>\n");
                    continue;
                }

                // Peer-to-peer request: <GET filename start_byte end_byte>
                vector<string> args;
                size_t args_ind = 0;
                if(!command_compare(command_str, COMMAND_PEER_GET, &args_ind) || !str_to_args(command_str.substr(args_ind), args) || args.size() != 3)
                {
                    send_string(client_fd, "<GET invalid>\n");
                    continue;
                }

                unsigned long long start_byte, end_byte;
                if(!str_to_u64(args[1], start_byte) || !str_to_u64(args[2], end_byte) || end_byte <= start_byte || (end_byte - start_byte) > (unsigned long long)MAX_CHUNK_SIZE)
                {
                    // The specification requires refusing chunks over 1024 bytes strictly
                    send_string(client_fd, "<GET invalid>\n");
                    continue;
                }

                const string& filename = args[0];
                filesystem::path path = filesystem::path(this -> server_config -> shared_directory) / filename;
                ifstream file(path, ios::binary | ios::ate);
                if(!file.is_open())
                {
                    send_string(client_fd, "<GET fail>\n");
                    cout << ip_str << " requested \"" << filename
                         << "\" but it is not in the shared folder" << endl;
                    continue;
                }

                // Clamp the served range to what this peer actually has
                unsigned long long have_size = (unsigned long long)file.tellg();
                if(start_byte >= have_size)
                {
                    send_string(client_fd, "<GET fail>\n");
                    continue;
                }
                unsigned long long available = min(end_byte, have_size) - start_byte;

                file.seekg(start_byte);
                vector<char> chunk(available);
                file.read(chunk.data(), available);
                unsigned long long read_bytes = (unsigned long long)file.gcount();
                file.close();

                // The chunk reply is the raw file bytes only; the requesting peer
                // knows exactly how many bytes to expect from its request
                send(client_fd, chunk.data(), read_bytes, 0);
                cout << "Served bytes " << start_byte << "-" << (start_byte + read_bytes)
                     << " of \"" << filename << "\" to " << ip_str << endl;
            }
        }

        close(client_fd);
    }

    /// Reply state machine: collects multi-line REP GET blocks; everything else is printed to the screen
    void Peer::handle_tracker_line(const string& line)
    {
        // Middle of a multi-line REP GET reply: keep collecting the content
        if(this -> collecting_rep_get)
        {
            if(line.rfind("<REP GET END", 0) == 0)
            {
                this -> collecting_rep_get = false;

                // "<REP GET END FileMD5>" -> extract the checksum argument
                string inner = line.substr(1, line.size() - 2); // strip < >
                vector<string> parts = str_split(inner, ' ');
                string claimed_md5 = parts.size() >= 4 ? parts.back() : "";

                this -> process_tracker_file(this -> rep_get_content, claimed_md5);
            }
            else
                this -> rep_get_content += line + "\n";
            return;
        }

        if(line == "<REP GET BEGIN>")
        {
            this -> collecting_rep_get = true;
            this -> rep_get_content.clear();
            return;
        }

        // Anything else is displayed on screen
        cout << line << endl;
    }

    /// Verifies the reply MD5 vs content, caches the .track file locally, sorts peers by newest timestamp (spec), starts the download
    void Peer::process_tracker_file(const string& content, const string& claimed_md5)
    {
        // Verify the integrity of the tracker file data against the MD5 checksum
        // attached to the protocol message, per the project specification
        char checksum[MD5_DIGEST_STRING_LENGTH];
        MD5Data((const uint8_t*)content.data(), content.size(), checksum);
        if(claimed_md5.empty() || claimed_md5 != checksum)
        {
            cout << "Received tracker file failed its MD5 integrity check; discarding it." << endl;
            return;
        }

        // Parse the tracker file content (same on-disk format the tracker writes)
        string filename, expected_md5;
        unsigned long long filesize = 0;
        // peers listed in the tracker file: (address, port, timestamp)
        vector<std::pair<std::pair<string, int16_t>, unsigned long long>> peer_entries;

        istringstream stream(content);
        string line;
        while(getline(stream, line))
        {
            if(line.empty() || line[0] == '#')
                continue; // comments are ignored, per the specification
            if(line.rfind("Filename: ", 0) == 0)
            {
                filename = line.substr(10);
                continue;
            }
            if(line.rfind("Filesize: ", 0) == 0)
            {
                str_to_u64(line.substr(10), filesize);
                continue;
            }
            if(line.rfind("MD5: ", 0) == 0)
            {
                expected_md5 = line.substr(5);
                continue;
            }

            // Peer entry: <ip>:<port>:<start>:<end>:<timestamp>
            vector<string> fields = str_split(line, ':');
            if(fields.size() == 5)
            {
                unsigned long long port, stamp;
                if(str_to_u64(fields[1], port) && str_to_u64(fields[4], stamp))
                    peer_entries.push_back({{fields[0], (int16_t)port}, stamp});
            }
        }

        if(filename.empty() || filesize == 0)
        {
            cout << "Received tracker file was malformed; cannot start the download." << endl;
            return;
        }
        if(peer_entries.empty())
        {
            cout << "No peers are currently sharing \"" << filename << "\"." << endl;
            return;
        }

        // The specification assigns the newest-timestamp peer the highest priority
        sort(peer_entries.begin(), peer_entries.end(),
             [](const auto& a, const auto& b)
             { return a.second > b.second; });

        // Save the tracker file into the local cache (the peer's shared folder)
        if(this -> server_config.has_value())
        {
            ofstream cache(filesystem::path(this -> server_config -> shared_directory) / (filename + ".track"), ios::trunc);
            if(cache.is_open())
                cache << content;
        }

        vector<std::pair<string, int16_t>> peers;
        for(const auto& entry : peer_entries)
            peers.push_back(entry.first);

        // The download starts automatically once the tracker file is received
        this -> download_file(filename, filesize, expected_md5, peers);
    }

    /// Sequential chunk download with resume-from-partial; verifies the file's MD5; announces the completed share via updatetracker
    void Peer::download_file(const string& filename, unsigned long long filesize,
                             const string& expected_md5,
                             const vector<std::pair<string, int16_t>>& peers)
    {
        filesystem::path path = filesystem::path(this -> server_config.has_value()
                                                     ? this -> server_config -> shared_directory
                                                     : ".") /
                                filename;

        // Resume support: a partially downloaded copy keeps its bytes,
        // so the download continues where it stopped instead of restarting
        unsigned long long have = 0;
        {
            std::error_code ec;
            if(filesystem::exists(path, ec))
                have = (unsigned long long)filesystem::file_size(path, ec);
        }
        if(have > 0 && have < filesize)
            cout << "Resuming \"" << filename << "\" from byte " << have << endl;

        for(const auto& [ip, port] : peers)
        {
            if(have >= filesize)
                break;

            // Connect to this sharing candidate; the tracker data may be stale,
            // so a failed attempt moves on to the next listed peer
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in peer_addr;
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_port = htons((uint16_t)port);
            peer_addr.sin_addr.s_addr = inet_addr(ip.c_str());

            if(connect(fd, (sockaddr*)&peer_addr, sizeof(peer_addr)) < 0)
            {
                cout << "Could not reach peer " << ip << ":" << port
                     << "; trying the next one." << endl;
                close(fd);
                continue;
            }

            // Fetch the remaining bytes chunk-by-chunk, sequentially
            // (segment selection is sequential per the specification)
            while(have < filesize)
            {
                unsigned long long end = min(have + (unsigned long long)MAX_CHUNK_SIZE, filesize);
                string request = "<GET " + filename + " " + to_string(have) + " " + to_string(end) + ">\n";
                if(send_string(fd, request) <= 0)
                    break;

                unsigned long long expected = end - have;

                // Peek at the reply: an error line starts with '<'; a chunk
                // reply is raw file bytes with a length the sender expects
                char peekbuf[16];
                ssize_t peeked = recv(fd, peekbuf, 14, MSG_PEEK);
                if(peeked <= 0)
                    break; // connection dropped
                peekbuf[peeked] = '\0';
                if(peekbuf[0] == '<') // TODO: raw file data beginning with '<' would mis-detect
                {
                    // Consume the error line through its newline
                    char errbuf[64];
                    recv(fd, errbuf, sizeof(errbuf), 0);
                    cout << "Peer " << ip << ":" << port << " refused chunk request ("
                         << string(errbuf).substr(0, string(errbuf).find('\n')) << ")." << endl;
                    break;
                }

                // Read exactly the expected number of chunk bytes
                vector<char> chunk(expected);
                unsigned long long received = 0;
                while(received < expected)
                {
                    ssize_t n = recv(fd, chunk.data() + received, expected - received, 0);
                    if(n <= 0)
                        break;
                    received += (unsigned long long)n;
                }
                if(received == 0)
                    break;

                // Append the chunk at the tail of the partially downloaded file
                ofstream out(path, ios::binary | ios::app);
                out.write(chunk.data(), received);
                out.close();
                have += received;
                cout << "downloading " << (have - received) << " to " << have
                     << " bytes of " << filename << " from " << ip << " " << port << endl;

                if(received < expected)
                    break; // short read; reconsider peers for the rest
            }

            close(fd);
        }

        if(have < filesize)
        {
            // Keep the partial file and tracker cache; a later attempt resumes
            cout << "Download of \"" << filename << "\" incomplete (" << have << "/"
                 << filesize << " bytes)." << endl;
            return;
        }

        cout << "File " << filename << " download complete" << endl;

        // Integrity check on the downloaded file against the tracker file's MD5
        char file_md5[MD5_DIGEST_STRING_LENGTH];
        if(MD5File(path.string().c_str(), file_md5) != nullptr && expected_md5 == file_md5)
            cout << "MD5 checksum of \"" << filename << "\" verified." << endl;
        else
            cout << "WARNING: MD5 checksum of \"" << filename << "\" does not match the tracker file!" << endl;

        // The completed file's tracker file may be dropped from the local cache
        std::error_code ec;
        filesystem::remove(path.string() + ".track", ec);

        // Announce the completed (full) share to the tracker server
        if(this -> server_config.has_value() && !this -> own_ip.empty())
        {
            string update = "<updatetracker " + filename + " 0 " + to_string(filesize) + " " + this -> own_ip + " " + to_string(this -> server_config -> port) + ">\n";
            this -> send_to_tracker_locked(update);
        }
    }

    /// Thread-safe send on the tracker socket (used by the listener thread for updatetracker)
    void Peer::send_to_tracker_locked(const string& message)
    {
        lock_guard<std::mutex> guard(*(this -> send_lock));
        send_string(this -> tracker_socket_fd, message);
    }
} // namespace peer_to_peer
