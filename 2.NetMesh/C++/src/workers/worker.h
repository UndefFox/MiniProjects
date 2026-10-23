#pragma once

#include "utils/pipe.h"

#include <pthread.h>



class Worker {
public:
    enum class Command : std::uint8_t {
        FinishThread = 1,
        ResetWorker
    };


public:
    Pipe m_commandPipe;
    pthread_t m_ownThread;

    void sendCommand(Command command);
    std::optional<Command> tryGetCommand();

protected:
    Worker() = default;

    template<class T>
    T& self() { return static_cast<T&>(*this); };
};
