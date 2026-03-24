#include "eventpipe.h"

#include <cstddef>
#include <fcntl.h>
#include <unistd.h>


EventPipe::EventPipe() {
    int fds[2];
    ::pipe(fds);

    output = fds[0];
    input = fds[1];
}

EventPipe::~EventPipe() {
    ::close(output);
    ::close(input);
}

int EventPipe::getOutput() const {
    return output;
}

int EventPipe::getInput() const {
    return input;
}

void EventPipe::write() {
    const std::byte byte((std::byte)1);
    ::write(input, &byte, 1);
}

void EventPipe::reset() {
    std::byte buffer;
    ::read(output, &buffer, 1);
}
