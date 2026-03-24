#include "backend.h"

#include "network/foxpackage.h"

#include <cstring>
#include <sstream>
#include <thread>
#include <iostream>
#include <poll.h>


Backend::Backend(const Params &params) :
    nodeId(params.nodeId)
{
    std::cout << "### Starting the server... " << std::flush;

    receiver.init();
    receiver.bind(params.ip, params.port);

    std::cout << "succesful!\n" << std::flush;
}

void Backend::run() {
    std::unique_lock lock(threads);

    std::thread incomingConnections = std::thread([this]() { incomingConnectionsThread(); });
    std::thread routing = std::thread([this]() { routingThread(); });
    std::thread GUI = std::thread([this]() { GUIThread(); });
    aThreadFinished.wait(lock);

    receiver.shutdown(Socket::Shutdown::Both);
    updateProcessing.write();

    lock.unlock();
    GUI.join();
    routing.join();
    incomingConnections.join();

    if (error) {
        std::rethrow_exception(error);
    }
}

void Backend::incomingConnectionsThread() {
    struct pollfd pfd;
    pfd.fd = receiver.get();
    pfd.events = POLLIN;

    try {
        receiver.listen(5);

        while (!stopThread) {
            if (::poll(&pfd, 1, -1) == -1) {
                throw std::runtime_error(std::strerror(errno));
            }

            Socket newClient;

            newClient = receiver.accept();

            if (newClient.get() != -1) {
                registerNewSocket(std::move(newClient));
            }
        }
    }
    catch (...) {
        if (!stopThread) {
            std::lock_guard lock(threads);
            error = std::current_exception();
        }
    }

    endThread();
}

void Backend::routingThread() {
    while (!stopThread) {
        std::unique_lock lock(connectionsLock);
        std::vector<struct pollfd> fds(connections.size() + 1);
        for (int i = 0; i < connections.size(); i++) {
            fds[i].fd = connections[i].getSocket().get();
            fds[i].events = POLLIN;
        }
        auto& lastFd = fds.back();
        lastFd.fd = updateProcessing.getOutput();
        lastFd.events = POLLIN;

        lock.unlock();
        if (::poll(fds.data(), fds.size(), -1) == -1) {
            throw std::runtime_error("listener poll has failed");
        }


        if (lastFd.revents & POLLIN) {
            updateProcessing.reset();

            if (stopThread) break;
        }

        for (const auto& fd : fds) {
            if (fd.revents & POLLIN) {
                lock.lock();
                auto t = std::find_if(connections.begin(), connections.end(), [fd](const FoxTunnel& connection) {
                    return connection.getSocket().get() == fd.fd;
                });

                if (t == connections.end()) {
                    continue;
                } else if (t->getSocket().isClosed()) {
                    connections.erase(t);
                    continue;
                }
                lock.unlock();

                const auto raw = t->read();
                const auto header = FoxPackage::Header::deserialize(raw);

                switch (header.type) {
                case FoxPackage::Header::Message: {
                        FoxPackage::Message message(raw);

                        if (message.receiver == nodeId) {
                            std::cout << "### New message from " << message.sender << ":\n";
                            std::cout << message.message << std::endl;
                        } else {
                            std::cout << "### Transmiting message to " << message.receiver << ":\n";
                            sendRoutedPackage(message.receiver, raw);
                        }

                        break;
                    }
                case FoxPackage::Header::DiffieHellman: break;
                case FoxPackage::Header::Routing: {
                        FoxPackage::Routing routing(raw);

                        switch (routing.cmd) {
                        case FoxPackage::Routing::Reset:
                            if (routingCache.erase(routing.sender)) {

                                lock.lock();
                                for (auto& m : connections) {
                                    try {
                                        m.write(raw);
                                    } catch (...) {}
                                }
                                lock.unlock();

                                std::cout << "### Routing reset from: " << routing.sender << std::endl;
                            }
                            break;
                        case FoxPackage::Routing::Trace:
                            if (!routingCache.contains(routing.sender)) {
                                routingCache[routing.sender] = t->nodeId;

                                lock.lock();
                                for (auto& m : connections) {
                                    try {
                                        m.write(raw);
                                    } catch (...) {}
                                }
                                lock.unlock();

                                std::cout << "### Routing trace from:  " << routing.sender << std::endl;
                            }
                            break;
                        }

                        break;
                    }
                }
            }
        }
    }

    endThread();
}

