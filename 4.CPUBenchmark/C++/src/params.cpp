#include "params.h"

#include <unistd.h>

#include "generalerror.h"



namespace {
void reportParsingError(char c, const char* value) {
    std::string message("Error parsing: -");
    message += c;

    if (value) {
        message += value;
    }

    throw GeneralError(message);
}
}

Params::Params(int argc, char* argv[]) :
    helpNeeded(false),
    concurentThreads(1),
    maxBytes(1024LL * 1024LL * 1024LL),
    iterations(100),
    useRealtimePriority(false)
{
    int opt;
    while((opt = getopt(argc, argv, "+hrc:m:i:")) != -1) {
        try {
            switch (opt) {
            case 'h':
                helpNeeded = true; return;
            case 'c':
                concurentThreads = std::stoi(optarg); break;
            case 'm': {
                size_t pos;
                maxBytes = std::stoll(optarg, &pos);

                std::string dim(optarg + pos);
                if (dim.empty() || dim == "KiB") {
                    maxBytes *= 1024LL;
                }
                else if (dim == "MiB") {
                    maxBytes *= 1024LL * 1024LL;
                }
                else if (dim == "GiB") {
                    maxBytes *= 1024LL * 1024LL * 1024LL;
                }
                else {
                    throw GeneralError("Unknow measure: " + dim);
                }

                break;
            }
            case 'i':
                iterations = std::stoi(optarg); break;
            case 'r':
                useRealtimePriority = true; break;

            default:
                throw GeneralError("Unknown flag: " + std::to_string(opt));
            }
        }
        catch (const std::invalid_argument& e) {
            reportParsingError(opt, optarg);
        }
        catch (const std::out_of_range& e) {
            reportParsingError(opt, optarg);
        }
    }

    if (optind < argc) {
        throw GeneralError("Unknown parameter: " + std::string(argv[optind]));
    }
}
