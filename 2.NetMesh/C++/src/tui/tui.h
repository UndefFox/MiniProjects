#pragma once

#include "widget.h"



class TUI {
public:
    Widget m_root = Widget{};


public:
    ~TUI();

    void draw();

    void resizeToScreen();
};
