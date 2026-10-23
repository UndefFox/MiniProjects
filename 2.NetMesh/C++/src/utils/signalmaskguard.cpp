#include "signalmaskguard.h"

#include <pthread.h>



SignalMaskGuard::SignalMaskGuard(std::initializer_list<SignalDesc::Signal> signals) {
    if (signals.size() == 0) {
        throw std::invalid_argument("SignalMaskGuard: empty signal list");
    }

    sigset_t set;
    if (::sigemptyset(&set) == -1) {
        throw std::system_error(errno, std::generic_category(), "SignalMaskGuard::sigemptyset");
    }

    for (SignalDesc::Signal signal : signals) {
        if (::sigaddset(&set, static_cast<int>(signal)) == -1) {
            throw std::system_error(errno, std::generic_category(), "SignalMaskGuard::sigaddset");
        }
    }

    int error = ::pthread_sigmask(SIG_BLOCK, &set, &m_previous);
    if (error != 0) {
        throw std::system_error(error, std::generic_category(), "SignalMaskGuard::pthread_sigmask");
    }

    m_active = true;
}

SignalMaskGuard::~SignalMaskGuard() {
    if (m_active) {
        restore();
    }
}

void SignalMaskGuard::restore() {
    if (!m_active) {
        return;
    }

    int error = ::pthread_sigmask(SIG_SETMASK, &m_previous, nullptr);
    if (error != 0) {
        throw std::system_error(error, std::generic_category(), "SignalMaskGuard::restore");
    }

    m_active = false;
}
