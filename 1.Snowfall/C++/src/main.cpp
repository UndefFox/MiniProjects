#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdlib>

#include <iostream>

#include "signalwatcher.h"
#include "particleengine.h"


namespace ANCIICodes {
    const char* RETURN_TO_TOP_LEFT = "\x1b[H";
    const char* CLEAR_TERMINAL = "\x1b[2J";
    const char* HIDE_CURSOR = "\x1b[?25l";
    const char* SHOW_CURSOR = "\x1b[?25h";
}

void showHelp() {
    std::cout << "\
usage: snowfall [hps:c:g:f:]\n\
\n\
Parametrs:\n\
    -p     Creates new particle with default values\n\
    \n\
    -s     Changes symbol of the last particle\n\
    -c     Changes amount of the last particle\n\
    -g     Changes gravity (rows per frame) of the last particle\n\
    \n\
    -f     Sets fps for the animation.\n\
\n\
Example: snowfall -p -s '*' -c 200 -g 2 -p -s '#' -c 100 -g 3 -f 12"
    << std::endl;
}

winsize getTerminalSize() {
    winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    return size;
}

/** @brief Processes all the input flags to the program.
 *
 *  @p argc Value passed from main function.
 *  @p argv Value passed from main function.
 *  @p engine Reference to the ParticleEngine to what all the flags will
 *            be applied.
 *
 *  @return true if the program can continue running; false otherwise.
 */
bool processInputs(int argc, char *argv[], ParticleEngine& engine) {
    if (argc == 1) {
        showHelp();
        return 0;
    }

    char opt;

    struct particlePreset {
        unsigned int count = 100;
        unsigned char sim = '*';
        unsigned int grav = 1;
    } preset;

    try {
        bool firstParticle = true;
        while((opt = getopt(argc, argv, "hps:c:g:f:")) != -1) {
            switch(opt) {
                case 'h':
                    showHelp();
                    return false;
                case 'p':
                    if (!firstParticle)
                        engine.addArchetype(preset.sim, preset.count, preset.grav);
                preset = particlePreset{};
                firstParticle = false; break;
                case 's':
                    preset.sim = optarg[0];
                    break;
                case 'c':
                    preset.count = std::atoi(optarg); break;
                case 'g':
                    preset.grav = std::atoi(optarg); break;
                case 'f':
                    engine.setFPS(std::atoi(optarg));
                    break;
            }
        }

        engine.addArchetype(preset.sim, preset.count, preset.grav);
    }
    catch (const std::exception& error) {
        std::cout << "Error in input parameters: " << error.what() << std::endl;
        return false;
    }

    return true;
}


int main(int argc, char *argv[]) {
    std::ios_base::sync_with_stdio(false);

    ParticleEngine engine{};
    if (!processInputs(argc, argv, engine)) {
        return 0;
    }

    std::cout << ANCIICodes::HIDE_CURSOR;

    SignalWatcher watcher{};
    winsize resolution = getTerminalSize();
    engine.resizeImage(resolution.ws_col, resolution.ws_row);

    while (true) {
        std::cout << ANCIICodes::RETURN_TO_TOP_LEFT;
        if (watcher.checkAndClearFlag(WatcherEvents::INTERRUPT)) {
            std::cout << ANCIICodes::CLEAR_TERMINAL;
            std::cout << "\nSnowfall is over :(" << std::endl;
            break;
        }

        engine.tick();

        if (watcher.checkAndClearFlag(WatcherEvents::TERMINAL_SIZE_CHANGED)) {
            resolution = getTerminalSize();
            engine.resizeImage(resolution.ws_col, resolution.ws_row);
        }
        else {
            engine.display();
        }

        engine.waitForFrameDelay();
    }

    std::cout << ANCIICodes::SHOW_CURSOR;

    return 0;
}


