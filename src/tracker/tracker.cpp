#include "../constants.hpp"
#include "../util.hpp"
#include "tracker.hpp"

// C++ Standard Library
#include <cerrno>
#include <iostream>
#include <mutex>
#include <string>

// Unix Specification
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
using namespace peer_to_peer::tracker;

namespace peer_to_peer
{
    ///
    void Tracker::listen_for_peers(NET_ERROR_HANDLER on_error)
    {
        int p = listen(this -> tracker_socket_fd, 5); // listen for max 5 clients in que. This prepares the FD to accept after connect from peer
        if (p == -1)
        {
            on_error(errno, true, "Starting the peer listening thread");
            return;
        }

        cout << "Listening for peers..." << endl;
        while(!(this -> killswitch -> load()))
        {
            this_thread::sleep_for(PEER_LISTEN_DELAY);

            // Max peer connection count has been hit, wait until a slot frees up to continue accepting connections
            if(next_peer_index -> load() == MAX_PEERS)
                continue;

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(this -> tracker_socket_fd, &fds);

            //timeout (needed or select will stall)
            struct timeval tm;
            tm.tv_usec = 100;
            tm.tv_sec = 0;

            int sel = select(this -> tracker_socket_fd + 1,&fds, NULL,NULL,&tm); //put timeout in final para
            if(sel <= 0)
            {
                continue;
            }

            in_addr addr;
            addr.s_addr = INADDR_ANY;

            sockaddr_in sockaddr_client; //where the addr of an IP from a client is stored
            sockaddr_client.sin_family = AF_INET;
            sockaddr_client.sin_addr = addr;
            sockaddr_client.sin_port = this -> tracker_addr.sin_port;

            socklen_t sockklen;
            sockklen = 4;

            int peer_fd = accept(
                this -> tracker_socket_fd,
                (struct sockaddr*)&sockaddr_client,
                &sockklen
            ); // accept any addr

            if (peer_fd != -1)
            {
                sockaddr_in IPV4 = (sockaddr_in)sockaddr_client;
                this -> open_peer_connection(peer_fd, IPV4);
                continue;
            }

            on_error(errno, false, "Searching for a suitable connection");
        }

        // Killswitch activated; no need to continue listening
        cout << "Stopped listening for peers." << endl;
    }

    ///
    void Tracker::manage_peer_connections()
    {
        cout << "Peer management thread activated." << endl;

        mutex guardian;
        bool foundActiveConnection = true;

        // Ensure that no more active connections need to be handled before shutting this thread off
        while(!(this -> killswitch -> load()) || foundActiveConnection)
        {
            this_thread::sleep_for(PEER_MANAGE_DELAY);

            foundActiveConnection = false;
            for(int i = 0; i < MAX_PEERS; i ++)
            {
                if(connected_peers[i].has_value())
                {
                    foundActiveConnection = true;

                    // Guard against main thread/peer threads manipulating peer slots/data                        
                    guardian.lock();
                    peer_connection* connection = connected_peers[i].value();

                    // Force peer worker thread to give up receiving bytes if the killswitch is activated
                    if(connection -> peer_fd != -1 && this -> killswitch -> load())
                    {
                        shutdown(connection -> peer_fd, SHUT_RD);
                    }

                    if(connection -> completed)
                    {
                        ip_index_mappings.erase(connection -> peer_address);

                        // Delete peer connection data and free slot
                        // Use mutex to prevent access from main thread
                        cout << "Clearing peer data for slot " << i << "." << endl;
                        delete connection -> worker_thread;
                        delete connection;
                        connected_peers[i] = nullopt;

                        // Update next peer slot if a lower one is available
                        if(next_peer_index -> load() > i)
                            next_peer_index -> store(i);
                    }
                    guardian.unlock();
                }
                else
                {
                    // Update next peer slot if a lower one is available
                    if(next_peer_index -> load() > i)
                        next_peer_index -> store(i);
                }
            }
        }

        cout << "Peer management thread ended." << endl;
    }

    ///
    void Tracker::open_peer_connection(int peer_fd, sockaddr_in peer_ipv4)
    {
        char ipaddr[INET_ADDRSTRLEN];
        inet_ntop(
            AF_INET,
            &(peer_ipv4.sin_addr),
            ipaddr,
            INET_ADDRSTRLEN
        ); //get ip as char*

        std::string Ipaddrstring(ipaddr); 
        if(this -> ip_index_mappings.find(Ipaddrstring) == ip_index_mappings.end()) //does the IP already exist?
        {
            ip_index_mappings[Ipaddrstring] = peer_fd; //if not, create new handle for IP
        }

        // i = 0; //debug for 1 peer
        peer_connection* new_connection = new peer_connection{ peer_fd, Ipaddrstring };
        new_connection -> worker_thread = new thread(&Tracker::peer_worker_task, this, new_connection);
        new_connection -> worker_thread -> detach(); //let it do its thang

        int peer_index = next_peer_index -> load();
        this -> connected_peers[peer_index] = new_connection;
        new_connection -> DEBUG_peerarray_index = peer_index;
        cout << "Opened peer connection on slot " << peer_index << "." << endl;

        // Find the next available slot for a peer connection
        do
        {
            peer_index ++;
        }
        while(this -> connected_peers[peer_index].has_value());
        next_peer_index -> store(peer_index);
    }

