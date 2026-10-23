#pragma once

#include <initializer_list>

#include "utils/signaldesc.h"



class SignalMaskGuard {
private:
    sigset_t m_previous{};
    bool m_active = false;


public:
    SignalMaskGuard(std::initializer_list<SignalDesc::Signal> signals);

    SignalMaskGuard(const SignalMaskGuard&) = delete;
    SignalMaskGuard& operator=(const SignalMaskGuard&) = delete;

    ~SignalMaskGuard();

    void restore();
};
