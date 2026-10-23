#include "textbox.h"

#include <iostream>

#include "sequences.h"



void TextBox::draw(Rectangle root, Rectangle drawRegion) {
    const auto foreground = [this]() -> std::string_view {
        switch (frontColor) {
        case RED:     return Sequences::FG_RED;
        case GREEN:   return Sequences::FG_GREEN;
        case YELLOW:  return Sequences::FG_YELLOW;
        case BLUE:    return Sequences::FG_BLUE;
        case MAGENTA: return Sequences::FG_MAGENTA;
        case CYAN:    return Sequences::FG_CYAN;
        case WHITE:   return Sequences::FG_WHITE;
        case BLACK:   return Sequences::FG_BLACK;
        case DEFAULT:
        default: return Sequences::FG_DEFAULT;
        }
    };

    const auto backgroundColor = [this]() -> std::string_view {
        switch (backColor) {
        case RED:     return Sequences::BG_RED;
        case GREEN:   return Sequences::BG_GREEN;
        case YELLOW:  return Sequences::BG_YELLOW;
        case BLUE:    return Sequences::BG_BLUE;
        case MAGENTA: return Sequences::BG_MAGENTA;
        case CYAN:    return Sequences::BG_CYAN;
        case WHITE:   return Sequences::BG_WHITE;
        case BLACK:   return Sequences::BG_BLACK;
        case DEFAULT:
        default: return Sequences::BG_DEFAULT;
        }
    };

    std::cout << backgroundColor()
              << foreground();

    const Rectangle screenOrigin = root.toPoint() + drawRegion.toPoint();

    for (int row = 0; row < drawRegion.height; ++row) {
        const Rectangle rowOffset{
            .x = 0, .y = static_cast<Rectangle::coord_t>(row),
            .width = 0, .height = 0
        };
        const Rectangle cursor = screenOrigin + rowOffset;

        std::cout << Sequences::CURSOR_TO(cursor.x, cursor.y);

        for (int column = 0; column < drawRegion.width; ++column) {
            const std::size_t index =
                static_cast<std::size_t>(drawRegion.y + row) * dimensions.width
              + static_cast<std::size_t>(drawRegion.x + column);

            std::cout << (index < text.size() ? text[index] : ' ');
        }
    }

    std::cout << Sequences::BG_DEFAULT
              << Sequences::FG_DEFAULT;

    Widget::draw(root, drawRegion);
}

void TextBox::calculateHeightToFit() {
    dimensions.height = text.size() / dimensions.width + (text.size() % dimensions.width != 0);
}

TextBox* TextBox::addTextBox(Widget* parent) {
    TextBox* newTextBox = new TextBox();

    std::unique_ptr<Widget> ptr{newTextBox};

    parent->m_children.push_back(std::move(ptr));

    return newTextBox;
}