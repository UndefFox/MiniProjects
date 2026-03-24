#include "socket.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <netinet/tcp.h>


Socket::Socket() noexcept :
    descriptor(-1)
{ }

Socket::Socket(int descriptor) noexcept :
    descriptor(descriptor)
{ }

Socket::Socket(Socket&& other) noexcept :
    descriptor(other.descriptor)
{
    other.descriptor = -1;
}

Socket::~Socket() {
    if (descriptor != -1) {
        try {
            close();
        }
        catch (...) {}
    }
}

void Socket::init() {
    if (descriptor != -1) {
        close();
    }

    descriptor = ::socket(AF_INET, SOCK_STREAM, 0);

    if (descriptor == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Socket::bind(std::string_view ip, uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.data(), &address.sin_addr.s_addr) == 0) {
        throw std::runtime_error("Invalid address");
    }

    if (::bind(descriptor, (struct sockaddr*)&address, sizeof(address)) == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Socket::connect(std::string_view ip, uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.data(), &address.sin_addr.s_addr) == 0) {
        throw std::runtime_error("Invalid address");
    }

    if (::connect(descriptor, (struct sockaddr*)&address, sizeof(address)) == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Socket::listen(int maxQueue) {
    if (::listen(descriptor, maxQueue) == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}

Socket Socket::accept() {
    while (true) {
        int result = ::accept(descriptor, nullptr, nullptr);

        if (result == -1) {
            if (errno == ECONNABORTED || errno == EINTR) continue;

            throw std::runtime_error(std::strerror(errno));
        }

        return Socket(result);
    }
}

void Socket::shutdown(Shutdown direction) {
    ::shutdown(descriptor, (int)direction);
}

void Socket::close() {
    if (::close(descriptor) == -1) {
        throw std::runtime_error(std::strerror(errno));
    }

    descriptor = -1;
}

void Socket::setOperationTimeout(int timeout) {
    struct timeval t{};
    t.tv_sec = timeout;
    t.tv_usec = 0;

    int check = 0;

    check -= ::setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(timeout));
    check -= ::setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(timeout));

    if (check < 0) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Socket::setKeepAlive(bool toggle, int timeout) {
    int check = 0;

    int flag = toggle ? 1 : 0;
    int keepintvl = 60;
    int count = 1;

    check += ::setsockopt(descriptor, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
    check += ::setsockopt(descriptor, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(timeout));
    check += ::setsockopt(descriptor, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
    check += ::setsockopt(descriptor, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    if (check < 0) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Socket::read(std::span<std::byte> buffer) {
    size_t bytesRead = 0;
    while (bytesRead != buffer.size()) {
        int read = ::recv(descriptor, buffer.data() + bytesRead, buffer.size() - bytesRead, 0);

        if (read == -1) {
            throw std::runtime_error("read has failed");
        }
        else if (read == 0) {
            throw SocketException(descriptor);
        }
        else {
            bytesRead += read;
        }
    }
}

void Socket::write(std::span<const std::byte> buffer) {
    size_t bytesWrote = 0;
    while (bytesWrote != buffer.size()) {
        int wrote = ::send(descriptor, buffer.data() + bytesWrote, buffer.size() - bytesWrote, 0);

        if (wrote == -1) {
            throw std::runtime_error("write has failed");
        }
        else if (wrote == 0) {
            throw SocketException(descriptor);
        }
        else {
            bytesWrote += wrote;
        }
    }
}

bool Socket::isClosed() const {
    char buffer[1];
    int result = ::recv(descriptor, buffer, 1, MSG_PEEK);

    if (result <= 0) {
        return true;
    }

    return false;
}

int Socket::get() const {
    return descriptor;
}

Socket& Socket::operator=(Socket&& other) {
    if (this != &other) {
        ::close(descriptor);
        descriptor = other.descriptor;
        other.descriptor = -1;
    }

    return *this;
}

SocketException::SocketException(int descriptor) {
    socklen_t len = sizeof(rawError);
    getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &rawError, &len);
}

const char *SocketException::what() const noexcept {
    return std::strerror(rawError);
}

