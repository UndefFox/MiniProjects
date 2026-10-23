#include "tui.h"

#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>

#include "sequences.h"



TUI::~TUI() {
    std::cout << Sequences::CURSOR_SHOW
              << Sequences::FG_DEFAULT
              << Sequences::BG_DEFAULT
              << Sequences::CLEAR_SCREEN
              << Sequences::CURSOR_HOME
              << std::flush;
}

void TUI::draw() {
    std::cout << Sequences::BG_DEFAULT
              << Sequences::FG_DEFAULT
              << Sequences::CLEAR_SCREEN;

    m_root.draw({}, m_root.dimensions);

    std::cout << std::flush;
}

void TUI::resizeToScreen() {
    winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

    m_root.dimensions.width = size.ws_col;
    m_root.dimensions.height = size.ws_row;
}