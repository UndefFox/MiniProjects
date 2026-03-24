#include "parameters.h"
#include "backend.h"

#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>


namespace {
void showHelp() {
    std::cout << "\
usage: oldcom [hs:p:n:]\n\
\n\
Parameters:\n\
    -s     IP address to listen for connections (default: 0.0.0.0)\n\
    -p     Port to listen for connections (default: 8080)\n\
    \n\
    -n     Node ID for this instance. Must be defined!\n\
\n\
Example: oldcom -s 127.0.0.1 -p 9000 -n 1"
    << std::endl;
}

} // namespace <anonymous>

int main(int argc, char *argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::srand(std::time(NULL));

    Params params(argc, argv);

    if (params.showHelp) {
        showHelp();

        return 0;
    }

    try {
        if (params.nodeId == -1) {
            throw std::runtime_error("Node id must be set.");
        }

        Backend backend(params);
        backend.run();
    }
    catch (std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}
