#pragma once

#include "parameters.h"
#include "network/foxtunnel.h"
#include "eventpipe.h"

#include <condition_variable>
#include <map>
#include <vector>
#include <mutex>
#include <string>


/**
 * @brief Core class of the program.
 */
class Backend {
private:
    Socket receiver;

    std::vector<FoxTunnel> connections;
    std::mutex connectionsLock;

    std::atomic<bool> stopThread;

    volatile std::vector<std::string> messages;

    std::mutex threads;
    std::condition_variable aThreadFinished;
    std::exception_ptr error;

    EventPipe updateProcessing;
    EventPipe updateGUI;

    uint32_t nodeId;
    std::map<int, int> routingCache;

public:
    Backend(const Params& params);
    Backend(Backend&) = delete;
    Backend(Backend&&) = delete;

    void run();


private:
    void incomingConnectionsThread();
    void routingThread();
    void GUIThread();
    void endThread();

    void registerNewSocket(Socket&& socket);
    void sendRoutedPackage(FoxPackage::Header::NodeId receiver, const FoxPackage::Raw &data);
};