    ///
    void Tracker::peer_worker_task(peer_connection* connection_data)
    {
        char buf[MAX_MESSAGE_BUFFER_SIZE];
        while (!this -> killswitch -> load())
        {
            this_thread::sleep_for(PEER_WORK_DELAY);

            fill_n(buf, MAX_MESSAGE_BUFFER_SIZE, '\0');
            int recvBytes = recv(connection_data -> peer_fd, buf, sizeof(buf), 0);
            if (recvBytes <= 0) // if failure to recieve stop. Store data at buf
            {
                break;
            }

            std::string str(buf); // convert buf to a string
            if(connection_data -> last_message.has_value())
                connection_data -> last_message = (connection_data -> last_message.value()) + str;
            else
                connection_data -> last_message = str;

            // output string (debug)
            std::cout << "["<< connection_data -> DEBUG_peerarray_index << "]" << "["<< recvBytes << "]"<< str << " : " << connection_data -> last_message . value_or("<null>") << std::endl;

        }

        // Close peer connection before end of thread
        if(connection_data -> peer_fd != -1)
        {
            close(connection_data -> peer_fd);
            connection_data -> peer_fd = -1;
        }
        connection_data -> completed = true;
    }

    ///
    void Tracker::init()
    {
        this -> tracker_addr = sockaddr_in();
        this -> tracker_socket_fd = -1;

        this -> ip_index_mappings = map<string, int>();
        for(int i = 0; i < MAX_PEERS; i ++)
        {
            connected_peers[i] = nullopt;
        }
        this -> next_peer_index = new atomic<int>(0);

        this -> peer_listener = nullptr;
        this -> peer_connection_manager = nullptr;
        this -> killswitch = new atomic<bool>(false);
    }

    ///
    Tracker::Tracker()
    {
        this -> tracker_config = TrackerServerConfig{
            10101,
            "shared/"
        };
        init();
    }

    ///
    Tracker::Tracker(TrackerServerConfig trackerConfig)
    {
        this -> tracker_config = trackerConfig;
        init();
    }

    ///
    Tracker::~Tracker()
    {
        this -> close_server();

        this -> ip_index_mappings.clear();
        delete this -> next_peer_index;
        delete this -> killswitch;
    }

    ///
    bool Tracker::is_listening()
    {
        return this -> peer_listener != nullptr;
    }

    ///
    bool Tracker::try_init()
    {   
        if(this -> tracker_socket_fd >= 0)
            return false;

        cout << "Attempting to initialize tracker server..." << endl;
        addrinfo* addrList = make_connection_list(this -> tracker_config.port); 

        for(addrinfo* addr = addrList; addr; addr = addrList->ai_next)
        {
            // try the socket
            this -> tracker_socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if(this -> tracker_socket_fd < 0) continue;
        
            // get around the "bind address already in use"
            int optval = 1;
            setsockopt(this -> tracker_socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
            //setsockopt(this -> tracker_socket_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

            // try the bind
            int bind_status = bind(this -> tracker_socket_fd, addr->ai_addr, addr->ai_addrlen);
            if (bind_status >= 0) // success, save and stop going through the list
            { 
                this -> peer_connection_manager = new thread(&Tracker::manage_peer_connections, this);

                cout << "Successfully initialized tracker server." << endl;
                freeaddrinfo(addrList);
                return true;
            }
            close(this -> tracker_socket_fd); // failure, clean the socket and continue
        } // end for(...)

        this -> tracker_socket_fd = -1;
        cout << "Failed to initialize tracker server; no suitable socket found." << endl;
        return false;
    };

    ///
    bool Tracker::try_listen_for_peers(NET_ERROR_HANDLER on_error)
    {
        if(this -> tracker_socket_fd < 0)
        {
            cout << "Listening socket is not running." << endl;
            return false;
        }

        if(this -> peer_listener != nullptr)
        {
            cout << "Peer listener is already running." << endl;
            return false;
        }

        this -> peer_listener = new thread(&Tracker::listen_for_peers, this, on_error);
        return true;
    }

    ///
    void Tracker::close_server()
    {
        if(this -> tracker_socket_fd < 0)
            return;

        cout << "Closing tracker server..." << endl;
        this -> killswitch -> store(true);
        //shutdown(this -> tracker_socket_fd, SHUT_RD);

        // Wait for peer listening thread to close
        if(this -> peer_listener -> joinable())
        {
            this -> peer_listener -> join();
        }

        delete this -> peer_listener;
        this -> peer_listener = nullptr;

        // Wait for all peer connections to be terminated before closing the socket
        if(this -> peer_connection_manager -> joinable())
        {
            this -> peer_connection_manager -> join();
        }

        delete this -> peer_connection_manager;
        this -> peer_connection_manager = nullptr;

        this -> killswitch -> store(false);

        close(this -> tracker_socket_fd);
        this -> tracker_socket_fd = -1;
        cout << "Tracking server closed." << endl;
    }
}