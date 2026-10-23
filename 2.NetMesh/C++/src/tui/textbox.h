#pragma once

#include <string>

#include "widget.h"



class TextBox final : public Widget {
public:
    enum Color {
        DEFAULT,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE,
        BLACK
    };

public:
    std::string text = "";
    Color backColor = Color::DEFAULT;
    Color frontColor = Color::DEFAULT;


public:
    ~TextBox() override final = default;

    void draw(Rectangle root, Rectangle drawRegion) override final;

    void calculateHeightToFit();

    static TextBox* addTextBox(Widget* parent);
};

