#pragma once

#include <mutex>

#include "server/serverstate.h"



struct ThreadsSharedState {
    ServerState serverState;
    std::mutex serverStateInUse;
};