#pragma once

#include <cstring>
#include <netinet/in.h>
#include <ranges>
#include <sys/poll.h>

#include "server/protocols.h"
#include "server/serverstate.h"
#include "workers/worker.h"
#include "utils/socket.h"
#include "workers/threadssharedstate.hpp"



template<class T>
struct PackageProcessorWorker : public Worker {
public:
    void operator()() {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        while (true) {
            std::unique_lock stateLock{ss.serverStateInUse};
            std::vector<pollfd> fds(ss.serverState.connectionsState.size() + 1);

            auto& pipeFd = fds[0];
            pipeFd.fd = m_commandPipe.getOutput();
            pipeFd.events = POLLIN;
            auto clientsFds = std::span(fds).subspan(1);

            for (auto [fd, client] : std::views::zip(clientsFds, ss.serverState.connectionsState)) {
                fd = {
                    .fd = static_cast<int>(client.socket.descriptor()),
                    .events = POLLIN | POLLERR,
                    .revents = 0
                };
            }

            stateLock.unlock();

            const int result = ::poll(fds.data(), fds.size(), -1);
            if (result == -1) {
                const std::error_code error(errno, std::generic_category());

                if (error == std::errc::interrupted)
                    continue;

                throw std::system_error(error, "PackageProcessorWorker::poll");
            }

            bool restart = false;
            if (pipeFd.revents & POLLIN) {
                while (auto commandOpt = tryGetCommand()) {
                    switch (commandOpt.value()) {
                    case Command::FinishThread:
                        return;
                    case Command::ResetWorker:
                        restart = true; break;
                    default:
                        break;
                    }
                }
            }
            if (restart) continue;

            std::vector<Socket::descriptor_t> toDelete;

            stateLock.lock();
            // If fds and clients aren't aligned, we rebuild fds to remove any desyncs. Quick check.
            if (clientsFds.size() != ss.serverState.connectionsState.size()) continue;

            for (auto [fd, clientState] : std::views::zip(clientsFds, ss.serverState.connectionsState)) {
                // If fds and clients aren't aligned, we rebuild fds to remove any desyncs. Slow check.
                if (fd.fd != clientState.socket.descriptor()) break;

                if (fd.revents & (POLLIN | POLLERR)) {
                    try {
                        if (fd.revents & POLLERR) {
                            throw SocketException(clientState.socket.getError(), "PackageProcessorWorker");
                        }

                        Protocols::PacketSize_t packetSize;
                        clientState.socket.read(std::as_writable_bytes(std::span{&packetSize, 1}));
                        packetSize = ntohl(packetSize);

                        std::vector<std::byte> buffer(packetSize + sizeof(packetSize));
                        clientState.socket.read(std::span(buffer).subspan(sizeof(packetSize)));

                        std::memcpy(
                            buffer.data(),
                            &packetSize,
                            sizeof(packetSize)
                        );

                        Protocols::PacketType protocol = Protocols::unpack(buffer, clientState.encryptor);

                        switch (protocol) {
                        case Protocols::PacketType::UserHandshake_Start:
                            processUserHandshakeStart(clientState, buffer); break;
                        case Protocols::PacketType::UserHandshake_Accept:
                            processUserHandshakeAccept(clientState, buffer); break;
                        case Protocols::PacketType::TextMessage_Message:
                            processNewMessage(clientState, buffer); break;
                        }
                    }
                    catch (const SocketException& e) {
                        const auto code = e.code();

                        if (code == std::errc::connection_reset ||
                            code == std::errc::connection_aborted ||
                            code == std::errc::not_connected ||
                            code == std::errc::timed_out ||
                            code == std::errc::resource_unavailable_try_again
                        ) {
                            toDelete.push_back(fd.fd);
                        } else {
                            throw;
                        }
                    }

                    break;
                }
            }

            auto connIter = ss.serverState.connectionsState.begin();
            for (Socket::descriptor_t desc : toDelete) {
                while (true) {
                    if (connIter->socket.descriptor() == desc) {

                        try {
                            connIter->socket.shutdown();
                        }
                        catch (const std::system_error& e) {
                            if (e.code() != std::errc::not_connected) throw;
                        }

                        connIter->socket.close();

                        if (connIter->linkedChat) {
                            connIter->linkedChat.value()->linkedConnection = std::nullopt;
                        }

                        connIter = ss.serverState.connectionsState.erase(connIter);

                        break;
                    }
                    else {
                        connIter++;
                    }
                }
            }

            if (!toDelete.empty()) {
                static_cast<T&>(*this).getTUIWorker().sendCommand(Worker::Command::ResetWorker);
            }
        }
    }

private:
    void processUserHandshakeStart(ConnectionState& connState, Protocols::RawPayloadSpan_t package) {
        auto remoteData = Protocols::UserHandshake::retrive(package);
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        connState.encryptor.initialize(true);

        Protocols::RawPayload_t buff = Protocols::UserHandshake::start({.pubKey = connState.encryptor.getPublickKey(), .name = ss.serverState.clientName});
        Protocols::Packet_t packet = Protocols::pack(Protocols::PacketType::UserHandshake_Accept, buff, connState.encryptor);
        connState.socket.write(packet);

        auto chatIter = std::ranges::find_if(ss.serverState.chats.begin(), ss.serverState.chats.end(), [&remoteData](const Chat& chat) {
            return chat.userName == remoteData.name;
        });

        if (chatIter != ss.serverState.chats.end()) {
            chatIter->linkedConnection = &connState;
            connState.linkedChat = &(*chatIter);
        }
        else {
            Chat newChat{
                .userName = remoteData.name,
                .linkedConnection = &connState
            };

            connState.linkedChat = &ss.serverState.chats.emplace_back(std::move(newChat));
        }

        connState.encryptor.pair(remoteData.pubKey);
        static_cast<T&>(*this).getTUIWorker().sendCommand(Worker::Command::ResetWorker);
    }

    void processUserHandshakeAccept(ConnectionState& connState, Protocols::RawPayloadSpan_t package) {
        auto remoteData = Protocols::UserHandshake::retrive(package);
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        auto chatIter = std::ranges::find_if(ss.serverState.chats.begin(), ss.serverState.chats.end(), [&remoteData](const Chat& chat) {
            return chat.userName == remoteData.name;
        });

        if (chatIter != ss.serverState.chats.end()) {
            chatIter->linkedConnection = &connState;
            connState.linkedChat = &(*chatIter);
        }
        else {
            Chat newChat{
                .userName = remoteData.name,
                .linkedConnection = &connState
            };

            connState.linkedChat = &ss.serverState.chats.emplace_back(std::move(newChat));
        }

        connState.encryptor.pair(remoteData.pubKey);
        static_cast<T&>(*this).getTUIWorker().sendCommand(Worker::Command::ResetWorker);
    }

    void processNewMessage(ConnectionState& connState, Protocols::RawPayloadSpan_t package) {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        std::string message = Protocols::TextMessage::readMessage(package);

        if (!connState.linkedChat) [[unlikely]] throw std::logic_error("Any socket accepting messages must have attached chat!");

        connState.linkedChat.value()->history.push_back({.own = false, .text = message});

        if (ss.serverState.currentChat != connState.linkedChat.value()) {
            connState.linkedChat.value()->allRead = false;
        }

        static_cast<T&>(*this).getTUIWorker().sendCommand(Worker::Command::ResetWorker);
    }
};
