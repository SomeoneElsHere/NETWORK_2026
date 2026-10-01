#pragma once

#include <iostream>
#include <thread>
#include <atomic>
#include <memory>

using namespace std;

/**
 * @brief This is a class for testing threads
 */
class ThreadTest
{
    private:
        thread* clientThread;
        thread* serverThread;

        atomic<bool>* clientStop;
        atomic<bool>* serverStop;

        void clientTask();
        void serverTask();

        /* Prevent copying this object */
        ThreadTest(const ThreadTest& original) = delete;
        ThreadTest& operator=(const ThreadTest& copy) = delete;

    public:
        /**
         * @brief Constructs a new object for testing thread
         */
        ThreadTest();

        /**
         * @brief Cleans up this object
         */
        ~ThreadTest();

        /**
         * @brief Checks if this object is running a client-side thread
         * @returns True if the client-side thread is running; otherwise, false
         */
        bool isClientRunning();

        /**
         * @brief Checks if this object is running a server-side thread
         * @returns True if the server-side thread is running; otherwise, false
         */
        bool isServerRunning();

        /**
         * @brief Runs the client-side thread
         */
        void runClientThread();

        /**
         * @brief Runs the server-side thread
         */
        void runServerThread();

        /**
         * @brief Stops the client-side thread
         */
        void stopClientThread();

        /**
         * @brief Stops the server-side thread
         */
        void stopServerThread();
};