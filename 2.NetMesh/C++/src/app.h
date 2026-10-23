#pragma once

#include "workers/packageprocessorworker.hpp"
#include "workers/registrationworker.hpp"
#include "workers/threadssharedstate.hpp"
#include "workers/tuiWorker.hpp"

#include "params.h"



class App :
    public TUIWorker<App>,
    public PackageProcessorWorker<App>,
    public RegistrationWorker<App>
{
private:
    ThreadsSharedState m_threadsSharedState;


public:
    App(const Params& params);

    void run();

private:
    TUIWorker<App>& getTUIWorker();
    PackageProcessorWorker<App>& getPackageWorker();
    RegistrationWorker<App>& getRegistrationWorker();
    ThreadsSharedState& getSharedState();


    friend class TUIWorker<App>;
    friend class PackageProcessorWorker<App>;
    friend class RegistrationWorker<App>;
};
