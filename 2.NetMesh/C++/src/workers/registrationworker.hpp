#pragma once

#include <mutex>
#include <sys/poll.h>

#include "utils/socket.h"
#include "workers/worker.h"
#include "workers/threadssharedstate.hpp"



template<class T>
struct RegistrationWorker : public Worker {
public:
    struct initial {
        Socket::port_t listenPort;
    } initial;

    void operator()() {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        Socket listenSocket;
        listenSocket.init();
        listenSocket.bind("0.0.0.0", initial.listenPort);
        listenSocket.listen(5);

        std::array<pollfd, 2> fds{};
        auto& pipeFd = fds[0];
        pipeFd.fd = m_commandPipe.getOutput();
        pipeFd.events = POLLIN;
        auto& listenFd = fds[1];
        listenFd.fd = listenSocket.descriptor();;
        listenFd.events = POLLIN;


        while (true) {
            std::unique_lock stateLock{ss.serverStateInUse, std::defer_lock};

            const int result = ::poll(fds.data(), fds.size(), -1);
            if (result == -1) {
                const std::error_code error(errno, std::generic_category());

                if (error == std::errc::interrupted)
                    continue;

                throw std::system_error(error, "RegistrationWorker::poll");
            }

            bool restart = false;
            if (pipeFd.revents & POLLIN) {
                while (auto commandOpt = tryGetCommand()) {
                    switch (commandOpt.value()) {
                    case Command::FinishThread:
                        listenSocket.shutdown();
                        listenSocket.close();
                        return;
                    case Command::ResetWorker:
                        restart = true; break;
                    default:
                        break;
                    }
                }
            }
            if (restart) continue;

            if (listenFd.revents & POLLIN) {
                try {
                    Socket client = listenSocket.accept();
                    client.setTimeout(std::chrono::seconds(5));
                    client.setKeepAlive(std::chrono::seconds(10),
                                        std::chrono::seconds(5),
                                        2);

                    stateLock.lock();
                    ss.serverState.connectionsState.emplace_back(std::move(client));

                    static_cast<T&>(*this).getPackageWorker().sendCommand(Command::ResetWorker);
                }
                catch (const SocketException& e) {
                    const std::error_code ec = e.code();

                    if (ec == std::errc::interrupted ||
                        ec == std::errc::resource_unavailable_try_again ||
                        ec == std::errc::connection_aborted ||
                        ec == std::errc::network_down ||
                        ec == std::errc::protocol_error ||
                        ec == std::errc::no_protocol_option ||
                        ec == std::errc::host_unreachable ||
                        ec == std::errc::operation_not_supported ||
                        ec == std::errc::network_unreachable
                    ) {
                        continue;
                    } else {
                        throw;
                    }
                }
            }
        }
    }
};
