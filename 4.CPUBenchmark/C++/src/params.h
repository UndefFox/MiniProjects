#pragma once



class Params {
public:
    bool helpNeeded;
    int concurentThreads;
    long long maxBytes;
    int iterations;
    bool useRealtimePriority;


public:
    Params() = default;
    Params(int argc, char* argv[]);
};
