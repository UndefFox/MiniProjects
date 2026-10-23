#include "serverstate.h"



ServerState::~ServerState() {
    terminate();
}

void ServerState::terminate() {
    for (auto& conn : connectionsState) {
        conn.socket.shutdown();
        conn.socket.close();
    }

    connectionsState.clear();

    for (auto& chat : chats) {
        chat.linkedConnection = std::nullopt;
    }
}
