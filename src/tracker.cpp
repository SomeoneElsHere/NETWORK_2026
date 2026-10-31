#include <cstring>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cerrno>
#include <atomic>
#include <functional>
#include <optional>
#include <map>
#include <arpa/inet.h>
#include "lib/util.hpp"
// TOTAL TODOS
// Line 21: Verify peersockets and peerinput has proper string data
// Line 52: Add Multithreading to process multiple peers at the same time

class Tracker
{    
    public:
        /**
         * The default port used by the tracker
         */
        static constexpr int16_t TRACKER_PORT = 10101;

        /**
         * The maximum number of peers that can be connected to a tracker server concurrently
         */
        static constexpr int16_t MAX_PEERS = 256;

    private:
        static constexpr int16_t MAX_MESSAGE_BUFFER_SIZE = 1024;

        struct peer_connection
        {
            const int peer_fd;
            const string peer_address;
            optional<string> last_message = nullopt;
            thread* worker_thread = nullptr;
            bool completed = false;

            int DEBUG_peerarray_index;
        };

        // Tracker server properties
        sockaddr_in tracker_addr;
        int tracker_socket_fd;

        // Per-peer management data
        map<string, int> ip_index_mappings;
        optional<peer_connection*> connected_peers[MAX_PEERS];
        atomic<int>* next_peer_index;
        thread* peer_connection_manager;
        atomic<bool>* killswitch;

