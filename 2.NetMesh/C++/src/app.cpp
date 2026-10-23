#include "app.h"

#include <thread>
#include <termios.h>
#include <fcntl.h>

#include "utils/signalmaskguard.h"
#include "utils/terminalguard.h"
#include "generalerror.h"



ThreadsSharedState& App::getSharedState() {
    return m_threadsSharedState;
}

App::App(const Params& params) {
    if (params.listenPort <= 1000) {
        throw GeneralError("Port must be greater than 1000");
    }
    else if (params.clientName.size() < 4) {
        throw GeneralError("Name must be longer than 4 character");
    }

    m_threadsSharedState.serverState.clientName = params.clientName;

    getRegistrationWorker().initial.listenPort = params.listenPort;
}

void App::run() {
    TerminalGuard terminalGuard({
        // None
    },{
        TerminalGuard::Flag::Canonical,
        TerminalGuard::Flag::Echo,
        TerminalGuard::Flag::Blocking
    });

    SignalMaskGuard signalMaskGuard({
        SignalDesc::Signal::WindowChange,
        //SignalDesc::Signal::Interrupt
    });

    auto tuiThread = std::thread([&]() { getTUIWorker()(); });
    std::array<std::thread, 2> threads {
        std::thread([&]() { getRegistrationWorker()(); }),
        std::thread([&]() { getPackageWorker()(); })
    };

    tuiThread.join();

    getRegistrationWorker().sendCommand(Worker::Command::FinishThread);
    getPackageWorker().sendCommand(Worker::Command::FinishThread);

    m_threadsSharedState.serverState.terminate();

    for (auto& t : threads) {
        t.join();
    }
}

TUIWorker<App>& App::getTUIWorker() { return *this; }
PackageProcessorWorker<App> &App::getPackageWorker() { return *this; }
RegistrationWorker<App> &App::getRegistrationWorker() { return *this;  }
