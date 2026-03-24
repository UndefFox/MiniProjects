#include "parameters.h"

#include <unistd.h>
#include <cstdlib>


Params::Params(int argc, char *argv[])  :
    ip("0.0.0.0"),
    port(8080),
    nodeId(-1),
    showHelp(false)
{
    char opt;
    while((opt = getopt(argc, argv, "hs:p:n:")) != -1) {
        switch (opt) {
        case 's':
            ip = std::string(optarg); break;
        case 'p':
            port = std::atoi(optarg); break;
        case 'n':
            nodeId = std::atoi(optarg); break;
        case 'h':
            showHelp = true; break;
        };
    }
}