        /**
         * Thread task used to manage peer connections and their threads
         */
        void manage_peer_connections()
        {
            cout << "Peer management thread activated." << endl;

            mutex guardian;
            bool foundActiveConnection = true;

            while(!(this -> killswitch -> load()) || foundActiveConnection)
            {
                foundActiveConnection = false;
                for(int i = 0; i < MAX_PEERS; i ++)
                {
                    if(connected_peers[i].has_value())
                    {
                        foundActiveConnection = true;

                        // Delete peer connection data and free slot
                        // Use mutex to prevent access from main thread
                        guardian.lock();
                        peer_connection* connection = connected_peers[i].value();
                        ip_index_mappings.erase(connection -> peer_address);
                        if(connection -> completed)
                        {
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

        /**
         * Initializes a peer connection and starts a worker thread for it
         * @param ipv4 The IPv4 address of the peer
         */
        void open_peer_connection(int peer_fd, sockaddr_in peer_ipv4)
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

        /**
         * Receives messages from peer until connection closes
         * @param connection_data A pointer to the connection data for the peer this worker thread is for
         */
        void peer_worker_task(peer_connection* connection_data)
        {
            char buf[MAX_MESSAGE_BUFFER_SIZE];
            while (!this -> killswitch -> load())
            {
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
            close(connection_data -> peer_fd);
            connection_data -> completed = true;
        }

    public:
        /**
         * Creates a new tracker server
         */
        Tracker()
        {
            this -> tracker_addr = sockaddr_in();
            this -> tracker_socket_fd = -1;

            this -> ip_index_mappings = map<string, int>();
            for(int i = 0; i < MAX_PEERS; i ++)
            {
                connected_peers[i] = nullopt;
            }
            this -> next_peer_index = new atomic<int>(0);

            this -> killswitch = new atomic<bool>(false);
        }

        /**
         * Clean up resources used by this tracker
         */
        ~Tracker()
        {
            this -> close_socket();

            this -> ip_index_mappings.clear();
            delete this -> next_peer_index;
            delete this -> killswitch;
        }

        /**
         * Attempts to initialize the socket this tracker server will use
         * @param port The port the tracker server will listen through
         * @returns true if the tracker server is ready to listen; otherwise, false
         */
        bool try_init(const char* port)
        {   
            if(this -> tracker_socket_fd >= 0)
                return false;

            cout << "Attempting to initialize tracker server..." << endl;
            struct addrinfo* addrList = make_connection_list(port); 
            for(struct addrinfo* addr = addrList; addr; addr = addrList->ai_next)
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

            cout << "Failed to initialize tracker server; no suitable socket found." << endl;
            return false;
        }

        /**
         * Listens for peers and creates worker threads when a connection with a peer is established
         */
        bool try_listen_for_peers(int* error_code)
        {
            if(this -> tracker_socket_fd < 0)
                return false;

            int p = listen(this -> tracker_socket_fd, 5); // listen for max 5 clients in que. This prepares the FD to accept after connect from peer
            if (p == -1)
            {
                if(error_code != nullptr)
                    *error_code = errno;
                return false;
            }

            cout << "Listening for peers..." << endl;
            while(!(this -> killswitch -> load()))
            {
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

                if (peer_fd == -1)
                {
                    if(error_code != nullptr)
                        *error_code = errno;
                    return false;
                }

                sockaddr_in IPV4 = (sockaddr_in)sockaddr_client;
                this -> open_peer_connection(peer_fd, IPV4);
            }

            // Killswitch activated; no need to continue listening
            return true;
        }

        /**
         * Closes all existing connections with peers and shuts down the tracker server
         */
        void close_socket()
        {
            cout << "Closing tracker server..." << endl;
            if(this -> tracker_socket_fd < 0)
                return;

            this -> killswitch -> store(true);
            
            if(this -> peer_connection_manager -> joinable()) {
                this -> peer_connection_manager -> join();
            }

            delete this -> peer_connection_manager;
            this -> peer_connection_manager = nullptr;
            this -> killswitch -> store(false);

            close(this -> tracker_socket_fd);
            this -> tracker_socket_fd = -1;
            cout << "Tracking server closed." << endl;
        }
};

//class tracker
//{
//public:
//    sockaddr_in TrackerAddr;
//    int TrackerSocket = 0;
//    // insert an empty string in the socket/ array if they leave pls :D - Natalie
//    
//
//    std::map<std::string, std::string> PeerInputStore = std::map<std::string, std::string>();
//    /*
//    PeerInputStore stores the data of the file per IP with a IP->File lookup
//    */
//   int PeerSockets[256];
//    /*
//    Peersockets is a vector with every single peer fd that is connected. For ease of tracking in the forloops,
//    change the disconeccted client from a fd to 0
//    */
//    std::string  PeerInput[256];
//    /*
//    PeerInput is a vector with strings. The nth string is connected to the nth peer and so on. The nth peer will
//    add onto the nth string. If the nth peer disconnects, put the string as empty so that a new peersocket can use it.
//    Can change to a binary format if needed.
//    */
//    
//    struct acceptArgs
//    {
//        std::string IPADDR;
//        int index;
//    };
//
//    void CreateTracker(const char* port) // tracker profile (see peer)
//    {
//        // while going through the connection list
//        struct addrinfo* addrList = make_connection_list(port); 
//        for(struct addrinfo* addr = addrList; addr; addr = addrList->ai_next)
//        {
//            // try the socket
//            TrackerSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
//            if(TrackerSocket < 0) continue;
//          
//            // get around the "bind address already in use"
//            int optval = 1;
//            setsockopt(TrackerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
//            setsockopt(TrackerSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
//
//            // try the bind
//            int bind_status = bind(TrackerSocket, addr->ai_addr, addr->ai_addrlen);
//            if (bind_status >= 0) // success, save and stop going through the list
//            { 
//                freeaddrinfo(addrList);
//                return;
//            }
//            close(TrackerSocket); // failure, clean the socket and continue
//        } // end for(...)
//        fprintf(stderr, "ERROR %d - could not create and bind to any network sockets -> %s:%d\n", errno, __FILE__, __LINE__);
//    }
//
//    void Create()
//    {
//        for(int i = 0; i< 256; i++)
//        {
//            PeerSockets[i] = 0;
//            PeerInput[i] = "";
//        }
//        CreateTracker("10101"); // Create tracker socket to recieve (IPv4,TCP,automatic protocol)
//    }
//    void Listen() 
//    {
//        int p = listen(TrackerSocket, 5); // listen for max 5 clients in que. This prepares the FD to accept after connect from peer
//        
//        if (p == -1)
//        {
//            std::cout << "Failure to listen: " << errno << std::endl;
//            exit(-1);
//        }
//        while (true)
//        {
//            fd_set fds;
//            FD_ZERO(&fds);
//            FD_SET(TrackerSocket, &fds);
//
//            //timeout (needed or select will stall)
//            struct timeval tm;
//            tm.tv_usec = 100;
//            tm.tv_sec = 0;
//
//            int sel = select(TrackerSocket+1,&fds, NULL,NULL,&tm); //put timeout in final para
//            if(sel <= 0)
//            {
//                continue;
//            }
//
//            
//            int index = 0;
//            for (index = 0; index < 256; index++) // get first empty peersocket
//            {
//                
//                if (PeerSockets[index] == 0)
//                {
//                    break;
//                }
//            }
//            if(index == 256)
//            {
//                
//                continue;
//            }
//            
//            
//            sockaddr_in sockaddr_client; //where the addr of an IP from a client is stored
//            sockaddr_client.sin_family = AF_INET;
//            in_addr addr;
//            addr.s_addr = INADDR_ANY;
//            sockaddr_client.sin_addr = addr;
//            sockaddr_client.sin_port = TrackerAddr.sin_port;
//            socklen_t sockklen;
//            sockklen = 4;
//            
//            
//
//            int q = accept(TrackerSocket,(struct sockaddr*)&sockaddr_client, &sockklen); // accept any addr,  q is peer fd
//            if (q == -1)
//            {
//                std::cout << "Accept failure: " << errno << std::endl;
//                exit(-2);
//            }
//
//            
//            sockaddr_in IPV4 = (sockaddr_in)sockaddr_client;
//            char ipaddr[INET_ADDRSTRLEN];
//            inet_ntop( AF_INET, &(IPV4.sin_addr), ipaddr, INET_ADDRSTRLEN ); //get ip as char*
//            
//            std::string Ipaddrstring(ipaddr); 
//            try
//            {
//                PeerInputStore.at(Ipaddrstring); //does the IP already exist?
//                
//            }
//            catch(...)
//            {
//                PeerInputStore[Ipaddrstring] = ""; //if not, create new handle for IP
//            }
//            
//
//            
//            // i = 0; //debug for 1 peer
//            PeerSockets[index] = q; // insert the ith peer fd at the last index.
//            acceptArgs arg;
//            arg.index = index;
//            arg.IPADDR = Ipaddrstring;
//            std::thread t = std::thread(&tracker::Accept,this,arg);
//            
//            t.detach(); //let it do its thang
//            
//        }
//    }
//    void Accept(acceptArgs arg)
//    {
//        int n = arg.index;
//        char buf[1024];
//        while (true)
//        {
//            int recvBytes = recv(PeerSockets[n], buf, sizeof(buf), 0);
//            if (recvBytes <= 0) // if failure to recieve stop. Store data at buf
//            {
//                close(PeerSockets[n]);
//                
//                PeerInputStore[arg.IPADDR] = PeerInputStore[arg.IPADDR] + PeerInput[n];
//                PeerInput[n] = "";
//                return;
//            }
//            
//            std::string str(buf); // convert  buf to a string
//            PeerInput[n] = PeerInput[n]+str;
//            // output string (debug)
//            std::cout << "["<< n << "]" << "["<< recvBytes << "]"<< str << " : " << PeerInputStore[arg.IPADDR] << std::endl;
//            
//        }
//        
//    }
//};

int main()
{
    Tracker tracker;
    if(!tracker.try_init("10101"))
    {
        cout << "Could not open a socket for listening. " << endl;
        return 1;
    }
    
    int error_code = 0;
    if(!tracker.try_listen_for_peers(&error_code))
    {
        cout << "A problem occurred while the tracker server was listening. Error code: " << error_code << endl;
        return 2;
    }

    tracker.close_socket();

    //peer.send_to_tracker("Test", 4);
    //peer.close_socket();
    
 //tracker t;
 //t.Create(); // create the tracker socket
 //t.Listen(); // listen for the peer connection and recieve data
 //// t.Accept(0); debug
 //
 //return 0;
}
