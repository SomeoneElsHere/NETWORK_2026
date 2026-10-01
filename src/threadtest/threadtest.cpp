#include <iostream>
#include <thread>
#include <memory>
#include <atomic>
#include <chrono>

#include "threadtest.h"

using namespace std;

/* Implementations of ThreadTest methods */
ThreadTest::ThreadTest()
{
    this -> clientThread = nullptr;
    this -> serverThread = nullptr;

    /* Create thread-safe(?) flags used 
       to request each thread to stop */
    this -> clientStop = new atomic<bool>(false);
    this -> serverStop = new atomic<bool>(false);
}

ThreadTest::~ThreadTest()
{
    /* Ensure client and server threads are stopped and cleaned up */
    this -> stopClientThread();
    this -> stopServerThread();

    /* Dispose of stop flags since they are no longer needed */
    delete this -> clientStop;
    delete this -> serverStop;
}

bool ThreadTest::isClientRunning() { return this -> clientThread != nullptr; }
bool ThreadTest::isServerRunning() { return this -> serverThread != nullptr; }

const chrono::milliseconds TIME_HEARTBEAT = chrono::milliseconds(2500);

void ThreadTest::clientTask()
{
    cout << "Hello from client thread!\n";

    /* Loop infinitely until client stop is requested */
    while(!(this -> clientStop -> load()))
    {
        this_thread::sleep_for(TIME_HEARTBEAT);
        cout << "Heartbeat from client thread.\n";
        this_thread::sleep_for(TIME_HEARTBEAT);
    }

    cout << "Goodbye from client thread!\n";
}

void ThreadTest::serverTask()
{
    cout << "Hello from server thread!\n";

    /* Loop infinitely until server stop is requested */
    while(!(this -> serverStop -> load()))
    {
        this_thread::sleep_for(TIME_HEARTBEAT);
        cout << "Heartbeat from server thread.\n";
        this_thread::sleep_for(TIME_HEARTBEAT);
    }
    cout << "Goodbye from server thread!\n";
}

void ThreadTest::runClientThread()
{
    /* Only create thread if client thread is not running already */
    if(!this -> isClientRunning()) {
        this -> clientThread = new thread(&ThreadTest::clientTask, this);
        cout << "Client thread started!\n";
    }
}

void ThreadTest::runServerThread()
{
    /* Only create thread if server thread is not running already */
    if(!this -> isServerRunning()) {
        this -> serverThread = new thread(&ThreadTest::serverTask, this);
        cout << "Server thread started!\n";
    }
}

void ThreadTest::stopClientThread()
{
    if(this -> isClientRunning())
    {
        cout << "Stopping client thread...\n";
        /* Request client thread to stop */
        this -> clientStop -> store(true);

        /* Wait for client thread to conclude */
        if(this -> clientThread -> joinable()) {
            this -> clientThread -> join();
        }
        
        /* Clean up and reset resources 
           related to client thread */
        delete this -> clientThread;
        this -> clientThread = nullptr;
        this -> clientStop -> store(false);
        cout << "Client thread stopped.\n";
    }
}

void ThreadTest::stopServerThread()
{
    if(this -> isServerRunning())
    {
        cout << "Stopping server thread...\n";
        /* Request server thread to stop */
        this -> serverStop -> store(true);

        /* Wait for server thread to conclude */
        if(this -> serverThread -> joinable()) {
            this -> serverThread -> join();
        }

        /* Clean up and reset resources 
           related to server thread */
        delete this -> serverThread;
        this -> serverThread = nullptr;
        this -> serverStop -> store(false);
        cout << "Server thread stopped.\n";
    }
}