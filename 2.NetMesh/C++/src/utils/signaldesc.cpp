#include "signaldesc.h"

#include <cerrno>
#include <poll.h>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <unistd.h>



SignalDesc::SignalDesc(SignalDesc&& other) noexcept
    : m_descriptor(std::exchange(other.m_descriptor, std::nullopt))
{ }

SignalDesc& SignalDesc::operator=(SignalDesc&& other) noexcept {
    if (this != &other) {
        close();
        m_descriptor = std::exchange(other.m_descriptor, std::nullopt);
    }

    return *this;
}

SignalDesc::~SignalDesc() {
    close();
}

void SignalDesc::init(std::initializer_list<Signal> signals) {
    if (m_descriptor) [[unlikely]] {
        throw std::logic_error("Failed to init SignalDesc: Already initialized");
    }
    if (signals.size() == 0) [[unlikely]] {
        throw std::logic_error("Failed to init SignalDesc: Signal list is empty");
    }

    sigset_t signal_set;
    if (::sigemptyset(&signal_set) == -1) [[unlikely]] {
        throw SignalDescException(errno, std::generic_category(), "SignalDesc::init::sigemptyset");
    }

    for (Signal signal : signals) {
        if (::sigaddset(&signal_set, static_cast<int>(signal)) == -1) [[unlikely]] {
            throw SignalDescException(errno, std::generic_category(), "SignalDesc::init::sigaddset");
        }
    }

    int fd = ::signalfd(-1, &signal_set, SFD_CLOEXEC | SFD_NONBLOCK);
    if (fd == -1) [[unlikely]] {
        throw SignalDescException(errno, std::generic_category(), "SignalDesc::init::SignalDesc");
    }

    m_descriptor = fd;
}

int SignalDesc::getDescriptor() const {
    if (!m_descriptor) [[unlikely]] {
        throw std::logic_error("Failed to getDescriptor of signal fd: Tried to use closed signal fd");
    }

    return *m_descriptor;
}

std::optional<signalfd_siginfo> SignalDesc::tryReadSignal() {
    if (!m_descriptor) [[unlikely]] {
        throw std::logic_error("Failed to tryReadSignal: Tried to use closed signal fd");
    }

    while (true) {
        signalfd_siginfo info{};
        ssize_t result = ::read(getDescriptor(), &info, sizeof(info));

        if (result == static_cast<ssize_t>(sizeof(info))) {
            return info;
        }

        if (result == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) {
                continue;
            }

            if (std::error_code(errno, std::generic_category()) == std::errc::operation_would_block) {
                return std::nullopt;
            }

            throw SignalDescException(errno, std::generic_category(), "SignalDesc::tryReadSignal::read");
        }

        throw SignalDescException(std::make_error_code(std::errc::io_error), "SignalDesc::tryReadSignal::read");
    }
}

void SignalDesc::close() {
    while (std::as_const(m_descriptor)) {
        if (::close(m_descriptor.value()) == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw SignalDescException(errno, std::generic_category(), "SignalDesc::close::close");
        }

        m_descriptor = std::nullopt;

        break;
    }
}
