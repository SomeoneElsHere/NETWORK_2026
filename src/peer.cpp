#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <sys/socket.h>
#include <errno.h>
#include <typeindex>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cerrno>
#include <string>

#include "common.hpp"

using namespace std;
using namespace peer_to_peer;

/**
 * A set of configuration values relating to a peer's client thread
 */
typedef struct {
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
static optional<PeerClientConfig> try_parse_peer_client_config(string filename)
{
    ifstream fileStream(filename);
    if(!fileStream.is_open())
    {
        // Failed to open the specified file for reading
        return nullopt;
    }
    int intPort, intUpdateInterval;
    in_addr_t convertedAddr;
    try
    {
        string strPort, strAddr, strUpdateInterval;

        // Port Number
        getline(fileStream, strPort);
        intPort = stoi(strPort);

        // IP Address
        getline(fileStream, strAddr);
        convertedAddr = inet_addr(strAddr.c_str());
        if(convertedAddr == -1)
            throw 0;

        // Update Interval
        getline(fileStream, strUpdateInterval);
        intUpdateInterval = stoi(strUpdateInterval);
        fileStream.close();
    }
    catch (...) 
    {
        // One of the options could not be parsed correctly
        fileStream.close();
        return nullopt;
    }

    sockaddr_in tracker_addr;
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(TRACKER_PORT);
    tracker_addr.sin_addr.s_addr = (in_addr_t)convertedAddr;
    
    return PeerClientConfig{
        tracker_addr,
        intUpdateInterval
    };
}

class Peer
{
    private:
        /// The configuration values for the client thread of this peer
        PeerClientConfig client_config;

        /// The socket used for communicating with the tracker server
        int tracker_socket_fd;

        /**
         * Checks if a fail condition passed, and if it did, sets an error code pointer if one was specified
         * @param fail_condition The result of the fail condition
         * @param error_code The memory location to store the error code, if the fail condition passes
         * @return false if fail_condition is true; otherwise, true
         */
        static bool check_for_error(bool fail_condition, int* error_code)
        {
            if(fail_condition)
            {
                if(error_code != nullptr)
                    *error_code = errno;

                return false;
            }

            return true;
        }
        
    public:
        /**
         * Creates a new peer with the default client thread configuration
         */
        Peer()
        {
            sockaddr_in default_addr;

            default_addr.sin_family = AF_INET;
            default_addr.sin_port = htons(TRACKER_PORT);
            default_addr.sin_addr.s_addr = INADDR_ANY;

            this -> client_config = PeerClientConfig{
                default_addr,
                1
            };
            this -> tracker_socket_fd = -1;
        }

        /**
         * Creates a new peer with a custom client thread configuration
         * @param clientConfig The configuration values for the client thread
         */
        Peer(PeerClientConfig clientConfig)
        {
            this -> client_config = clientConfig;
            this -> tracker_socket_fd = -1;
        }

        /**
         * Clean up resources used by this peer
         */
        ~Peer()
        {
            if(this -> tracker_socket_fd >= 0)
                this -> close_socket();
        }

        /**
         * Attempts to initialize the socket and tracker address this peer will use
         * @param error_code The memory location to store the error code, if creating the socket fails
         * @return true if creation of the socket was successful; otherwise, false
         */
        bool try_init_socket(int* error_code)
        {
            if(this -> tracker_socket_fd >= 0)
                return false;

            tracker_socket_fd = socket(
                AF_INET,
                SOCK_STREAM,
                0
            );

            return check_for_error(tracker_socket_fd < 0, error_code);
        }

        /**
         * Attempts to connect to the tracker server using the created socket
         * @param error_code The memory location to store the error code, if the connection attempt fails
         * @return true if connection was successful; otherwise, false
         */
        bool try_connect(int* error_code)
        {
            if(this -> tracker_socket_fd < 0)
                return false;

            sockaddr* tracker_socket_addr = (struct sockaddr*)(&this -> client_config.tracker_addr);

            int success = connect(
                this -> tracker_socket_fd,
                tracker_socket_addr,
                sizeof(this -> client_config.tracker_addr)
            );

            return check_for_error(success == -1, error_code);
        }

        /**
         * Attempts to send a string message to the tracker
         * @param message The message to be sent
         * @param len The length of the message
         * @param error_code The memory location to store the error code, if the send attempt fails
         * @return true if the message was successfully sent; otherwise, false
         */
        bool try_send_to_tracker(std::string message, int len, int* error_code)
        {
            if(this -> tracker_socket_fd < 0)
                return false;

            std::string content = (std::to_string(len) +":"+ message);
            size_t sentBytes = send_string(
                this -> tracker_socket_fd, 
                content
            );

            return check_for_error(sentBytes == -1, error_code);
        }

        /**
         * Closes the file descriptor for the socket connected to the tracker server, if its open
         */
        void close_socket()
        {
            if(this -> tracker_socket_fd < 0)
                return;

            close(this -> tracker_socket_fd);
            this -> tracker_socket_fd = -1;
        }
};

//class peer
//{
//    public:
//    sockaddr_in TrackerAddr;
//    int PeerSocket = 0;
//    void CreateTracker() //Creates the socket profile at 9999
//    {
//        TrackerAddr.sin_family = AF_INET; //Family is IPv4
//        TrackerAddr.sin_port = htons(10101); //Socket is 9999
//        TrackerAddr.sin_addr.s_addr = INADDR_ANY;// IP addr is any ip addr
//        
//    }
//    void Create() //Sets the Peersocket and creates the socket profile for tracker
//    {
//        PeerSocket = 0;
//        PeerSocket = socket(AF_INET, SOCK_STREAM, 0); //Socket: same family, TCP, and 0 for automatic protocol
//        if (PeerSocket < 0)
//        {
//            std::cout << "No sock juj: " << errno;
//            exit(-2);
//        }
//        CreateTracker();
//    }
//    void Connect() //Connects to the socket.
//    {
//       //std::cout << PeerSocket;
//       int q = connect(PeerSocket, (struct sockaddr*)&TrackerAddr, sizeof(TrackerAddr));  //Connect: FD for sender, casted sockaddr of reciver, size of sockaddr_in reciver
//       if(q == -1)
//       {
//        std::cout << "Failure to connect: " << errno << std::endl << PeerSocket;
//       }
//    }
//        
//    void Send(std::string str, int len)
//    {
//        std::string s = (std::to_string(len) +":"+ str);
//        const char *c = s.c_str(); //convert to byte
//        //std::cout << "sent message" << std::endl;
//        send(PeerSocket,c, s.length(),0); //send bytes (fd, pointer, length, no flags)
//    }
//};

const string commandstr_exit = "exit";

int main()
{
    optional<PeerClientConfig> clientConfig = try_parse_peer_client_config("./clientThreadConfig.cfg");
    if(!clientConfig.has_value())
    {
        cout << "The specified clientThreadConfig file could not be read. Does it exist, and is it formatted properly?" << endl;
        return 1;
    }

    int error_code = 0;
    Peer peer(clientConfig.value());
    if(!peer.try_init_socket(&error_code))
    {
        cout << "Could not open a socket for connection. Error code: " << error_code << endl;
        return 2;
    }
    
    if(!peer.try_connect(&error_code))
    {
        cout << "Connection could not be successfully established. Error code: " << error_code << endl;
        return 3;
    }

    cout << "Connection successfully established." << endl;
    
    string message;
    bool sentSuccess;
    do {
        message.clear();
        getline(cin, message);
        if(message == commandstr_exit)
            break;

        try
        {
            sentSuccess = peer.try_send_to_tracker(message, message.length() - 1, &error_code);
            if(!sentSuccess)
                cout << "Message failed to send. Error code: " << error_code << endl;
        }
        catch (...) {
            sentSuccess = false;
            cout << "An exception was thrown while trying to send message." << endl;
        }
    }
    while(sentSuccess);
    peer.close_socket();

    cout << "Socket closed." << endl;

    //peer p;
    //p.Create();
    //p.Connect();
    ////p.Send(" ",1);
    //p.Send("Hiy",3);
    //close(p.PeerSocket); //close fd. Remove this if you want to send something else.
    //
    //peer q;
    //q.Create();
    //q.Connect();
    //q.Send("abc",3);
    //q.Send("def",3);
    //close(q.PeerSocket); //close fd. Remove this if you want to send something else.
    
    return 0;
}