#include "pipe.h"

#include <poll.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <fcntl.h>



Pipe::Pipe() {
    int fds[2];
    if (::pipe2(fds, O_NONBLOCK) == -1) [[unlikely]] {
        throw PipeException(errno, std::generic_category(), "Pipe::Pipe::pipe2");
    }

    m_descriptors = std::pair<descriptor_t, descriptor_t>{fds[0], fds[1]};
}


Pipe::Pipe(Pipe &&other) noexcept :
    m_descriptors(std::exchange(other.m_descriptors, std::nullopt))
{ }

Pipe& Pipe::operator=(Pipe&& other) {
    if (m_descriptors) {
        close();
    }

    m_descriptors = std::exchange(other.m_descriptors, std::nullopt);

    return *this;
}

Pipe::~Pipe() {
    close();
}

int Pipe::getOutput() const {
    if (!m_descriptors) [[unlikely]] {
        throw std::logic_error("Failed to getOutput of pipe: Tried to use closed pipe");
    }

    return m_descriptors->first;
}

int Pipe::getInput() const {
    if (!m_descriptors) [[unlikely]] {
        throw std::logic_error("Failed to getInput of pipe: Tried to use closed pipe");
    }

    return m_descriptors->second;
}

void Pipe::write(uint8_t byte) {
    if (!m_descriptors) [[unlikely]] {
        throw std::logic_error("Failed to write into pipe: Tried to use closed pipe");
    }

    while (true) {
        if (::write(getInput(), &byte, sizeof(byte)) == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw PipeException(errno, std::generic_category(), "Pipe::write::write");
        }

        break;
    }
}

std::optional<uint8_t> Pipe::tryReadByte() {
    if (!m_descriptors) [[unlikely]] {
        throw std::logic_error("Failed to readByte of pipe: Tried to use closed pipe");
    }

    uint8_t output;

    while (true) {
        int result = ::read(getOutput(), &output, sizeof(output));

        if (result == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            if (std::error_code(errno, std::generic_category()) == std::errc::operation_would_block) {
                return std::nullopt;
            }

            throw PipeException(errno, std::generic_category(), "Pipe::read::read");
        }
        else if (result == 0) {
            throw PipeException(std::make_error_code(std::errc::broken_pipe), "Pipe::read::read");
        }

        break;
    }

    return output;
}

void Pipe::close() noexcept {
    if (m_descriptors) {
        ::close(getOutput());
        ::close(getInput());

        m_descriptors = std::nullopt;
    }
}
