#include "worker.h"



void Worker::sendCommand(Command command) {
    m_commandPipe.write(static_cast<std::uint8_t>(command));
}

std::optional<Worker::Command> Worker::tryGetCommand() {
    if (auto opt = m_commandPipe.tryReadByte()) {
        return static_cast<Worker::Command>(opt.value());
    }
    else {
        return std::nullopt;
    }
}