void Backend::GUIThread() {
    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = updateGUI.getOutput();
    pfds[1].events = POLLIN;

    try {
        while (!stopThread) {
            int result = ::poll(pfds, 2, -1);
            if (stopThread) {
                break;
            } else if (result == -1) {
                throw std::runtime_error("Poll has failed!");
            }

            if (pfds[1].revents & POLLIN) { // Update console signal
                updateGUI.reset();
            }

            std::string input;
            std::getline(std::cin, input);

            if (input.starts_with("exit")) {
                stopThread = true;;
                break;
            }
            else if (input.starts_with("connect")) {
                std::stringstream ss(input);
                ss.ignore(input.size(), ' ');
                std::string ip(16, '\0');
                ss.getline(ip.data(), ip.size(), ':');
                std::string port(6, '\0');
                ss.getline(port.data(), port.size(), ' ');

                Socket newConnection;

                try {
                    newConnection.init();
                    newConnection.connect(ip, std::stoi(port));
                    registerNewSocket(std::move(newConnection));
                }
                catch (std::exception& e) {
                    std::cout << "!!! Error connecting to " << ip << ':' << port << "   " << e.what() << std::endl;
                    continue;
                }

                std::cout << "### Connection succesful!\n";
            }
            else if (input.starts_with("send")) {
                std::stringstream ss(input);
                ss.ignore(input.size(), ' ');
                std::string ids(4, '\0');
                ss.getline(ids.data(), ids.size(), ' ');
                std::string message;
                std::getline(ss, message);

                try {
                    int id = std::stoi(ids);

                    FoxPackage::Message m;
                    m.sender = nodeId;
                    m.receiver = id;
                    m.message = message;

                    sendRoutedPackage(id, m.serialize());
                } catch (std::exception& e) {
                    std::cout << "!!! Error sending to " << ids << e.what() << std::endl;
                    continue;
                }
            }
            else if (input.starts_with("listc")) {
                std::cout << "### Current connections:\n";

                std::lock_guard lock(connectionsLock);
                for (const auto& m : connections) {
                    std::cout << m.nodeId << '\n';
                }
            }
            else if (input.starts_with("trace")) {
                std::cout << "### Sending trace signal\n";

                FoxPackage::Routing routing{};
                routing.cmd = FoxPackage::Routing::Trace;
                routing.sender = nodeId;

                try {
                    auto routingRaw = routing.serialize();

                    std::lock_guard lock(connectionsLock);
                    for (auto& m : connections) {
                        try {
                        m.write(routingRaw);
                        } catch (...) {}
                    }
                } catch (std::exception& e) {
                    std::cout << "!!! Error sending trace signal: " << e.what() << std::endl;
                    continue;
                }
            }
            else if (input.starts_with("rtrace")) {
                std::cout << "### Sending reset trace signal\n";

                FoxPackage::Routing routing{};
                routing.cmd = FoxPackage::Routing::Reset;
                routing.sender = nodeId;

                try {
                    auto routingRaw = routing.serialize();

                    std::lock_guard lock(connectionsLock);
                    for (auto& m : connections) {
                        try {
                            m.write(routingRaw);
                        } catch (...) {}
                    }
                } catch (std::exception& e) {
                    std::cout << "!!! Error sending reset trace signal: " << e.what() << std::endl;
                    continue;
                }
            }
            else {
                std::cout << "### Uncnown command\n";
            }

            std::cout << std::flush;
        }
    }
    catch (...) {
        if (!stopThread) {
            std::lock_guard lock(threads);
            error = std::current_exception();
        }
    }

    endThread();
}

void Backend::endThread() {
    stopThread = true;
    aThreadFinished.notify_all();
}

void Backend::registerNewSocket(Socket&& socket) {
    socket.setKeepAlive(true, 10);
    socket.setOperationTimeout(5);

    FoxTunnel newTunnel;
    newTunnel.setSocket(std::move(socket));

    newTunnel.handshake(nodeId);

    std::lock_guard lock(connectionsLock);
    connections.emplace_back(std::move(newTunnel));

    updateProcessing.write();

    std::cout << "### New connection with ID: " << newTunnel.nodeId << std::endl;
}

void Backend::sendRoutedPackage(FoxPackage::Header::NodeId receiver, const FoxPackage::Raw& data) {
    std::unique_lock lock(connectionsLock);

    auto it = std::find_if(connections.begin(), connections.end(),
                           [receiver](const FoxTunnel &entry) {
                               return entry.nodeId == receiver;
                           });

    if (it != connections.end()) {
        it->write(data);

        std::cout << "### Sent succesful!" << std::endl;

        return;
    }

    if (routingCache.contains(receiver)) {
        const auto routeId = routingCache[receiver];
        it = std::find_if(connections.begin(), connections.end(),
                          [routeId](const FoxTunnel &entry) {
                              return entry.nodeId == routeId;
                          });

        if (it != connections.end()) {
            it->write(data);

            std::cout << "### Routing succesful!" << std::endl;

            return;
        }
        else {
            std::cout << "### Routing has expired" << std::endl;
            routingCache.erase(receiver);

            return;
        }
    }

    std::cout << "### No know path to the target" << std::endl;
}
