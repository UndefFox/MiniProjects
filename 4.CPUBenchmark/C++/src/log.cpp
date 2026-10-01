#include "log.h"

#include <iostream>
#include <iomanip>
#include <sstream>



void Log::logIterationFinished(int index, int max, bool valid, double flops) {
    std::lock_guard lock(mutex);

    const int numCount = std::to_string(max).size();

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(numCount) << index
        << "/" << std::setw(numCount) << max
        << ' ' << (valid ? "valid" : "invalid")
        << ' ' << std::scientific << flops
        << " flops/s\n";

    std::cout << oss.str() << std::flush;
}

void Log::showStartMessage(int threadsCount, int matrixSize, bool realTime, std::string_view realTimeError) {
    std::lock_guard lock(mutex);

    std::ostringstream oss;
    oss << "[Starting Fox flops benchmark]\n"
        << "Realtime: " << (realTime ? "ON" : "OFF");

    if (!realTimeError.empty()) {
        oss << "\n[ERROR] Realtime unavailable: " << realTimeError;
    }

    oss << "\nThreads: " << threadsCount << '\n'
        << "Matrix size: " << matrixSize << '\n';

    std::cout << oss.str() << std::flush;
}

void Log::showConclusionMessage(int iterations, int matSize, double avg, double max) {
    std::lock_guard lock(mutex);

    std::ostringstream message;
    message << "-----------------\n"
            << "Average speed after "
            << iterations
            << " iterations and matrix size of "
            << matSize
            << ": "
            << avg
            << " flops/s\n"
            << "Max value: "
            << max << " flops/s\n";

    std::cout << message.str() << std::flush;
}



