#include <iostream>

#include "generalerror.h"
#include "params.h"

#include "app.h"



namespace {

void showHelp() {
    std::cout
        << "usage: program [-h] [-p <port>] [-n <name>]\n"
        << "\n"
        << "Parameters:\n"
        << "    -h          Show this help message\n"
        << "    -p <port>   Listening port (default: 2042)\n"
        << "    -n <name>   Client name (default: \"New user\")\n"
        << "\n"
        << "Commands:\n"
        << "    connect <ip> <port>  Connect to a new user\n"
        << "    chat <index>         Open chat with the user with id = <index>\n"
        << "    send <text>          Send a message to the currently open chat\n"
        << "    exit                 Close app (chats don't save)\n"
        << "\n"
        << "Example:\n"
        << "    program -p 3000 -n Foxy\n"
        << std::endl;
}


} // namespace

int main(int argc, char *argv[])  try {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const Params params(argc, argv);

    App app(params);
    app.run();

    return EXIT_SUCCESS;
}
catch (const GeneralError& e) {
    std::cerr << e.what() << std::endl;

    return EXIT_FAILURE;
}
