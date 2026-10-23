#pragma once

#include <list>
#include <vector>

#include "utils/socket.h"
#include "enctryptor.h"



struct ConnectionState;

struct Chat {
    struct Message {
        bool own;
        std::string text;
    };

    bool allRead = true;
    std::string userName;
    std::vector<Message> history;

    std::optional<ConnectionState*> linkedConnection;
};

struct ConnectionState {
    Socket socket;
    Enctryptor encryptor;

    std::optional<Chat*> linkedChat;
};

struct ServerState {
    std::list<ConnectionState> connectionsState;
    std::list<Chat> chats;
    std::optional<Chat*> currentChat;

    std::string lastSytemMessage;
    std::string clientName;


public:
    ServerState() = default;
    ServerState(ServerState&& other) noexcept = delete;
    ServerState& operator=(ServerState&& other) = delete;
    ServerState(const ServerState&) = delete;
    ServerState& operator=(const ServerState&) = delete;

    ~ServerState();

    void terminate();
};
