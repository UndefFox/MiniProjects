#include <iostream>

#include "benchmark.h"
#include "params.h"
#include "generalerror.h"



namespace {

void showHelp() {
    std::cout
        << "usage: foxbench [hc:m:i:r]\n"
        << "\n"
        << "Parameters:\n"
        << "    -h          Show this help message\n"
        << "    -c <count>  Number of concurrent threads (default: 1)\n"
        << "    -m <size>   Maximum number of bytes to use (default: 1GiB)\n"
        << "                Supported units: KiB, MiB, GiB\n"
        << "                Examples: 1024, 64KiB, 2MiB, 1GiB\n"
        << "    -i <count>  Number of iterations (default: 100)\n"
        << "    -r          Use realtime priority (requires root access)\n"
        << "\n"
        << "Example:\n"
        << "    foxbench -c 6 -m 64MiB -i 100\n"
        << std::endl;
}

} // namespace


int main(int argc, char* argv[]) try {
    Params params(argc, argv);

    if (params.helpNeeded) {
        showHelp();
        return EXIT_SUCCESS;
    }

    const Benchmark::Properties setting{
        .iterations = params.iterations,
        .threadCount = params.concurentThreads,
        .bytesToAllocate = params.maxBytes,
        .realtimeScheduling = params.useRealtimePriority
    };

    Benchmark::run(setting);

    return EXIT_SUCCESS;
}
catch (const GeneralError& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
