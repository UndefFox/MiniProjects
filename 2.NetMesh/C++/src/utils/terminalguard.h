#pragma once

#include <initializer_list>
#include <termios.h>



class TerminalGuard {
public:
    enum Flag {
        Blocking,
        Echo,
        Canonical
    };


private:
    termios m_original;
    int m_oldFlags;
    bool m_active;


public:
    TerminalGuard(std::initializer_list<Flag> positive, std::initializer_list<Flag> negative);
    ~TerminalGuard();

    TerminalGuard(const TerminalGuard&) = delete;
    TerminalGuard& operator=(const TerminalGuard&) = delete;

    void restore();
};

