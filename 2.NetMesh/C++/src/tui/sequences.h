#pragma once

#include <string>
#include <array>



namespace Sequences {
constexpr char CLEAR_SCREEN[]          = "\x1b[2J";
constexpr char CLEAR_TO_END_SCREEN[]   = "\x1b[J";
constexpr char CLEAR_TO_START_SCREEN[] = "\x1b[1J";

constexpr char CLEAR_LINE[]            = "\x1b[2K";
constexpr char CLEAR_TO_END_LINE[]     = "\x1b[K";
constexpr char CLEAR_TO_START_LINE[]   = "\x1b[1K";

constexpr char CURSOR_HOME[]           = "\x1b[H";
constexpr char CURSOR_HIDE[]           = "\x1b[?25l";
constexpr char CURSOR_SHOW[]           = "\x1b[?25h";
constexpr char CURSOR_SAVE[]           = "\x1b[s";
constexpr char CURSOR_RESTORE[]        = "\x1b[u";

constexpr char RESET_STYLE[]           = "\x1b[0m";
constexpr char BOLD_ON[]               = "\x1b[1m";
constexpr char DIM_ON[]                = "\x1b[2m";
constexpr char UNDERLINE_ON[]          = "\x1b[4m";
constexpr char INVERSE_ON[]            = "\x1b[7m";

constexpr char FG_DEFAULT[]            = "\x1b[39m";
constexpr char FG_RED[]                = "\x1b[31m";
constexpr char FG_GREEN[]              = "\x1b[32m";
constexpr char FG_YELLOW[]             = "\x1b[33m";
constexpr char FG_BLUE[]               = "\x1b[34m";
constexpr char FG_MAGENTA[]            = "\x1b[35m";
constexpr char FG_CYAN[]               = "\x1b[36m";
constexpr char FG_WHITE[]              = "\x1b[37m";
constexpr char FG_BLACK[]              = "\x1b[30m";

constexpr char BG_DEFAULT[]            = "\x1b[49m";
constexpr char BG_RED[]                = "\x1b[41m";
constexpr char BG_GREEN[]              = "\x1b[42m";
constexpr char BG_YELLOW[]             = "\x1b[43m";
constexpr char BG_BLUE[]               = "\x1b[44m";
constexpr char BG_MAGENTA[]            = "\x1b[45m";
constexpr char BG_CYAN[]               = "\x1b[46m";
constexpr char BG_WHITE[]              = "\x1b[47m";
constexpr char BG_BLACK[]              = "\x1b[40m";

constexpr std::string CURSOR_TO(int col, int row) {
    std::array<char, 32> buf{};
    std::snprintf(buf.data(), buf.size(), "\x1b[%d;%dH", row + 1, col + 1);

    buf.back() = '\0';

    return buf.data();
}
}