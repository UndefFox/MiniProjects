#include "params.h"

#include <string>
#include <unistd.h>

#include "generalerror.h"



Params::Params(int argc, char* argv[]) :
    helpNeeded(false),
    listenPort(2042),
    clientName("New user")
{
    int opt;
    while((opt = getopt(argc, argv, "hp:n:")) != -1) {
        try {
            switch (opt) {
            case 'h':
                helpNeeded = true; return;
            case 'p':
                listenPort = std::stoi(optarg); break;
            case 'n':
                clientName = std::string(optarg); break;
            default:
                throw GeneralError("Unknown flag: " + std::to_string(opt));
            }
        }
        catch (const std::logic_error& e) {
            std::string message("Error parsing: -");
            message += opt;

            if (optarg) {
                message += optarg;
            }

            throw GeneralError(message);
        }
    }

    if (optind < argc) {
        throw GeneralError("Unknown parameter: " + std::string(argv[optind]));
    }
}
