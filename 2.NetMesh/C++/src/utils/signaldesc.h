#pragma once

#include <initializer_list>
#include <optional>
#include <system_error>
#include <signal.h>
#include <sys/signalfd.h>



struct SignalDescException : public std::system_error {
    using std::system_error::system_error;
};

struct SignalDesc {
public:
    enum class Signal : int {
        Hangup          = SIGHUP,
        Interrupt       = SIGINT,
        Quit            = SIGQUIT,
        User1           = SIGUSR1,
        User2           = SIGUSR2,
        Alarm           = SIGALRM,
        Child           = SIGCHLD,
        Continue        = SIGCONT,
        Pipe            = SIGPIPE,
        Terminate       = SIGTERM,
        TerminalStop    = SIGTSTP,
        TerminalInput   = SIGTTIN,
        TerminalOutput  = SIGTTOU,
        Urgent          = SIGURG,
        CpuLimit        = SIGXCPU,
        FileSizeLimit   = SIGXFSZ,
        VirtualAlarm    = SIGVTALRM,
        Profiling       = SIGPROF,
        WindowChange    = SIGWINCH,
        Io              = SIGIO
    };

    using descriptor_t = int;


private:
    std::optional<descriptor_t> m_descriptor;


public:
    SignalDesc() = default;
    SignalDesc(SignalDesc&& other) noexcept;
    SignalDesc& operator=(SignalDesc&& other) noexcept;
    SignalDesc(const SignalDesc&) = delete;
    SignalDesc& operator=(const SignalDesc&) = delete;

    ~SignalDesc();

    void init(std::initializer_list<Signal> signals);

    int getDescriptor() const;
    std::optional<signalfd_siginfo> tryReadSignal();

    void close();
};
