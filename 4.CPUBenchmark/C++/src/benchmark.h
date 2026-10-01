#pragma once



namespace Benchmark {
    struct Properties {
        // How many times to repeat test
        int iterations;
        // How many paralel tests to run
        int threadCount;
        // How much memory allocate for test totall
        long long bytesToAllocate;
        // Try setting working threads to realtime priority
        bool realtimeScheduling;
    };

    void run(const Properties& args);
};
