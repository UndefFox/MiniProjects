#include "benchmark.h"

#include <cmath>
#include <cstring>
#include <thread>
#include <vector>

#include "squarematrix.h"
#include "log.h"



namespace {

struct WorkerSharedState {
    std::atomic_int hasStarted;
    std::atomic_int hasFinished;
    int iterationLimit;
    Log& log;
    std::atomic<double> avg;
    std::atomic<double> max;
    int matSize;
};

struct Worker {
    void operator()(WorkerSharedState& params)
    const {
        SquareMatrix left{params.matSize, 42};
        SquareMatrix right{params.matSize, 926};

        SquareMatrix expected = left * right;

        while (true) {
            if (params.hasStarted.fetch_add(1) >= params.iterationLimit) break;

            SquareMatrix result(params.matSize);

            const auto start = std::chrono::high_resolution_clock::now();
              SquareMatrix::multiplyAdd(left, right, result);
            const auto end = std::chrono::high_resolution_clock::now();

            const bool isValid = (result == expected);
            const double elapsedSeconds = std::chrono::duration<double>(end - start).count();
            const double flops = std::pow(params.matSize, 2) * (2.0 * (params.matSize - 1));
            const double flopsPerSecond = flops / elapsedSeconds;

            int finishedCount;
            while (true) {
                auto expected = params.avg.load();

                const double delta = flopsPerSecond - expected;
                const double newValue = expected + delta / (params.hasFinished + 1);

                if (params.avg.compare_exchange_weak(expected, newValue)) {
                    finishedCount = params.hasFinished.fetch_add(1);
                    break;
                }
            };

            double currentMax = params.max.load();
            while (currentMax < flopsPerSecond &&
                   !params.max.compare_exchange_weak(currentMax,flopsPerSecond)
            ) {}

            params.log.logIterationFinished(
                finishedCount + 1,
                params.iterationLimit,
                isValid,
                flopsPerSecond
            );
        }
    }
};
}


void Benchmark::run(const Benchmark::Properties& args) {
    Log log;

    bool realtTimeIsOn = false;
    std::string error;
    if (args.realtimeScheduling) {
        sched_param param{};
        param.sched_priority = 10;

        int result = pthread_setschedparam(
            pthread_self(),
            SCHED_RR,
            &param
        );

        if (result != 0) {
            error = std::strerror(result);
        }
        else {
            realtTimeIsOn = true;
        }
    }

    const int parallelCount = args.iterations > args.threadCount ? args.threadCount : args.iterations;
    const auto tileSize = SquareMatrix::TILE_SIDE_SIZE;
    const auto bytesPerValue = sizeof(SquareMatrix::storageValue_t);
    constexpr int NUM_OF_MATRIX_PER_THREAD = 4;

    const int matSize = std::ceil(std::sqrt((double)args.bytesToAllocate / bytesPerValue / NUM_OF_MATRIX_PER_THREAD / parallelCount) / tileSize) * tileSize;

    log.showStartMessage(
        args.threadCount,
        matSize,
        realtTimeIsOn,
        error
    );

    std::vector<std::thread> threads;
    threads.reserve(parallelCount);

    WorkerSharedState sharedState{
        .hasStarted = 0,
        .hasFinished = 0,
        .iterationLimit = args.iterations,
        .log = log,
        .avg = 0,
        .max = 0,
        .matSize = matSize
    };

    for (int i = 0; i < parallelCount; ++i) {
        threads.emplace_back(
            Worker{},
            std::ref(sharedState)
        );
    }

    for (auto& thread : threads) {
        thread.join();
    }

    log.showConclusionMessage(
        args.iterations,
        matSize,
        sharedState.avg,
        sharedState.max
    );
}
