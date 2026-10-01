#pragma once

#include <mutex>
#include <string_view>



class Log {
private:
    std::mutex mutex;

public:
    Log() = default;

    void logIterationFinished(int index, int max, bool valid, double flops);
    void showStartMessage(int threadsCount, int matrixSize, bool realTime, std::string_view realTimeError);
    void showConclusionMessage(int iterations, int matSize, double avg, double max);
};
