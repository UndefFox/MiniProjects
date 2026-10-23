#include "terminalguard.h"

#include <system_error>
#include <fcntl.h>
#include <unistd.h>



TerminalGuard::TerminalGuard(std::initializer_list<Flag> positive, std::initializer_list<Flag> negative) {
    if (::tcgetattr(STDIN_FILENO, &m_original) == -1) {
        throw std::system_error(errno, std::generic_category(), "TerminalGuard::TerminalGuard::tcgetattr");
    }

    termios current = m_original;
    current.c_cc[VMIN] = 1;
    current.c_cc[VTIME] = 0;

    m_oldFlags = ::fcntl(STDIN_FILENO, F_GETFL);
    if (m_oldFlags == -1) {
        throw std::system_error(errno, std::generic_category(), "TerminalGuard::TerminalGuard::fcntl(F_GETFL)");
    }

    int currentFdFlags = m_oldFlags;

    const auto apply = [](auto& value, auto flag, bool add) {
        if (add) {
            value |= flag;
        } else {
            value &= ~flag;
        }
    };

    const auto applyFlag = [&](Flag flag, bool add) {
        switch (flag) {
        case Blocking:
            apply(currentFdFlags, O_NONBLOCK, !add);
            break;
        case Echo:
            apply(current.c_lflag, ECHO, add);
            break;
        case Canonical:
            apply(current.c_lflag, ICANON, add);
            break;
        }
    };

    for (Flag flag : positive) {
        applyFlag(flag, true);
    }

    for (Flag flag : negative) {
        applyFlag(flag, false);
    }

    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &current) == -1) {
        throw std::system_error(errno, std::generic_category(), "TerminalGuard::TerminalGuard::tcsetattr");
    }

    if (::fcntl(STDIN_FILENO, F_SETFL, currentFdFlags) == -1) {
        const int error = errno;
        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_original);
        throw std::system_error(error, std::generic_category(), "TerminalGuard::TerminalGuard::fcntl(F_SETFL)");
    }

    m_active = true;
}


TerminalGuard::~TerminalGuard() {
    if (m_active) {
        restore();
    }
}

void TerminalGuard::restore() {
    if (!m_active) {
        return;
    }

    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_original) == -1) {
        throw std::system_error(errno, std::generic_category(), "TerminalGuard::tcsetattr::restore");
    }

    if (::fcntl(STDIN_FILENO, F_SETFL, m_oldFlags) == -1) {
        throw std::system_error(errno, std::generic_category(), "TerminalGuard::fcntl::restore");
    }

    m_active = false;
}
